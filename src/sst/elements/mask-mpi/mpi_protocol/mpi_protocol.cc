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

#include <cstdint>
#include <cstring>
#include <tuple>
#include <utility>
#include <mpi_protocol/mpi_protocol.h>
#include <mpi_queue/mpi_queue.h>
#include <mpi_api.h>
#include <mpi_queue/mpi_queue_recv_request.h>
#include <sst/core/params.h>

namespace SST::MASKMPI {

MpiProtocol::MpiProtocol(SST::Params&  /*params*/, MpiQueue *queue) :
  queue_(queue), mpi_(queue_->api())
{
}

void*
MpiProtocol::fillSendBuffer(int count, void* buffer, MpiType* typeobj)
{
  uint64_t length = count * typeobj->packed_size();
  void* eager_buf = new char[length];
  if (typeobj->contiguous()){
    ::memcpy(eager_buf, buffer, length);
  } else {
    typeobj->packSend(buffer, eager_buf, count);
  }
  return eager_buf;
}

void
MpiProtocol::logRecvDelay(int stage, SST::Hg::TimeDelta timeSinceQuiesce,
                          MpiMessage* msg, MpiQueueRecvRequest* req)
{
  SST::Hg::TimeDelta active_delay;
  SST::Hg::TimeDelta sync_delay;
  if (req->req()->activeWait()){
    SST::Hg::Timestamp wait_start = req->req()->waitStart();
    active_delay = mpi_->activeDelay(wait_start);
    if (stage == 0 && msg->timeArrived() > wait_start){
      sync_delay = msg->timeArrived() - wait_start;
    }
  }
  mpi_->logMessageDelay(msg, msg->payloadSize(), stage,
                        sync_delay, active_delay, timeSinceQuiesce);
}

DirectPut::~DirectPut()
{
}

void
DirectPut::start(void* buffer, int src_rank, int dst_rank, SST::Hg::TaskId tid, int count, MpiType* type,
                 int tag, MPI_Comm comm, int seq_id, MpiRequest* req)
{
  int qos = 0;
  auto* msg = mpi_->rdmaPut<MpiMessage>(tid, count*type->packed_size(), nullptr, nullptr,
                queue_->pt2ptCqId(), queue_->pt2ptCqId(),
                Iris::sumi::Message::pt2pt, qos,
                src_rank, dst_rank, type->id, tag, comm, seq_id, count, type->packed_size(),
                buffer, DIRECT_PUT);

  send_flows_.emplace(std::piecewise_construct,
                      std::forward_as_tuple(msg->flowId()),
                      std::forward_as_tuple(req));
}

void
DirectPut::incomingAck(MpiMessage *msg)
{
  //mpi_queue_protocol_debug("RDMA get incoming ack %s", msg->toString().c_str());
  auto iter = send_flows_.find(msg->flowId());
  if (iter == send_flows_.end()){
    sst_hg_abort_printf("could not find matching ack for %s", msg->toString().c_str());
  }

  MpiRequest* req = iter->second;
  req->complete();
  send_flows_.erase(iter);
  delete msg;
}

void
DirectPut::incoming(MpiMessage* msg)
{
  //mpi_queue_protocol_debug("RDMA put incoming %s", msg->toString().c_str());
  switch(msg->SST::Hg::NetworkMessage::type()){
  case SST::Hg::NetworkMessage::rdma_put_sent_ack: {
    incomingAck(msg);
    break;
  }
  case SST::Hg::NetworkMessage::rdma_put_payload: {
    incomingPayload(msg);
    break;
  }
  default:
    sst_hg_abort_printf("Invalid message type %s to RDMA put protocol",
                      SST::Hg::NetworkMessage::tostr(msg->SST::Hg::NetworkMessage::type()));
  }
}

void
DirectPut::incoming(MpiMessage *msg, MpiQueueRecvRequest *req)
{
  queue_->finalizeRecv(msg, req);
  delete msg;
}

void
DirectPut::incomingPayload(MpiMessage* msg)
{
  MpiQueueRecvRequest* req = queue_->findMatchingRecv(msg);
  if (req) incoming(msg, req);

}

}
