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

#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS
#endif

#include <iostream>
#include <ostream>
#include <inttypes.h>

#include <mercury/hardware/common/packet.h>
#include <mercury/hardware/common/recv_cq.h>
#include <mercury/hardware/common/flow.h>

namespace SST {
namespace Hg {

#define DEBUG_CQ 0

void
RecvCQ::print()
{
  std::cout << "Completion Queue" << std::endl;
  for (auto& pair : bytes_recved_){
    incomingMsg& incoming = pair.second;
    std::cout << "Message " << pair.first << " has "
        << incoming.bytes_arrived << " bytes arrived "
        << " out of " << incoming.bytes_total << "\n";
  }
}

Flow*
RecvCQ::recv(uint64_t unique_id, uint32_t bytes, Flow* orig)
{
  incomingMsg& incoming  = bytes_recved_[unique_id];
#if SST_HG_SANITY_CHECK
  if (incoming.msg && orig){
    sst_hg_abort_printf(
        "RecvCQ::recv: only one message chunk should carry the parent payload for %" PRIu64 ": %s",
        unique_id, incoming.msg->toString().c_str());
  }
#endif
  if (orig){
    //this guy is actually carrying the payload
    incoming.msg = orig;
    incoming.bytes_total = orig->byteLength();
  }
  incoming.bytes_arrived += bytes;

  if (incoming.bytes_arrived == incoming.bytes_total){
    Flow* ret = incoming.msg;
    bytes_recved_.erase(unique_id);
    return ret;
  } else {
    return NULL;
  }
}

Flow*
RecvCQ::recv(Packet* pkt)
{
  Flow* payload = dynamic_cast<Flow*>(pkt->flow());
  return recv(pkt->flowId(), pkt->byteLength(), payload);
}

} // end namespace Hg
} // end namespace SST
