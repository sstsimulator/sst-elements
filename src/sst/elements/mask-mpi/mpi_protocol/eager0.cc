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
#include <mpi_api.h>
#include <mpi_queue/mpi_queue.h>
#include <mpi_queue/mpi_queue_recv_request.h>
//#include <mercury/operating_system/process/backtrace.h>
//#include <mercury/common/null_buffer.h>
#include <sst/core/params.h>

#include <unusedvariablemacro.h>

namespace SST::MASKMPI {

Eager0::Eager0(SST::Params &params, MpiQueue *queue) :
  MpiProtocol(params, queue)
{
  int default_qos = params.find<int>("default_qos", 0);
  qos_ = params.find<int>("eager0_qos", default_qos);
}

void
Eager0::start(void* buffer, int src_rank, int dst_rank, SST::Hg::TaskId tid, int count, MpiType* typeobj,
              int tag, MPI_Comm comm, int seq_id, MpiRequest* req)
{
  //SST_HG_MAYBE_UNUSED CallGraphAppend(MPIEager0Protocol_Send_Header);
  void* temp_buf = nullptr;
  if (isNonNullBuffer(buffer)){
    temp_buf = fillSendBuffer(count, buffer, typeobj);
  }
  uint64_t payload_bytes = count*typeobj->packed_size();
  queue_->memcopy(payload_bytes);
  auto* msg = mpi_->smsgSend<MpiMessage>(tid, payload_bytes, temp_buf,
                              queue_->pt2ptCqId(), queue_->pt2ptCqId(), Iris::sumi::Message::pt2pt, qos_,
                              src_rank, dst_rank, typeobj->id,  tag, comm, seq_id,
                              count, typeobj->packed_size(), nullptr, EAGER0);

  send_flows_[msg->flowId()] = temp_buf;
  req->complete();
}

void
Eager0::incoming(MpiMessage *msg, MpiQueueRecvRequest *req)
{
  //1 = stage, TimeDelay() = time since last quiesce
  logRecvDelay(1, SST::Hg::TimeDelta(), msg, req);
  if (req->recv_buffer_){
#if SST_HG_SANITY_CHECK
    if (!msg->smsgBuffer()){
      spkt_abort_printf("have receive buffer, but no send buffer on %s", msg->toString().c_str());
    }
#endif
    ::memcpy(req->recv_buffer_, msg->smsgBuffer(), msg->byteLength());
  }
  queue_->notifyProbes(msg);
  queue_->memcopy(msg->payloadBytes());
  queue_->finalizeRecv(msg, req);
  delete msg;
}

void
Eager0::incoming(MpiMessage* msg)
{
  //SST_HG_MAYBE_UNUSED CallGraphAppend(MPIEager0Protocol_Handle_Header);
  if (msg->SST::Hg::NetworkMessage::type() == MpiMessage::payload_sent_ack){
    char* temp_buf = (char*) msg->smsgBuffer();
    if (temp_buf) delete[] temp_buf;
    delete msg;
  } else {
    //I recv this
    MpiQueueRecvRequest* req = queue_->findMatchingRecv(msg);
    if (req){
      incoming(msg, req);
    } else {
      queue_->bufferUnexpected(msg);
    }
    queue_->notifyProbes(msg);
  }
}

}
