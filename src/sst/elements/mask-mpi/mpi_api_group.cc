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

#include <mpi_api.h>
#include <mercury/components/operating_system.h>

namespace SST::MASKMPI {

int
MpiApi::groupRangeIncl(MPI_Group oldgrp, int n, int ranges[][3], MPI_Group* newgrp)
{
  std::vector<int> new_ranks;
  for (int i=0; i < n; ++i){
    int start = ranges[i][0];
    int stop = ranges[i][1];
    int stride = ranges[i][2];
    int rank = start;
    if (stride < 0){ //stride can be negativve
      if (start < stop){
        sst_hg_abort_printf("MPI_group_range_incl: negative stride, but start < stop");
      }
      while (rank >= stop){
        new_ranks.push_back(rank);
        rank += stride;
      }
    } else {
      if (stop < start){
        sst_hg_abort_printf("MPI_group_range_incl: positive stride, but stop < start");
      }
      while (rank <= stop){
        new_ranks.push_back(rank);
        rank += stride;
      }
    }
  }

  return groupIncl(oldgrp, new_ranks.size(), new_ranks.data(), newgrp);
}

int
MpiApi::groupIncl(MPI_Group oldgrp, int num_ranks, const int *ranks, MPI_Group *newgrp)
{
  MpiGroup* oldgrpPtr = getGroup(oldgrp);
  if (num_ranks > oldgrpPtr->size()) {
    sst_hg_abort_printf("MPI_Group_incl: invalid group size %d", num_ranks);
  }

  std::vector<TaskId> vec_ranks(num_ranks, TaskId(0));
  for (int i = 0; i < num_ranks; i++) {
    vec_ranks[i] = oldgrpPtr->at(ranks[i]);
  }
  MpiGroup* newgrpPtr = new MpiGroup(vec_ranks);
  addGroupPtr(newgrpPtr, newgrp);

//  mpi_api_debug(sprockit::dbg::mpi, "MPI_Group_incl(%d,%d,*%d)",
//                num_ranks, oldgrp, *newgrp);

  return MPI_SUCCESS;
}

bool
MpiApi::groupCreateWithId(MPI_Group group, int num_members, const int* members)
{
//  mpi_api_debug(sprockit::dbg::mpi, "MPI_Group_create_with_id(id=%d,n=%d)",
//                group, num_members);

  int my_rank = commWorld()->rank();
  bool in_group = false;
  for (int i=0; i < num_members; ++i){
    if (members[i] == my_rank){
      in_group = true;
      break;
    }
  }

  if (!in_group) return false;

  std::vector<TaskId> vec_ranks(num_members);
  for (int i=0; i < num_members; ++i){
    vec_ranks[i] = members[i];
  }
  MpiGroup* grpPtr = new MpiGroup(vec_ranks);
  addGroupPtr(grpPtr, &group);

  return true;
}

int
MpiApi::groupFree(MPI_Group *grp)
{
  //do not delete it, leave it around
  //forever and ever and ever
  if (*grp != MPI_GROUP_WORLD){
    auto iter = grp_map_.find(*grp);
    if (iter == grp_map_.end()){
      sst_hg_abort_printf("Invalid MPI_Group %d passed to group free", *grp);
    }
    delete iter->second;
    grp_map_.erase(iter);
  }
  *grp = MPI_GROUP_NULL;

  return MPI_SUCCESS;
}

int
MpiApi::groupTranslateRanks(MPI_Group grp1, int n, const int *ranks1, MPI_Group grp2, int *ranks2)
{
  MpiGroup* grp1ptr = getGroup(grp1);
  MpiGroup* grp2ptr = getGroup(grp2);
  grp1ptr->translateRanks(n, ranks1, ranks2, grp2ptr);


  return MPI_SUCCESS;
}

}

