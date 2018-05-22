// Copyright 2009-2018 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2018, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef C_CMDRESEVENT_H
#define C_CMDRESEVENT_H

#include "c_BankCommand.hpp"

namespace SST {
namespace n_Bank {
  class c_CmdResEvent : public SST::Event {
  public:
    c_BankCommand *m_payload; // FIXME: change this to a unique_ptr
    c_CmdResEvent() : SST::Event() {}

    void serialize_order(SST::Core::Serialization::serializer &ser)  override {
        Event::serialize_order(ser);
        ser & m_payload;
    }
    
    ImplementSerializable(SST::n_Bank::c_CmdResEvent);     

  };

}
}

#endif // C_CMDRESEVENT_H
