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

#include <mpi_protocol/mpi_protocol.h>
#include <mpi_queue/mpi_queue.h>
#include <mpi_api.h>
#include <mpi_queue/mpi_queue_recv_request.h>
//#include <sstmac/software/process/backtrace.h>
//#include <mercury/common/null_buffer.h>
#include <sst/core/params.h>

namespace SST::MASKMPI {

Eager1::Eager1(SST::Params &params, MpiQueue *queue) :
  MpiProtocol(params, queue)
{
  int default_qos = params.find<int>("default_qos", 0);
  header_qos_ = params.find<int>("eager1_header_qos", default_qos);
  rdma_get_qos_ = params.find<int>("eager1_rdma_get_qos", default_qos);
  ack_qos_ = params.find<int>("eager1_ack_qos", default_qos);
}

void
Eager1::start(void* buffer, int src_rank, int dst_rank, SST::Hg::TaskId tid, int count,
              MpiType* typeobj, int tag, MPI_Comm comm, int seq_id, MpiRequest* key)
{
  void* eager_buf = nullptr;
  if (isNonNullBuffer(buffer)){
    eager_buf = fillSendBuffer(count, buffer, typeobj);
  }

  mpi_->smsgSend<MpiMessage>(tid, 64/*metadata size - use fixed to avoid sizeof*/,
                   nullptr, Iris::sumi::Message::no_ack, queue_->pt2ptCqId(), Iris::sumi::Message::pt2pt, header_qos_,
                   src_rank, dst_rank, typeobj->id, tag, comm, seq_id,
                   count, typeobj->packed_size(), eager_buf, EAGER1);

  key->complete();
}

void
Eager1::incomingHeader(MpiMessage* msg)
{
  char* send_buf = (char*) msg->partnerBuffer();
  char* recv_buf = nullptr;
  if (send_buf){
    recv_buf = new char[msg->payloadSize()];
  }
  msg->advanceStage();
  mpi_->rdmaGetRequestResponse(msg, msg->payloadSize(), recv_buf, send_buf,
                               queue_->pt2ptCqId(), queue_->pt2ptCqId(),
                               ack_qos_);
}

void
Eager1::incomingPayload(MpiMessage* msg)
{
  //CallGraphAppend(MPIEager1Protocol_Handle_RDMA_Payload);
  MpiQueueRecvRequest* req = queue_->findMatchingRecv(msg);
  if (req) incoming(msg, req);
}

void
Eager1::incoming(MpiMessage *msg, MpiQueueRecvRequest* req)
{
  //1 = stage, TimeDelay() = time since last quiesce
  logRecvDelay(1, SST::Hg::TimeDelta(), msg, req);
  if (req->recv_buffer_){
    char* temp_recv_buf = (char*) msg->localBuffer();
#if SST_HG_SANITY_CHECK
    if (!temp_recv_buf){
      spkt_abort_printf("have receiver buffer but no local buffer on %s", msg->toString().c_str());
    }
#endif
    ::memcpy(req->recv_buffer_, temp_recv_buf, msg->payloadBytes());
    delete[] temp_recv_buf;
  }
  queue_->memcopy(msg->payloadBytes());
  queue_->finalizeRecv(msg, req);
  delete msg;
}

void
Eager1::incomingAck(MpiMessage *msg)
{
  if (msg->remoteBuffer()){
    char* temp_send_buf = (char*) msg->remoteBuffer();
    delete[] temp_send_buf;
  }
  delete msg;
}

void
Eager1::incoming(MpiMessage* msg)
{
  switch(msg->NetworkMessage::type()){
  case SST::Hg::NetworkMessage::smsg_send:
    incomingHeader(msg);
    break;
  case SST::Hg::NetworkMessage::rdma_get_sent_ack:
    incomingAck(msg);
    break;
  case SST::Hg::NetworkMessage::rdma_get_payload:
    incomingPayload(msg);
    break;
  default:
    sst_hg_abort_printf("Got bad message type %s for eager1::incoming",
                      SST::Hg::NetworkMessage::tostr(msg->NetworkMessage::type()));
  }
}


}
