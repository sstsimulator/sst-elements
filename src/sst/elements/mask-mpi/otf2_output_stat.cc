/**
Copyright 2009-2025 National Technology and Engineering Solutions of Sandia,
LLC (NTESS).  Under the terms of Contract DE-NA-0003525, the U.S. Government
retains certain rights in this software.

Sandia National Laboratories is a multimission laboratory managed and operated
by National Technology and Engineering Solutions of Sandia, LLC., a wholly
owned subsidiary of Honeywell International, Inc., for the U.S. Department of
Energy's National Nuclear Security Administration under contract DE-NA0003525.

Copyright (c) 2009-2025, NTESS

All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.

    * Redistributions in binary form must reproduce the above
      copyright notice, this list of conditions and the following
      disclaimer in the documentation and/or other materials provided
      with the distribution.

    * Neither the name of the copyright holder nor the names of its
      contributors may be used to endorse or promote products derived
      from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

Questions? Contact sst-macro-help@sandia.gov
*/

//#include <sumi-mpi/otf2_output_stat.h>

#ifdef SST_HG_OTF2_ENABLED

#include <sumi-mpi/mpi_integers.h>
#include <sumi-mpi/mpi_comm/mpi_comm.h>
#include <sumi-mpi/mpi_api.h>
#include <sstmac/backends/common/parallel_runtime.h>
#include <sstmac/common/timestamp.h>
#include <limits>

namespace SST::MASKMPI {

OTF2Writer::OTF2Writer(SST::BaseComponent* parent, const std::string& name,
                       const std::string& subName, SST::Params& params) :
  SST::Statistics::CustomStatistic(parent, name, subName, params),
  min_time_(std::numeric_limits<uint64_t>::max()),
  max_time_(std::numeric_limits<uint64_t>::min())
{
}

void
OTF2Writer::init(uint64_t start, uint64_t stop, int rank, int size)
{
  rank_ = rank;
  size_ = size;
  writer_.set_verbosity(dumpi::OWV_WARN);

  std::string fileroot = groupName();

  writer_.register_comm_world(MPI_COMM_WORLD, size, rank);
  writer_.register_comm_self(MPI_COMM_SELF);
  writer_.register_comm_null(MPI_COMM_NULL);
  writer_.register_null_request(MPI_REQUEST_NULL);
  writer_.open_archive(fileroot);
  writer_.set_clock_resolution(sstmac::TimeDelta(1.0).ticks());
  writer_.generic_call(start, stop, "MPI_Init");

  std::shared_ptr<dumpi::OTF2_MPI_Comm> world(new dumpi::OTF2_MPI_Comm);
  world->local_id = MPI_COMM_WORLD;
  world->global_id = MPI_COMM_WORLD;
  world->is_root = rank == 0;
  world->local_rank = rank;
  world->world_rank = rank;

  std::shared_ptr<dumpi::OTF2_MPI_Group> group(new dumpi::OTF2_MPI_Group);
  world->group = group;
  group->is_comm_world = true;
  group->local_id = world->local_id;
  group->global_id = world->global_id;
}

void
OTF2Writer::addComm(MpiComm* comm, dumpi::mpi_comm_t parent_id)
{
  dumpi::OTF2_MPI_Comm::shared_ptr stored = writer_.make_new_comm(comm->id());

  stored->local_id = comm->id();
  stored->global_id = comm->id();
  stored->is_root = comm->rank() == 0;
  stored->local_rank = comm->rank();
  stored->parent = writer_.get_comm(parent_id);
  stored->world_rank = comm->group()->at(comm->rank());

  std::shared_ptr<dumpi::OTF2_MPI_Group> group(new dumpi::OTF2_MPI_Group);
  if (comm->group()->isCommWorld()){
    group->is_comm_world = true;
  } else {
    group->global_ranks = comm->group()->worldRanks();
  }
  group->local_id = comm->group()->id();
  group->global_id = group->local_id;
  stored->group = group;
}

void
OTF2Writer::reduce(OTF2Writer* other)
{
  auto unique_comms = other->writer().find_unique_comms();
  all_comms_.insert(all_comms_.end(), unique_comms.begin(), unique_comms.end());

  if (event_counts_.empty()) event_counts_.resize(size_);

  event_counts_[other->rank_] = other->writer().event_count();

  min_time_ = std::min(min_time_, other->writer().start_time());
  max_time_ = std::min(max_time_, other->writer().stop_time());
}

void
OTF2Writer::assignGlobalCommIds(MpiApi* mpi)
{
  //first, we have to do a parallel scan to assign globally unique ids
  //to each MPI rank

  /**


   **/

  auto& comm_map = writer_.all_comms();

  int numOwnedComms = 0;
  int numComms = 0;
  for (auto& pair : comm_map){
    auto& list = pair.second;
    for (auto& comm : list){
      if (comm->is_root) ++numOwnedComms;
      ++numComms;
    }
  }

  int myCommOffset = 0;

  auto op = mpi->startScan("OTF2_id_agree", MPI_COMM_WORLD, 1, MPI_INT,
                             MPI_SUM,  &numOwnedComms, &myCommOffset);
  mpi->waitCollective(std::move(op));

  //this is an inclusive scan... we needed an exclusive scan
  myCommOffset -= numOwnedComms;

  int myCommId = myCommOffset;

  //now broadcast the IDs to everyone
  std::vector<CollectiveOpBase::ptr> ops(numComms);
  std::vector<int> globalIds(numComms);
  int idx = 0;
  for (auto& pair : comm_map){
    auto& list = pair.second;
    for (auto& comm : list){
      if (comm->is_root){
        comm->global_id = globalIds[idx] = myCommId;
        comm->group->global_id = comm->global_id + 1;
        ++myCommId;
      }
      ops[idx] = mpi->startBcast("OTF2_finalize_bcast", comm->local_id,
                                 1, MPI_INT, 0, &globalIds[idx]);
      ++idx;
    }
  }

  mpi->waitCollectives(std::move(ops));

  /** all global ids have been received, log them */
  idx = 0;
  for (auto& pair : comm_map){
    auto& list = pair.second;
    for (auto& comm : list){
      if (!comm->is_root){
        comm->global_id = globalIds[idx];
        comm->group->global_id = comm->global_id;
      }
      ++idx;
    }
  }

}

void
OTF2Writer::dumpLocalData()
{
  writer_.write_local_def_file();
  if (rank_ != 0){
    //still need rank 0 around
    writer_.close_archive();
  }
}

void
OTF2Writer::dumpGlobalData()
{
  if (rank_ != 0){
    spkt_abort_printf("main OTF2Writer::dumpGlobalData() called on rank %d != 0", rank_);
  }
  writer_.write_global_def_file(event_counts_, all_comms_, min_time_, max_time_);
  writer_.close_archive();
}

OTF2Output::OTF2Output(SST::Params &params)
 : first_in_grp_(nullptr),
   StatisticOutput(params)
{
}

void
OTF2Output::output(SST::Statistics::StatisticBase *statistic, bool endOfSimFlag)
{
  auto* writer = dynamic_cast<OTF2Writer*>(statistic);
  if (!writer){
    spkt_abort_printf("OTF2Output received non-OTF2 stat");
  }

  writer->dumpLocalData();

  if (!first_in_grp_) first_in_grp_ = writer;
  first_in_grp_->reduce(writer);

}

void
OTF2Output::startOutputGroup(SST::Statistics::StatisticGroup *grp)
{
  first_in_grp_ = nullptr;
}

void
OTF2Output::stopOutputGroup()
{
  first_in_grp_->dumpGlobalData();
  first_in_grp_ = nullptr;
}

}

#endif

