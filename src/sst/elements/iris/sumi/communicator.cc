/**
Copyright 2009-2023 National Technology and Engineering Solutions of Sandia,
LLC (NTESS).  Under the terms of Contract DE-NA-0003525, the U.S. Government
retains certain rights in this software.

Sandia National Laboratories is a multimission laboratory managed and operated
by National Technology and Engineering Solutions of Sandia, LLC., a wholly
owned subsidiary of Honeywell International, Inc., for the U.S. Department of
Energy's National Nuclear Security Administration under contract DE-NA0003525.

Copyright (c) 2009-2023, NTESS

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

#include <sumi/communicator.h>
#include <sumi/transport.h>
#include <mercury/common/errors.h>

namespace SST::Iris::sumi {

void
Communicator::rankResolved(int global_rank, int comm_rank)
{
  for (RankCallback* cback : rank_callbacks_){
    cback->rankResolved(global_rank, comm_rank);
  }
}

void
Communicator::createSmpCommunicator(const std::set<int>& neighbors, CollectiveEngine *engine,
                                    int cq_id)
{
  if (!supportsSmp()) return;

  if (neighbors.size() == 1) return; //no smp parallelism

  auto neighbors_subset = globalRankSetIntersection(neighbors);
  if (neighbors_subset.size() == 1) return; //no smp parallelism


  int myGlobalRank = commToGlobalRank(my_comm_rank_);
  if (neighbors_subset.size() > 1){ //there is smp parallelism to be had here
    int idx = 0;
    int my_smp_rank = 0;
    std::vector<int> local_to_global(neighbors_subset.size());
    for (int glblRank : neighbors_subset){
      local_to_global[idx] = glblRank;
      if (glblRank == myGlobalRank){
        my_smp_rank = idx;
      }
      idx++;
    }



    smp_comm_ = new MapCommunicator(my_smp_rank, std::move(local_to_global));
    if (smp_comm_->nproc() == 0){
      sst_hg_abort_printf("Created SMP communicator of size 0!"
                        " Neighbor set has size %d, intersection with comm is %d",
                        int(neighbors.size()), int(neighbors_subset.size()));
    }

    std::vector<int> smp_ranks(this->nproc());
    int tag = -2;
// FIXME
//    engine->allgather(smp_ranks.data(), &my_smp_rank, 1, sizeof(int), tag, cq_id, this);
//    engine->blockUntilNext(cq_id);

    std::map<int,int> rank_counts;
    int my_owner_rank = -1;
    if (my_smp_rank == 0){
      std::vector<int> owner_to_global;
      idx = 0;
      for (int rank=0; rank < this->nproc(); ++rank){
        int local_smp_rank = smp_ranks[rank];
        rank_counts[local_smp_rank]++;
        if (local_smp_rank == 0){
          owner_to_global.push_back(commToGlobalRank(rank));
          if (rank == this->myCommRank()){
            my_owner_rank = idx;
          }
          ++idx;
        }
      }
      int nranks = idx;
      owner_comm_ = new IndexCommunicator(my_owner_rank, nranks, std::move(owner_to_global));
      if (owner_comm_->nproc() == 0){
        sst_hg_abort_printf("Created owner communicator of size 0!"
                          "Parent comm has size %d, SMP comm has size %d",
                          nproc(), smp_comm_->nproc());
      }
    }

    smp_balanced_ = true;
    int rank0_smp_count = rank_counts[0];
    for (auto& pair : rank_counts){
      if (pair.second != rank0_smp_count){
        smp_balanced_ = false;
        break;
      }
    }

  }



}

GlobalCommunicator::GlobalCommunicator(Transport *tport) :
  Communicator(tport->rank()),
  transport_(tport) 
{
}

int
GlobalCommunicator::nproc() const
{
  return transport_->nproc();
}

int
GlobalCommunicator::commToGlobalRank(int comm_rank) const
{
  return comm_rank;
}

int
GlobalCommunicator::globalToCommRank(int global_rank) const
{
  return global_rank;
}

MapCommunicator::MapCommunicator(int rank, std::vector<int>&& local_to_global)
 : Communicator(rank),
   local_to_global_(std::move(local_to_global))
{
  for (int idx=0; idx < local_to_global_.size(); ++idx){
    global_to_local_[local_to_global_[idx]] = idx;
  }
}

int
MapCommunicator::commToGlobalRank(int comm_rank) const
{
  return local_to_global_[comm_rank];

}

int
MapCommunicator::globalToCommRank(int global_rank) const
{
  auto iter = global_to_local_.find(global_rank);
  if (iter == global_to_local_.end()){
    sst_hg_abort_printf("Bad global rank %d requested for comm of size %d",
                      global_rank, global_to_local_.size());
  }
  return iter->second;
}

std::set<int>
MapCommunicator::globalRankSetIntersection(const std::set<int> &neighbors) const
{
  std::set<int> intersc;
  for (auto& pair : global_to_local_){
    int rank = pair.first;
    if (neighbors.find(rank) != neighbors.end()){
      intersc.insert(rank);
    }
  }
  return intersc;
}

int
IndexCommunicator::globalToCommRank(int  /*global_rank*/) const
{
  SST::Hg::abort("index_domain::global_to_comm_rank: this should only be involved in failures");
  return 0;
}

std::set<int>
IndexCommunicator::globalRankSetIntersection(const std::set<int>& neighbors) const
{
  std::set<int> intersc;
  for (int rank : proc_list_){
    if (neighbors.find(rank) != neighbors.end()){
      intersc.insert(rank);
    }
  }
  return intersc;
}

std::set<int>
SubrangeCommunicator::globalRankSetIntersection(const std::set<int> &neighbors) const
{
  std::set<int> intersc;
  int stop = start_ + nproc_;
  for (int nbr : neighbors){
    if (nbr >= start_ && nbr < stop){
      intersc.insert(nbr);
    }
  }
  return intersc;
}

std::set<int>
RotateCommunicator::globalRankSetIntersection(const std::set<int> & /*neighbors*/) const
{
  sst_hg_abort_printf("RotateCommunicator: does not support rank intersection");
  return std::set<int>{};
}

}
