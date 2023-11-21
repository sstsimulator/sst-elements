// Copyright 2009-2023 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2023, NTESS
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

#include <unordered_map>
#include <mercury/hardware/common/packet.h>
#include <mercury/common/allocator.h>

namespace SST {
namespace Hg {
/**
A Receive Completion Queue.
Based on packets coming in, it determines when a message has completely arrived.
A large message, in some models, is broken up into many #message_chunk
objects.  When using minimal, in-order routing, tracking message completion is easier
because a packet can be marked as the "tail" and used to track when an entire message has arrived.
When using adaptive or multipath routing, messages arrive out-of-order.
This class tracks whether all packets have been received and signals to some handler
that an entire message has fully arrived.
@class nic_RecvCQ
*/
class RecvCQ
{

 public:
  /**
      Log packet and determine if parent message has fully arrived
      @param packet The arriving packet
      @return The completed msg or a null msg indicating not yet complete
  */
  Flow* recv(Packet* pkt);

  Flow* recv(uint64_t unique_id, uint32_t bytes, Flow* payload);

  void print();

 protected:
  struct incomingMsg {
    Flow* msg;
    uint64_t bytes_arrived;
    uint64_t bytes_total;
    incomingMsg() :
        msg(0),
        bytes_arrived(0),
        bytes_total(0)
    {
    }
  };

  /**
      Keys are unique network ID for all messages.
      Value is the number of bytes receved.
  */
  using pair_type = std::pair<const uint64_t, incomingMsg>;
  using alloc = SST::Hg::threadSafeAllocator<pair_type>;
  using received_map = std::map<uint64_t, incomingMsg, std::less<uint64_t>, alloc>;
  received_map bytes_recved_;
};

} // end namespace Hg
} // end namespace SST
