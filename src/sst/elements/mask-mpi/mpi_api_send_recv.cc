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

#include <mpi_api.h>
#include <mpi_queue/mpi_queue.h>
//#include <sumi-mpi/otf2_output_stat.h>
#include <mercury/components/operating_system.h>
#include <mercury/operating_system/process/thread.h>
//#include <sstmac/software/process/ftq_scope.h>

//#define start_pt2pt_call(fxn, count, type, partner, tag, comm) \
//  StartMPICall(fxn); \
//  mpi_api_debug(sprockit::dbg::mpi | sprockit::dbg::mpi_pt2pt, \
//   "%s(%d,%s,%s,%s,%s)", #fxn, \
//   count, typeStr(type).c_str(), srcStr(partner).c_str(), \
//   tagStr(tag).c_str(), commStr(comm).c_str());

//#define start_Ipt2pt_call(fxn,count,type,partner,tag,comm,reqPtr) \
//  StartMPICall(fxn)
#define start_pt2pt_call(fxn, count, type, partner, tag, comm)
#define start_Ipt2pt_call(fxn,count,type,partner,tag,comm,reqPtr)
#define FinishMPICall(fxn)

namespace SST::MASKMPI {

static struct MPI_Status proc_null_status = {
  MPI_PROC_NULL, MPI_ANY_TAG, 0, 0, 0
};

int
MpiApi::send(const void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm)
{  
#ifdef SST_HG_OTF2_ENABLED
  auto start_clock = traceClock();
#endif

  start_pt2pt_call(MPI_Send,count,datatype,dest,tag,comm);
  MpiComm* commPtr = getComm(comm);

  if (dest != MPI_PROC_NULL){
    MpiRequest* req = MpiRequest::construct(MpiRequest::Send);
    queue_->send(req, count, datatype, dest, tag, commPtr, const_cast<void*>(buf));
    queue_->progressLoop(req);
    delete req;
  }

  FinishMPICall(MPI_Send);

#ifdef SST_HG_OTF2_ENABLED
  if (OTF2Writer_){
    OTF2Writer_->writer().mpi_send(start_clock, traceClock(),
     datatype, count, dest, comm, tag);
  }
#endif

  return MPI_SUCCESS;
}

int
MpiApi::sendrecv(const void *sendbuf, int sendcount,
 MPI_Datatype sendtype, int dest, int sendtag,
 void *recvbuf, int recvcount,
 MPI_Datatype recvtype, int source, int recvtag,
 MPI_Comm comm, MPI_Status *status)
{
#ifdef SST_HG_OTF2_ENABLED
  auto start_clock = traceClock();
#endif

  int rc = MPI_SUCCESS;
  if (dest == MPI_PROC_NULL && source == MPI_PROC_NULL){
    // pass, do nothing
    *status = proc_null_status;
  } else if (source == MPI_PROC_NULL) {
    //really just a send
    rc = send(sendbuf, sendcount, sendtype, dest, sendtag, comm);
    *status = proc_null_status;
  } else if (dest == MPI_PROC_NULL){
    //really just a receive
    rc = recv(recvbuf, recvcount, recvtype, source, recvtag, comm, status);
  } else {
    start_pt2pt_call(MPI_Sendrecv,sendcount,sendtype,source,recvtag,comm);
    MpiRequest* req = doIsend(sendbuf, sendcount, sendtype, dest, sendtag, comm);
    rc = doRecv(recvbuf, recvcount, recvtype, source, recvtag, comm, status);
    queue_->progressLoop(req);
    delete req;
  }

  FinishMPICall(MPI_Sendrecv);

#ifdef SST_HG_OTF2_ENABLED
  if (OTF2Writer_){
    OTF2Writer_->writer().generic_call(start_clock, traceClock(), "MPI_Sendrecv");
  }
#endif

  return rc;
}

int
MpiApi::request_free(MPI_Request *req)
{
//  mpi_api_debug(sprockit::dbg::mpi | sprockit::dbg::mpi_request | sprockit::dbg::mpi_pt2pt,
//    "MPI_Request_free(REQ=%d)", *req);

#ifdef SST_HG_OTF2_ENABLED
  auto start_clock = traceClock();
#endif

  MpiRequest* reqPtr = getRequest(*req);
  if (reqPtr){
    if (reqPtr->isComplete()){
      eraseRequestPtr(*req);
    } else {
      //you freed an active - request
      //that was silly
      //to punish you, I shall leak this memory
      req_map_.erase(*req);
    }
  }
  *req = MPI_REQUEST_NULL;

#ifdef SST_HG_OTF2_ENABLED
  if (OTF2Writer_){
    OTF2Writer_->writer().generic_call(start_clock, traceClock(), "MPI_Request_free");
  }
#endif

  return MPI_SUCCESS;
}

void
MpiApi::doStart(MPI_Request req)
{
  MpiRequest* reqPtr = getRequest(req);
  PersistentOp* op = reqPtr->persistentData();
  if (op == nullptr){
    sst_hg_throw_printf(SST::Hg::ValueError,
                      "Starting MPI_Request that is not persistent");
  }

  if (op->partner == MPI_PROC_NULL){
    reqPtr->setComplete(true); //just mark it done
  } else {
    reqPtr->setComplete(false);
    MpiComm* commPtr = getComm(op->comm);
    if (reqPtr->optype() == MpiRequest::Send){
      queue_->send(reqPtr, op->count, op->datatype, op->partner, op->tag, commPtr, op->content);
    } else {
      queue_->recv(reqPtr, op->count, op->datatype, op->partner, op->tag, commPtr, op->content);
    }
  }

}

int
MpiApi::start(MPI_Request* req)
{
#ifdef SST_HG_OTF2_ENABLED
  auto start_clock = traceClock();
#endif

  //_StartMPICall_(MPI_Start);
  doStart(*req);
  //endAPICall();

#ifdef SST_HG_OTF2_ENABLED
  if(OTF2Writer_) {
    sprockit::abort("OTF2 trace MPI_Start unimplemented");
  }
#endif
  return MPI_SUCCESS;
}

int
MpiApi::startall(int count, MPI_Request* req)
{
  //_StartMPICall_(MPI_Startall);
  for (int i=0; i < count; ++i){
    doStart(req[i]);
  }
  //endAPICall();

#ifdef SST_HG_OTF2_ENABLED
  if (OTF2Writer_) {
    sprockit::abort("OTF2 trace MPI_Start_all unimplemented");
  }
#endif

  return MPI_SUCCESS;
}

int
MpiApi::sendInit(const void *buf, int count,
                 MPI_Datatype datatype, int dest, int tag,
                 MPI_Comm comm, MPI_Request *request)
{
  //_StartMPICall_(MPI_Send_init);

  MpiRequest* req = MpiRequest::construct(MpiRequest::Send);
  addRequestPtr(req, request);

//  mpi_api_debug(sprockit::dbg::mpi | sprockit::dbg::mpi_request | sprockit::dbg::mpi_pt2pt,
//    "MPI_Send_init(%d,%s,%d:%d,%s,%s;REQ=%d)",
//    count, typeStr(datatype).c_str(), int(dest),
//    int(getComm(comm)->peerTask(dest)),
//    tagStr(tag).c_str(), commStr(comm).c_str(),
//    *request);

  PersistentOp* op = new PersistentOp;
  op->content = const_cast<void*>(buf);
  op->count = count;
  op->datatype = datatype;
  op->partner = dest;
  op->tag = tag;
  op->comm = comm;

  req->setPersistent(op);
  //endAPICall();

#ifdef SST_HG_OTF2_ENABLED
  if (OTF2Writer_){
    sprockit::abort("OTF2 trace MPI_Send_init unimplemented");
  }
#endif

  return MPI_SUCCESS;
}

MpiRequest*
MpiApi::doIsend(const void *buf, int count, MPI_Datatype datatype, int dest,
               int tag, MPI_Comm comm)
{
  MpiComm* commPtr = getComm(comm);
  MpiRequest* req = MpiRequest::construct(MpiRequest::Send);
  if (dest == MPI_PROC_NULL){
    req->setComplete(true); //just mark complete
  } else {
    queue_->send(req, count, datatype, dest, tag, commPtr, const_cast<void*>(buf));
  }
  return req;
}

int
MpiApi::isend(const void *buf, int count, MPI_Datatype datatype, int dest,
               int tag, MPI_Comm comm, MPI_Request *request)
{
#ifdef SST_HG_OTF2_ENABLED
  auto start_clock = traceClock();
#endif

  start_Ipt2pt_call(MPI_Isend,count,datatype,dest,tag,comm,request);
  MpiRequest* req = doIsend(buf, count, datatype, dest, tag, comm);
  addRequestPtr(req, request);
//  mpi_api_debug(sprockit::dbg::mpi | sprockit::dbg::mpi_request | sprockit::dbg::mpi_pt2pt,
//    "MPI_Isend(%d,%s,%d,%s,%s;REQ=%d)",
//    count, typeStr(datatype).c_str(), int(dest),
//    tagStr(tag).c_str(), commStr(comm).c_str(), *request);
  queue_->nonblockingProgress();
  FinishMPICall(MPI_Isend);

#ifdef SST_HG_OTF2_ENABLED
  if (OTF2Writer_){
    OTF2Writer_->writer().mpi_isend(start_clock, traceClock(),
      datatype, count, dest, comm, tag, *request);
  }
#endif

  return MPI_SUCCESS;
}

int
MpiApi::recv(void *buf, int count, MPI_Datatype datatype, int source,
              int tag, MPI_Comm comm, MPI_Status *status)
{
#ifdef SST_HG_OTF2_ENABLED
  auto start_clock = traceClock();
#endif

  start_pt2pt_call(MPI_Recv,count,datatype,source,tag,comm);
  int rc = MPI_SUCCESS;
  if (source != MPI_PROC_NULL){
    rc = doRecv(buf,count,datatype,source,tag,comm,status);
  } else {
    *status = proc_null_status;
  }
  FinishMPICall(MPI_Recv);

#ifdef SST_HG_OTF2_ENABLED
  if (OTF2Writer_){
    OTF2Writer_->writer().mpi_recv(start_clock, traceClock(),
      datatype, count, source, comm, tag);
  }
#endif

  return rc;
}

int
MpiApi::doRecv(void *buf, int count, MPI_Datatype datatype, int source,
              int tag, MPI_Comm comm, MPI_Status *status)
{
  MpiRequest* req = MpiRequest::construct(MpiRequest::Recv);
  MpiComm* commPtr = getComm(comm);
  queue_->recv(req, count, datatype, source, tag, commPtr, buf);
  queue_->progressLoop(req);
  if (status != MPI_STATUS_IGNORE){
    *status = req->status();
  }
  delete req;
  return MPI_SUCCESS;
}

int
MpiApi::recvInit(void *buf, int count, MPI_Datatype datatype, int source,
                   int tag, MPI_Comm comm, MPI_Request *request)
{
  //_StartMPICall_(MPI_Recv_init);

  MpiRequest* req = MpiRequest::construct(MpiRequest::Recv);
  addRequestPtr(req, request);

//  mpi_api_debug(sprockit::dbg::mpi | sprockit::dbg::mpi_pt2pt,
//    "MPI_Recv_init(%d,%s,%s,%s,%s;REQ=%d)",
//    count, typeStr(datatype).c_str(),
//    srcStr(source).c_str(), tagStr(tag).c_str(),
//    commStr(comm).c_str(), *request);

  PersistentOp* op = new PersistentOp;
  op->content = buf;
  op->count = count;
  op->datatype = datatype;
  op->partner = source;
  op->tag = tag;
  op->comm = comm;

#ifdef SST_HG_OTF2_ENABLED
  if (OTF2Writer_){
    sprockit::abort("OTF2 trace MPI_Recv_init unimplemented");
  }
#endif

  req->setPersistent(op);
  //endAPICall();
  return MPI_SUCCESS;
}

int
MpiApi::irecv(void *buf, int count, MPI_Datatype datatype, int source,
               int tag, MPI_Comm comm, MPI_Request *request)
{
#ifdef SST_HG_OTF2_ENABLED
  auto start_clock = traceClock();
#endif

  start_Ipt2pt_call(MPI_Irecv,count,datatype,dest,tag,comm,request);

  using namespace SST::Hg;
  MpiComm* commPtr = getComm(comm);

  MpiRequest* req = MpiRequest::construct(MpiRequest::Recv);
  addRequestPtr(req, request);

//  mpi_api_debug(dbg::mpi | dbg::mpi_request | dbg::mpi_pt2pt,
//      "MPI_Irecv(%d,%s,%s,%s,%s;REQ=%d)",
//      count, typeStr(datatype).c_str(),
//      srcStr(commPtr, source).c_str(), tagStr(tag).c_str(),
//      commStr(comm).c_str(), *request);

  if (source != MPI_PROC_NULL){
    queue_->recv(req, count, datatype, source, tag, commPtr, buf);
  } else {
    req->setComplete(true); //just mark done
    req->setStatus(proc_null_status);
  }
  queue_->nonblockingProgress();
  FinishMPICall(MPI_Irecv);

#ifdef SST_HG_OTF2_ENABLED
  if(OTF2Writer_){
    OTF2Writer_->writer().mpi_irecv(start_clock, traceClock(),
      datatype, count, source, comm, tag, *request);
  }
#endif

  return MPI_SUCCESS;
}


}
