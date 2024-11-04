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
#//include <mercury/operating_system/process/ftq_scope.h>

//#define do_coll(coll, fxn, ...) \
//  StartMPICall(fxn); \
//  auto op = start##coll(#fxn, __VA_ARGS__); \
//  waitCollective(std::move(op)); \
//  FinishMPICall(fxn);

#define do_coll(coll, fxn, ...) \
  auto op = start##coll(#fxn, __VA_ARGS__); \
  waitCollective(std::move(op));

//#define start_coll(coll, fxn, ...) \
//  StartMPICall(fxn); \
//  auto op = start##coll(#fxn, __VA_ARGS__); \
//  addImmediateCollective(std::move(op), req); \
//  FinishMPICall(fxn)

#define start_coll(coll, fxn, ...) \
  auto op = start##coll(#fxn, __VA_ARGS__); \
  addImmediateCollective(std::move(op), req);

using SST::Iris::sumi::Collective;

namespace SST::MASKMPI {

MpiRequest*
MpiApi::addImmediateCollective(CollectiveOpBase::ptr&& op)
{
  MpiRequest* reqPtr = MpiRequest::construct(MpiRequest::Collective);
  op->comm->addRequest(op->tag, reqPtr);
  if (op->complete){
    finishCollective(op.get());
    reqPtr->complete();
  }
  reqPtr->setCollective(std::move(op));
  return reqPtr;
}

void
MpiApi::addImmediateCollective(CollectiveOpBase::ptr&& op, MPI_Request* req)
{
  auto* reqPtr = addImmediateCollective(std::move(op));
  addRequestPtr(reqPtr, req);
}

void
MpiApi::startMpiCollective(Iris::sumi::Collective::type_t ty,
                           const void *sendbuf, void *recvbuf,
                           MPI_Datatype sendtype, MPI_Datatype recvtype,
                           CollectiveOpBase* op)
{  
  op->ty = ty;
  op->sendbuf = const_cast<void*>(sendbuf);
  op->recvbuf = recvbuf;

  if (sendbuf == MPI_IN_PLACE){
    if (recvbuf){
      MpiType* type = typeFromId(recvtype);
      int offset;
      switch(ty){
        case Iris::sumi::Collective::gather:
        case Iris::sumi::Collective::allgather:
          offset = type->extent() * op->recvcnt * op->comm->rank();
          break;
        default:
          offset = 0;
          break;
      }
      op->sendbuf = ((char*)recvbuf) + offset;
    }
    op->sendcnt = op->recvcnt;
    sendtype = recvtype;
  }

  if (sendtype != recvtype){
    if (sendtype == MPI_DATATYPE_NULL || sendtype == MPI_NULL){
      sendtype = recvtype;
      op->sendcnt = op->recvcnt;
      if (sendbuf != MPI_IN_PLACE) op->sendbuf = nullptr;
    } else if (recvtype == MPI_DATATYPE_NULL || recvtype == MPI_NULL){
      recvtype = sendtype;
      op->recvcnt = op->sendcnt;
      if (recvbuf != MPI_IN_PLACE) op->recvbuf = nullptr;
    }
  }

  op->tmp_sendbuf = op->sendbuf;
  op->tmp_recvbuf = op->recvbuf;

  op->sendtype = typeFromId(sendtype);
  op->recvtype = typeFromId(recvtype);
  op->packed_recv = false;
  op->packed_send = false;

  if (op->sendbuf && !op->sendtype->contiguous()){
    void* newbuf = allocateTempPackBuffer(op->sendcnt, op->sendtype);
    op->sendtype->packSend(op->sendbuf, newbuf, op->sendcnt);
    op->tmp_sendbuf = newbuf;
    op->packed_send = true;
  } else {
    op->tmp_sendbuf = op->sendbuf;
  }

  if (op->recvbuf && !op->recvtype->contiguous()){
    void* newbuf = allocateTempPackBuffer(op->recvcnt, op->recvtype);
    op->tmp_recvbuf = newbuf;
    op->packed_recv = true;
  } else {
    op->tmp_recvbuf = recvbuf;
  }


}

void*
MpiApi::allocateTempPackBuffer(int count, MpiType* type)
{
  char* newbuf = new char[type->packed_size()*count];
  return newbuf;
}

void
MpiApi::freeTempPackBuffer(void* srcbuf)
{
  char* buf = (char*) srcbuf;
  delete[] buf;
}

void
MpiApi::finishCollectiveOp(CollectiveOpBase* op_)
{
  CollectiveOp* op = static_cast<CollectiveOp*>(op_);
//  mpi_api_debug(sprockit::dbg::mpi_collective,
//                "finishing op on tag %d for collective %s: packed=(%d,%d)",
//                op->tag, Collective::tostr(op->ty),
//                op->packed_send, op->packed_recv);

  if (op->packed_recv){
    op->recvtype->unpack_recv(op->tmp_recvbuf, op->recvbuf, op->recvcnt);
    freeTempPackBuffer(op->tmp_recvbuf);
  }
  if (op->packed_send){
    freeTempPackBuffer(op->tmp_sendbuf);
  }
}

void
MpiApi::finishCollective(CollectiveOpBase* op)
{
  switch(op->ty){
    case Iris::sumi::Collective::donothing:
      sst_hg_abort_printf("do nothing collective should not call finishCollective");
    case Collective::reduce:
    case Collective::alltoall:
    case Collective::gather:
    case Collective::scatter:
    case Collective::allreduce:
    case Collective::scan:
    case Collective::allgather:
    case Collective::barrier:
    case Collective::reduce_scatter:
    case Collective::bcast:
      finishCollectiveOp(op);
      break;
    case Collective::alltoallv:
    case Collective::gatherv:
    case Collective::scatterv:
    case Collective::allgatherv:
      finishVcollectiveOp(op);
      break;
  }
}

void
MpiApi::waitCollectives(std::vector<CollectiveOpBase::ptr>&& ops)
{
  std::vector<MpiRequest*> reqs;
  for (auto&& op : ops){
    reqs.emplace_back(addImmediateCollective(std::move(op)));
  }

  for (auto* req : reqs){
    if (!req->isComplete()){
      queue_->progressLoop(req);
    }
    delete req;
  }
}

void
MpiApi::waitCollective(CollectiveOpBase::ptr&& op)
{
  bool is_comm_world = op->comm->id() == MPI_COMM_WORLD;
  MpiRequest* req = addImmediateCollective(std::move(op));
  if (!req->isComplete()){
    queue_->progressLoop(req);
  }
  if (is_comm_world) {
    crossed_comm_world_barrier_ = true;
  }
  delete req;
}

Iris::sumi::CollectiveDoneMessage*
MpiApi::startAllgather(CollectiveOp *op)
{
  return engine_->allgather(op->tmp_recvbuf, op->tmp_sendbuf,
                  op->sendcnt, op->sendtype->packed_size(), op->tag,
                  queue_->collCqId(), op->comm);
}

CollectiveOpBase::ptr
MpiApi::startAllgather(const char* name, MPI_Comm comm, int sendcount, MPI_Datatype sendtype,
                         int recvcount, MPI_Datatype recvtype, const void *sendbuf, void *recvbuf)
{
//  mpi_api_debug(sprockit::dbg::mpi | sprockit::dbg::mpi_collective,
//    "%s(%d,%s,%d,%s,%s)", name,
//    sendcount, typeStr(sendtype).c_str(),
//    recvcount, typeStr(recvtype).c_str(),
//    commStr(comm).c_str());

  auto op = CollectiveOp::create(sendcount, recvcount, getComm(comm));
  startMpiCollective(Collective::allgather, sendbuf, recvbuf, sendtype, recvtype, op.get());
  auto* msg = startAllgather(op.get());
  if (msg){
    op->complete = true;
    delete msg;
  }
  return std::move(op);
}

int
MpiApi::allgather(int sendcount, MPI_Datatype sendtype,
                   int recvcount, MPI_Datatype recvtype, MPI_Comm comm)
{
  return allgather(NULL, sendcount, sendtype, NULL, recvcount, recvtype, comm);
}

int
MpiApi::allgather(int count, MPI_Datatype type, MPI_Comm comm){
  return allgather(count, type, count, type, comm);
}


int
MpiApi::allgather(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                   void *recvbuf, int recvcount, MPI_Datatype recvtype, MPI_Comm comm)
{
#ifdef SST_HG_OTF2_ENABLED
  auto start_clock = traceClock();
#endif

  do_coll(Allgather, MPI_Allgather, comm,
          sendcount, sendtype, recvcount, recvtype,
          sendbuf, recvbuf);

#ifdef SST_HG_OTF2_ENABLED
  if (OTF2Writer_){
    OTF2Writer_->writer().mpi_allgather(start_clock, traceClock(),
            sendcount, sendtype, recvcount, recvtype, comm);
  }
#endif

  return MPI_SUCCESS;

}

int
MpiApi::iallgather(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                    void *recvbuf, int recvcount, MPI_Datatype recvtype,
                    MPI_Comm comm, MPI_Request *req)
{
  start_coll(Allgather, MPI_Iallgather, comm,
             sendcount, sendtype,
             recvcount, recvtype,
             sendbuf, recvbuf);
  return MPI_SUCCESS;
}

int
MpiApi::iallgather(int sendcount, MPI_Datatype sendtype,
                    int recvcount, MPI_Datatype recvtype,
                    MPI_Comm comm, MPI_Request *req)
{
  return iallgather(NULL, sendcount, sendtype, NULL, recvcount, recvtype, comm, req);
}

Iris::sumi::CollectiveDoneMessage*
MpiApi::startAlltoall(CollectiveOp* op)
{
  return engine_->alltoall(op->tmp_recvbuf, op->tmp_sendbuf, op->sendcnt,
                      op->sendtype->packed_size(), op->tag,
                      queue_->collCqId(), op->comm);
}

CollectiveOpBase::ptr
MpiApi::startAlltoall(const char* name, MPI_Comm comm, int sendcount, MPI_Datatype sendtype,
                      int recvcount, MPI_Datatype recvtype, const void *sendbuf, void *recvbuf)
{
//  mpi_api_debug(sprockit::dbg::mpi | sprockit::dbg::mpi_collective,
//    "%s(%d,%s,%d,%s,%s)", name,
//    sendcount, typeStr(sendtype).c_str(),
//    recvcount, typeStr(recvtype).c_str(),
//    commStr(comm).c_str());

  auto op = CollectiveOp::create(sendcount, recvcount, getComm(comm));
  startMpiCollective(Collective::alltoall, sendbuf, recvbuf, sendtype, recvtype, op.get());
  auto* msg = startAlltoall(op.get());
  if (msg){
    op->complete = true;
    delete msg;
  }
  return std::move(op);
}

int
MpiApi::alltoall(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                 void *recvbuf, int recvcount, MPI_Datatype recvtype,
                 MPI_Comm comm)
{
#ifdef SST_HG_OTF2_ENABLED
  auto start_clock = traceClock();
#endif

  do_coll(Alltoall, MPI_Alltoall, comm,
         sendcount, sendtype,
         recvcount, recvtype,
         sendbuf, recvbuf);

#ifdef SST_HG_OTF2_ENABLED
  if (OTF2Writer_){
    OTF2Writer_->writer().mpi_alltoall(start_clock, traceClock(),
                           sendcount, sendtype, recvcount, recvtype, comm);
  }
#endif

  return MPI_SUCCESS;
}


int
MpiApi::alltoall(int sendcount, MPI_Datatype sendtype,
                  int recvcount, MPI_Datatype recvtype, MPI_Comm comm)
{
  return alltoall(NULL, sendcount, sendtype, NULL, recvcount, recvtype, comm);
}

int
MpiApi::ialltoall(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                  void *recvbuf, int recvcount, MPI_Datatype recvtype,
                  MPI_Comm comm, MPI_Request* req)
{
  start_coll(Alltoall, MPI_Ialltoall, comm,
             sendcount, sendtype,
             recvcount, recvtype,
             sendbuf, recvbuf);
  return MPI_SUCCESS;
}

int
MpiApi::ialltoall(int sendcount, MPI_Datatype sendtype,
                  int recvcount, MPI_Datatype recvtype,
                   MPI_Comm comm, MPI_Request* req)
{
  return ialltoall(NULL, sendcount, sendtype, NULL,
                   recvcount, recvtype, comm, req);
}

Iris::sumi::CollectiveDoneMessage*
MpiApi::startAllreduce(CollectiveOp* op)
{
  Iris::sumi::reduce_fxn fxn = getCollectiveFunction(op);
  return engine_->allreduce(op->tmp_recvbuf, op->tmp_sendbuf, op->sendcnt,
                       op->sendtype->packed_size(), op->tag,
                       fxn, queue_->collCqId(), op->comm);
}

CollectiveOpBase::ptr
MpiApi::startAllreduce(MpiComm* commPtr, int count, MPI_Datatype type,
                       MPI_Op mop, const void* src, void* dst)
{
  auto op = CollectiveOp::create(count, commPtr);
  if (src == MPI_IN_PLACE){
    src = dst;
  }

  op->op = mop;
  startMpiCollective(Collective::allreduce, src, dst, type, type, op.get());
  auto* msg = startAllreduce(op.get());
  if (msg){
    op->complete = true;
    delete msg;
  }
  return std::move(op);
}


CollectiveOpBase::ptr
MpiApi::startAllreduce(const char* name, MPI_Comm comm, int count, MPI_Datatype type,
                       MPI_Op mop, const void* src, void* dst)
{
//  mpi_api_debug(sprockit::dbg::mpi | sprockit::dbg::mpi_collective,
//    "%s(%d,%s,%s)", name,
//    count, typeStr(type).c_str(),
//    commStr(comm).c_str());

  return startAllreduce(getComm(comm), count, type, mop, src, dst);
}

int
MpiApi::allreduce(const void *src, void *dst, int count,
                   MPI_Datatype type, MPI_Op mop, MPI_Comm comm)
{
#ifdef SST_HG_OTF2_ENABLED
  auto start_clock = traceClock();
#endif

  do_coll(Allreduce, MPI_Allreduce, comm,
           count, type, mop, src, dst);

#ifdef SST_HG_OTF2_ENABLED
  if (OTF2Writer_){
    OTF2Writer_->writer().mpi_allreduce(start_clock, traceClock(),
                            count, type, comm);
  }
#endif

  return MPI_SUCCESS;
}

int
MpiApi::allreduce(int count, MPI_Datatype type, MPI_Op op, MPI_Comm comm)
{
  return allreduce(NULL, NULL, count, type, op, comm);
}

int
MpiApi::iallreduce(const void *src, void *dst, int count,
                   MPI_Datatype type, MPI_Op mop,
                    MPI_Comm comm, MPI_Request* req)
{
  start_coll(Allreduce, MPI_Iallreduce,
              comm, count, type, mop, src, dst);
  return MPI_SUCCESS;
}

int
MpiApi::iallreduce(int count, MPI_Datatype type, MPI_Op op,
                    MPI_Comm comm, MPI_Request* req)
{
  return iallreduce(NULL, NULL, count, type, op, comm, req);
}

Iris::sumi::CollectiveDoneMessage*
MpiApi::startBarrier(CollectiveOp* op)
{
  op->ty = Collective::barrier;
  return engine_->barrier(op->tag, queue_->collCqId(), op->comm);
}

CollectiveOpBase::ptr
MpiApi::startBarrier(const char* name, MPI_Comm comm)
{
  auto op = CollectiveOp::create(0, getComm(comm));
//  mpi_api_debug(sprockit::dbg::mpi, "%s(%s) on tag %d",
//    name, commStr(comm).c_str(), int(op->tag));
  auto* msg = startBarrier(op.get());
  if (msg){
    op->complete = true;
    delete msg;
  }
  return std::move(op);
}

int
MpiApi::barrier(MPI_Comm comm)
{
#ifdef SST_HG_OTF2_ENABLED
  auto start_clock = traceClock();
#endif

  //StartMPICall(MPI_Barrier);
  waitCollective( startBarrier("MPI_Barrier", comm) );
  //FinishMPICall(MPI_Barrier);

#ifdef SST_HG_OTF2_ENABLED
  if(OTF2Writer_) {
    OTF2Writer_->writer().mpi_barrier(start_clock, traceClock(), comm);
  }
#endif

  return MPI_SUCCESS;
}

int
MpiApi::ibarrier(MPI_Comm comm, MPI_Request *req)
{
  //StartMPICall(MPI_Ibarrier);
  addImmediateCollective(startBarrier("MPI_Ibarrier", comm), req);
  //FinishMPICall(MPI_Ibarrier);
  return MPI_SUCCESS;
}

Iris::sumi::CollectiveDoneMessage*
MpiApi::startBcast(CollectiveOp* op)
{
  void* buf = op->comm->rank() == op->root ? op->tmp_sendbuf : op->tmp_recvbuf;
  return engine_->bcast(op->root, buf, op->sendcnt,
                 op->sendtype->packed_size(), op->tag,
                 queue_->collCqId(), op->comm);
}

CollectiveOpBase::ptr
MpiApi::startBcast(const char* name, MPI_Comm comm, int count, MPI_Datatype datatype, int root, void *buffer)
{
//  mpi_api_debug(sprockit::dbg::mpi | sprockit::dbg::mpi_collective,
//    "%s(%d,%s,%d,%s)", name,
//    count, typeStr(datatype).c_str(),
//    root, commStr(comm).c_str());

  auto op = CollectiveOp::create(count, getComm(comm));
  void* sendbuf, *recvbuf;
  op->root = root;
  MPI_Datatype sendtype, recvtype;
  if (op->comm->rank() == root){
    sendbuf = buffer;
    recvbuf = nullptr;
    sendtype = datatype;
    recvtype = MPI_DATATYPE_NULL;
  } else {
    sendbuf = nullptr;
    recvbuf = buffer;
    sendtype = MPI_DATATYPE_NULL;
    recvtype = datatype;
  }

  startMpiCollective(Collective::bcast, sendbuf, recvbuf, sendtype, recvtype, op.get());
  auto* msg = startBcast(op.get());
  if (msg){
    op->complete = true;
    delete msg;
  }
  return std::move(op);
}

int
MpiApi::bcast(void* buffer, int count, MPI_Datatype type, int root, MPI_Comm comm)
{
#ifdef SST_HG_OTF2_ENABLED
  auto start_clock = traceClock();
#endif
  do_coll(Bcast, MPI_Bcast, comm,
           count, type, root, buffer);

#ifdef SST_HG_OTF2_ENABLED
  if(OTF2Writer_) {
    MpiComm* commPtr = getComm(comm);
    OTF2Writer_->writer().mpi_bcast(start_clock, traceClock(),
        count, type, root, comm);
  }
#endif

  return MPI_SUCCESS;
}

int
MpiApi::bcast(int count, MPI_Datatype datatype, int root, MPI_Comm comm)
{
  return bcast(NULL, count, datatype, root, comm);
}

int
MpiApi::ibcast(void* buffer, int count, MPI_Datatype type, int root,
                MPI_Comm comm, MPI_Request* req)
{

  start_coll(Bcast, MPI_Ibcast, comm, count, type, root, buffer);
  return MPI_SUCCESS;
}

int
MpiApi::ibcast(int count, MPI_Datatype datatype, int root,
                MPI_Comm comm, MPI_Request* req)
{
  return ibcast(NULL, count, datatype, root, comm, req);
}

Iris::sumi::CollectiveDoneMessage*
MpiApi::startGather(CollectiveOp* op)
{
  return engine_->gather(op->root, op->tmp_recvbuf, op->tmp_sendbuf, op->sendcnt,
                    op->sendtype->packed_size(), op->tag,
                    queue_->collCqId(), op->comm);
}

CollectiveOpBase::ptr
MpiApi::startGather(const char* name, MPI_Comm comm, int sendcount, MPI_Datatype sendtype, int root,
                      int recvcount, MPI_Datatype recvtype, const void *sendbuf, void *recvbuf)
{
//  mpi_api_debug(sprockit::dbg::mpi | sprockit::dbg::mpi_collective,
//    "%s(%d,%s,%d,%s,%s)", name,
//    sendcount, typeStr(sendtype).c_str(),
//    recvcount, typeStr(recvtype).c_str(),
//    commStr(comm).c_str());

  if (sendbuf == MPI_IN_PLACE){
    if (recvbuf){
      MpiType* type = typeFromId(recvtype);
      MpiComm* cm = getComm(comm);
      int offset = type->extent() * recvcount * cm->rank();
      sendbuf = ((char*)recvbuf) + offset;
    }
    sendcount = recvcount;
    sendtype = recvtype;
  }

  auto op = CollectiveOp::create(sendcount, recvcount, getComm(comm));
  op->root = root;

  if (root == op->comm->rank()){
    //pass
  } else {
    recvtype = MPI_DATATYPE_NULL;
    recvbuf = nullptr;
  }

  startMpiCollective(Collective::gather, sendbuf, recvbuf, sendtype, recvtype, op.get());
  auto* msg = startGather(op.get());
  if (msg){
    op->complete = true;
    delete msg;
  }
  return std::move(op);
}

int
MpiApi::gather(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                void *recvbuf, int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm)
{
#ifdef SST_HG_OTF2_ENABLED
  auto start_clock = traceClock();
#endif
  do_coll(Gather, MPI_Gather, comm, sendcount, sendtype, root,
          recvcount, recvtype, sendbuf, recvbuf);

#ifdef SST_HG_OTF2_ENABLED
  if(OTF2Writer_){
    OTF2Writer_->writer().mpi_gather(start_clock, traceClock(),
        sendcount, sendtype, recvcount, recvtype, root, comm);
  }
#endif

  return MPI_SUCCESS;
}

int
MpiApi::gather(int sendcount, MPI_Datatype sendtype,
                int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm)
{
  return gather(NULL, sendcount, sendtype, NULL, recvcount, recvtype, root, comm);
}

int
MpiApi::igather(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                void *recvbuf, int recvcount, MPI_Datatype recvtype, int root,
                MPI_Comm comm, MPI_Request* req)
{
  start_coll(Gather, MPI_Igather, comm, sendcount, sendtype, root,
             recvcount, recvtype, sendbuf, recvbuf);
  return MPI_SUCCESS;
}

int
MpiApi::igather(int sendcount, MPI_Datatype sendtype,
                int recvcount, MPI_Datatype recvtype, int root,
                MPI_Comm comm, MPI_Request* req)
{
  return igather(NULL, sendcount, sendtype, NULL,
                 recvcount, recvtype, root, comm, req);
}

Iris::sumi::reduce_fxn
MpiApi::getCollectiveFunction(CollectiveOpBase* op)
{
  if (op->op >= first_custom_op_id){
    auto iter = custom_ops_.find(op->op);
    if (iter == custom_ops_.end()){
      sst_hg_throw_printf(SST::Hg::ValueError,
                        "Got invalid MPI_Op %d",
                        op->op);
   }
    MPI_User_function* mpifxn = iter->second;
    MPI_Datatype dtype = op->sendtype->id;
    Iris::sumi::reduce_fxn fxn = ([=](void* dst, const void* src, int count){
      MPI_Datatype copy_type = dtype;
      (*mpifxn)(const_cast<void*>(src), dst, &count, &copy_type);
    });
    return fxn;
  } else if (op->tmp_sendbuf){
    return op->sendtype->op(op->op);
  } else {
    //the function is irrelevant
    //just give it the integer add - function
    return &ReduceOp<Add,int>::op;
  }
}

Iris::sumi::CollectiveDoneMessage*
MpiApi::startReduce(CollectiveOp* op)
{
  Iris::sumi::reduce_fxn fxn = getCollectiveFunction(op);
  return engine_->reduce(op->root, op->tmp_recvbuf, op->tmp_sendbuf, op->sendcnt,
                    op->sendtype->packed_size(), op->tag,
                    fxn, queue_->collCqId(), op->comm);
}

CollectiveOpBase::ptr
MpiApi::startReduce(const char* name, MPI_Comm comm, int count, MPI_Datatype type, int root,
                      MPI_Op mop, const void* src, void* dst)
{
//  mpi_api_debug(sprockit::dbg::mpi | sprockit::dbg::mpi_collective,
//    "%s(%d,%s,%d,%s)", name,
//    count, typeStr(type).c_str(),
//    root,  commStr(comm).c_str());

  auto op = CollectiveOp::create(count, getComm(comm));
  op->root = root;
  op->op = mop;
  MPI_Datatype sendtype, recvtype;
  if (root == op->comm->rank()){
    sendtype = type;
    recvtype = type;
  } else {
    sendtype = type;
    recvtype = MPI_DATATYPE_NULL;
    dst = nullptr;
  }

  startMpiCollective(Collective::reduce, src, dst, sendtype, recvtype, op.get());
  auto* msg = startReduce(op.get());
  if (msg){
    op->complete = true;
    delete msg;
  }
  return std::move(op);
}

int
MpiApi::reduce(const void *src, void *dst, int count,
                MPI_Datatype type, MPI_Op mop, int root, MPI_Comm comm)
{
#ifdef SST_HG_OTF2_ENABLED
  auto start_clock = traceClock();
#endif
  do_coll(Reduce, MPI_Reduce, comm, count,
          type, root, mop, src, dst);

#ifdef SST_HG_OTF2_ENABLED
  if(OTF2Writer_){
    OTF2Writer_->writer().mpi_reduce(start_clock, traceClock(),
      count, type, root, comm);
  }
#endif

  return MPI_SUCCESS;
}

int
MpiApi::reduce(int count, MPI_Datatype type, MPI_Op op, int root, MPI_Comm comm)
{
  return reduce(NULL, NULL, count, type, op, root, comm);
}

int
MpiApi::ireduce(const void* sendbuf, void* recvbuf, int count,
                 MPI_Datatype type, MPI_Op mop, int root, MPI_Comm comm,
                 MPI_Request* req)
{
  start_coll(Reduce, MPI_Ireduce, comm, count,
             type, root, mop, sendbuf, recvbuf);
  return MPI_SUCCESS;
}

int
MpiApi::ireduce(int count, MPI_Datatype type, MPI_Op op, int root, MPI_Comm comm, MPI_Request* req)
{
  return ireduce(NULL, NULL, count, type, op, root, comm, req);
}

Iris::sumi::CollectiveDoneMessage*
MpiApi::startReduceScatter(CollectiveOp* op)
{
  SST::Hg::abort("sumi::reduce_scatter");

  Iris::sumi::reduce_fxn fxn = getCollectiveFunction(op);
  return nullptr;
  //transport::allreduce(op->tmp_recvbuf, op->tmp_sendbuf, op->sendcnt,
  //                     op->sendtype->packed_size(), op->tag,
  //                     fxn, false, options::initial_context, op->comm);

}

CollectiveOpBase::ptr
MpiApi::startReduceScatter(const char*  /*name*/, MPI_Comm  /*comm*/, const int*  /*recvcounts*/,
                           MPI_Datatype type, MPI_Op mop, const void* src, void* dst)
{
  SST::Hg::abort("sumi::reduce_scatter");

  CollectiveOp::ptr op;
  startMpiCollective(Iris::sumi::Collective::reduce_scatter, src, dst, type, type, op.get());
  auto* msg = startReduceScatter(op.get());
  if (msg){
    op->complete = true;
    delete msg;
  }
  op->op = mop;

  return std::move(op);
}

int
MpiApi::reduceScatter(const void *src, void *dst, const int *recvcnts,
                        MPI_Datatype type, MPI_Op mop, MPI_Comm comm)
{
#ifdef SST_HG_OTF2_ENABLED
  auto start_clock = traceClock();
#endif
  do_coll(ReduceScatter, MPI_Reduce_scatter,
          comm, recvcnts, type, mop, src, dst);

#ifdef SST_HG_OTF2_ENABLED
  if (OTF2Writer_){
    OTF2Writer_->writer().mpi_reduce_scatter(start_clock, traceClock(),
          getComm(comm)->size(), recvcnts, type, comm);
  }
#endif

  return MPI_SUCCESS;
}

int
MpiApi::reduceScatter(int *recvcnts, MPI_Datatype type, MPI_Op op, MPI_Comm comm)
{
  return reduceScatter(NULL, NULL, recvcnts, type, op, comm);
}

int
MpiApi::ireduceScatter(const void *src, void *dst, const int *recvcnts,
                        MPI_Datatype type, MPI_Op mop,
                        MPI_Comm comm, MPI_Request* req)
{
  start_coll(ReduceScatter, MPI_Ireduce_scatter,
             comm, recvcnts, type, mop, src, dst);
  return MPI_SUCCESS;
}

int
MpiApi::ireduceScatter(int *recvcnts, MPI_Datatype type,
                         MPI_Op op, MPI_Comm comm, MPI_Request* req)
{
  return ireduceScatter(NULL, NULL, recvcnts, type, op, comm, req);
}

CollectiveOpBase::ptr
MpiApi::startReduceScatterBlock(const char*  /*name*/, MPI_Comm  /*comm*/, int  /*count*/, MPI_Datatype type,
                                    MPI_Op mop, const void* src, void* dst)
{
  SST::Hg::abort("sumi::reduce_scatter: not implemented");

  CollectiveOp::ptr op;
  startMpiCollective(Collective::reduce_scatter, src, dst, type, type, op.get());
  auto* msg = startReduceScatter(op.get());
  op->op = mop;
  if (msg){
    op->complete = true;
    delete msg;
  }
  return std::move(op);
}

int
MpiApi::reduceScatterBlock(const void *src, void *dst, int recvcnt,
                        MPI_Datatype type, MPI_Op mop, MPI_Comm comm)
{
  do_coll(ReduceScatterBlock, MPI_Reduce_scatter_block,
        comm, recvcnt, type, mop, src, dst);
  return MPI_SUCCESS;
}

int
MpiApi::reduceScatterBlock(int recvcnt, MPI_Datatype type, MPI_Op op, MPI_Comm comm)
{
  return reduceScatterBlock(NULL, NULL, recvcnt, type, op, comm);
}

int
MpiApi::ireduceScatterBlock(const void *src, void *dst, int recvcnt,
                        MPI_Datatype type, MPI_Op mop,
                        MPI_Comm comm, MPI_Request* req)
{
  start_coll(ReduceScatterBlock,
          MPI_Ireduce_scatter_block,
          comm, recvcnt, type, mop, src, dst);
  return MPI_SUCCESS;
}

int
MpiApi::ireduceScatterBlock(int recvcnt, MPI_Datatype type,
                         MPI_Op op, MPI_Comm comm, MPI_Request* req)
{
  return ireduceScatterBlock(NULL, NULL, recvcnt, type, op, comm, req);
}

Iris::sumi::CollectiveDoneMessage*
MpiApi::startScan(CollectiveOp* op)
{
  Iris::sumi::reduce_fxn fxn = getCollectiveFunction(op);
  return engine_->scan(op->tmp_recvbuf, op->tmp_sendbuf, op->sendcnt,
                  op->sendtype->packed_size(), op->tag,
                  fxn, queue_->collCqId(), op->comm);
}

CollectiveOpBase::ptr
MpiApi::startScan(const char* name, MPI_Comm comm, int count, MPI_Datatype type,
                  MPI_Op mop, const void* src, void* dst)
{
//  mpi_api_debug(sprockit::dbg::mpi | sprockit::dbg::mpi_collective,
//    "%s(%d,%s,%s)", name,
//    count, typeStr(type).c_str(),
//    commStr(comm).c_str());

  auto op = CollectiveOp::create(count, getComm(comm));
  if (src == MPI_IN_PLACE){
    src = dst;
  }

  op->op = mop;
  startMpiCollective(Collective::scan, src, dst, type, type, op.get());
  auto* msg = startScan(op.get());
  if (msg){
    op->complete = true;
    delete msg;
  }
  //some compiler require this, despit RVO
  return std::move(op);
}

int
MpiApi::scan(const void *src, void *dst, int count, MPI_Datatype type, MPI_Op mop, MPI_Comm comm)
{
#ifdef SST_HG_OTF2_ENABLED
  auto start_clock = traceClock();
#endif
  do_coll(Scan, MPI_Scan, comm, count, type, mop, src, dst);

#ifdef SST_HG_OTF2_ENABLED
  if(OTF2Writer_){
    OTF2Writer_->writer().mpi_scan(start_clock, traceClock(), count, type, comm);
  }
#endif

  return MPI_SUCCESS;
}

int
MpiApi::scan(int count, MPI_Datatype type, MPI_Op op, MPI_Comm comm)
{
  return scan(NULL, NULL, count, type, op, comm);
}

int
MpiApi::iscan(const void *src, void *dst, int count, MPI_Datatype type,
               MPI_Op mop, MPI_Comm comm, MPI_Request* req)
{
  start_coll(Scan, MPI_Iscan, comm, count, type, mop, src, dst);
  return MPI_SUCCESS;
}

int
MpiApi::iscan(int count, MPI_Datatype type, MPI_Op op,
               MPI_Comm comm, MPI_Request* req)
{
  return iscan(NULL, NULL, count, type, op, comm, req);
}

Iris::sumi::CollectiveDoneMessage*
MpiApi::startScatter(CollectiveOp* op)
{
  return engine_->scatter(op->root, op->tmp_recvbuf, op->tmp_sendbuf, op->sendcnt,
                     op->sendtype->packed_size(), op->tag,
                     queue_->collCqId(), op->comm);
}

CollectiveOpBase::ptr
MpiApi::startScatter(const char* name, MPI_Comm comm, int sendcount, MPI_Datatype sendtype, int root,
                     int recvcount, MPI_Datatype recvtype, const void *sendbuf, void *recvbuf)
{
//  mpi_api_debug(sprockit::dbg::mpi | sprockit::dbg::mpi_collective,
//    "%s(%d,%s,%d,%s)", name,
//    sendcount, typeStr(sendtype).c_str(),
//    recvcount, typeStr(recvtype).c_str(),
//    commStr(comm).c_str());

  auto op = CollectiveOp::create(sendcount, recvcount, getComm(comm));

  op->root = root;
  if (root == op->comm->rank()){
    //pass
  } else {
    sendtype = MPI_DATATYPE_NULL;
    sendbuf = nullptr;
  }

  startMpiCollective(Collective::scatter, sendbuf, recvbuf, sendtype, recvtype, op.get());
  auto* msg = startScatter(op.get());
  if (msg){
    op->complete = true;
    delete msg;
  }
  //move required, instead of RVO, for some compilers
  return std::move(op);
}

int
MpiApi::scatter(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                 void *recvbuf, int recvcount, MPI_Datatype recvtype, int root,
                 MPI_Comm comm)
{
#ifdef SST_HG_OTF2_ENABLED
  auto start_clock = traceClock();
#endif
  do_coll(Scatter, MPI_Scatter, comm, sendcount, sendtype, root,
          recvcount, recvtype, sendbuf, recvbuf);

#ifdef SST_HG_OTF2_ENABLED
  if (OTF2Writer_){
    OTF2Writer_->writer().mpi_scatter(start_clock, traceClock(),
      sendcount, sendtype, recvcount, recvtype, root, comm);
  }
#endif

  return MPI_SUCCESS;
}

int
MpiApi::scatter(int sendcount, MPI_Datatype sendtype,
                 int recvcount, MPI_Datatype recvtype, int root, MPI_Comm comm)
{
  return scatter(NULL, sendcount, sendtype, NULL, recvcount, recvtype, root, comm);
}

int
MpiApi::iscatter(const void *sendbuf, int sendcount, MPI_Datatype sendtype,
                 void *recvbuf, int recvcount, MPI_Datatype recvtype, int root,
                 MPI_Comm comm, MPI_Request* req)
{
  start_coll(Scatter, MPI_Iscatter, comm, sendcount, sendtype, root,
             recvcount, recvtype, sendbuf, recvbuf);
  return MPI_SUCCESS;
}

int
MpiApi::iscatter(int sendcount, MPI_Datatype sendtype,
                 int recvcount, MPI_Datatype recvtype,
                 int root, MPI_Comm comm, MPI_Request* req)
{
  return iscatter(NULL, sendcount, sendtype,
                 NULL, recvcount, recvtype,
                 root, comm, req);
}

}
