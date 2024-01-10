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

#include <mpi_comm/mpi_comm.h>
#include <mpi_comm/keyval.h>
//#include <sprockit/debug.h>
#include <mercury/common/errors.h>
//#include <sprockit/statics.h>

//static sprockit::NeedDeletestatics<sumi::MpiComm> cleanup_comm;

namespace SST::MASKMPI {

MpiComm* MpiComm::comm_null = nullptr;

MpiComm::MpiComm() :
  Communicator(-1),
  group_(nullptr),
  next_collective_tag_(0),
  del_grp_(false),
  topotype_(TOPO_NONE),
  id_(MPI_COMM_NULL),
  rank_(-1)
{
}

MpiComm::~MpiComm()
{
  if (del_grp_) delete group_;
}

MpiComm::MpiComm(
  MPI_Comm id, //const appid &aid,
  int rank, MpiGroup* peers,
  AppId aid,
  bool del_grp,
  topotypes ty) :
  SST::Iris::sumi::Communicator(rank),
  group_(peers),
  next_collective_tag_(MPI_COMM_WORLD + 100),
  aid_(aid),
  del_grp_(del_grp),
  topotype_(ty),
  id_(id),
  rank_(rank)
{
  if (peers->size() == 0) {
    sst_hg_throw_printf(SST::Hg::ValueError,
         "trying to build communicator of size 0");
  }

  if (!comm_null) {
    comm_null = new MpiComm;
  }
}

std::set<int>
MpiComm::globalRankSetIntersection(const std::set<int> &neighbors) const
{
  if (group_->isCommWorld()){
    return neighbors;
  } else {
    std::set<int> intersc;
    for (int entry : group_->worldRanks()){
      if (neighbors.find(entry) != neighbors.end()){
        intersc.insert(entry);
      }
    }
    return intersc;
  }
}

std::set<int>
MpiComm::commNeighbors(const std::set<int>& commWorldNeighbors) const
{
  std::set<int> ret;
  for (int i=0; i < nproc(); ++i){
    int glbl = peerTask(i);
    auto iter = commWorldNeighbors.find(glbl);
    if (iter != commWorldNeighbors.end()){
      ret.insert(i);
    }
  }
  return ret;
}

void
MpiComm::deleteStatics()
{
  if (comm_null) delete comm_null;
}

int
MpiComm::globalToCommRank(int  /*global_rank*/) const
{
  SST::Hg::abort("mpi_comm::global_to_comm_rank");
  return 0;
}

void
MpiComm::dupKeyvals(MpiComm* m)
{
  std::unordered_map<int, keyval*>::iterator it, end = m->keyvals_.end();
  for (it = m->keyvals_.begin(); it != end; it++) {
    SST::Hg::abort("dup_keyvals: not implemented");
    //keyval* c = (it->second)->clone(keyval::get_new_key());
    //keyvals_[c->key()] = c;
  }
}

std::string
MpiComm::toString() const
{
  return SST::Hg::sprintf("mpicomm(id=%d,size=%d,rank=%d)", id_, size(), rank_);
}

int
MpiComm::size() const
{
  if (id_ == MPI_COMM_NULL) {
    SST::Hg::abort("mpicomm: trying to call size() on a null mpicomm");
  }
  if (!group_) {
    SST::Hg::abort("mpicomm: group is null for some reason in size()");
  }
  return group_->size();
}

void
MpiComm::setKeyval(keyval* k, void* val)
{
  keyvals_[k->key()] = k;
  k->set_val(val);
}

void
MpiComm::getKeyval(keyval* k, void* val, int* flag)
{

  if (keyvals_.find(k->key()) == keyvals_.end()) {
    *flag = 1;
  } else {
    *flag = 0;
  }

  void** vcast = (void**)val;

  *vcast = k->val();

  //memcpy(val, (k->val()), sizeof(void*));

  //val = (k->val());
}

//
// Get a unique tag for a collective operation.
//
int
MpiComm::nextCollectiveTag()
{
  uint16_t id = id_;
  int next_tag = (id << 16) | next_collective_tag_;
  next_collective_tag_++;
  return next_tag;
}

TaskId
MpiComm::myTask() const
{
  return group_->at(rank_);
}

TaskId
MpiComm::peerTask(int rank) const
{
  return group_->at(rank);
}

}
