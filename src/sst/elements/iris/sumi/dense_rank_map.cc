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

#include <iris/sumi/dense_rank_map.h>
#include <iris/sumi/communicator.h>
#include <mercury/common/errors.h>
#include <algorithm>

namespace SST::Iris::sumi {

DenseRankMap::DenseRankMap(const std::set<int>& failed,
  Communicator* dom) :
  num_failed_ranks_(failed.size()),
  failed_ranks_(0)
{
  init(failed, dom);
}

DenseRankMap::DenseRankMap() :
  num_failed_ranks_(0),
  failed_ranks_(0)
{
}

DenseRankMap::~DenseRankMap()
{
  if (failed_ranks_){
    delete[] failed_ranks_;
  }
}

void
DenseRankMap::init(const std::set<int>& failed, Communicator* dom)
{
  if (failed_ranks_){ //clear out old data
    delete[] failed_ranks_;
  }

  num_failed_ranks_ = failed.size();

  if (num_failed_ranks_ == 0)
    return;

  failed_ranks_ = new int[num_failed_ranks_];

  int idx = 0;
  for (int global_rank : failed){
    int comm_rank = dom ? dom->globalToCommRank(global_rank) : global_rank;
    failed_ranks_[idx++] = comm_rank;
  }

  if (dom){ //we have to sort the failed ranks, potentially
    //the failed rank stuff assumes ascending order
    std::sort(failed_ranks_, failed_ranks_ + num_failed_ranks_);
  }
}

int
DenseRankMap::linearFindRank(int sparse_rank) const
{
  for (int i=0; i < num_failed_ranks_; ++i){
    if (sparse_rank < failed_ranks_[i]){
        return sparse_rank - i;
    }
  }
  return sparse_rank - num_failed_ranks_;
}

int
DenseRankMap::sparseRank(int dense_rank) const
{
  int rank = dense_rank;
  for (int i=0; i < num_failed_ranks_; ++i){
    if (rank >= failed_ranks_[i]){
      ++rank;
    } else if (rank < failed_ranks_[i]){
      return rank; //all done, found it
    }
  }
  return rank;
}

int
DenseRankMap::denseRank(int sparse_rank) const
{
  if (num_failed_ranks_ == 0){
    return sparse_rank;
  } else if (num_failed_ranks_ <= tree_cutoff){
    return linearFindRank(sparse_rank);
  } else {
    return treeFindRank(sparse_rank,
        0, num_failed_ranks_, failed_ranks_);
  }
}

int
DenseRankMap::treeFindRank(
  int sparse_rank,
  int offset,
  int num_failed,
  int* failed_array) const
{
  //printf("Finding dense rank %d at offset=%d for num=%d on array=%p\n",
  //  sparse_rank, offset, num_failed, failed_array);

  if (num_failed == 1){
    if (sparse_rank > failed_array[0]){
      return sparse_rank - (offset + 1);
    } else if (sparse_rank < failed_array[0]){
      return sparse_rank - offset;
    } else {
      sst_hg_throw_printf(SST::Hg::ValueError,
        "dense_rank_map::trying to get dense rank for failed process %d",
        sparse_rank);
    }
  }

  int middle_index = num_failed / 2;
  if (sparse_rank < failed_array[middle_index]){
    return treeFindRank(sparse_rank, offset, middle_index, failed_array);
  } else if (sparse_rank > failed_array[middle_index]){
    return treeFindRank(sparse_rank, 
        offset +  middle_index, 
        num_failed - middle_index, 
        failed_array + middle_index);
  } else {
    sst_hg_throw_printf(SST::Hg::ValueError,
      "dense_rank_map::trying to get dense rank for failed process %d",
      sparse_rank);
  }

}

}
