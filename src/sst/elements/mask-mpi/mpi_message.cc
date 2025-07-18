/**
Copyright 2009-2025 National Technology and Engineering Solutions of Sandia,
LLC (NTESS).  Under the terms of Contract DE-NA-0003525, the U.S. Government
retains certain rights in this software.

Sandia National Laboratories is a multimission laboratory managed and operated
by National Technology and Engineering Solutions of Sandia, LLC., a wholly
owned subsidiary of Honeywell International, Inc., for the U.S. Department of
Energy's National Nuclear Security Administration under contract DE-NA0003525.

Copyright (c) 2009-2025, NTESS

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

#include <mpi_message.h>
#include <mpi_status.h>
#include <mpi_protocol/mpi_protocol.h>
//#include <sprockit/debug.h>
#include <mercury/common/errors.h>
#include <stdlib.h>
#include <sstream>

#define enumcase(x) case x: return #x

using SST::Core::Serialization::serializer;

namespace SST::MASKMPI {

void
MpiMessage::serialize_order(serializer& ser)
{
  ProtocolMessage::serialize_order(ser);
  SST_SER(src_rank_);
  SST_SER(dst_rank_);
  SST_SER(type_);
  SST_SER(tag_);
  SST_SER(commid_);
  SST_SER(seqnum_);
}

MpiMessage::~MpiMessage() throw ()
{
}

void
MpiMessage::buildStatus(MPI_Status* stat) const
{
  stat->MPI_SOURCE = src_rank_;
  stat->MPI_TAG = tag_;
  stat->count = count();
  stat->bytes_received = payloadSize();
}

std::string
MpiMessage::toString() const
{
  std::stringstream ss;
  ss << "mpimessage("
     << (void*) localBuffer()
     << "," << (void*) remoteBuffer()
     << ", flow=" << flowId()
     << ", count=" << count()
     << ", type=" << type_
     << ", src=" << src_rank_
     << ", dst=" << dst_rank_
     << ", tag=" << tag_
     << ", seq=" << seqnum_
     << ", stage=" << stage()
     << ", protocol=" << protocol()
     << ", type=" << SST::Hg::NetworkMessage::typeStr();

  return ss.str();
}

}
