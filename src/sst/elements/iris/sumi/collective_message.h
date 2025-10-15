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
#include <iris/sumi/message.h>
#include <iris/sumi/collective.h>
#include <mercury/common/thread_safe_new.h>
#include <sst/core/serialization/serializable.h>

namespace SST::Iris::sumi {

/**
 * @class collective_done_message
 * The message that is actually delivered when calling #sumi::comm_poll
 * This encapsulates all the information about a collective that has completed in the background
 */
class CollectiveDoneMessage :
  public SST::Hg::thread_safe_new<CollectiveDoneMessage>
{

 public:
  CollectiveDoneMessage(int tag, Collective::type_t ty, Communicator* dom, uint8_t  /*cq_id*/) :
    tag_(tag), result_(0), vote_(0), type_(ty), dom_(dom)
  {
  }

  int tag() const {
    return tag_;
  }

  Collective::type_t type() const {
    return type_;
  }

  Communicator* dom() const {
    return dom_;
  }

  void set_type(Collective::type_t ty) {
    type_ = ty;
  }

  void set_result(void* buf){
    result_ = buf;
  }

  void* result() {
    return result_;
  }

  void set_vote(int v){
    vote_ = v;
  }

  int vote() const {
    return vote_;
  }

  int comm_rank() const {
    return comm_rank_;
  }

  void set_comm_rank(int rank){
    comm_rank_ = rank;
  }

 protected:
  int tag_;
  void* result_;
  int vote_;
  Collective::type_t type_;
  int comm_rank_;
  Communicator* dom_;
};

/**
 * @class collective_work_message
 * Main message type used by collectives
 */
class CollectiveWorkMessage final :
  public ProtocolMessage
{
  ImplementSerializable(CollectiveWorkMessage)
 public:
  typedef enum {
    eager, get, put
  } protocol_t;

 public:
  template <class... Args>
  CollectiveWorkMessage(
    Collective::type_t type,
    int dom_sender, int dom_recver,
    int tag, int round,
    int nelems, int type_size, void* buffer, protocol_t p,
    Args&&... args) :
    ProtocolMessage(nelems, type_size, buffer, p,
                     std::forward<Args>(args)...),
    tag_(tag),
    type_(type),
    round_(round),
    dom_sender_(dom_sender),
    dom_recver_(dom_recver)
  {
    if (this->classType() != collective){
      sst_hg_abort_printf("collective work message is not of type collect");
    }
  }

  void reverse(){
    std::swap(dom_sender_, dom_recver_);
  }

  std::string toString() const override;

  static const char* tostr(int p);

  void serialize_order(SST::Core::Serialization::serializer& ser) override;

  int tag() const {
    return tag_;
  }

  int domSender() const {
    return dom_sender_;
  }

  int domRecver() const {
    return dom_recver_;
  }

  int domTargetRank() const {
    switch (NetworkMessage::type()){
     case NetworkMessage::smsg_send:
     case NetworkMessage::posted_send:
     case NetworkMessage::rdma_get_payload:
     case NetworkMessage::rdma_put_payload:
     case NetworkMessage::rdma_get_nack:
      return dom_recver_;
     case NetworkMessage::payload_sent_ack:
     case NetworkMessage::rdma_get_sent_ack:
     case NetworkMessage::rdma_put_sent_ack:
      return dom_sender_;
     default:
      sst_hg_abort_printf("Bad payload type %d to CQ id", NetworkMessage::type());
      return -1;
    }
  }

  int round() const {
    return round_;
  }

  Collective::type_t type() const {
    return type_;
  }

  SST::Hg::NetworkMessage* cloneInjectionAck() const override {
    CollectiveWorkMessage* cln = new CollectiveWorkMessage(*this);
    cln->convertToAck();
    return cln;
  }

 protected:
  CollectiveWorkMessage(){} //for serialization

 private:
  int tag_;

  Collective::type_t type_;

  int round_;

  int dom_sender_;

  int dom_recver_;

};

}
