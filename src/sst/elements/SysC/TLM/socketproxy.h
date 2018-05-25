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

#ifndef TLM_SOCKETPROXY_H
#define TLM_SOCKETPROXY_H

#define SC_INCLUDE_DYNAMIC_PROCESSES
#include <systemc.h>
#include "tlm.h"
#include "tlm_utils/simple_initiator_socket.h"
#include "tlm_utils/simple_target_socket.h"
namespace SST{
namespace SysC{
class SocketProxy_base 
    : public sc_core::sc_module
{
 private:                 
  typedef SST::SysC::SocketProxy_base                   ThisType;
  typedef sc_core::sc_module                            BaseType;
 public:
  typedef tlm::tlm_generic_payload                      Payload_t;
  typedef tlm::tlm_phase                                Phase_t;
  typedef tlm::tlm_sync_enum                            TLMReturn_t;
  typedef sc_core::sc_time                              SysCTime_t;
  typedef sc_core::sc_module_name                       Name_t;
 public:
  typedef ThisType                  SC_CURRENT_USER_MODULE;
  SocketProxy_base(Name_t _name)
      : BaseType(_name)
  {}
};

template<typename Parent_T>
class InitiatorSocketProxy 
: public SocketProxy_base
{
 private:
  typedef SST::SysC::InitiatorSocketProxy<Parent_T>     ThisType;
  typedef SocketProxy_base                              BaseType;
 public:
  typedef tlm_utils::simple_initiator_socket<Parent_T>  Socket_t;
  typedef ThisType                  SC_CURRENT_USER_MODULE;
 public:
  InitiatorSocketProxy(Name_t _name,const char* _socket_name)
      : BaseType(_name)
        , socket(_socket_name)
  {
  }
  Socket_t socket;
};
template<typename Parent_T>
class TargetSocketProxy 
: public SocketProxy_base
{
 private:
  typedef SST::SysC::TargetSocketProxy<Parent_T>        ThisType;
  typedef SocketProxy_base                              BaseType;
 public:
  typedef tlm_utils::simple_target_socket<Parent_T>     Socket_t;
  typedef ThisType                  SC_CURRENT_USER_MODULE;
 public:
  TargetSocketProxy(Name_t _name,const char* _socket_name)
      : BaseType(_name)
        , socket(_socket_name)
  {
  }
  Socket_t socket;
};
}//namespace SysC
}//namespace SST
#endif//TLM_SOCKETPROXY_H
