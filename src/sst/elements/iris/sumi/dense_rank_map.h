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

#pragma once

#include <set>
#include <iris/sumi/communicator_fwd.h>

namespace SST::Iris::sumi {

/**
* @class dense_rank_map
* physical <= dense <= virtual
* sparse rank is a synonym for physical.
* It's the rank you started with when there were no failures
* A job starts with 3 nodes {0,1,2}.
* Node 1 dies. There are 2 live nodes.
* 0 -> 0
* 2 -> 1
*/
class DenseRankMap {

 public:
  int denseRank(int sparseRank) const;

  int sparseRank(int denseRank) const;

  DenseRankMap();

  DenseRankMap(const std::set<int>& failed, Communicator* dom = nullptr);

  ~DenseRankMap();

  void init(const std::set<int>& failed, Communicator* dom = nullptr);

 protected:
  static const int tree_cutoff = 4;

  /**
   * O(N) search algorithm for new rank for N = # failures
   * @param sparse_rank
   * @return
   */
  int linearFindRank(int sparseRank) const;

  /**
   * O(log N) search algorithm for new rank,
   * but with larger prefactor for N = # failures
   * @param sparse_rank
   * @return
   */
  int treeFindRank(
    int sparseRank,
    int offset,
    int num_failed_ranks,
    int* failed_array) const;

 protected:
  int num_failed_ranks_;
  int* failed_ranks_;

};

}
