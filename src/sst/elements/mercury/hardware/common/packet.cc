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

#include <sst/core/event.h>
#include <hardware/common/packet.h>
#include <hardware/common/unique_id.h>
#include <hardware/common/flow.h>

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
