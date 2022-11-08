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

#include <sst/core/event.h>
#include <common/node_address.h>
#include <hardware/common/flow_fwd.h>
//# include <sprockit/printable.h>

namespace SST {
namespace Hg {

#define MAX_HEADER_BYTES 16
#define MAX_CONTROL_BYTES 8
#define MAX_NIC_BYTES 8
#define MAX_STAT_BYTES 8

class Packet : public SST::Event
{

  ImplementSerializable(Packet);

 public:
  struct Header {
    char is_tail : 1; //whether this is the last packet in a flow
    uint16_t edge_port; //the outport number on the edge (not an internal port)
    uint8_t deadlock_vc : 4; //the vc needed for routing deadlock (without QoS)
  };

  Flow* flow() const {
    return payload_;
  }

  std::string toString() const override {
    return "packet";
  }

  template <class T>
  T* rtrHeader() {
    static_assert(sizeof(T) <= sizeof(rtr_metadata_),
                  "given header type too big");
    return (T*) (&rtr_metadata_);
  }

  template <class T>
  const T* rtrHeader() const {
    static_assert(sizeof(T) <= sizeof(rtr_metadata_),
                  "given header type too big");
    return (T*) (&rtr_metadata_);
  }

  NodeId toaddr() const {
    return toaddr_;
  }

  NodeId fromaddr() const {
    return fromaddr_;
  }

  void setToaddr(NodeId to) {
    toaddr_ = to;
  }

  void setFromaddr(NodeId from) {
    fromaddr_ = from;
  }

  int deadlockVC() const {
    auto hdr = rtrHeader<Header>();
    return hdr->deadlock_vc;
  }

  void setEdgeOutport(const int port) {
    auto hdr = rtrHeader<Header>();
    hdr->edge_port = port;
  }

  void setDeadlockVC(const int vc) {
    auto hdr = rtrHeader<Header>();
    hdr->deadlock_vc = vc;
  }

  int edgeOutport() const {
    auto hdr = rtrHeader<Header>();
    return hdr->edge_port;
  }

  void serialize_order(Core::Serialization::serializer& ser) override;

  bool isTail() const {
    auto hdr = rtrHeader<Header>();
    return hdr->is_tail;
  }

  uint32_t byteLength() const {
    return num_bytes_;
  }

  uint32_t numBytes() const {
    return num_bytes_;
  }

  uint64_t flowId() const {
    return flow_id_;
  }

  int qos() const {
    return qos_;
  }

 private:
  NodeId toaddr_;

  NodeId fromaddr_;

  uint64_t flow_id_;

  uint32_t num_bytes_;

  int qos_;

  Flow* payload_;

  char rtr_metadata_[MAX_HEADER_BYTES];

  char nic_metadata_[MAX_NIC_BYTES];

  char stats_metadata_[MAX_STAT_BYTES];

 protected:
  Packet() : Packet(nullptr, 0, 0, false, 0, 0, 0) {}

  Packet(Flow* payload,
    uint32_t numBytes,
    uint64_t flowId,
    bool isTail,
    NodeId fromaddr,
    NodeId toadadr,
    int qos = 0);

};

} // end namespace Hg
} // end namespace SST
