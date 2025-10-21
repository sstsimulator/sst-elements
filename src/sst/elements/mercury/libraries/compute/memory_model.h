// Copyright 2009-2025 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2025, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// of the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <cstdint>
#include <mercury/common/events.h>
#include <mercury/components/node_CL_fwd.h>
#include <mercury/common/unique_id.h>
#include <mercury/libraries/compute/compute_event.h>

namespace SST {
namespace Hg {

class MemoryModel {
public:

  struct Request {
    uint32_t bytes;
    uintptr_t addr;
    uint32_t rspId;
  };

  struct RequestHandlerBase {
    virtual void handle(Request* req) = 0;
  };

  template <class T, class Fxn>
  class RequestHandler : public RequestHandlerBase {
   public:
    RequestHandler(T* t, Fxn f) :
      t_(t), f_(f)
    {
    }

    void handle(Request* req) override {
      (t_->*f_)(req);
    }
   private:
    T* t_;
    Fxn f_;
  };

  template <class T, class Fxn>
  static RequestHandlerBase* makeHandler(T* t, Fxn f){
    return new RequestHandler<T,Fxn>(t,f);
  }

  MemoryModel(SST::Params &params, NodeCL *parent);

  ~MemoryModel() {}

  std::string toString() const { return "packet flow memory model"; }

  void accessFlow(uint64_t bytes, TimeDelta min_byte_delay, ExecutionEvent *cb);

  void accessRequest(int linkId, Request *req);

  void flowRequestResponse(Request *req);

private:

  int initialize(RequestHandlerBase* handler);

  struct FlowRequest : public Request {
    uint32_t flowId;
  };

  struct Flow {
    ExecutionEvent *callback;
    uint64_t bytesLeft;
    TimeDelta byteRequestDelay;
  };

  struct ChannelQueue {
    TimeDelta byte_delay;
    std::vector<Request *> reqs;
    Timestamp next_free;
  };

  NodeCL *parent_node_;
  std::vector<RequestHandlerBase *> rsp_handlers_;
  TimeDelta channel_byte_delay_;
  std::unordered_map<uint32_t, Flow> flows_;
  std::vector<ChannelQueue> channels_;
  uint32_t flowId_;
  uint32_t channelInterleaver_;
  uint32_t flow_mtu_;
  int flow_rsp_id_;
};

} // end namespace Hg
} // end namespace SST
