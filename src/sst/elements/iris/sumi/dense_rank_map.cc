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
