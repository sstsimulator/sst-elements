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

#include <mercury/common/component.h>

#include <sst/core/event.h>
#include <sst/core/interfaces/simpleNetwork.h>

#include <mercury/components/node_fwd.h>
#include <mercury/components/node_base_fwd.h>
#include <mercury/components/operating_system_api_fwd.h>
#include <mercury/common/thread_safe_new.h>
#include <mercury/common/node_address.h>
#include <mercury/common/timestamp.h>
#include <mercury/common/event_handler.h>
#include <mercury/common/connection.h>
#include <mercury/hardware/common/packet_fwd.h>
#include <mercury/hardware/common/recv_cq.h>
#include <mercury/hardware/common/flow_fwd.h>
#include <mercury/hardware/network/network_message.h>

#include <functional>


namespace SST::Hg {

class NicEvent :
  public Event, public SST::Hg::thread_safe_new<NicEvent>
{
  ImplementSerializable(NicEvent)
 public:
  NicEvent(NetworkMessage* msg) : msg_(msg) {}

  NetworkMessage* msg() const {
    return msg_;
  }

  void serialize_order(SST::Core::Serialization::serializer& ser) override;

 private:
  NicEvent(){} //for serialization

  NetworkMessage* msg_{};
};

/**
 * A networkinterface is a delegate between a node and a server module.
 * This object helps ornament network operations with information about
 * the process (ppid) involved.
 */
class NicAPI : public SST::Hg::SubComponent
{
 public:

  SST_ELI_REGISTER_SUBCOMPONENT_API(SST::Hg::NicAPI)

  using Port = enum {
    Injection,
    LogP
  };

  class FlowTracker : public Event {
private:
    uint64_t flow_id;
public:
    FlowTracker(uint64_t id) : flow_id(id) {}
    uint64_t id() const {return flow_id;}
  };

protected:
  struct Pending {
    NetworkMessage* payload;
    uint64_t bytesLeft;
    Pending(NetworkMessage* p) :
      payload(p),
      bytesLeft(p->byteLength())
    {
    }
  };

public:

  NicAPI(uint32_t id, SST::Params& params);

  virtual ~NicAPI() = default;

  /**
   * @return A unique ID for the NIC positions. Opaque typedef to an int.
   */
  virtual NodeId addr() const = 0;

  virtual std::string toString() = 0;

  virtual void init(unsigned int phase) = 0;

  virtual void inject(int vn, NetworkMessage* payload) = 0;

  virtual void setup() = 0;

  virtual void complete(unsigned int phase) = 0;

  virtual void finish() = 0;

  virtual bool incomingCredit(int vn) = 0;

  virtual bool incomingPacket(int vn) = 0;

  virtual void connectOutput(int  /*src_outport*/, int  /*dst_inport*/, EventLink::ptr&&  /*link*/) = 0;

  virtual void connectInput(int  /*src_outport*/, int  /*dst_inport*/, EventLink::ptr&&  /*link*/) = 0;

  virtual SST::Event::HandlerBase* creditHandler(int  /*port*/) = 0;

  virtual SST::Event::HandlerBase* payloadHandler(int  /*port*/) = 0;

  virtual void sendWhatYouCan(int vn) = 0;

  virtual bool sendWhatYouCan(int vn, Pending& p) = 0;

  virtual void set_link_control(SST::Interfaces::SimpleNetwork* link_control) = 0;

  /**
   * @brief injectSend Perform an operation on the NIC.
   *  This assumes an exlcusive model of NIC use. If NIC is busy,
   *  operation may complete far in the future. If wishing to query for how busy the NIC is,
   *  use #next_free. Calls to hardware taking an OS parameter
   *  indicate 1) they MUST occur on a user-space software thread
   *  and 2) that they should us the os to block and compute
   * @param netmsg The message being injected
   * @param os     The OS to use form software compute delays
   */
  virtual void injectSend(NetworkMessage* netmsg) = 0;

  virtual EventHandler* mtlHandler() const = 0;

  virtual void mtlHandle(Event* ev) = 0;

  /**
   * Delete all static variables associated with this class.
   * This should be registered with the runtime system via need_deleteStatics
   */
  static void deleteStatics();

  /**
    Perform the set of operations standard to all NICs.
    This then passes control off to a model-specific #doSend
    function to actually carry out the send
    @param payload The network message to send
  */
  virtual void internodeSend(NetworkMessage* payload) = 0;

  /**
    Perform the set of operations standard to all NICs
    for transfers within a node. This function is model-independent,
    unlike #internodeSend which must pass control to #doSend.
   * @param payload
   */
  virtual void intranodeSend(NetworkMessage* payload) = 0;

  /**
   The NIC can either receive an entire message (bypass the byte-transfer layer)
   or it can receive packets.  If an incoming message is a full message (not a packet),
   it gets routed here. Unlike #recv_chunk, this has a default implementation and does not throw.
   @param chunk
   */
  virtual void recvMessage(NetworkMessage* msg) = 0;

  virtual void sendToNode(NetworkMessage* netmsg) = 0;

  virtual void deadlockCheck() = 0;

  virtual void sendManagerMsg(NetworkMessage* msg) = 0;

  virtual std::function<void(NetworkMessage*)> ctrlIoctl() = 0;

  virtual std::function<void(NetworkMessage*)> dataIoctl() = 0;

  virtual std::string toString() const = 0;

  virtual void doSend(NetworkMessage* payload) = 0;

  virtual void set_parent(NodeBase*) = 0;

protected:

  virtual SST::Hg::NodeBase *parent() const = 0;

  virtual bool negligibleSize(int bytes) const = 0;
};

} // end of namespace SST::Hg
