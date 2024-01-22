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

#include <mpi_protocol/mpi_protocol.h>
#include <mpi_queue/mpi_queue.h>
#include <mpi_queue/mpi_queue_recv_request.h>
#include <mpi_api.h>
#include <mpi_debug.h>
//#include <mercury/operating_system/process/backtrace.h>
#include <sst/core/params.h>
//#include <mercury/common/null_buffer.h>

namespace SST::MASKMPI {

RendezvousProtocol::RendezvousProtocol(SST::Params& params, MpiQueue* queue) :
  MpiProtocol(params, queue)
{
  int default_qos = params.find<int>("default_qos", 0);
  software_ack_ = params.find<bool>("software_ack", true);
  header_qos_ = params.find<int>("rendezvous_header_qos", default_qos);
  rdma_get_qos_ = params.find<int>("rendezvous_rdma_get_qos", default_qos);
  ack_qos_ = params.find<int>("rendezvous_ack_qos", default_qos);

  if (params.contains("catch_up_qos")){
    auto sync_bw = params.find<SST::UnitAlgebra>("catch_up_sync_bandwidth_cutoff");
    sync_byte_delay_catch_up_cutoff_ = SST::Hg::TimeDelta(sync_bw.getValue().inverse().toDouble());
    auto quiesce_bw = params.find<SST::UnitAlgebra>("catch_up_quiesce_bandwidth_cutoff");
    quiesce_byte_delay_catch_up_cutoff_ = SST::Hg::TimeDelta(quiesce_bw.getValue().inverse().toDouble());
    catch_up_qos_ = params.find<int>("catch_up_qos");
  } else {
    catch_up_qos_ = -1;
  }
}

void
RendezvousProtocol::newOutstanding()
{
  if (numOutstanding_ == 0){
    lastQuiesce_ = queue_->now();
    minPartnerQuiesce_ = lastQuiesce_;
  }
  ++numOutstanding_;
}

void
RendezvousProtocol::finishedOutstanding()
{
  --numOutstanding_;
}

RendezvousGet::~RendezvousGet()
{
}

void*
RendezvousGet::configureSendBuffer(int count, void *buffer, MpiType* type)
{
  //CallGraphAppend(MPIRendezvousProtocol_RDMA_Configure_Buffer);
  mpi_->pinRdma(count*type->packed_size());

  if (isNonNullBuffer(buffer)){
    if (type->contiguous()){
      return buffer;
    } else {
      void* eager_buf = fillSendBuffer(count, buffer, type);
      return eager_buf;
    }
  }//
  return buffer;
}

void
RendezvousGet::start(void* buffer, int src_rank, int dst_rank, SST::Hg::TaskId tid, int count, MpiType* type,
                      int tag, MPI_Comm comm, int seq_id, MpiRequest* req)
{
  void* send_buf = configureSendBuffer(count, buffer, type);
  auto* msg = mpi_->smsgSend<MpiMessage>(tid, 64/*fixed size, not sizeof()*/, nullptr,
                           Iris::sumi::Message::no_ack, queue_->pt2ptCqId(), Iris::sumi::Message::pt2pt, header_qos_,
                           src_rank, dst_rank, type->id,  tag, comm, seq_id,
                           count, type->packed_size(), send_buf, RENDEZVOUS_GET);
  msg->setMinQuiesce(minPartnerQuiesce_);
  //sstmac::Timestamp now = mpi_->now();
  //sstmac::Timestamp time_since_quiesce = now - minPartnerQuiesce_;
  //if (time_since_quiesce > 0.00
  send_flows_.emplace(std::piecewise_construct,
                      std::forward_as_tuple(msg->flowId()),
                      std::forward_as_tuple(req, buffer, send_buf));
  newOutstanding();
}

void
RendezvousGet::incomingAck(MpiMessage *msg)
{
  //mpi_queue_protocol_debug("RDMA get incoming ack %s", msg->toString().c_str());
  auto iter = send_flows_.find(msg->flowId());
  if (iter == send_flows_.end()){
    sst_hg_abort_printf("could not find matching ack for %s", msg->toString().c_str());
    incomingHeader(msg);
  }

  auto& s = iter->second;
  if (s.req->activeWait()){
    SST::Hg::TimeDelta active_delay = mpi_->activeDelay(s.req->waitStart());
    mpi_->logMessageDelay(msg, msg->payloadSize(), 2,
                          msg->recvSyncDelay(), active_delay,
                          queue_->now() - lastQuiesce_);
  }

  if (s.original && s.original != s.temporary){
    //temp got allocated for some reason
    delete[] (char*) s.temporary;
  }
  s.req->complete();
  send_flows_.erase(iter);
  delete msg;
  finishedOutstanding();
}

void
RendezvousGet::incoming(MpiMessage* msg)
{
  minPartnerQuiesce_ = std::min(minPartnerQuiesce_, msg->minQuiesce());
  //mpi_queue_protocol_debug("RDMA get incoming %s", msg->toString().c_str());
  switch(msg->SST::Hg::NetworkMessage::type()){
  case SST::Hg::NetworkMessage::smsg_send: {
    if (msg->stage() == 0){
      incomingHeader(msg);
    } else {
      incomingAck(msg);
    }
    break;
  }
  case SST::Hg::NetworkMessage::rdma_get_payload:
    incomingPayload(msg);
    break;
  case SST::Hg::NetworkMessage::rdma_get_sent_ack: {
    incomingAck(msg);
    break;
  }
  default:
    sst_hg_abort_printf("Invalid message type %s to rendezvous protocol",
                      SST::Hg::NetworkMessage::tostr(msg->SST::Hg::NetworkMessage::type()));
  }
}

void
RendezvousGet::incomingHeader(MpiMessage* msg)
{
  //mpi_queue_protocol_debug("RDMA get incoming header %s", msg->toString().c_str());
  //CallGraphAppend(MPIRendezvousProtocol_RDMA_Handle_Header);
  MpiQueueRecvRequest* req = queue_->findMatchingRecv(msg);
  if (req){
    incoming(msg, req);
  }

  queue_->notifyProbes(msg);
}

void
RendezvousGet::incoming(MpiMessage *msg, MpiQueueRecvRequest* req)
{
  newOutstanding();
  //mpi_queue_protocol_debug("RDMA get matched payload %s", msg->toString().c_str());

  SST::Hg::Timestamp now = mpi_->now();
  SST::Hg::TimeDelta timeSinceQuiesce = now - minPartnerQuiesce_;//lastQuiesce_;
  logRecvDelay(0/*stage*/, timeSinceQuiesce, msg, req);
  msg->addRecvSyncDelay(now - msg->timeArrived());
  msg->setSendSyncDelay(now - msg->timeStarted());

  int qos_to_use = rdma_get_qos_;
  if (catch_up_qos_ >= 0){
    //sstmac::TimeDelta maxQuiesceDelay = quiesce_byte_delay_catch_up_cutoff_ * msg->payloadSize();
    //if (maxQuiesceDelay < timeSinceQuiesce){
    //  qos_to_use = catch_up_qos_;
    //}
    if (timeSinceQuiesce > SST::Hg::TimeDelta(0.0004)){
      qos_to_use = catch_up_qos_;
    }
  }

  mpi_->pinRdma(msg->payloadSize());
//  mpi_queue_action_debug(
//    queue_->api()->commWorld()->rank(),
//    "found matching request for %s",
//    msg->toString().c_str());

  recv_flows_[msg->flowId()] = req;
  msg->advanceStage();
  msg->setMinQuiesce(minPartnerQuiesce_);
  mpi_->rdmaGetRequestResponse(msg, msg->payloadSize(), req->recv_buffer_, msg->partnerBuffer(),
                   queue_->pt2ptCqId(), software_ack_ ? Iris::sumi::Message::no_ack : queue_->pt2ptCqId(),
                   qos_to_use);

}

void
RendezvousGet::incomingPayload(MpiMessage* msg)
{
  auto iter = recv_flows_.find(msg->flowId());
  if (iter == recv_flows_.end()){
    sst_hg_abort_printf("RDMA get protocol has no matching receive for %s", msg->toString().c_str());
  }

  MpiQueueRecvRequest* req = iter->second;
  recv_flows_.erase(iter);

  SST::Hg::Timestamp now = mpi_->now();
  logRecvDelay(1/*stage*/, now - minPartnerQuiesce_, msg, req);
  if (now > msg->timeArrived()){
    SST::Hg::TimeDelta sync_delay = now - msg->timeArrived();
    msg->addRecvSyncDelay(sync_delay);
  }

  queue_->finalizeRecv(msg, req);
  if (software_ack_){
    msg->advanceStage();
    mpi_->smsgSendResponse(msg, 64/*more sizeof(...) fixes*/, nullptr,
                           Iris::sumi::Message::no_ack, queue_->pt2ptCqId(),
                           ack_qos_);
  } else {
    delete msg;
  }
  finishedOutstanding();
}

}
