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

#include <common/null_buffer.h>
#include <common/errors.h>
#include <common/serializable.h>
#include <hardware/network/network_message.h>

#define enumcase(x) case x: return #x;

namespace SST {
namespace Hg {

constexpr uintptr_t NetworkMessage::bad_recv_buffer;

NetworkMessage::~NetworkMessage()
{
  if (isNonNullBuffer(wire_buffer_)){
    delete[] (char*) wire_buffer_;
  }
  if (isNonNullBuffer(smsg_buffer_)){
    delete[] (char*) smsg_buffer_;
  }
}

bool
NetworkMessage::isNicAck() const
{
  switch(type_)
  {
  case payload_sent_ack:
  case rdma_put_sent_ack:
  case rdma_get_sent_ack:
    return true;
  default:
    return false;
  }
}

void
NetworkMessage::putOnWire()
{
  switch(type_){
    case rdma_get_payload:
    case nvram_get_payload:
      putBufferOnWire(remote_buffer_, payload_bytes_);
      break;
    case rdma_put_payload:
      putBufferOnWire(local_buffer_, payload_bytes_);
      break;
    case smsg_send:
    case posted_send:
      putBufferOnWire(smsg_buffer_, byteLength());
      smsg_buffer_ = nullptr;
      break;
    default:
      break; //nothing to do
  }
}

void
NetworkMessage::putBufferOnWire(void* buf, uint64_t sz)
{
  if (isNonNullBuffer(buf)){
    wire_buffer_ = new char[sz];
    ::memcpy(wire_buffer_, buf, sz);
  }
}

void
NetworkMessage::takeBufferOffWire(void *buf, uint64_t sz)
{
  if (isNonNullBuffer(buf)){
    ::memcpy(buf, wire_buffer_, sz);
    delete[] (char*) wire_buffer_;
    wire_buffer_ = nullptr;
  }
}

void
NetworkMessage::takeOffWire()
{
  switch (type_){
    case rdma_get_payload:
      takeBufferOffWire(local_buffer_, payload_bytes_);
      break;
    case rdma_put_payload:
      takeBufferOffWire(remote_buffer_, payload_bytes_);
      break;
    case smsg_send:
    case posted_send:
      smsg_buffer_ = wire_buffer_;
      wire_buffer_ = nullptr;
      break;
    default:
      break;
  }
}

void
NetworkMessage::intranodeMemmove()
{
  switch (type()){
    case rdma_get_payload:
      memmoveRemoteToLocal();
      break;
    case rdma_put_payload:
      memmoveLocalToRemote();
      break;
    case smsg_send:
    case posted_send:
      putBufferOnWire(smsg_buffer_, byteLength());
      smsg_buffer_ = wire_buffer_;
      wire_buffer_ = nullptr;
      break;
    default:
      break;
  }
}

void
NetworkMessage::memmoveLocalToRemote()
{
  //due to scatter-gather elements, it's now allowed
  //to have a null remote buffer
  if (isNonNullBuffer(remote_buffer_) && isNonNullBuffer(local_buffer_)){
    ::memcpy(remote_buffer_, local_buffer_, payload_bytes_);
  }
}

void
NetworkMessage::memmoveRemoteToLocal()
{
  //due to scatter-gather elements, it's now allowed
  //to have a null local buffer
  if (isNonNullBuffer(remote_buffer_) && isNonNullBuffer(local_buffer_)){
    ::memcpy(local_buffer_, remote_buffer_, payload_bytes_);
  }
}

void
NetworkMessage::matchRecv(void *recv_buffer)
{
  void* message_buffer = nullptr;
  switch (type_){
    case rdma_get_payload:
    case rdma_put_payload:
      break;
    case smsg_send:
    case posted_send:
      if (isNonNullBuffer(smsg_buffer_) && isNonNullBuffer(recv_buffer)){
        ::memcpy(recv_buffer, smsg_buffer_, payload_bytes_);
        delete[] (char*) smsg_buffer_;
      }
      break;
    default:
      break;
  }
}

void
NetworkMessage::setNoRecvMatch()
{
  if (isNonNullBuffer(local_buffer_)){
    delete[] (char*) local_buffer_;
  }
  local_buffer_ = (void*) bad_recv_buffer;
}


void
NetworkMessage::convertToAck()
{
  reverse();
  switch(type_)
  {
    case rdma_get_payload:
      type_ = rdma_get_sent_ack;
      break;
    case rdma_put_payload:
      type_ = rdma_put_sent_ack;
      break;
    case smsg_send:
    case posted_send:
      type_ = payload_sent_ack;
      break;
    default:
      sst_hg_abort_printf("NetworkMessage::clone_injection_ack: cannot ack msg type %s", tostr(type_));
      break;
  }
  Flow::byte_length_ = 32;
  wire_buffer_ = nullptr;
  smsg_buffer_ = nullptr;
}

void
NetworkMessage::reverse()
{
  //also flip the addresses
  NodeId dst = fromaddr_;
  NodeId src = toaddr_;
  toaddr_ = dst;
  fromaddr_ = src;
}

const char*
NetworkMessage::tostr(nic_event_t mut)
{
  switch (mut) {
      enumcase(RDMA_GET_REQ_TO_RSP);
      enumcase(RDMA_GET_FAILED);
      enumcase(NVRAM_GET_REQ_TO_RSP);
  }
  sst_hg_throw_printf(SST::Hg::ValueError,
       "NetworkMessage: invalid nic event %d", mut);
}

const char*
NetworkMessage::tostr(type_t ty)
{
  switch(ty)
  {
      enumcase(smsg_send);
      enumcase(posted_send);
      enumcase(payload_sent_ack);
      enumcase(rdma_get_request);
      enumcase(rdma_get_payload);
      enumcase(rdma_get_sent_ack);
      enumcase(rdma_get_nack);
      enumcase(rdma_put_payload);
      enumcase(rdma_put_sent_ack);
      enumcase(rdma_put_nack);
      enumcase(null_netmsg_type);
      enumcase(nvram_get_request);
      enumcase(nvram_get_payload);
      enumcase(failure_notification);
  }
  sst_hg_throw_printf(SST::Hg::ValueError,
    "NetworkMessage::tostr: unknown type_t %d",
    ty);
}

void
NetworkMessage::serialize_order(serializer& ser)
{
  Flow::serialize_order(ser);

  // Can't serialize a nullptr!
  bool wire_is_null = wire_buffer_ == nullptr;
  ser & wire_is_null;
  if(!wire_is_null){
    ser & SST::Hg::array(wire_buffer_, payload_bytes_);
  } else {
    ser & payload_bytes_; 
  }

  ser & time_started_;
  ser & time_arrived_;
  ser & injection_started_;
  ser & aid_;
  ser & needs_ack_;
  ser & toaddr_;
  ser & fromaddr_;
  ser & type_;
  ser & qos_;
  ser & time_started_;
  ser & injection_started_;
  ser & injection_delay_;
  ser & min_delay_;
  ser & congestion_delay_;
  if (type_ == null_netmsg_type){
    sst_hg_abort_printf("failed serializing network message - got null type");
  }
  ser.primitive(remote_buffer_);
  ser.primitive(local_buffer_);
  ser.primitive(smsg_buffer_);
}

bool
NetworkMessage::isMetadata() const
{
  switch(type_)
  {
    case nvram_get_request:
    case rdma_get_request:
    case rdma_get_sent_ack:
    case rdma_put_sent_ack:
    case payload_sent_ack:
    case rdma_get_nack:
    case rdma_put_nack:
    case failure_notification:
    case null_netmsg_type:
      return true;
    case smsg_send:
    case posted_send:
    case rdma_get_payload:
    case nvram_get_payload:
    case rdma_put_payload:
      return false;
    default:
     sst_hg_abort_printf("Bad message type %d", type_);
     return false; //make gcc happy
  }
}

void
NetworkMessage::nicReverse(type_t newtype)
{
  //hardware level reverse only
  NetworkMessage::reverse();
  type_ = newtype;
  switch(newtype){
  case rdma_get_payload:
    setFlowSize(payload_bytes_);
    break;
  default: break;
  }
}

} // end namespace Hg
} // end namespace SST
