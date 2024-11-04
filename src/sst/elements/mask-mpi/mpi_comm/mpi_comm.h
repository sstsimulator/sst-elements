/**
Copyright 2009-2024 National Technology and Engineering Solutions of Sandia,
LLC (NTESS).  Under the terms of Contract DE-NA-0003525, the U.S. Government
retains certain rights in this software.

Sandia National Laboratories is a multimission laboratory managed and operated
by National Technology and Engineering Solutions of Sandia, LLC., a wholly
owned subsidiary of Honeywell International, Inc., for the U.S. Department of
Energy's National Nuclear Security Administration under contract DE-NA0003525.

Copyright (c) 2009-2024, NTESS

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

#include <iris/sumi/communicator.h>
#include <mercury/common/errors.h>
#include <mercury/common/node_address.h>
#include <mercury/operating_system/process/task_id.h>
#include <mercury/operating_system/process/app_id.h>
#include <mercury/operating_system/process/software_id.h>
#include <mercury/common/errors.h>
#include <mpi_comm/keyval_fwd.h>
#include <mpi_comm/mpi_group.h>
#include <mpi_integers.h>
#include <mpi_request_fwd.h>
#include <unordered_map>
#include <string>
#include <map>

#pragma once

namespace SST::MASKMPI {

using SST::Hg::AppId;
using SST::Hg::SoftwareId;
using SST::Hg::NodeId;

/**
 * An MPI communicator handle.
 */
class MpiComm : public SST::Iris::sumi::Communicator
{
 public:
  friend class MpiApi;

  enum topotypes {
    TOPO_NONE, TOPO_GRAPH, TOPO_CART
  };

  static const int proc_null = -1;

 public:
  MpiComm();

  MpiComm(
    MPI_Comm id,
    int rank,
    MpiGroup* peers,
    AppId aid,
    bool del_grp = false,
    topotypes ty = TOPO_NONE);

  ~MpiComm() override;

  void setName(std::string name) {
    name_ = name;
  }

  std::string name() const {
    return name_;
  }

  static void deleteStatics();

  topotypes topoType() const {
    return topotype_;
  }

  MpiGroup* group() {
    return group_;
  }

  bool deleteGroup() const {
    return del_grp_;
  }

  bool supportsSmp() const override {
    return true;
  }

  std::set<int> globalRankSetIntersection(const std::set<int>& neighbors) const override;

  void dupKeyvals(MpiComm* m);

  static MpiComm* comm_null;

  std::string toString() const;

  int rank() const {
    return rank_;
  }

  int size() const;

  MPI_Comm id() const {
    return id_;
  }

  void setKeyval(keyval* k, void* val);

  void getKeyval(keyval* k, void* val, int* flag);

  AppId app() const {
    return aid_;
  }

  int commToGlobalRank(int comm_rank) const override {
    return int(peerTask(comm_rank));
  }

  std::set<int> commNeighbors(const std::set<int>& commWorldNeighbors) const;

  int globalToCommRank(int global_rank) const override;

  int nproc() const override {
    return size();
  }

  int nextCollectiveTag();

  TaskId myTask() const;

  TaskId peerTask(int rank) const;

  inline bool operator==(MpiComm* other) const {
    return ((rank_ == other->rank_) && (id_ == other->id_));
  }

  inline bool operator!=(MpiComm* other) const {
    return !this->operator==(other);
  }

  void addRequest(int tag, MpiRequest* req){
    ireqs_[tag] = req;
  }

  MpiRequest* getRequest(int tag) const {
    auto it = ireqs_.find(tag);
    if (it == ireqs_.end()){
      sst_hg_throw_printf(SST::Hg::ValueError,
          "cannot find tag %d on comm %d for returning collective MPI_Request",
          tag, id_);
    }
    return it->second;
  }

 private:
  friend class MpiCommFactory;

  void setId(MPI_Comm id){
    id_ = id;
  }

  /// The tasks participating in this communicator.  This is only used for an mpicomm* which is NOT WORLD_COMM.
  MpiGroup* group_;

  uint16_t next_collective_tag_;

  std::unordered_map<int, keyval*> keyvals_;

  AppId aid_;

  bool del_grp_;

  topotypes topotype_;

  std::string name_;

  std::map<int, MpiRequest*> ireqs_;

  MPI_Comm id_;

 protected:
  int rank_;

};

}
