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
#include <mercury/components/nic_api.h>

#include <vector>
#include <queue>
#include <functional>


namespace SST::Hg {

/**
 * A networkinterface is a delegate between a node and a server module.
 * This object helps ornament network operations with information about
 * the process (ppid) involved.
 */
class NIC : public NicAPI
{
 public:

  SST_ELI_REGISTER_SUBCOMPONENT(
    NIC,
    "hg",
    "nic",
    SST_ELI_ELEMENT_VERSION(0,0,1),
    "Mercury NIC",
    SST::Hg::NIC
  )

  NIC(uint32_t id, SST::Params& params);

  virtual ~NIC();

  NodeId addr() const override;

  std::string toString() override;

  void init(unsigned int phase) override;

  void inject(int vn, NetworkMessage* payload) override;

  void setup() override;

  void complete(unsigned int phase) override;

  void finish() override;

  bool incomingCredit(int vn) override;

  bool incomingPacket(int vn) override;

  void connectOutput(int  /*src_outport*/, int  /*dst_inport*/, EventLink::ptr&&  /*link*/) override;

  void connectInput(int  /*src_outport*/, int  /*dst_inport*/, EventLink::ptr&&  /*link*/) override;

  SST::Event::HandlerBase* creditHandler(int  /*port*/) override;

  SST::Event::HandlerBase* payloadHandler(int  /*port*/) override;

  void sendWhatYouCan(int vn) override;

  bool sendWhatYouCan(int vn, Pending& p) override;

  void set_link_control(SST::Interfaces::SimpleNetwork *link_control) override;

  void injectSend(NetworkMessage* netmsg) override;

  EventHandler* mtlHandler() const override;

  virtual void mtlHandle(Event* ev) override;

  /**
   * Delete all static variables associated with this class.
   * This should be registered with the runtime system via need_deleteStatics
   */
  static void deleteStatics();

  void internodeSend(NetworkMessage* payload) override;

  void intranodeSend(NetworkMessage* payload) override;

  void recvMessage(NetworkMessage* msg) override;

  void sendToNode(NetworkMessage* netmsg) override;

  virtual void deadlockCheck() override {}

  virtual void sendManagerMsg(NetworkMessage* msg) override;

  virtual std::function<void(NetworkMessage*)> ctrlIoctl() override ;

  virtual std::function<void(NetworkMessage*)> dataIoctl() override;

  void doSend(NetworkMessage *payload) override;

  std::string toString() const override { return "nic"; }

  void set_parent(NodeBase*) override;

protected:
  NodeBase *parent() const override;

  bool negligibleSize(int bytes) const override;

private:

  int negligibleSize_;
  SST::Hg::NodeBase* parent_;
  NodeId my_addr_;
  EventLink::ptr logp_link_;
  SST::Hg::OperatingSystemAPI* os_;

  SST::Interfaces::SimpleNetwork* link_control_;
  std::vector<std::queue<Pending>> pending_;
  std::vector<std::queue<NetworkMessage*>> ack_queue_;
  uint32_t mtu_;
  RecvCQ cq_;
  int vns_;
  int test_size_;
  std::unique_ptr<SST::Output> out_;

  /**
   For messages requiring an NIC ACK to signal that the message
   has injected into the interconnect.  Create an ack and
   send it up to the parent node.
   */
  void ackSend(NetworkMessage* payload);

  void recordMessage(NetworkMessage* msg);

  void finishMemcpy(NetworkMessage* msg);
};

} // end of namespace SST::Hg
