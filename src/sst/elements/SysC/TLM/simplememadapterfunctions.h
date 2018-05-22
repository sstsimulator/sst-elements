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

#ifndef SIMPLEMEMADAPTERFUNCTIONS_H
#define SIMPLEMEMADAPTERFUNCTIONS_H
#define SC_INCLUDE_DYNAMIC_PROCESSES
#include "systemc.h"
#include "tlm.h"
#include "tlm_utils/simple_initiator_socket.h"
#include "tlm_utils/simple_target_socket.h"

#include "sst/core/sst_types.h"

#include "sst/elements/memHierarchy/memEvent.h"

#include "../TLM/adapter.h"

namespace SST{
namespace SysC{

//////////////////////////////////////////////////////////////////////////////
//Specialized util methods
//Placing them here instead of .cc makes them available to anyone else who 
//would like to use them.
/////////////////////////////////////////////////////////////////////////////
template<>
bool isRead(Output& out,
            SST::Interfaces::SimpleMem::Request& _e){
  typedef SST::Interfaces::SimpleMem::Request EVENT;
  switch(_e.cmd){
  case EVENT::Write:
  case EVENT::WriteResp:
    break;
  case EVENT::Read:
  case EVENT::ReadResp:
    return true;
    break;
  default:
    out.fatal(CALL_INFO,-1,
              "Unsupoorted command '%s'\n",
              MemHierarchy::CommandString[_e.getCmd()]
             );
  }//switch
  return false;
}

template<>
bool isWrite(Output& out,
            SST::Interfaces::SimpleMem::Request& _e){
  return !isRead(out,_e);
}

template<>
SST::Interfaces::SimpleMem::Request* makeEvent(Output& out,
                                       tlm::tlm_generic_payload& _trans,
                                       Adapter_base* _adapter,
                                       bool _copy_data) {
  typedef uint64_t                              Address_t;
  typedef SST::Interfaces::SimpleMem::Request   EVENT;
  typedef SST::SysC::Adapter_b<EVENT>           Adapter_t;
  auto _ad=dynamic_cast<Adapter_t* >(_adapter);
  assert(_ad);
  out.verbose(CALL_INFO,200,0,
              "ID %d: creating MemEvent 0x%llX\n",
              _ad->streaming_width_id,_trans.get_address());
  EVENT::Command cmd;
  Address_t address=_trans.get_address();
  if(_trans.is_write())
    cmd = EVENT::Write;
  else if(_trans.is_read())
    cmd = EVENT::Read;
  else
    out.fatal(CALL_INFO,-1,"Unsupported Command\n");
  EVENT* ev = new EVENT(cmd,
                        address,
                        _trans.get_data_length()
                       );
  if(_copy_data)
    ev->setPayload(_trans.get_data_ptr(),_trans.get_data_length());
  return ev;
}

template<>
tlm::tlm_generic_payload* makePayload(Output& out,
                                      SST::Interfaces::SimpleMem::Request& _event,
                                      Adapter_base* _adapter,
                                      bool _copy_data) {

  typedef uint64_t Address_t;
  typedef tlm::tlm_generic_payload Payload_t;
  typedef SST::Interfaces::SimpleMem::Request   EVENT;
  typedef SST::SysC::Adapter_b<EVENT>   Adapter_t;
  auto _ad=dynamic_cast<Adapter_t* >(_adapter);
  assert(_ad);
  out.verbose(CALL_INFO,200,0,"ID %d: creating Payload 0x%lX\n",
              _ad->streaming_width_id,_event.addr);
  Payload_t* trans = new Payload_t(_ad->manager);
  if(isWrite(out,_event))
    trans->set_write();
  else
    trans->set_read();
  Address_t new_address = _event.addr;
  if(_ad->force_align)
    new_address %= _ad->enforced_size;
  else if(_ad->check_align)
    if(new_address % _ad->enforced_size)
      out.fatal(CALL_INFO,-1,
                "Transaction alignment does not match forced alignment\n");
  trans->set_address(new_address);
  uint32_t new_size = _event.size;
  if(_ad->force_size)
    new_size = _ad->enforced_size;
  else if(_ad->check_size)
    if(new_size != _ad->enforced_size)
      out.fatal(CALL_INFO,-1,
                "Transaction Size '%d' does not match forced size '%d'\n"
                ,new_size,_ad->enforced_size);
  trans->set_data_length(new_size);
  if(_ad->use_streaming_width_as_id)
    trans->set_streaming_width(_ad->streaming_width_id);
  else
    trans->set_streaming_width(new_size);
  trans->set_data_ptr(new uint8_t[new_size]);
  assert(trans->get_data_ptr());
  if(_copy_data){
    if(new_size >= _event.getSize())
      memcpy(trans->get_data_ptr(),_event.data.data(),_event.getSize());
    else
      memcpy(trans->get_data_ptr(),_event.data.data(),new_size);
    //    }
  }
  trans->set_response_status(tlm::TLM_INCOMPLETE_RESPONSE);
  return trans;
}

template<>
void updatePayload(Output& out,
                   tlm::tlm_generic_payload& _p,
                   SST::Interfaces::SimpleMem::Request& _e,
                   Adapter_base* _adapter,
                   bool _copy_data) {
  typedef SST::Interfaces::SimpleMem::Request   EVENT;
  typedef SST::SysC::Adapter_b<EVENT>   Adapter_t;
  auto _ad=dynamic_cast<Adapter_t* >(_adapter);
  assert(_ad);
  out.verbose(CALL_INFO,200,0,
              "ID %d: updating Payload 0x%llX with event 0x%lX\n",
              _ad->streaming_width_id,_p.get_address(),_e.getAddr());
  //Assume that if size is forced, that payload size already matches it
  if(_copy_data){
    if(!_ad->force_size){
      if(_p.get_data_length() == _e.getSize())
        memcpy(_p.get_data_ptr(),_e.data.data(),_e.getSize());
      else
        out.fatal(CALL_INFO,-1,
                  "Sizes do not match\n");
    }
    else{
      if(_p.get_data_length() >= _e.getSize())
        memcpy(_p.get_data_ptr(),_e.data.data(),_e.getSize());
      else
        memcpy(_p.get_data_ptr(),_e.data.data(),_p.get_data_length());
    }
  }
}

template<>
void updateEvent(Output& out,
                 SST::Interfaces::SimpleMem::Request& _e,
                 tlm::tlm_generic_payload& _p,
                 Adapter_base* _adapter,
                 bool _copy_data) {
  typedef SST::Interfaces::SimpleMem::Request   EVENT;
  typedef SST::SysC::Adapter_b<EVENT>   Adapter_t;
  auto _ad=dynamic_cast<Adapter_t* >(_adapter);
  assert(_ad);
  out.verbose(CALL_INFO,200,0,
              "ID %d:updating Event 0x%lX with payload 0x%llX\n",
              _ad->streaming_width_id,_e.getAddr(),_p.get_address());
  if(_copy_data){
    if(!_ad->force_size){
      if(_p.get_data_length() == _e.getSize())
        memcpy(_e.data.data(),_p.get_data_ptr(),_e.getSize());
      else
        out.fatal(CALL_INFO,-1,
                  "Sizes do not match\n");
    }
    else{
      if(_p.get_data_length() <= _e.getSize())
        memcpy(_e.data.data(),_p.get_data_ptr(),_p.get_data_length());
      else                                            
        memcpy(_e.data.data(),_p.get_data_ptr(),_e.getSize());
    }
  }
}

template<>
SST::Interfaces::SimpleMem::Request* makeResponse(Output& out,
                                          tlm::tlm_generic_payload& _trans,
                                          Adapter_base* _adapter) {
  typedef SST::Interfaces::SimpleMem::Request   EVENT;
  typedef SST::SysC::Adapter_b<EVENT>   Adapter_t;
  auto _ad=dynamic_cast<Adapter_t* >(_adapter);
  assert(_ad);
  out.verbose(CALL_INFO,200,0,
              "ID %d:Make response to payload 0x%llX\n",
              _ad->streaming_width_id,_trans.get_address());
  auto it=_ad->event_map.find(_trans.get_address());
  if(_ad->event_map.end()==it)
    out.fatal(CALL_INFO,-1,
              "Could not find matching event for 0x%llX\n",
              _trans.get_address());
  EVENT *ev=it->second;
  if(isWrite(out,*ev))
    ev->cmd = EVENT::WriteResp;
  else
    ev->cmd = EVENT::ReadResp;
  //delete it->second;
  return ev;
}

template<>
tlm::tlm_generic_payload* makeResponse(Output& out,
                                       SST::Interfaces::SimpleMem::Request& _e,
                                       Adapter_base* _adapter){
  typedef SST::Interfaces::SimpleMem::Request   EVENT;
  typedef SST::SysC::Adapter_b<EVENT>   Adapter_t;
  auto _ad=dynamic_cast<Adapter_t* >(_adapter);
  assert(_ad);
  out.verbose(CALL_INFO,200,0,
              "ID %d:Make response to Event 0x%lX\n",
              _ad->streaming_width_id,_e.getAddr());
  auto it = _ad->payload_map.find(_e.getAddr());
  if(_ad->payload_map.end() == it)
    out.fatal(CALL_INFO,-1,
              "Could not find matching payload");
  return it->second;
}

template<>
uint64_t getAddress(Output& out,
                    SST::Interfaces::SimpleMem::Request& _e){
  return _e.addr;
}

template<>
void loadPayload(Output& out,
                 SST::Interfaces::SimpleMem::Request& _e,
                 void *_src,
                 uint32_t _length){
  memcpy(_e.data.data(),_src,_length);
}
template<>
void storePayload(Output& out,
                  SST::Interfaces::SimpleMem::Request& _e,
                  void *_dest,
                  uint32_t _length){
  memcpy(_dest,_e.data.data(),_length);
}

template<>
tlm::tlm_sync_enum sendResponse(Output& out,
                                tlm::tlm_generic_payload& _p,
                                SST::Interfaces::SimpleMem::Request& _e,
                                Adapter_base* _adapter,
                                SST::Link* _link)
{
  typedef SST::Interfaces::SimpleMem::Request   EVENT;
  typedef SST::SysC::Adapter_b<EVENT>   Adapter_t;
  auto _ad=dynamic_cast<Adapter_t* >(_adapter);
  assert(_ad);
  _link->send(&_e);
  return tlm::TLM_COMPLETED;

}

template<>
tlm::tlm_sync_enum sendResponse(Output& out,
                                tlm::tlm_generic_payload& _p,
                                SST::Interfaces::SimpleMem::Request& _e,
                                Adapter_base* _adapter,
                                tlm_utils::simple_target_socket<Adapter_b<SST::Interfaces::SimpleMem::Request> >* _socket)
{
  typedef tlm::tlm_phase                                Phase_t;
  typedef sc_core::sc_time                              SysCTime_t;
  Phase_t phase=tlm::BEGIN_RESP;
  SysCTime_t delay=SC_ZERO_TIME;
  return (*_socket)->nb_transport_bw(_p,phase,delay);
}
  
}//namespace SysC
}//namespace SST
#endif// SIMPLEMEMADAPTERFUNCTIONS_H
