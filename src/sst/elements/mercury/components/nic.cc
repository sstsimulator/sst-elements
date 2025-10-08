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

#include <mercury/components/nic.h>
#include <sst/core/params.h>
#include <mercury/common/util.h>
#include <mercury/components/node_base.h>
#include <mercury/components/operating_system.h>
#include <mercury/hardware/network/network_message.h>

#include <sstream>

static std::string _tick_spacing_string_("1ps");

namespace SST {
namespace Hg {

void
NicEvent::serialize_order(SST::Core::Serialization::serializer& ser)
{
  Event::serialize_order(ser);
  SST_SER(msg_);
}

NIC::NIC(uint32_t id, SST::Params& params) :
  SST::Hg::SubComponent(id)
{
  unsigned int verbose = params.find<unsigned int>("verbose", 0);
  out_ = std::unique_ptr<SST::Output>(
      new SST::Output(toString(), verbose, 0, Output::STDOUT));

  pending_.resize(1);
  ack_queue_.resize(1);

  mtu_ = params.find<unsigned int>("mtu", 4096);
  out_->debug(CALL_INFO, 1, 0, "setting mtu to %d\n", mtu_);
}

std::string
NIC::toString() {
  return sprintf("Node%d:HgNIC:", int(addr()));
}

void
NIC::sendManagerMsg(NetworkMessage *msg) {
  inject(1, msg);
}

void
NIC::init(unsigned int phase) {
  if (phase == 0) {
    auto recv_notify = new SST::Interfaces::SimpleNetwork::Handler2<SST::Hg::NIC,&SST::Hg::NIC::incomingPacket>(this);
    auto send_notify = new SST::Interfaces::SimpleNetwork::Handler2<SST::Hg::NIC,&SST::Hg::NIC::incomingCredit>(this);
    link_control_->setNotifyOnReceive(recv_notify);
    link_control_->setNotifyOnSend(send_notify);

    my_addr_ = parent_->os()->addr();
    os_ = parent_->os();
  }
  link_control_->init(phase);
}

void
NIC::setup() {
  link_control_->setup();
#if MERLIN_DEBUG_PACKET
  if (test_size_ != 0 && addr() == 0){
    std::cout << "Injecting test messsage of size " << test_size_ << std::endl;
    auto* req = new MyRequest;
    req->size_in_bits = test_size_ * 8;
    req->tail = true;
    req->head = true;
    req->flow_id = -1;
    req->src = 0;
    req->dest = 1;
    link_control_->send(req,0);
    ack_queue_[0].push(nullptr);
  }
#endif
}

void
NIC::complete(unsigned int phase) {
  link_control_->complete(phase);
}

void
NIC::finish() {
  link_control_->finish();
}

bool
NIC::incomingCredit(int vn){
  auto* ack = ack_queue_[vn].front();
  ack_queue_[vn].pop();
  if (ack){
    sendToNode(ack);
  }
  sendWhatYouCan(vn);
  return true; //keep me
}

bool
NIC::incomingPacket(int vn){
  auto* req = link_control_->recv(vn);
  while (req){

    auto bytes = req->size_in_bits/8;
    auto* payload = req->takePayload();

    uint64_t flow_id;
    auto* tracker = dynamic_cast<FlowTracker*>(payload);
    if (tracker != nullptr) {
      flow_id = tracker->id();
      Flow* flow = cq_.recv(flow_id, bytes, nullptr);
    }

    if (req->tail) {
      auto* incoming_msg = payload ? static_cast<NetworkMessage*>(payload) : nullptr;
      if (incoming_msg == nullptr) sst_hg_abort_printf("couldn't cast event to NetworkMessage\n");
      Flow* flow = cq_.recv(incoming_msg->flowId(), bytes, incoming_msg);
      if (flow == nullptr) sst_hg_abort_printf("couldn't get a flow\n");
      auto* msg = static_cast<NetworkMessage*>(flow);
      if (msg == nullptr) sst_hg_abort_printf("couldn't cast flow to message\n");
      out_->debug(CALL_INFO, 1, 0, "fully received message %s\n",
                  msg->toString().c_str());
      recvMessage(msg);
    }
    req = link_control_->recv(vn);
  }
  return true; //keep me active
}

void
NIC::connectOutput(int  /*src_outport*/, int  /*dst_inport*/, EventLink::ptr&&  /*link*/) {
  abort("should never be called on Merlin NIC");
}

void
NIC::connectInput(int  /*src_outport*/, int  /*dst_inport*/, EventLink::ptr&&  /*link*/) {
  SST::Hg::abort("should never be called on Merlin NIC");
}

SST::Event::HandlerBase*
NIC::creditHandler(int  /*port*/) {
  SST::Hg::abort("should never be called on Merlin NIC");
  return nullptr;
}

SST::Event::HandlerBase*
NIC::payloadHandler(int  /*port*/) {
  SST::Hg::abort("should never be called on Merlin NIC");
  return nullptr;
}

void
NIC::inject(int vn, NetworkMessage* payload){
  pending_[vn].emplace(payload);
  out_->debug(CALL_INFO, 1, 0, "sending message on vn %d: %s\n", vn, payload->toString().c_str());
  sendWhatYouCan(vn);
}

void
NIC::sendWhatYouCan(int vn) {
  auto& pendingList = pending_[vn];
  while (!pendingList.empty()){
    Pending& p = pendingList.front();
    bool done = sendWhatYouCan(vn,p);
    if (done){
      pendingList.pop();
    } else {
      return;
    }
  }
}

bool
NIC::sendWhatYouCan(int vn, Pending& p) {
  uint64_t next_bytes = std::min(uint64_t(mtu_), p.bytesLeft);
  uint32_t next_bits = next_bytes * 8; //this is fine for 32-bits
  while (link_control_->spaceToSend(vn, next_bits)){

    auto* req = new SST::Interfaces::SimpleNetwork::Request;
    req->head = p.bytesLeft == p.payload->byteLength();
    p.bytesLeft -= next_bytes;
    req->tail = p.bytesLeft == 0;
    //req->start = now();

    if (req->tail){
      if (p.payload->needsAck()){
        ack_queue_[vn].push(p.payload->cloneInjectionAck());
      } else {
        ack_queue_[vn].push(nullptr);
      }
      req->givePayload(p.payload);
    } else {
      // Use a lightweight event to track flows
      FlowTracker* flow_tracker = new FlowTracker(p.payload->flowId());
      req->givePayload(flow_tracker);
      ack_queue_[vn].push(nullptr);
    }
    req->src = my_addr_;
    req->dest = p.payload->toaddr() / os_->ranksPerNode();
    req->size_in_bits = next_bits;
    req->vn = 0;

   out_->debug(CALL_INFO, 1, 0, "injecting request of size %" PRIu64 " on vn %d: head? %d tail? %d -> %s\n",
              next_bytes, vn, req->head, req->tail,
              p.payload->toString().c_str());
    link_control_->send(req, vn);

    next_bytes = std::min(uint64_t(mtu_), p.bytesLeft);
    next_bits = next_bytes * 8; //this is fine for 32-bits
    if (next_bytes == 0) return true;
  }
  return false;
}

NIC::~NIC()
{
}

EventHandler*
NIC::mtlHandler() const
{
  return newHandler(const_cast<NIC*>(this), &NIC::mtlHandle);
}

void
NIC::mtlHandle(Event *ev)
{
  out_->debug(CALL_INFO, 1, 0, "MTL handle\n");
  NicEvent* nev = static_cast<NicEvent*>(ev);
  NetworkMessage* msg = nev->msg();
  delete nev;
  recvMessage(msg);
}

void
NIC::deleteStatics()
{
}

std::function<void(NetworkMessage*)>
NIC::ctrlIoctl()
{
  auto f = [=](NetworkMessage* msg){
    this->injectSend(msg);
  };
  return f;
}

std::function<void(NetworkMessage*)>
NIC::dataIoctl()
{
  return std::bind(&NIC::injectSend, this, std::placeholders::_1);
}

void
NIC::injectSend(NetworkMessage* netmsg)
{
  if (netmsg->toaddr() / os_->ranksPerNode() == my_addr_){
    intranodeSend(netmsg);
  } else {
    netmsg->putOnWire();
    internodeSend(netmsg);
  }
}

void
NIC::recvMessage(NetworkMessage* netmsg)
{
  out_->debug(CALL_INFO, 1, 0, "handling %s:%lu of type %s from node %d while running\n",
         netmsg->toString().c_str(),
         (unsigned long) netmsg->flowId(),
         NetworkMessage::tostr(netmsg->type()),
         int(netmsg->fromaddr()));

  switch (netmsg->type()) {
    case NetworkMessage::rdma_get_request: {
     netmsg->nicReverse(NetworkMessage::rdma_get_payload);
     netmsg->putOnWire();
     internodeSend(netmsg);
      break;
    }
    case NetworkMessage::nvram_get_request: {
      sst_hg_abort_printf("case NetworkMessage::nvram_get_request unimplemented\n");
//      netmsg->nicReverse(NetworkMessage::nvram_get_payload);
//      //internodeSend(netmsg);
//      parent_->handle(netmsg);
      break;
    }
    case NetworkMessage::failure_notification:
    case NetworkMessage::rdma_get_sent_ack:
    case NetworkMessage::payload_sent_ack:
    case NetworkMessage::rdma_put_sent_ack: {
      parent_->handle(netmsg);
      break;
    }
    case NetworkMessage::rdma_get_nack:
    case NetworkMessage::rdma_get_payload:
    case NetworkMessage::rdma_put_payload:
    case NetworkMessage::nvram_get_payload:
    case NetworkMessage::smsg_send:
    case NetworkMessage::posted_send: {
      netmsg->takeOffWire();
      parent_->handle(netmsg);
      break;
    }
    default: {
      sst_hg_throw_printf(SST::Hg::ValueError,
        "nic::handle: invalid message type %s: %s",
        NetworkMessage::tostr(netmsg->type()), netmsg->toString().c_str());
    }
  }
}

void
NIC::ackSend(NetworkMessage* payload)
{
  if (payload->needsAck()){
    NetworkMessage* ack = payload->cloneInjectionAck();
    out_->debug(CALL_INFO, 1, 0, "acking payload %s\n",
                payload->toString().c_str());
    sendToNode(ack);
  }
}

void
NIC::intranodeSend(NetworkMessage* payload)
{
  out_->debug(CALL_INFO, 1, 0, "intranode send payload %s\n",
              payload->toString().c_str());

  switch(payload->type())
  {
  case NetworkMessage::nvram_get_request:
    payload->nicReverse(NetworkMessage::nvram_get_payload);
    break;
  case NetworkMessage::rdma_get_request:
    payload->nicReverse(NetworkMessage::rdma_get_payload);
    break;
  default:
    break; //nothing to do
  }

//  MemoryModel* mem = parent_->mem();
//   use 64 as a negligible number of compute bytes
//  uint64_t byte_length = payload->byteLength();
//  if (byte_length > 64){
//    mem->accessFlow(payload->byteLength(),
//                TimeDelta(), //assume NIC can issue mem requests without delay
//                newCallback(this, &NIC::finishMemcpy, payload));
//  } else {
//    finishMemcpy(payload);
//  }
finishMemcpy(payload);
}

void
NIC::finishMemcpy(NetworkMessage* payload)
{
  ackSend(payload);
  payload->intranodeMemmove();
  sendToNode(payload);
}

void
NIC::recordMessage(NetworkMessage *netmsg) {

  std::ostringstream debug_stream;
  debug_stream << "sending message " << netmsg->flowId() << " of size "
               << netmsg->byteLength() << " of type "
               << NetworkMessage::tostr(netmsg->type()) << " to node "
               << netmsg->toaddr() << ": netid=" << netmsg->flowId() << " for "
               << netmsg->toString();
  out_->debug(CALL_INFO, 1, 0, "%s\n", debug_stream.str().c_str());

  if (netmsg->type() == NetworkMessage::null_netmsg_type) {
    // assume this is a simple payload
    netmsg->setType(NetworkMessage::smsg_send);
  }
}

void
NIC::internodeSend(NetworkMessage* netmsg)
{
  recordMessage(netmsg);
  doSend(netmsg);
}

void
NIC::sendToNode(NetworkMessage* payload)
{
  auto forward_ev = newCallback(parent_, &NodeBase::handle, payload);
  parent_->sendExecutionEventNow(forward_ev);
}

void 
NIC::set_parent(NodeBase *parent) {
  parent_ = parent;
}

} // end of namespace Hg
} // end of namespace SST
