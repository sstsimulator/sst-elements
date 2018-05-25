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

#ifndef TLM_TLMSIMPLEMEM_H
#define TLM_TLMSIMPLEMEM_H

#define SC_INCLUDE_DYNAMIC_PROCESSES
#include <systemc.h>
#include "tlm.h"
#include "tlm_utils/simple_initiator_socket.h"
#include "tlm_utils/peq_with_cb_and_phase.h"

#include <sst/core/sst_types.h>

#include "socketproxy.h"

#include <sst/core/interfaces/simpleMem.h>

#include <sst/core/link.h>
#include <sst/core/output.h>

#include <deque>
#include <map>
#include <utility>

namespace SST{
class Component;
class Params;
namespace SysC{
class Controller;
class MemoryManager;


class TLMSimpleMem 
    : public SST::Interfaces::SimpleMem
//      , public sc_core::sc_module
{
 private:
  typedef SST::SysC::TLMSimpleMem     ThisType;
  typedef SST::Interfaces::SimpleMem  BaseType;

 protected:
  typedef BaseType::Addr                                Address_t;
  typedef BaseType::Request                             Request_t;
  typedef Request_t::id_t                               Id_t;
  typedef Request_t::flags_t                            Flags_t;
  typedef Request_t::Command                            RCommand_t;
  typedef Request_t::dataVec                            DateVector_t;
  typedef BaseType::Handler<ThisType>                   RequestHandler_t;
  typedef std::deque<Request_t*>                        RQueue_t;
  typedef uint32_t                                      GroupId_t;
  typedef std::pair<Id_t,GroupId_t>                     IdPair_t;
  typedef std::map<Address_t,IdPair_t>                  IdMap_t;

  typedef SST::SysC::Controller                         Controller_t;

  typedef tlm::tlm_generic_payload                      Payload_t;
  typedef tlm::tlm_phase                                Phase_t;
  typedef tlm_utils::simple_initiator_socket<ThisType>  Socket_t;
  typedef tlm::tlm_sync_enum                            TLMReturn_t;
  typedef sc_core::sc_time                              SysCTime_t;
  typedef tlm_utils::peq_with_cb_and_phase<ThisType>    PQueue_t;

  typedef SST::Link                                     Link_t;

 public:
  TLMSimpleMem(SST::Component* _component,SST::Params& _params);
  virtual bool initialize(const std::string &_port_name,
                          BaseType::HandlerBase *_handler=NULL);
  virtual void sendInitData(Request_t *_request);
  virtual void sendInitData(SST::Event *_event);
  virtual SST::Link* getLink(void) const;
  virtual void sendRequest(Request_t *_request);
  virtual Request_t* recvResponse(void);

  inline Payload_t* makePayload(const Request_t& _request) const;
  inline Request_t* makeRequest(const Payload_t& _payload,
                                bool is_response=false) const;
  inline Request_t* updateRequest(Request_t& _request,
                                  const Payload_t& _payload,
                                  bool is_response=false);

  Socket_t* getSocket(void);
  void setAcknowledgeHandler(BaseType::HandlerBase *_handler);

 private:
  Request_t* nextResponse();
  TLMReturn_t handleBackward(Payload_t& _trans,
                             Phase_t& _phase,
                             SysCTime_t& _delay);
  void callBack(Payload_t& _trans,
                const Phase_t& _phase);
  void defaultAcknowledgeHandler(Request_t* _request);
  void defaultResponseHandler(Request_t* _request);
  void pollingResponseHandler(Request_t* _request);

 private:
  InitiatorSocketProxy<ThisType>  proxy;
  MemoryManager    *memory_manager;
  Controller_t     *controller;
  SST::Component   *parent_component;
  IdMap_t           id_map;
  Output            out;
  //Socket_t          socket;
  PQueue_t          payload_queue;
  RQueue_t          request_queue;
  BaseType::HandlerBase *response_handler;
  BaseType::HandlerBase *acknowledge_handler;
  uint32_t          streaming_width_id;
  uint32_t          enforced_size;
  bool              is_polling;
  bool              acknowledge_request;
  bool              track_request_ids;
  bool              use_streaming_width_as_id;
  bool              ignore_init_data;
  bool              no_payload;
  bool              force_size;
  bool              force_align;
  bool              check_size;
  bool              check_align;
};

#define TLMSM_IS_POLLING  "is_polling"
#define TLMSM_ACKNOWLEDGE "acknowledge_request"
#define TLMSM_TRACK_ID    "track_request_id"
#define TLMSM_USE_SW_ID   "use_streaming_width_as_id"
#define TLMSM_SW_ID       "streaming_width_id"
#define TLMSM_IGNORE_INIT "ingore_init_data"
#define TLMSM_NO_PAYLOAD  "no_payload"
#define TLMSM_FORCE_SIZE  "force_size"
#define TLMSM_SIZE        "size"
#define TLMSM_FORCE_ALIGN "force_align"
#define TLMSM_CHECK_SIZE  "check_size"
#define TLMSM_CHECK_ALIGN "check_align"

#define TLMSIMPLEMEM_PARAMS_1                                                 \
{TLMSM_IS_POLLING,                                                            \
  "BOOL: Determines if this module is to be used as a polling interface",     \
  "0"},                                                                       \
{TLMSM_ACKNOWLEDGE,                                                           \
  "BOOL: Determines if module will generate acknowledgements for END_REQ",    \
  "0"},                                                                       \
{TLMSM_TRACK_ID,                                                              \
  "BOOL: Determines if module will track Request/group Ids, performance",     \
  "0"},                                                                       \
{TLMSM_NO_PAYLOAD,                                                            \
  "BOOL: Determines if payloads are copied back and forthor if timing only",  \
  "1"},                                                                       \
{TLMSM_FORCE_SIZE,                                                            \
  "BOOL: Determines if we force transactions to a certain size",              \
  "0"},                                                                       \
{TLMSM_SIZE,                                                                  \
  "UINT: Size in bytes to force transaction size to",                         \
  "32"},                                                                      \
{TLMSM_FORCE_ALIGN,                                                           \
  "BOOL: Determines if module forces address to align to [" TLMSM_SIZE "]",   \
  "0"},                                                                       \
{TLMSM_CHECK_SIZE,                                                            \
  "BOOL: Determine if we error on transactions != [" TLMSM_SIZE "]",          \
  "0"},                                                                       \
{TLMSM_CHECK_ALIGN,                                                           \
  "BOOL: Determine if we error on transaction alignment != [" TLMSM_SIZE "]", \
  "0"}

#define TLMSIMPLEMEM_PARAMS_2                                                 \
{TLMSM_USE_SW_ID,                                                             \
  "Determine if this module should use an ID in place of streaming_width",    \
  "0"},                                                                       \
{TLMSM_SW_ID,                                                                 \
  "Integer Id to use in streaming width",                                     \
  "0"},                                                                       \
{TLMSM_IGNORE_INIT,                                                           \
  "Determine if we ignore init data or error on it",                          \
  "0"}
    

}//namespace SysC
}//namespace SST 

#endif//TLM_TLMSIMPLEMEM_H
