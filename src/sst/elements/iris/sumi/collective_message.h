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

#pragma once

#include <iris/sumi/message.h>
#include <iris/sumi/collective.h>
#include <mercury/common/thread_safe_new.h>

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

  void serialize_order(SST::Hg::serializer& ser) override;

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
