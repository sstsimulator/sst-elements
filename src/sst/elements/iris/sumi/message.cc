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

#include <iris/sumi/message.h>

namespace SST::Iris::sumi {

const int Message::ack_size = 16;
const int Message::header_size = 64;

Message::~Message()
{
}

std::string
Message::toString() const
{
  return SST::Hg::sprintf("message %s %d->%d: %s",
            tostr(class_), sender_, recver_,
            SST::Hg::NetworkMessage::typeStr());
}

#define enumcase(x) case x: return #x

const char*
Message::tostr(class_t ty)
{
  switch(ty)
  {
    enumcase(pt2pt);
    enumcase(collective);
    enumcase(ping);
    enumcase(bcast);
    enumcase(terminate);
    enumcase(no_class);
    enumcase(fake);
  }
  sst_hg_throw_printf(SST::Hg::ValueError,
    "message::tostr: invalid message type %d", ty);
}

void
Message::serialize_order(SST::Core::Serialization::serializer& ser)
{
  SST_SER(arrived_);
  SST_SER(recv_sync_delay_);
  SST_SER(sender_);
  SST_SER(recver_);
  SST_SER(class_);
  SST_SER(send_cq_);
  SST_SER(recv_cq_);
  NetworkMessage::serialize_order(ser);
}

static uint32_t crc32_for_byte(uint32_t r) {
  for(int j = 0; j < 8; ++j){
    r = (r & 1? 0: (uint32_t)0xEDB88320L) ^ r >> 1;
  }
  return r ^ (uint32_t)0xFF000000L;
}

static uint32_t crc32(const void *data, size_t n_bytes)
{
  uint32_t crc = 11;
  static uint32_t table[0x100];
  if(*table == 0){
    for(size_t i = 0; i < 0x100; ++i)
      table[i] = crc32_for_byte(i);
  }
  for(size_t i = 0; i < n_bytes; ++i){
    crc = table[(uint8_t)crc ^ ((uint8_t*)data)[i]] ^ crc >> 8;
  }
  return crc;
}

size_t
Message::hash() const
{
  uint32_t data[4] = { uint32_t(sender_), uint32_t(recver_),
                      uint32_t(class_), uint32_t(byte_length_) };
  return crc32(data, sizeof(data));
}

}
