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

#include <sst/core/event.h>
#include <mercury/common/unique_id.h>
#include <mercury/hardware/common/packet.h>
#include <mercury/hardware/common/flow.h>

namespace SST {
namespace Hg {

Packet::Packet(
  Flow* flow,
  uint32_t num_bytes,
  uint64_t flow_id,
  bool is_tail,
  NodeId fromaddr,
  NodeId toaddr,
  int qos) :
 toaddr_(toaddr),
 fromaddr_(fromaddr),
 flow_id_(flow_id),
 num_bytes_(num_bytes),
 qos_(qos),
 payload_(flow)
{
  ::memset(rtr_metadata_, 0, sizeof(rtr_metadata_));
  ::memset(stats_metadata_, 0, sizeof(stats_metadata_));
  ::memset(nic_metadata_, 0, sizeof(nic_metadata_));
  auto hdr = rtrHeader<Header>();
  hdr->is_tail = is_tail;
}

void
Packet::serialize_order(Core::Serialization::serializer& ser)
{
  SST::Event::serialize_order(ser);
  ser & toaddr_;
  ser & fromaddr_;
  ser & flow_id_;
  ser & num_bytes_;
  ser & qos_;
  ser & payload_;
  ser & rtr_metadata_;
  ser & stats_metadata_;
  ser & nic_metadata_;
}

} // end namespace Hg
} // end namespace SST
