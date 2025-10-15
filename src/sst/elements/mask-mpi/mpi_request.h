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

#include <memory>
#include <iris/sumi/collective.h>
#include <mpi_status.h>
#include <mpi_message.h>
#include <mpi_comm/mpi_comm_fwd.h>
//#include <sstmac/common/sstmac_config.h>

#pragma once

namespace SST::MASKMPI {

/**
 * Persistent send operations (send, bsend, rsend, ssend)
 */
class PersistentOp
{
 public:
  /// The arguments.
  int count;
  MPI_Datatype datatype;
  MPI_Comm comm;
  int partner;
  int tag;
  void* content;
};

struct CollectiveOpBase
{
  using ptr = std::unique_ptr<CollectiveOpBase>;

  bool packed_send;
  bool packed_recv;
  void* sendbuf;
  void* recvbuf;
  void* tmp_sendbuf;
  void* tmp_recvbuf;
  int tag;
  MPI_Op op;
  MpiType* sendtype;
  MpiType* recvtype;
  SST::Iris::sumi::Collective::type_t ty;
  MpiComm* comm;
  int sendcnt;
  int recvcnt;
  int root;
  bool complete;

  friend class std::default_delete<CollectiveOpBase>;

 protected:
  virtual ~CollectiveOpBase(){}
  CollectiveOpBase(MpiComm* cm);

};

struct CollectiveOp :
  public CollectiveOpBase,
  public SST::Hg::thread_safe_new<CollectiveOp>
{
  using ptr = std::unique_ptr<CollectiveOp>;

  template <class... Args> static
  CollectiveOp::ptr create(Args&&...args){
    return CollectiveOp::ptr(new CollectiveOp(std::forward<Args>(args)...));
  }

 private:
  friend class std::default_delete<CollectiveOp>;
 ~CollectiveOp() override{}

  CollectiveOp(int count, MpiComm* comm);
  CollectiveOp(int sendcnt, int recvcnt, MpiComm* comm);
};

struct CollectivevOp :
  public CollectiveOpBase,
  public SST::Hg::thread_safe_new<CollectivevOp>
{
  using ptr = std::unique_ptr<CollectivevOp>;

  template <class... Args> static
  CollectivevOp::ptr create(Args&&...args){
    return CollectivevOp::ptr(new CollectivevOp(std::forward<Args>(args)...));
  }

  int* recvcounts;
  int* sendcounts;
  int* sdisps;
  int* rdisps;
  int size;

 private:
  friend class std::default_delete<CollectivevOp>;
  ~CollectivevOp() override{}

  CollectivevOp(int scnt, int* recvcnts, int* disps, MpiComm* comm);
  CollectivevOp(int* sendcnts, int* disps, int rcnt, MpiComm* comm);
  CollectivevOp(int* sendcnts, int* sdisps,
                int* recvcnts, int* rdisps, MpiComm* comm);
};

class MpiRequest :
  public SST::Hg::thread_safe_new<MpiRequest>
{
 public:
  typedef enum {
    Send,
    Recv,
    Collective,
    Probe
  } op_type_t;

  MpiRequest(op_type_t ty) :
   complete_(false),
   cancelled_(false),
   optype_(ty),
   persistent_op_(nullptr),
   collective_op_(nullptr)
  {
  }

  std::string toString() const {
    return "mpirequest";
  }

  std::string typeStr() const;

  static MpiRequest* construct(op_type_t ty){
    return new MpiRequest(ty);
  }

  ~MpiRequest();

  void complete(MpiMessage* msg);

  bool isComplete() const {
    return complete_;
  }

  void cancel() {
    cancelled_ = true;
    complete();
  }

  void complete() {
    complete_ = true;
  }

  void setComplete(bool flag){
    complete_ = flag;
  }

  void setPersistent(PersistentOp* op) {
    persistent_op_ = op;
  }

  PersistentOp* persistentData() const {
    return persistent_op_;
  }

  CollectiveOpBase* setCollective(CollectiveOpBase::ptr&& op) {
    collective_op_ = std::move(op);
    return collective_op_.get();
  }

  CollectiveOpBase* collectiveData() const {
    return collective_op_.get();
  }

  const MPI_Status& status() const {
    return stat_;
  }

  void setStatus(const MPI_Status& stat) {
    stat_ = stat;
  }

  bool isCancelled() const {
    return cancelled_;
  }

  bool isPersistent() const {
    return persistent_op_;
  }

  bool isCollective() const {
    return bool(collective_op_);
  }

  op_type_t optype() const {
    return optype_;
  }

  SST::Hg::Timestamp waitStart() const {
    return wait_start_;
  }

  void setWaitStart(SST::Hg::Timestamp t){
    wait_start_ = t;
  }

  bool activeWait() const {
    return !wait_start_.empty();
  }

 private:
  MPI_Status stat_;
  bool complete_;
  bool cancelled_;
  op_type_t optype_;

  PersistentOp* persistent_op_;
  CollectiveOpBase::ptr collective_op_;

  SST::Hg::Timestamp wait_start_;

};

}
