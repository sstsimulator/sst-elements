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

#include <unordered_map>
#include <hardware/common/packet.h>
#include <common/allocator.h>

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
