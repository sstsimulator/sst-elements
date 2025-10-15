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

#pragma once

#include <cstddef>
#include <mercury/operating_system/process/task_id.h>
#include <mpi_integers.h>
#include <cstring>
#include <vector>

namespace SST::MASKMPI {

using SST::Hg::TaskId;

class MpiGroup  {

 public:
  MpiGroup(const std::vector<TaskId>& tl);

  MpiGroup(size_t size);

  virtual ~MpiGroup() {}

  TaskId at(int rank);

  size_t size() const {
    return size_;
  }

  MPI_Group id() const {
    return id_;
  }

  void setId(MPI_Group grp){
    id_ = grp;
  }

  bool isCommWorld() const {
    return is_comm_world_;
  }

  const std::vector<TaskId>& worldRanks() const {
    return local_to_world_map_;
  }

  /**
   * @brief rank_of_task See if task exists in this group.
   *      If yes, return its rank within the group.
   *      If not, return MPI_UNDEFINED.
   * @param t The world task_id to find in the group
   * @return The local rank of task within the group.
   */
  int rankOfTask(TaskId t) const;

  void translateRanks(int n_ranks, const int* my_ranks, int* other_ranks, MpiGroup* other_grp);

 protected:
  //map the local gorup rank to the world rank
  std::vector<TaskId> local_to_world_map_;
  MPI_Group id_;
  size_t size_; //used for comm_world
  bool is_comm_world_;  //we don't save all the peers to save space

};

}
