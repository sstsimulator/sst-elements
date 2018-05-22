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

#ifndef TLMGENERATOR_H
#define TLMGENERATOR_H

#define SC_INCLUDE_DYNAMIC_PROCESSES
#include <systemc.h>
#include "tlm.h"
#include "tlm_utils/simple_initiator_socket.h"
//#include "tlm_utils/simple_target_socket.h"
#include "tlm_utils/peq_with_cb_and_phase.h"


#include <iostream>
#include <iomanip>
using std::cout;
using std::endl;
using std::hex;
using std::dec;
using std::setw;
using std::setfill;
class mm:public tlm::tlm_mm_interface
{
  typedef tlm::tlm_generic_payload  Payload_t;
 public:
  mm(){}
  void free(Payload_t* _trans){
    cout<<"Freeing: "<<std::hex<<_trans->get_address()<<endl;
    delete _trans;
  }
};
//////////////////////////////////////////////////////////////////////////////
  
class TLMGenerator: public sc_module {
 protected:
  typedef TLMGenerator              ThisType_t;
  typedef sc_module                 BaseType_t;
  typedef uint64_t                  Address_t;
  typedef tlm::tlm_generic_payload  Payload_t;
  typedef tlm::tlm_phase            Phase_t;
  typedef sc_core::sc_time          Time_t;
  typedef tlm_utils::peq_with_cb_and_phase<ThisType_t> Queue_t;
  typedef tlm_utils::simple_initiator_socket<ThisType_t> Socket_t;
  typedef mm                        Manager_t;
 public:
  Socket_t  socket;
  Manager_t *manager;

  SC_CTOR(TLMGenerator)
      : socket("GeneratorSocket")
      , trans_queue(this,&ThisType_t::callBack)
  {
    socket.register_nb_transport_bw(this, &ThisType_t::handleBackward);
    manager=new Manager_t();
    buffer=new unsigned char[32];
    SC_THREAD(thread_process);
  }
 private:
  Queue_t trans_queue;
  int  data[32];
  unsigned char* buffer;

  tlm::tlm_sync_enum handleBackward(Payload_t& _trans,
                                   Phase_t&   _phase,
                                   sc_time&   _delay)
  {
    Phase_t phase=_phase;
    _trans.acquire();
    switch(_phase){
    case tlm::END_REQ:
      _trans.release();
      cout<< "Generator:" 
          << sc_time_stamp() 
          << ":Recieved transaction: Phase=" << phase 
          << " Address= 0x" 
          << setw(8) <<setfill('0') << hex <<_trans.get_address() << dec
          << " Command=" 
          << (_trans.get_command()==tlm::TLM_WRITE_COMMAND?"write":"read")
          << endl;
      cout<< "Size: "<<_trans.get_data_length()
          << ", Response Status "<<_trans.get_response_status()
          <<endl;
      //_trans.release();
      return tlm::TLM_UPDATED;
      break;
    case tlm::BEGIN_RESP:
      trans_queue.notify(_trans,phase,_delay);
      //phase=tlm::END_RESP;
      //socket->nb_transport_fw(_trans,phase,SC_ZERO_TIME);
      return tlm::TLM_ACCEPTED;
      break;
    case tlm::BEGIN_REQ:
    case tlm::END_RESP:
    default:
      cout<<"Invalid Phase!!!!!! = "<<_phase<<endl;
    }
    return tlm::TLM_COMPLETED;
  }

  void  callBack(Payload_t& _trans,
                 const Phase_t& _phase) {
    Phase_t phase=_phase;
    //Time_t  delay=SC_ZERO_TIME;
    Address_t address = _trans.get_address();
    //uint32_t  length = _trans.get_data_length();
    //char*     buffer = _trans.get_data_ptr();
    cout<< "Generator:" 
        << sc_time_stamp() 
        << ":Recieved transaction: Phase=" << phase 
        << " Address= 0x" 
        << setw(8) <<setfill('0') << hex <<_trans.get_address() << dec
        << " Command=" 
        << (_trans.get_command()==tlm::TLM_WRITE_COMMAND?"write":"read")
        << endl;
    cout<< "Size: "<<_trans.get_data_length()
        << ", Response Status "<<_trans.get_response_status()
        <<endl;
    if(_trans.is_read())
      memcpy(&data[address%16],_trans.get_data_ptr(),32);
      //data[address%16]=*reinterpret_cast<int*>(_trans.get_data_ptr());
    phase=tlm::END_RESP;
    //socket->nb_transport_fw(_trans,phase,delay);
    _trans.release();
    _trans.release();
  }
/////////////////////////////////
//thread_process()
//Generates traffic
////////////////////////////////
  void thread_process()
  {
    tlm::tlm_generic_payload* trans;
    tlm::tlm_phase phase;
    sc_time delay;

    // Generate a sequence of random transactions
    for (int i = 0; i < 1000; i++)
    {
      int adr = rand();
      tlm::tlm_command cmd = static_cast<tlm::tlm_command>(rand() % 2);
      if (cmd == tlm::TLM_WRITE_COMMAND) data[adr % 16] = rand();

      // Grab a new transaction from the memory manager
      trans = new Payload_t(manager);
      trans->acquire();

      // Set all attributes except byte_enable_length and extensions (unused)
      trans->set_command( cmd );
      trans->set_address( adr );
      memcpy(buffer,&data[adr%16],32);
      trans->set_data_ptr(buffer);
      //trans->set_data_ptr( reinterpret_cast<unsigned char*>(&data[adr % 16]) );
      trans->set_data_length( 32 );
      trans->set_streaming_width( 0 ); // = data_length to indicate no streaming
      trans->set_byte_enable_ptr( 0 ); // 0 indicates unused
      trans->set_dmi_allowed( false ); // Mandatory initial value
      trans->set_response_status( tlm::TLM_INCOMPLETE_RESPONSE ); // Mandatory initial value

      phase = tlm::BEGIN_REQ;

      // Timing annotation models processing time of initiator prior to call
      //delay = sc_time(rand_ps(), SC_PS);
      delay=sc_time(1,SC_NS);


      // Non-blocking transport call on the forward path
      //tlm::tlm_sync_enum status;
      //status = 
      cout<<"["<<dec<<setw(4)<<i<<"] "<<sc_time_stamp()<<" Sending "
          <<(cmd==tlm::TLM_WRITE_COMMAND?"write":"read")
          <<" at address 0x"<<hex<<setw(8)<<setfill('0')<<adr<<dec
          <<endl;
     tlm::tlm_sync_enum status=socket->nb_transport_fw( *trans, phase, delay );
      switch(status){
      case tlm::TLM_ACCEPTED:
        cout<<"Waiting for acceptance"<<endl;
        break;
      case tlm::TLM_UPDATED:
        cout<<"Accepted!!"<<endl;
        break;
      default:
        cout<<"ERROR!!!!"<<endl;
      }
      //cout<<"release()"<<endl;
      //trans->release();

      //wait( sc_time(rand_ps(), SC_PS) );
      cout<<"wait()"<<endl;
      wait(100, SC_NS);
    }
  }
 public:
  void printData(){
    cout<<"Generator memory:"<<std::hex<<std::setfill('0');
    for(int N:data)
      cout<<" 0x"<<std::setw(8)<<N;
    cout<<endl;
  }
};




#endif //TLMGENERATOR_H
