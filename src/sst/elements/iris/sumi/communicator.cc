// Copyright 2009-2025 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2025, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// of the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include <iris/sumi/communicator.h>
#include <iris/sumi/transport.h>
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
