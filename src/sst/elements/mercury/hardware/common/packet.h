// Copyright 2009-2024 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2024, NTESS
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

#include <sst/core/event.h>
#include <mercury/common/node_address.h>
#include <mercury/hardware/common/flow_fwd.h>
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
