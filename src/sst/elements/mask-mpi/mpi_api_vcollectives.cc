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

#include <mpi_api.h>
#include <mpi_queue/mpi_queue.h>
//#include <sumi-mpi/otf2_output_stat.h>
#include <mercury/components/operating_system.h>
#include <mercury/operating_system/process/thread.h>
//#include <sstmac/software/process/ftq_scope.h>

//#define do_vcoll(coll, fxn, ...) \
//  StartMPICall(fxn); \
//  auto op = start##coll(#fxn, __VA_ARGS__); \
//  waitCollective(std::move(op)); \
//  FinishMPICall(fxn);

#define do_vcoll(coll, fxn, ...) \
  auto op = start##coll(#fxn, __VA_ARGS__); \
  waitCollective(std::move(op));

//#define start_vcoll(coll, fxn, ...) \
//  StartMPICall(fxn); \
//  auto op = start##coll(#fxn, __VA_ARGS__); \
//  addImmediateCollective(std::move(op), req); \
//  FinishMPICall(fxn)

#define start_vcoll(coll, fxn, ...) \
  auto op = start##coll(#fxn, __VA_ARGS__); \
  addImmediateCollective(std::move(op), req);

namespace SST::MASKMPI {

void
MpiApi::finishVcollectiveOp(CollectiveOpBase* op_)
{
  CollectivevOp* op = static_cast<CollectivevOp*>(op_);
  if (op->packed_recv){
    sst_hg_throw_printf(SST::Hg::UnimplementedError,
               "cannot handle non-contiguous types in collective %s",
               Iris::sumi::Collective::tostr(op->ty));
  }
  if (op->packed_send){
    sst_hg_throw_printf(SST::Hg::UnimplementedError,
               "cannot handle non-contiguous types in collective %s",
               Iris::sumi::Collective::tostr(op->ty));
  }
}

Iris::sumi::CollectiveDoneMessage*
MpiApi::startAllgatherv(CollectivevOp* op)
{
  return engine_->allgatherv(op->tmp_recvbuf, op->tmp_sendbuf,
                  op->recvcounts, op->sendtype->packed_size(), op->tag,
                  queue_->collCqId(), op->comm);

}

CollectiveOpBase::ptr
MpiApi::startAllgatherv(const char* name, MPI_Comm comm, int sendcount, MPI_Datatype sendtype,
                        const int *recvcounts, const int *displs, MPI_Datatype recvtype,
                        const void *sendbuf, void *recvbuf)
{
//  mpi_api_debug(sprockit::dbg::mpi | sprockit::dbg::mpi_collective,
//    "%s(%d,%s,<...>,%s,%s)", name,
//    sendcount, typeStr(sendtype).c_str(),
//    typeStr(recvtype).c_str(),
//    commStr(comm).c_str());

  auto op = CollectivevOp::create(sendcount, const_cast<int*>(recvcounts),
                                  const_cast<int*>(displs), getComm(comm));
  startMpiCollective(Iris::sumi::Collective::allgatherv, sendbuf, recvbuf, sendtype, recvtype, op.get());
  auto* msg = startAllgatherv(op.get());
  if (msg){
    op->complete = true;
    delete msg;
  }
  return std::move(op);
}

int
MpiApi::allgatherv(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                    void *recvbuf, const int *recvcounts, const int *displs,
                    MPI_Datatype recvtype, MPI_Comm comm)
{
#ifdef SST_HG_OTF2_ENABLED
  auto start_clock = traceClock();
#endif

  do_vcoll(Allgatherv, MPI_Allgatherv, comm, sendcount, sendtype,
           recvcounts, displs, recvtype, sendbuf, recvbuf);

#ifdef SST_HG_OTF2_ENABLED
  if (OTF2Writer_){
    OTF2Writer_->writer().mpi_allgatherv(start_clock, traceClock(),
       getComm(comm)->size(), sendcount, sendtype,
       recvcounts, recvtype, comm);
  }
#endif
  return MPI_SUCCESS;
}

int
MpiApi::allgatherv(int sendcount, MPI_Datatype sendtype,
                    const int *recvcounts, MPI_Datatype recvtype, MPI_Comm comm)
{
  return allgatherv(NULL, sendcount, sendtype, NULL, recvcounts, NULL, recvtype, comm);
}

int
MpiApi::iallgatherv(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                    void *recvbuf, const int *recvcounts, const int *displs,
                    MPI_Datatype recvtype, MPI_Comm comm, MPI_Request* req)
{
  start_vcoll(Allgatherv, MPI_Iallgatherv, comm, sendcount, sendtype,
           recvcounts, displs, recvtype, sendbuf, recvbuf);
  return MPI_SUCCESS;
}

int
MpiApi::iallgatherv(int sendcount, MPI_Datatype sendtype,
                     const int *recvcounts, MPI_Datatype recvtype,
                     MPI_Comm comm, MPI_Request* req)
{
  return iallgatherv(NULL, sendcount, sendtype,
                    NULL, recvcounts, NULL,
                    recvtype, comm, req);
}

Iris::sumi::CollectiveDoneMessage*
MpiApi::startAlltoallv(CollectivevOp* op)
{
  return engine_->alltoallv(op->tmp_recvbuf, op->tmp_sendbuf,
                  op->sendcounts, op->recvcounts,
                  op->sendtype->packed_size(), op->tag,
                  queue_->collCqId(), op->comm);
}

CollectiveOpBase::ptr
MpiApi::startAlltoallv(const char* name, MPI_Comm comm,
                       const int *sendcounts, MPI_Datatype sendtype, const int *sdispls,
                       const int *recvcounts, MPI_Datatype recvtype, const int *rdispls,
                       const void *sendbuf,  void *recvbuf)
{
  if (sendbuf || recvbuf){
//    mpi_api_debug(sprockit::dbg::mpi | sprockit::dbg::mpi_collective,
//      "%s(<...>,%s,<...>,%s,%s)", name,
//      typeStr(sendtype).c_str(), typeStr(recvtype).c_str(), commStr(comm).c_str());
    auto op = CollectivevOp::create(const_cast<int*>(sendcounts), const_cast<int*>(sdispls),
                                    const_cast<int*>(recvcounts), const_cast<int*>(rdispls),
                                    getComm(comm));
    startMpiCollective(Iris::sumi::Collective::alltoallv, sendbuf, recvbuf, sendtype, recvtype, op.get());
    auto* msg = startAlltoallv(op.get());
    if (msg){
      op->complete = true;
      delete msg;
    }
    return std::move(op);
  } else {
    MpiComm* commPtr = getComm(comm);
    int send_count = 0;
    int recv_count = 0;
    int nproc = commPtr->size();
    for (int i=0; i < nproc; ++i){
      send_count += sendcounts[i];
      recv_count += recvcounts[i];
    }
    send_count /= nproc;
    recv_count /= nproc;
    return startAlltoall(name, comm, send_count, sendtype,
                         recv_count, recvtype, sendbuf, recvbuf);
  }
}

int
MpiApi::alltoallv(const void *sendbuf, const int *sendcounts,
                   const int *sdispls, MPI_Datatype sendtype,
                   void *recvbuf, const int *recvcounts,
                   const int *rdispls, MPI_Datatype recvtype, MPI_Comm comm)
{
#ifdef SST_HG_OTF2_ENABLED
  auto start_clock = traceClock();
#endif

  do_vcoll(Alltoallv, MPI_Alltoallv, comm,
           sendcounts, sendtype, sdispls,
           recvcounts, recvtype, rdispls, sendbuf, recvbuf);

#ifdef SST_HG_OTF2_ENABLED
  if (OTF2Writer_){
    OTF2Writer_->writer().mpi_alltoallv(start_clock, traceClock(),
        getComm(comm)->size(), sendcounts, sendtype,
        recvcounts, recvtype, comm);
  }
#endif

  return MPI_SUCCESS;
}

int
MpiApi::alltoallv(const int *sendcounts, MPI_Datatype sendtype,
                   const int *recvcounts, MPI_Datatype recvtype, MPI_Comm comm)
{
  return alltoallv(NULL, sendcounts, NULL, sendtype,
                   NULL, recvcounts, NULL, recvtype, comm);
}

int
MpiApi::ialltoallv(const void *sendbuf, const int *sendcounts,
                   const int *sdispls, MPI_Datatype sendtype,
                   void *recvbuf, const int *recvcounts,
                   const int *rdispls, MPI_Datatype recvtype,
                   MPI_Comm comm, MPI_Request* req)
{
  start_vcoll(Alltoallv, MPI_Ialltoallv, comm, sendcounts, sendtype, sdispls,
              recvcounts, recvtype, rdispls, sendbuf, recvbuf);
  return MPI_SUCCESS;
}

int
MpiApi::ialltoallv(const int *sendcounts, MPI_Datatype sendtype,
                   const int *recvcounts, MPI_Datatype recvtype,
                   MPI_Comm comm, MPI_Request* req)
{
  return ialltoallv(NULL, sendcounts, NULL, sendtype,
                    NULL, recvcounts, NULL, recvtype,
                    comm, req);
}

int
MpiApi::ialltoallw(const void * /*sendbuf*/, const int  /*sendcounts*/[],
                    const int  /*sdispls*/[], const MPI_Datatype  /*sendtypes*/[],
                    void * /*recvbuf*/, const int  /*recvcounts*/[],
                    const int  /*rdispls*/[], const MPI_Datatype  /*recvtypes*/[],
                    MPI_Comm  /*comm*/, MPI_Request * /*request*/)
{
  SST::Hg::abort("MPI_Ialltoallw: unimplemented"
                  "but, seriously, why are you using this collective anyway?");
  return MPI_SUCCESS;
}

Iris::sumi::CollectiveDoneMessage*
MpiApi::startGatherv(CollectivevOp*  /*op*/)
{
  SST::Hg::abort("sumi::gatherv: not implemented");
  return nullptr;
  //transport::gatherv(op->tmp_recvbuf, op->tmp_sendbuf,
  //                     op->sendcnt, typeSize, op->tag,
  //                false, options::initial_context, op->comm);
}

CollectiveOpBase::ptr
MpiApi::startGatherv(const char* name, MPI_Comm comm, int sendcount, MPI_Datatype sendtype, int root,
                     const int *recvcounts, const int *displs, MPI_Datatype recvtype,
                     const void *sendbuf, void *recvbuf)
{
  if (sendbuf || recvbuf){
//    mpi_api_debug(sprockit::dbg::mpi,
//      "%s(%d,%s,<...>,%s,%d,%s)", name,
//      sendcount, typeStr(sendtype).c_str(),
//      typeStr(recvtype).c_str(),
//      int(root), commStr(comm).c_str());
    auto op = CollectivevOp::create(sendcount,
                const_cast<int*>(recvcounts),
                const_cast<int*>(displs), getComm(comm));

    op->root = root;
    if (root == op->comm->rank()){
      //pass
    } else {
      recvtype = MPI_DATATYPE_NULL;
      recvbuf = nullptr;
    }

    startMpiCollective(Iris::sumi::Collective::gatherv, sendbuf, recvbuf, sendtype, recvtype, op.get());
    auto* msg = startGatherv(op.get());
    if (msg){
      op->complete = true;
      delete msg;
    }
    return std::move(op);
  } else {
    MpiComm* commPtr = getComm(comm);
    int recvcount = sendcount;
    if (commPtr->rank() == root){
      int total_count = 0;
      int nproc = commPtr->size();
      for (int i=0; i < nproc; ++i){
        total_count += recvcounts[i];
      }
      recvcount = total_count / nproc;
    }
    auto op = startGather(name, comm, sendcount, sendtype, root, recvcount, recvtype,
                          sendbuf, recvbuf);
    return op;
  }
}

int
MpiApi::gatherv(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                 void *recvbuf, const int *recvcounts, const int *displs,
                 MPI_Datatype recvtype, int root, MPI_Comm comm)
{
#ifdef SST_HG_OTF2_ENABLED
  auto start_clock = traceClock();
#endif

  do_vcoll(Gatherv, MPI_Gatherv, comm, sendcount, sendtype, root,
           recvcounts, displs, recvtype, sendbuf, recvbuf);

#ifdef SST_HG_OTF2_ENABLED
  if (OTF2Writer_){
    OTF2Writer_->writer().mpi_gatherv(start_clock, traceClock(),
      getComm(comm)->size(), sendcount, sendtype,
      recvcounts, recvtype, root, comm);
  }
#endif

  return MPI_SUCCESS;
}

int
MpiApi::gatherv(int sendcount, MPI_Datatype sendtype,
                 const int *recvcounts, MPI_Datatype recvtype,
                 int root, MPI_Comm comm)
{
  return gatherv(NULL, sendtype, sendcount, NULL, recvcounts, NULL, recvtype, root, comm);
}

int
MpiApi::igatherv(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                 void *recvbuf, const int *recvcounts, const int *displs,
                 MPI_Datatype recvtype, int root, MPI_Comm comm, MPI_Request* req)
{
  start_vcoll(Gatherv, MPI_Igatherv, comm, sendcount, sendtype, root,
              recvcounts, displs, recvtype, sendbuf, recvbuf);
  return MPI_SUCCESS;
}

int
MpiApi::igatherv(int sendcount, MPI_Datatype sendtype,
                 const int *recvcounts, MPI_Datatype recvtype,
                 int root, MPI_Comm comm, MPI_Request* req)
{
  return igatherv(NULL, sendcount, sendtype, NULL,
                  recvcounts, NULL, recvtype, root, comm, req);
}

Iris::sumi::CollectiveDoneMessage*
MpiApi::startScatterv(CollectivevOp*  /*op*/)
{
  SST::Hg::abort("sumi::scatterv: not implemented");
  return nullptr;
  //transport::allgatherv(op->tmp_recvbuf, op->tmp_sendbuf,
  //                     op->sendcnt, typeSize, op->tag,
  //                false, options::initial_context, op->comm);
}

CollectiveOpBase::ptr
MpiApi::startScatterv(const char* name, MPI_Comm comm, const int *sendcounts, MPI_Datatype sendtype, int root,
                      const int *displs, int recvcount, MPI_Datatype recvtype, const void *sendbuf, void *recvbuf)
{
  if (sendbuf || recvbuf){
//    mpi_api_debug(sprockit::dbg::mpi,
//      "%s(<...>,%s,%d,%s,%d,%s)", name,
//      typeStr(sendtype).c_str(),
//      recvcount, typeStr(recvtype).c_str(),
//      int(root), commStr(comm).c_str());

    auto op = CollectivevOp::create(const_cast<int*>(sendcounts),
                      const_cast<int*>(displs), recvcount, getComm(comm));
    op->root = root;
    if (root == op->comm->rank()){
      //pass
    } else {
      sendtype = MPI_DATATYPE_NULL;
      sendbuf = nullptr;
    }

    startMpiCollective(Iris::sumi::Collective::scatterv, sendbuf, recvbuf, sendtype, recvtype, op.get());
    auto* msg = startScatterv(op.get());
    if (msg){
      op->complete = true;
      delete msg;
    }
    //move required by some compilers, despite RVO
    return std::move(op);
  } else {
    MpiComm* commPtr = getComm(comm);
    int sendcount = recvcount;
    if (commPtr->rank() == root){
      int total_count = 0;
      int nproc = commPtr->size();
      for (int i=0; i < nproc; ++i){
        total_count += sendcounts[i];
      }
      sendcount = total_count / nproc;
    }
    return startScatter(name, comm, sendcount, sendtype, root,
                        recvcount, recvtype, sendbuf, recvbuf);
  }
}

int
MpiApi::scatterv(const void* sendbuf, const int* sendcounts, const int *displs, MPI_Datatype sendtype,
                  void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm)
{
#ifdef SST_HG_OTF2_ENABLED
  auto start_clock = traceClock();
#endif

  do_vcoll(Scatterv, MPI_Scatterv, comm, sendcounts, sendtype, root, displs,
           recvcount, recvtype, sendbuf, recvbuf);

#ifdef SST_HG_OTF2_ENABLED
  if (OTF2Writer_){
    OTF2Writer_->writer().mpi_scatterv(start_clock, traceClock(),
      getComm(comm)->size(), sendcounts, sendtype,
      recvcount, recvtype, root, comm);
  }
#endif

  return MPI_SUCCESS;
}

int
MpiApi::scatterv(const int *sendcounts, MPI_Datatype sendtype,
                  int recvcount, MPI_Datatype recvtype,
                  int root, MPI_Comm comm)
{
  return scatterv(NULL, sendcounts, NULL, sendtype, NULL,
                  recvcount, recvtype, root, comm);
}


int
MpiApi::iscatterv(const void* sendbuf, const int* sendcounts, const int *displs, MPI_Datatype sendtype,
                  void *recvbuf, int recvcount, MPI_Datatype recvtype,
                  int root, MPI_Comm comm, MPI_Request* req)
{
  start_vcoll(Scatterv, MPI_Iscatterv, comm, sendcounts, sendtype, root, displs,
              recvcount, recvtype, sendbuf, recvbuf);
  return MPI_SUCCESS;
}

int
MpiApi::iscatterv(const int *sendcounts, MPI_Datatype sendtype,
                  int recvcount, MPI_Datatype recvtype,
                  int root, MPI_Comm comm, MPI_Request* req)
{
  return iscatterv(NULL, sendcounts, NULL, sendtype,
                   NULL, recvcount, recvtype, root, comm, req);
}

}
