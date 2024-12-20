// Copyright 2009-2024 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2024, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// of the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include <inttypes.h>
#include <mercury/libraries/compute/memory_model.h>
#include <mercury/components/operating_system.h>
#include <mercury/components/node_CL.h>

namespace SST {
namespace Hg {

using Callback = ExecutionEvent;

MemoryModel::MemoryModel(SST::Params &params, NodeCL* parent) :
  parent_node_(parent),
  flowId_(0),
  channelInterleaver_(0) 
{
  flow_mtu_ = params.find<SST::UnitAlgebra>("flow_mtu", "512").getRoundedValue();

  auto max_bw = params.find<SST::UnitAlgebra>("channel_bandwidth", "12.0 GB/s");
  channel_byte_delay_ = TimeDelta(max_bw.getValue().inverse().toDouble());

  int num_channels = params.find<int>("num_channels",4);
  channels_.resize(num_channels);
  for (ChannelQueue& q : channels_){
    q.byte_delay = channel_byte_delay_;
  }

  flow_rsp_id_ = initialize( makeHandler(this, &MemoryModel::flowRequestResponse) );
}

int
MemoryModel::initialize(RequestHandlerBase* handler)
{
  int idx = rsp_handlers_.size();
  rsp_handlers_.push_back(handler);
  return idx;
}

void
MemoryModel::accessFlow(uint64_t bytes, TimeDelta byte_request_delay, Callback *cb)
{
  if (bytes == 0){
    parent_node_->os()->sendExecutionEventNow(cb);
    return;
  }

  uint32_t flowId = flowId_++;
  // debug("Starting flow of size %" PRIu64 " on ID %" PRIu32 " with request delay %10.5e",
  //       bytes, flowId, byte_request_delay.sec());
  uint32_t initial_bytes = bytes % flow_mtu_;
  if (initial_bytes == 0){
    initial_bytes = flow_mtu_;
  }

  FlowRequest* req = new FlowRequest;
  req->bytes = initial_bytes;
  req->flowId = flowId;
  auto* ev = newCallback(this, &MemoryModel::accessRequest, flow_rsp_id_, req);
  TimeDelta delay = byte_request_delay * initial_bytes;
  parent_node_->os()->sendDelayedExecutionEvent(delay, ev);

  Flow& f = flows_[flowId];
  f.bytesLeft = bytes - initial_bytes;
  f.callback = cb;
  f.byteRequestDelay = byte_request_delay;
}

void
MemoryModel::flowRequestResponse(Request* req)
{
  FlowRequest* freq = static_cast<FlowRequest*>(req);
  Flow& f  = flows_[freq->flowId];
  if (f.bytesLeft == 0){
    // debug("Receive %" PRIu32 " bytes of flow %" PRIu64 ": completing flow",
    //       req->bytes, freq->flowId);
    //this is a bit weird... we have to add the delay of the final packet
    //we are actually receiving the HEAD bit of the request right now
    parent_node_->os()->sendDelayedExecutionEvent(req->bytes * channel_byte_delay_, f.callback);
    flows_.erase(freq->flowId);
    delete freq;
  } else {
    // debug("Receive %" PRIu32 " bytes of flow %" PRIu64 ": %" PRIu64 " bytes left",
    //       req->bytes, freq->flowId, f.bytesLeft);
    f.bytesLeft -= flow_mtu_; //we have constructed so that the bytes left is always a multiple
    freq->bytes = flow_mtu_;
    auto* ev = newCallback(this, &MemoryModel::accessRequest, flow_rsp_id_, req);
    TimeDelta delay = flow_mtu_ * f.byteRequestDelay;
    parent_node_->os()->sendDelayedExecutionEvent(delay, ev);
  }
}

void
MemoryModel::accessRequest(int linkId, Request *req)
{
  req->rspId = linkId;
  ChannelQueue& q = channels_[channelInterleaver_];
  q.next_free = std::max(q.next_free, parent_node_->os()->now());
  TimeDelta timeToSend = q.byte_delay * req->bytes;
  //we send a response when the HEAD of the request is available
  parent_node_->os()->sendExecutionEvent(q.next_free, newCallback(rsp_handlers_[linkId], &RequestHandlerBase::handle, req));
  q.next_free += timeToSend;
  // debug("Channel %d busy with %d until %10.5e after delay of %10.5e",
  //       channelInterleaver_, linkId, q.next_free.sec(), timeToSend.sec());
  channelInterleaver_ = (channelInterleaver_ + 1) % channels_.size();
}

} // end namespace Hg
} // end namespace SST
