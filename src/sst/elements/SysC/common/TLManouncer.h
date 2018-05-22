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

#ifndef TLMANOUNCER_H
#define TLMANOUNCER_H

#define SC_INCLUDE_DYNAMIC_PROCESSES
#include <systemc.h>
#include "tlm.h"
//#include "tlm_utils/simple_initiator_socket.h"
#include "tlm_utils/simple_target_socket.h"
#include "tlm_utils/peq_with_cb_and_phase.h"


#include <iostream>
#include <iomanip>
using std::cout;
using std::endl;

//////////////////////////////////////////////////////////////////////////////
class TLMAnnouncer: public sc_module {
 protected:
  typedef TLMAnnouncer               ThisType_t;
  typedef sc_module                 BaseType_t;
  typedef uint64_t                  Address_t;
  typedef tlm::tlm_generic_payload  Payload_t;
  typedef tlm::tlm_phase            Phase_t;
  typedef sc_core::sc_time          Time_t;
  typedef tlm_utils::peq_with_cb_and_phase<ThisType_t> Queue_t;
  typedef tlm_utils::simple_target_socket<ThisType_t> Socket_t;
 public:
  Socket_t socket;

  SC_CTOR(TLMAnnouncer)
      : socket("AnnouncerSocket")
      , trans_queue(this,&ThisType_t::callBack)
  {
    socket.register_nb_transport_fw(this, &ThisType_t::handleForward);
  }
 private:
  Queue_t trans_queue;
  int data[16];

  tlm::tlm_sync_enum handleForward(Payload_t& _trans,
                                   Phase_t&   _phase,
                                   sc_time&   _delay) {
    cout<<"TLMAnnouncer::handleForward() "<<sc_time_stamp()<<endl;
    Phase_t phase=_phase;
    //Time_t delay=SC_ZERO_TIME;
    _trans.acquire();
    switch(_phase){
    case tlm::END_RESP:
      _trans.release();
    cout<< "Announcer:" << sc_time_stamp() << ":Recieved transaction: Phase="
        << phase << " Address=" <<std::hex <<_trans.get_address() 
        << " Command=" << _trans.get_command()
        << endl;
      return tlm::TLM_COMPLETED;
      break;
    case tlm::BEGIN_REQ:
      trans_queue.notify(_trans,_phase,_delay);
      //phase=tlm::END_REQ;
      //socket->nb_transport_bw(_trans,phase,delay);
      return tlm::TLM_ACCEPTED;
      break;
    case tlm::BEGIN_RESP:
    case tlm::END_REQ:
    default:
      cout<<"Invalid Phase!!!!!! = "<<_phase<<endl;
    }
    return tlm::TLM_COMPLETED;
  }

  void  callBack(Payload_t& _trans,
                 const Phase_t& _phase) {
    cout<<"TLMAnnouncer: callBack()"<<endl;
    Phase_t phase=_phase;
    Time_t delay=Time_t(1,SC_NS);
    Address_t address = _trans.get_address();
    //uint32_t  length = _trans.get_data_length();
    //char*     buffer = _trans.get_data_ptr();
    cout<< "Announcer:" << sc_time_stamp() << ":Recieved transaction: Phase=" 
        << _phase << " Address=" << std::hex <<address << " Command=" 
        << _trans.get_command()
        << endl;
      phase=tlm::END_REQ;
      socket->nb_transport_bw(_trans,phase,delay);
      if(_trans.is_write())
        memcpy(&data[address%16],_trans.get_data_ptr(),4);
//        data[address%16]=*reinterpret_cast<int*>(_trans.get_data_ptr());
      if(_trans.is_read())
        memcpy(_trans.get_data_ptr(),&data[address%16],4);
//        *reinterpret_cast<int*>(_trans.get_data_ptr())=data[address%16];
    phase=tlm::BEGIN_RESP;
    socket->nb_transport_bw(_trans,phase,delay);
    _trans.release();
  }

 public:
  void printData(){
    cout<<"Announcer memory:"<<std::hex<<std::setfill('0');
    for(int N:data)
      cout<<" 0x"<<std::setw(8)<<N;
    cout<<endl;
  }
};

#endif //TLMANOUNCER_H
