#ifndef C_CMDRESEVENT_H
#define C_CMDRESEVENT_H

#include "c_BankCommand.hpp"

namespace SST {
namespace n_Bank {
  class c_CmdResEvent : public SST::Event {
  public:
    c_BankCommand *m_payload; // FIXME: change this to a unique_ptr
    c_CmdResEvent() : SST::Event() {}

    void serialize_order(SST::Core::Serialization::serializer &ser) {
        Event::serialize_order(ser);
//        ser & m_payload;
    }
    
    ImplementSerializable(SST::n_Bank::c_CmdResEvent);     

  };

}
}

#endif // C_CMDRESEVENT_H
