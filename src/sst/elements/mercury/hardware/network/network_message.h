/**
Copyright 2009-2021 National Technology and Engineering Solutions of Sandia, 
LLC (NTESS).  Under the terms of Contract DE-NA-0003525, the U.S.  Government 
retains certain rights in this software.

Sandia National Laboratories is a multimission laboratory managed and operated
by National Technology and Engineering Solutions of Sandia, LLC., a wholly 
owned subsidiary of Honeywell International, Inc., for the U.S. Department of 
Energy's National Nuclear Security Administration under contract DE-NA0003525.

Copyright (c) 2009-2021, NTESS

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

#include <hardware/common/flow.h>
#include <hardware/network/network_id.h>
#include <operating_system/process/task_id.h>
#include <operating_system/process/app_id.h>
#include <common/timestamp.h>

namespace SST {
namespace Hg {

class NetworkMessage : public Flow
{
 public:
  typedef enum {
    RDMA_GET_FAILED,
    RDMA_GET_REQ_TO_RSP,
    NVRAM_GET_REQ_TO_RSP
  } nic_event_t;

  struct rdma_get {};
  struct rdma_put {};
  struct smsg {};
  struct post_send {};

  static constexpr uintptr_t bad_recv_buffer = 0x1;

  typedef enum {
    null_netmsg_type=0,
    rdma_get_request=1,
    rdma_get_sent_ack=2,
    rdma_get_nack=3,
    rdma_put_sent_ack=4,
    rdma_put_nack=5,
    payload_sent_ack=6,
    smsg_send=7,
    posted_send=8,
    rdma_get_payload=9,
    rdma_put_payload=10,
    nvram_get_request=11,
    nvram_get_payload=12,
    failure_notification=13
  } type_t;

 public:
  NetworkMessage(
   int qos,
   uint64_t flow_id,
   const std::string& libname,
   AppId aid,
   NodeId to,
   NodeId from,
   uint64_t size,
   bool needs_ack,
   void* buf,
   smsg  /*ctor_tag*/) :
    NetworkMessage(qos, flow_id, libname, aid, to, from,
                    size, size, needs_ack, nullptr, nullptr, buf,
                    smsg_send)
  {
  }

  NetworkMessage(
   int qos,
   uint64_t flow_id,
   const std::string& libname,
   AppId aid,
   NodeId to,
   NodeId from,
   uint64_t size,
   bool needs_ack,
   void* buf,
   post_send  /*ctor_tag*/) :
    NetworkMessage(qos, flow_id, libname, aid, to, from,
                    size, size, needs_ack, nullptr, nullptr, buf,
                    posted_send)
  {
  }

  NetworkMessage(
   int qos,
   uint64_t flow_id,
   const std::string& libname,
   AppId aid,
   NodeId to,
   NodeId from,
   uint64_t payload_size,
   bool needs_ack,
   void* local_buf,
   void* remote_buf,
   rdma_get  /*ctor_tag*/) :
    NetworkMessage(qos, flow_id, libname, aid, to, from,
                    64/*default to 64 bytes for now*/,
                    payload_size, needs_ack, local_buf, remote_buf, nullptr,
                    rdma_get_request)
  {
  }

  NetworkMessage(
   int qos,
   uint64_t flow_id,
   const std::string& libname,
   AppId aid,
   NodeId to,
   NodeId from,
   uint64_t payload_size,
   bool needs_ack,
   void* local_buf,
   void* remote_buf,
   rdma_put  /*ctor_tag*/) :
    NetworkMessage(qos, flow_id, libname, aid, to, from,
                    payload_size, payload_size, needs_ack, local_buf, remote_buf, nullptr,
                    rdma_put_payload)
  {
  }



  std::string toString() const override {
    return "network message";
  }

  ~NetworkMessage() override;

  static const char* tostr(nic_event_t mut);

  static const char* tostr(type_t ty);

  const char* typeStr() const {
    return tostr(type_);
  }

  bool isMetadata() const;

  virtual NetworkMessage* cloneInjectionAck() const = 0;

  void nicReverse(type_t newtype);

  bool isNicAck() const;

  int qos() const {
    return qos_;
  }

  uint64_t payloadBytes() const {
    return payload_bytes_;
  }

  NodeId toaddr() const {
    return toaddr_;
  }

  NodeId fromaddr() const {
    return fromaddr_;
  }

  void setupSmsg(void* buf, uint64_t sz){
    payload_bytes_ = 0;
    setFlowSize(sz);
    smsg_buffer_ = buf;
    type_ = smsg_send;
  }

  void setupRdmaPut(void* local_buf, void* remote_buf, uint64_t sz){
    type_ = rdma_put_payload;
    local_buffer_ = local_buf;
    remote_buffer_ = remote_buf;
    payload_bytes_ = sz;
    setFlowSize(sz);
  }

  void setupRdmaGet(void* local_buf, void* remote_buf, uint64_t sz){
    type_ = rdma_get_request;
    local_buffer_ = local_buf;
    remote_buffer_ = remote_buf;
    payload_bytes_ = sz;
  }

  void setType(type_t ty){
    type_ = ty;
  }

  void putOnWire();

  void takeOffWire();

  void intranodeMemmove();

  void memmoveRemoteToLocal();

  void memmoveLocalToRemote();

  void matchRecv(void* recv_buffer);

  void setNoRecvMatch();

  bool isBadRecv() const {
    return local_buffer_ == (void*) bad_recv_buffer;
  }

  void* localBuffer() const { return local_buffer_; }

  void* remoteBuffer() const { return remote_buffer_; }

  void* smsgBuffer() const { return smsg_buffer_; }

  void* wireBuffer() const { return wire_buffer_; }

  bool needsAck() const {
    //only paylods get acked
    return needs_ack_ && type_ >= smsg_send;
  }

  void setNeedsAck(bool flag){
    needs_ack_ = flag;
  }

  void serialize_order(Core::Serialization::serializer& ser) override;

  AppId aid() const {
    return aid_;
  }

  type_t type() const {
    return type_;
  }

  void reverse();

  void setQoS(int qos){
    qos_ = qos;
  }

  bool started() const {
    return !time_started_.empty();
  }

  void setTimeStarted(Timestamp start){
    time_started_ = start;
  }

  Timestamp timeStarted() const {
    return time_started_;
  }

  void addInjectionDelay(TimeDelta delay){
    injection_delay_ += delay;
  }

  TimeDelta injectionDelay() const {
    return injection_delay_;
  }

  void setMinDelay(TimeDelta delay){
    min_delay_ = delay;
  }

  TimeDelta minDelay() const {
    return min_delay_;
  }

  void setCongestionDelay(TimeDelta delay){
    congestion_delay_ = delay;
  }

  TimeDelta congestionDelay() const {
    return congestion_delay_;
  }

  Timestamp injectionStarted() const {
    return injection_started_;
  }

  void setInjectionStarted(Timestamp now){
    injection_started_ = now;
  }

  void setTimeArrived(Timestamp now){
    time_arrived_ = now;
  }

 protected:
  void convertToAck();

  void clearSmsgBuffer(){
    smsg_buffer_ = nullptr;
  }

  NetworkMessage() : //for serialization
   Flow(-1, 0),
   needs_ack_(true),
   payload_bytes_(0),
   type_(null_netmsg_type),
   qos_(0)
  {
  }

 private:
  NetworkMessage(
   int qos,
   uint64_t flow_id,
   const std::string& libname,
   AppId aid,
   NodeId to,
   NodeId from,
   uint64_t size,
   uint64_t payload_bytes,
   bool needs_ack,
   void* local_buf,
   void* remote_buf,
   void* smsg_buf,
   type_t ty) :
    Flow(flow_id, size, libname),
    smsg_buffer_(smsg_buf),
    local_buffer_(local_buf),
    remote_buffer_(remote_buf),
    aid_(aid),
    needs_ack_(needs_ack),
    payload_bytes_(payload_bytes),
    toaddr_(to),
    fromaddr_(from),
    type_(ty),
    qos_(qos)
  {
  }

  void putBufferOnWire(void* buf, uint64_t sz);

  void takeBufferOffWire(void* buf, uint64_t sz);

  void* smsg_buffer_ = nullptr;

  void* local_buffer_ = nullptr;

  void* remote_buffer_ = nullptr;

  /**
   * @brief wire_buffer Represents a payload injected on the wire
 */
  void* wire_buffer_ = nullptr;

  AppId aid_;

  bool needs_ack_;

  uint64_t payload_bytes_;

  NodeId toaddr_;

  NodeId fromaddr_;

  type_t type_;

  int qos_;

  Timestamp time_started_;

  Timestamp time_arrived_;

  Timestamp injection_started_;

  TimeDelta injection_delay_;

  TimeDelta min_delay_;

  TimeDelta congestion_delay_;

};

} // end namespace Hg
} // end namespace SST
