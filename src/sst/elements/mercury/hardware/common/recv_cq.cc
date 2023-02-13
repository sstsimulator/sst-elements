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

#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS
#endif

#include <inttypes.h>

#include <hardware/common/packet.h>
#include <hardware/common/recv_cq.h>
#include <hardware/common/flow.h>
//#include <sprockit/output.h>

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
#if SSTMAC_SANITY_CHECK
  if (incoming.msg && orig){
    spkt_abort_printf(
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
