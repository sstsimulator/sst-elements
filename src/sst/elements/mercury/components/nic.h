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

#pragma once

#include <mercury/common/component.h>
#include <sst/core/event.h>
#include <mercury/common/thread_safe_new.h>
#include <mercury/common/node_address.h>
#include <mercury/common/timestamp.h>
#include <mercury/components/node_fwd.h>
//#include <sstmac/hardware/common/failable.h>
#include <mercury/common/connection.h>
#include <mercury/hardware/common/packet_fwd.h>
#include <mercury/hardware/common/recv_cq.h>
#include <mercury/hardware/network/network_message_fwd.h>
//#include <sstmac/hardware/logp/logp_switch_fwd.h>
//#include <sstmac/common/stats/stat_spyplot_fwd.h>
//#include <sstmac/common/stats/stat_histogram_fwd.h>
#include <mercury/hardware/common/flow_fwd.h>
#include <mercury/hardware/network/network_message_fwd.h>
#include <mercury/components/operating_system_fwd.h>
//#include <mercury/operating_system/process/progress_queue.h>
//#include <sstmac/hardware/topology/topology_fwd.h>
//#include <sprockit/debug.h>
//#include <mercury/common/factory.h>
#include <sst/core/eli/elementbuilder.h>
#include <mercury/common/event_handler.h>
#include <mercury/hardware/network/network_message.h>
#include <sst/core/interfaces/simpleNetwork.h>

#include <vector>
#include <queue>
#include <functional>

//DeclareDebugSlot(nic);

//#define nic_debug(...) \
//  debug_printf(sprockit::dbg::nic, "NIC on node %d: %s", \
//    int(addr()), sprockit::sprintf(__VA_ARGS__).c_str())

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

  void serialize_order(serializer& ser) override;

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
//class NIC : public ConnectableComponent
{
 public:

  SST_ELI_REGISTER_SUBCOMPONENT_API(SST::Hg::NIC,
                                    SST::Hg::Node*)

  SST_ELI_REGISTER_SUBCOMPONENT(
    NIC,
    "hg",
    "nic",
    SST_ELI_ELEMENT_VERSION(0,0,1),
    "Mercury NIC",
    SST::Hg::NIC
  )

//  SST_ELI_DOCUMENT_SUBCOMPONENT_SLOTS(
//      {"link_control_slot", "Slot for a link control", "SST::Interfaces::SimpleNetwork" }
//  )

  typedef enum {
    Injection,
    LogP
  } Port;

  struct MyRequest : public SST::Interfaces::SimpleNetwork::Request {
    uint64_t flow_id;
    Timestamp start;
  };

//  struct MessageEvent : public Event {
//    MessageEvent(NetworkMessage* msg) :
//      msg_(msg)
//    {
//    }
//
//    NetworkMessage* msg() const {
//      return msg_;
//    }
//
//   private:
//    NetworkMessage*  msg_;
//  };

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

//  Topology* topology() const {
//    return top_;
//  }

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
  NIC(uint32_t id, SST::Params& params, SST::Hg::Node* parent);

 protected:

  void configureLogPLinks();

  void configureLinks();

  Node* parent() const {
    return parent_;
  }

  /**
    Start the message sending and inject it into the network
    This performs all model-specific work
    @param payload The network message to send
  */
  //virtual void doSend(NetworkMessage* payload) = 0;

  bool negligibleSize(int bytes) const {
    return bytes <= negligibleSize_;
  }

protected:
  int negligibleSize_;
  Node* parent_;
  NodeId my_addr_;
  EventLink::ptr logp_link_;
  //Topology* top_;

 private:

  //StatSpyplot<int,uint64_t>* spy_bytes_;
  //Statistic<uint64_t>* xmit_flows_;
  //sw::SingleProgressQueue<NetworkMessage> queue_;
  SST::Interfaces::SimpleNetwork* link_control_;
  std::vector<std::queue<Pending>> pending_;
  std::vector<std::queue<NetworkMessage*>> ack_queue_;
  uint32_t mtu_;
  RecvCQ cq_;
  int vns_;
  int test_size_;

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

//class NullNIC : public NIC
//{
// public:

//  SST_ELI_REGISTER_SUBCOMPONENT_DERIVED(
//    NullNIC,
//    "hg",
//    "null_nic",
//    SST_ELI_ELEMENT_VERSION(1,0,0),
//    "implements a nic that models nothing - stand-in only",
//    SST::Hg::NIC
//          )

////  SST_ELI_REGISTER_SUBCOMPONENT_DERIVED(
////    NullNIC,
////    "hg",
////    "nullnic",
////    SST_ELI_ELEMENT_VERSION(0,0,1),
////    "Mercury Null NIC",
////    SST::Hg::NIC
////  )

//  NullNIC(uint32_t id, SST::Params& params, SST::Hg::SimpleNode* parent) :
//      NIC(id, params, parent)
//  {
//  }

//  std::string toString() const override { return "null nic"; }
//  //std::string toString() const { return "null nic"; }

////  void connectOutput(int, int, EventLink::ptr&&) override {}

////  void connectInput(int, int, EventLink::ptr&&) override {}

////  SST::Event::HandlerBase* payloadHandler(int) override { return nullptr; }

////  SST::Event::HandlerBase* creditHandler(int) override { return nullptr; }

//  SST::Event::HandlerBase* payloadHandler(int) { return nullptr; }

//  SST::Event::HandlerBase* creditHandler(int) { return nullptr; }
//};

} // end of namespace Hg
} // end of namespace SST
