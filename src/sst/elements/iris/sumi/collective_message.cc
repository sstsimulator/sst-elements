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

#include <iris/sumi/collective_message.h>
#include <sst/core/serialization/serializable.h>

namespace SST::Iris::sumi {

#define enumcase(x) case x: return #x;
const char*
CollectiveWorkMessage::tostr(int p)
{
  switch(p)
  {
    enumcase(eager);
    enumcase(get);
    enumcase(put);
  }
  sst_hg_throw_printf(SST::Hg::ValueError,
    "collective_work_message::invalid action %d",
    p);
}

void
CollectiveWorkMessage::serialize_order(SST::Core::Serialization::serializer& ser)
{
  ProtocolMessage::serialize_order(ser);
  ser & tag_;
  ser & type_;
  ser & round_;
  ser & dom_recver_;
  ser & dom_sender_;
}

std::string
CollectiveWorkMessage::toString() const
{
  return SST::Hg::sprintf(
    "message for collective %s recver=%d sender=%d nbytes=%d round=%d tag=%d %s %d->%d",
     Collective::tostr(type_), recver(), sender(), payloadBytes(), round_, tag_,
     SST::Hg::NetworkMessage::typeStr(), fromaddr(), toaddr());
}


}
