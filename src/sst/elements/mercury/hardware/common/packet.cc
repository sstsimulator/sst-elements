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
  SST_SER(toaddr_);
  SST_SER(fromaddr_);
  SST_SER(flow_id_);
  SST_SER(num_bytes_);
  SST_SER(qos_);
  SST_SER(payload_);
  SST_SER(rtr_metadata_);
  SST_SER(stats_metadata_);
  SST_SER(nic_metadata_);
}

} // end namespace Hg
} // end namespace SST
