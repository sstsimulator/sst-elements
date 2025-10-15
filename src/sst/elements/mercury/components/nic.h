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

#include <cstdint>
#include <memory>
#include <string>
#include <mercury/common/component.h>

#include <sst/core/event.h>
#include <sst/core/eli/elementbuilder.h>
#include <sst/core/interfaces/simpleNetwork.h>

#include <mercury/components/node_base_fwd.h>
#include <mercury/components/operating_system_fwd.h>
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
#include <queue>
#include <vector>

namespace SST {
namespace Hg {

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

  NetworkMessage* msg_;
};

/**
 * A networkinterface is a delegate between a node and a server module.
 * This object helps ornament network operations with information about
 * the process (ppid) involved.
 */
class NIC : public SST::Hg::SubComponent
{
 public:

  SST_ELI_REGISTER_SUBCOMPONENT_API(SST::Hg::NIC,
                                    SST::Hg::NodeBase*)

  SST_ELI_REGISTER_SUBCOMPONENT(
    NIC,
    "hg",
    "nic",
    SST_ELI_ELEMENT_VERSION(0,0,1),
    "Mercury NIC",
    SST::Hg::NIC
  )

  typedef enum {
    Injection,
    LogP
  } Port;

  class FlowTracker : public Event {
private:
    uint64_t flow_id;
public:
    FlowTracker(uint64_t id) : flow_id(id) {}
    uint64_t id() const {return flow_id;}
  };

private:
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

  virtual ~NIC();

  /**
   * @return A unique ID for the NIC positions. Opaque typedef to an int.
   */
  NodeId addr() const {
    return my_addr_;
  }

  std::string toString();

  void init(unsigned int phase);

  void inject(int vn, NetworkMessage* payload);

  void setup();

  void complete(unsigned int phase);

  void finish();

  bool incomingCredit(int vn);

  bool incomingPacket(int vn);

  void connectOutput(int  /*src_outport*/, int  /*dst_inport*/, EventLink::ptr&&  /*link*/);

  void connectInput(int  /*src_outport*/, int  /*dst_inport*/, EventLink::ptr&&  /*link*/);

  SST::Event::HandlerBase* creditHandler(int  /*port*/);

  SST::Event::HandlerBase* payloadHandler(int  /*port*/);

  void sendWhatYouCan(int vn);

  bool sendWhatYouCan(int vn, Pending& p);

  void set_link_control(SST::Interfaces::SimpleNetwork* link_control) {
      link_control_ = link_control;
  }

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
  void injectSend(NetworkMessage* netmsg);

  EventHandler* mtlHandler() const;

  virtual void mtlHandle(Event* ev);

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
  void internodeSend(NetworkMessage* payload);

  /**
    Perform the set of operations standard to all NICs
    for transfers within a node. This function is model-independent,
    unlike #internodeSend which must pass control to #doSend.
   * @param payload
   */
  void intranodeSend(NetworkMessage* payload);

  /**
   The NIC can either receive an entire message (bypass the byte-transfer layer)
   or it can receive packets.  If an incoming message is a full message (not a packet),
   it gets routed here. Unlike #recv_chunk, this has a default implementation and does not throw.
   @param chunk
   */
  void recvMessage(NetworkMessage* msg);

  void sendToNode(NetworkMessage* netmsg);

  virtual void deadlockCheck(){}

  virtual void sendManagerMsg(NetworkMessage* msg);

  virtual std::function<void(NetworkMessage*)> ctrlIoctl();

  virtual std::function<void(NetworkMessage*)> dataIoctl();

  virtual std::string toString() const { return "nic"; }

  void doSend(NetworkMessage* payload) {
    inject(0, payload);
  }

 public:
  NIC(uint32_t id, SST::Params& params, SST::Hg::NodeBase* parent);

 protected:

  SST::Hg::NodeBase* parent() const {
    return parent_;
  }

  bool negligibleSize(int bytes) const {
    return bytes <= negligibleSize_;
  }

protected:
  int negligibleSize_;
  SST::Hg::NodeBase* parent_;
  NodeId my_addr_;
  EventLink::ptr logp_link_;

 private:

  SST::Interfaces::SimpleNetwork* link_control_;
  std::vector<std::queue<Pending>> pending_;
  std::vector<std::queue<NetworkMessage*>> ack_queue_;
  uint32_t mtu_;
  RecvCQ cq_;
  int vns_;
  int test_size_;
  std::unique_ptr<SST::Output> out_;

 protected:
  SST::Hg::OperatingSystem* os_;

 private:
  /**
   For messages requiring an NIC ACK to signal that the message
   has injected into the interconnect.  Create an ack and
   send it up to the parent node.
   */
  void ackSend(NetworkMessage* payload);

  void recordMessage(NetworkMessage* msg);

  void finishMemcpy(NetworkMessage* msg);
};

} // end of namespace Hg
} // end of namespace SST
