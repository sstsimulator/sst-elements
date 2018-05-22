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

#ifndef COMMON_DOWNSTREMADAPTER_H
#define COMMON_DOWNSTREMADAPTER_H
///////////////////////////////////////////////////////
#define SC_INCLUDE_DYNAMIC_PROCESSES
#include <systemc.h>
#include "tlm.h"
#include "tlm_utils/simple_initiator_socket.h"
#include "tlm_utils/simple_target_socket.h"
#include "tlm_utils/peq_with_cb_and_phase.h"
///////////////////////////////////////////////////////////
#include "../TLM/adapterfunctions.h"
#include "../common/controller.h"
#include "../common/memorymanager.h"
#include "../TLM/socketproxy.h"
#include "../common/util.h"
#include <map>
#include <string>
#include <queue>

//#include <sst/core/component.h>
//#include <sst/core/event.h>
#include <sst/core/link.h>
#include <sst/core/params.h>
#include <sst/core/output.h>
#include <sst/core/module.h>


namespace SST{
namespace SysC{
/** Base class for constructing a SystemC adapter */
class Adapter_base
:public SST::Module
{
 public:
  typedef SST::SimTime_t              SSTTime_t;
 private:
  typedef Adapter_base                ThisType;
  typedef SST::Module                 BaseType;
  typedef SST::SysC::SocketProxy_base Proxy_t;
 protected:
  //typedef BaseType::Request           Request_t;

  typedef uint64_t                    Address_t;
  typedef Event::Handler<ThisType>    Handler_t;
  typedef SST::Event                  Event_t;
  typedef uint64_t                    Id_t;
  typedef SST::SysC::MemoryManager    Manager_t;
  typedef Proxy_t::Payload_t          Payload_t;
  typedef Proxy_t::Phase_t            Phase_t;
  typedef Proxy_t::TLMReturn_t        TLMReturn_t;
  typedef Proxy_t::SysCTime_t         SysCTime_t;
  typedef tlm_utils::peq_with_cb_and_phase<ThisType>    PQueue_t;
 public:
  Adapter_base(SST::Component *_comp,SST::Params& _params);
  inline Controller* getController() const {return controller;}
  inline SST::Link* getLink() { return sst_link;}
  //Part of the SimpleMem interface
 /* void sendInitData(Request_t *_request);
  void sendInitData(Event_t *_event);
  Request_t* recvResponse(void);
  void initialize(const std::string &_port_name,HandlerBase *_handler=NULL);
  */
 protected:
  virtual void handleSSTEvent(Event_t* _ev)=0;
  /*virtual TLMReturn_t handlePayload(Payload_t& _trans,
    Phase_t& _phase, 
    SysCTime_t& _delay) = 0;
    */
  virtual void callBack(Payload_t& _trans,const Phase_t& _phase) = 0;
  virtual void forwardEnd(Payload_t& _trans,const SSTTime_t& _delay) = 0;
  virtual void createEnd(Payload_t& _trans,const SSTTime_t& _delay) = 0;
  void proxy(Event_t* _ev);
  virtual void callBackProxy(Payload_t& _trans,const Phase_t& _phase);
  


 protected:
  static Id_t       main_id;
  Id_t              id;
  Controller       *controller;
  Proxy_t          *socket_proxy;
  Output            out;
  PQueue_t          payload_queue;
  uint8_t           verbose_level;
  bool  forward_systemc_end;
  bool  create_systemc_end;
  bool  use_link;
  //bool  ignore_init_data;
 public:
  SST::Component   *parent_component;
  Manager_t        *manager;
  SST::Link        *sst_link;
  uint32_t streaming_width_id;
  uint32_t enforced_size;
  bool  use_streaming_width_as_id;
  bool  force_align;
  bool  force_size;
  bool  check_align;
  bool  check_size;
  bool  no_payload;
};

/** Intermediate class to customize SystemC adapter to a specific EVENT type
 * @tparam EVENT The event type that is coming in
 */
template<typename EVENT>
class Adapter_b
: public Adapter_base
{
 private:
  typedef SST::SysC::Adapter_b<EVENT>               ThisType;
  typedef SST::SysC::Adapter_base                   BaseType;
 protected:
  typedef std::queue<EVENT*>                        EventQueue_t;
  typedef std::map<Address_t,EventQueue_t*>         EventMap_t;
  typedef std::queue<Payload_t*>                    PayloadQueue_t;
  typedef std::map<Address_t,PayloadQueue_t*>       PayloadMap_t;
 public:
  Adapter_b(SST::Component* _comp,SST::Params& _params)
      : BaseType(_comp,_params)
  {
    out.verbose(CALL_INFO,3,0,
                "Constructing Adapter_b<EVENT>\n");
  }
  virtual TLMReturn_t handlePayload(Payload_t& _trans,
                                    Phase_t& _phase,
                                    SysCTime_t& _delay) 
  {
    out.verbose(CALL_INFO,200,0,
                "ID %d: handlePayload 0x%llX\n",
                streaming_width_id,_trans.get_address());
    payload_queue.notify(_trans,_phase,_delay);
    return tlm::TLM_ACCEPTED;
  }
 public:
  EventMap_t event_map; ///< Used to keep track of EVENTs
  PayloadMap_t payload_map;///< Used to keep track of payloads
};

/** Refines the basic adapter to be downstream specific, acting like a
 * SystemC initiator socket
 * @tparam EVENT Specifies the type of event we are adapting */
template<typename EVENT>
class DownStreamAdapter
: public Adapter_b<EVENT>
{
 private:
  typedef SST::SysC::DownStreamAdapter<EVENT>                 ThisType;
  typedef SST::SysC::Adapter_b<EVENT>                         BaseType;
 protected:
  typedef typename SST::SysC::InitiatorSocketProxy<BaseType>  Proxy_t;
  typedef typename Proxy_t::Socket_t                          Socket_t;
  typedef typename BaseType::TLMReturn_t                      TLMReturn_t;
  typedef typename BaseType::Payload_t                        Payload_t;
  typedef typename BaseType::Phase_t                          Phase_t;
  typedef typename BaseType::Event_t                          Event_t;
  typedef typename BaseType::SysCTime_t                       SysCTime_t;
  typedef typename BaseType::SSTTime_t                        SSTTime_t;
  typedef typename BaseType::EventQueue_t                     EventQueue_t;
 public:
  DownStreamAdapter(SST::Component *_comp,SST::Params& _params)
      : BaseType(_comp,_params)
  {
    this->out.verbose(CALL_INFO,3,0,
                "Constructing DownStreamAdapter<EVENT>\n");
    std::string module_name="DownStreamAdapter";
    module_name.append(std::to_string(this->id));
    std::string socket_name="DownStreamAdapterSocket";
    socket_name.append(std::to_string(this->id));
    this->socket_proxy = new Proxy_t(typename Proxy_t::Name_t(module_name.c_str()),
                               socket_name.c_str()
                              );
    socket=&(static_cast<Proxy_t*>(this->socket_proxy)->socket);
    this->out.verbose(CALL_INFO,10,0,
                "Created Socket Proxy: name = %s,socket = %s\n",
                module_name.c_str(),socket_name.c_str()
               );
    (*socket).register_nb_transport_bw(this,
                                       &BaseType::handlePayload);
    this->out.verbose(CALL_INFO,10,0,
                "Registered Backward Transport Handler\n");
  }

  Socket_t* getSocket(){return socket;}

  void callBack(Payload_t& _trans, const Phase_t& _phase) {
    this->out.verbose(CALL_INFO,200,0,
                "ID %d: callBack 0x%llX\n",
                this->streaming_width_id,_trans.get_address());
    switch(_phase){
    case tlm::END_REQ:
      if(this->forward_systemc_end)
        this->forwardEnd(_trans,0);
      break;
    case tlm::BEGIN_RESP:
      {
        EVENT* ev=makeResponse<EVENT>(this->out,_trans,this);
        updateEvent<EVENT>(this->out,*ev,_trans,this,!this->no_payload);
        sendResponse(this->out,_trans,*ev,this,this->sst_link);
        EventQueue_t* q=this->event_map[_trans.get_address()];
        this->out.verbose(CALL_INFO,220,0,"Poping off queue\n");
        delete q->front();
        q->pop();
        if(0 == q->size()){
          this->out.verbose(CALL_INFO,220,0,"Queue empty, deleting\n");
          delete q;
          this->event_map.erase(_trans.get_address());
        }
        if(this->create_systemc_end)
          this->createEnd(_trans,0);
      }
      //TODO: Fix when HMC is fixed
      for(auto i=_trans.get_ref_count();i>0;--i)
        _trans.release();
      break;
    case tlm::BEGIN_REQ:
    case tlm::END_RESP:
      this->out.fatal(CALL_INFO,-1,
                "Improper phase for backward path!\n");
      break;
    default:
      this->out.fatal(CALL_INFO,-1,
                "Unknown phase!\n");
    }
  }

  void handleSSTEvent(Event_t* _event){
    EVENT* ev=dynamic_cast<EVENT*>(_event);
    assert(ev);
    this->out.verbose(CALL_INFO,200,0,
                "handleSSTEvent 0x%lX\n",ev->getAddr());
    auto q = this->event_map.find(ev->getAddr());
    if(this->event_map.end() == q){
      this->out.verbose(CALL_INFO,220,0,"Creating new EventQueue\n");
      this->event_map[ev->getAddr()]=new EventQueue_t();
    }
    this->out.verbose(CALL_INFO,220,0,"Pushing onto queue\n");
    this->event_map[ev->getAddr()]->push(ev);
    Phase_t phase=tlm::BEGIN_REQ;
    SysCTime_t delay=SC_ZERO_TIME;
    Payload_t *trans=makePayload<EVENT>(this->out,*ev,this,!this->no_payload);
    //TODO: fix when HMC is fixed
    trans->acquire();
    trans->acquire();
    trans->acquire();
    TLMReturn_t status=(*socket)->nb_transport_fw(*trans,phase,delay);
    switch(status){
    case tlm::TLM_UPDATED:
      this->out.verbose(CALL_INFO,100,0,"Payload Sent, Status = TLM_UPDATED\n");
      //This should indicate that the return path was used, send it to the
      //callback queue
      phase=tlm::BEGIN_RESP;
      //payload_queue.notify(*trans,phase,delay); //FIX when fixed in HMC
      break;
    case tlm::TLM_ACCEPTED:
      this->out.verbose(CALL_INFO,100,0,"Payload Sent, Status = TLM_ACCEPTED\n");
      //This indicates that nb_transport_bw path will be used
      break;
    case tlm::TLM_COMPLETED:
      this->out.verbose(CALL_INFO,100,0,"Payload Sent, Status = TLM_COMPLETED\n");
      //can indicate an error
      break;
    default:
      this->out.fatal(CALL_INFO,-1,"Payload Sent, Status = Unexpected Status\n");
    }
    this->controller->requestCrunch(this->parent_component->getId());
  }
  void forwardEnd(Payload_t& _trans,
                 const SSTTime_t& _delay) {
    this->out.fatal(CALL_INFO,-1,"Unimplemented forwardEnd\n");
    //TODO ACK/NACK?
  }

  void createEnd(Payload_t& _trans,
                 const SSTTime_t& _delay) {
    Phase_t phase=tlm::END_RESP;
    SysCTime_t delay=SC_ZERO_TIME;
    (*socket)->nb_transport_fw(_trans,phase,delay);
  }

 protected:
  Socket_t  *socket;
};

/** Refines the basic adapter to an upstream adapter, functioning as a
 * SystemC target socket
 * @tparam EVENT Specifies the type of event we are adapting */
template<typename EVENT>
class UpStreamAdapter
: public Adapter_b<EVENT>
{
 private:
  typedef SST::SysC::UpStreamAdapter<EVENT>               ThisType;
  typedef SST::SysC::Adapter_b<EVENT>                     BaseType;
 protected:
  typedef typename SST::SysC::TargetSocketProxy<BaseType> Proxy_t;
  typedef typename Proxy_t::Socket_t                          Socket_t;
  typedef typename BaseType::TLMReturn_t                      TLMReturn_t;
  typedef typename BaseType::Payload_t                        Payload_t;
  typedef typename BaseType::Phase_t                          Phase_t;
  typedef typename BaseType::Event_t                          Event_t;
  typedef typename BaseType::SysCTime_t                       SysCTime_t;
  typedef typename BaseType::SSTTime_t                        SSTTime_t;
  typedef typename BaseType::PayloadQueue_t                   PayloadQueue_t;
 public:
  UpStreamAdapter(SST::Component *_comp,SST::Params& _params)
      : BaseType(_comp,_params)
  {
    this->out.verbose(CALL_INFO,3,0,
                "Constructing UpStreamAdapter<EVENT>\n");
    std::string module_name="UpStreamAdapter";
    module_name.append(std::to_string(this->id));
    std::string socket_name="UpStreamAdapterSocket";
    socket_name.append(std::to_string(this->id));
    this->socket_proxy = new Proxy_t(typename Proxy_t::Name_t(module_name.c_str()),
                               socket_name.c_str()
                              );
    socket=&(static_cast<Proxy_t*>(this->socket_proxy)->socket);
    this->out.verbose(CALL_INFO,10,0,
                "Created Socket Proxy: name = %s,socket = %s\n",
                module_name.c_str(),socket_name.c_str()
               );
    (*socket).register_nb_transport_fw(this,
                                       &BaseType::handlePayload);
    this->out.verbose(CALL_INFO,10,0,
                "Registered Backward Transport Handler\n");
  }
  Socket_t* getSocket(){return socket;}

  void callBack(Payload_t& _trans, const Phase_t& _phase) {
    this->out.verbose(CALL_INFO,200,0,
                "ID %d: callBack 0x%llX\n",
                this->streaming_width_id,_trans.get_address());
    switch(_phase){
    case tlm::END_RESP:
      if(this->forward_systemc_end)
        this->forwardEnd(_trans,0);
      break;
    case tlm::BEGIN_REQ:
      {
        auto q=this->payload_map.find(_trans.get_address());
        if(this->payload_map.end() == q){
          this->out.verbose(CALL_INFO,220,0,"Creating Queue\n");
          this->payload_map[_trans.get_address()]=new PayloadQueue_t();
        }
        this->out.verbose(CALL_INFO,220,0,"Pushing on to queue\n");
        this->payload_map[_trans.get_address()]->push(&_trans);
        EVENT* ev=makeEvent<EVENT>(this->out,_trans,this,!this->no_payload);
        this->sst_link->send(ev);
        if(this->create_systemc_end)
          this->createEnd(_trans,0);
      }
      //TODO: Fix when HMC is fixed
      for(auto i=_trans.get_ref_count();i>0;--i)
        _trans.release();
      break;
    case tlm::END_REQ:
    case tlm::BEGIN_RESP:
      this->out.fatal(CALL_INFO,-1,
                "Improper phase for forward path!\n");
      break;
    default:
      this->out.fatal(CALL_INFO,-1,
                "Unknown phase!\n");
    }
  }
  void handleSSTEvent(Event_t* _event){
    EVENT* ev=dynamic_cast<EVENT*>(_event);
    assert(ev);
    this->out.verbose(CALL_INFO,200,0,
                "handleSSTEvent 0x%lX\n",ev->getAddr());
    Payload_t *trans=makeResponse<EVENT>(this->out,*ev,this);
    updatePayload<EVENT>(this->out,*trans,*ev,this,!this->no_payload);
    PayloadQueue_t *q=this->payload_map[ev->getAddr()];
    this->out.verbose(CALL_INFO,220,0,"Popping from queue\n");
    delete q->front();
    q->pop();
    if(0 == q->size()){
      this->out.verbose(CALL_INFO,220,0,"Queue empty, deleting\n");
      delete q;
      this->payload_map.erase(ev->getAddr());
    }
    auto status = sendResponse(this->out,*trans,*ev,this,socket);
    switch(status){
    case tlm::TLM_UPDATED:
      this->out.verbose(CALL_INFO,100,0,"Payload Sent, Status = TLM_UPDATED\n");
      //This should indicate that the return path was used, send it to the
      //callback queue
      //phase=tlm::BEGIN_RESP;
      //payload_queue.notify(*trans,phase,delay); //FIX when fixed in HMC
      break;
    case tlm::TLM_ACCEPTED:
      this->out.verbose(CALL_INFO,100,0,"Payload Sent, Status = TLM_ACCEPTED\n");
      //This indicates that nb_transport_bw path will be used
      break;
    case tlm::TLM_COMPLETED:
      this->out.verbose(CALL_INFO,100,0,"Payload Sent, Status = TLM_COMPLETED\n");
      //can indicate an error
      break;
    default:
      this->out.fatal(CALL_INFO,-1,"Payload Sent, Status = Unexpected Status\n");
    }
    this->controller->requestCrunch(this->parent_component->getId());
  }
  void forwardEnd(Payload_t& _trans,
                  const SSTTime_t& _delay) {
    this->out.fatal(CALL_INFO,-1,
              "Unimplemented forwardEnd\n");
    //TODO ACK/NACK?
  }

  void createEnd(Payload_t& _trans,
                 const SSTTime_t& _delay) {
    this->out.fatal(CALL_INFO,-1,
              "Unimplemented createEnd\n");
  }
  
 protected:
  Socket_t  *socket;
};

}//namespace SysC
}//namespace SST


#define AD_VERBOSE "verbose_level"
#define DEF_AD_VERBOSE 1
#define AD_USE_LINK "use_link"
#define DEF_AD_USE_LINK 1
#define AD_LINK_NAME "link_name"
#define DEF_AD_LINK_NAME HMCport
#define AD_FW_SC_END "forward_systemc_end"
#define DEF_AD_FW_SC_END 0
#define AD_CR_SC_END "create_systemc_end"
#define DEF_AD_CR_SC_END 0
#define AD_USE_SW_ID "use_streaming_width_as_id"
#define DEF_AD_USE_SW_ID 0
#define AD_SW_ID "streaming_width_id"
#define DEF_AD_SW_ID 0
#define AD_NO_PAYLOAD  "no_payload"
#define DEF_AD_NO_PAYLOAD  1
#define AD_FORCE_SIZE  "force_size"
#define DEF_AD_FORCE_SIZE  0
#define AD_SIZE        "size"
#define DEF_AD_SIZE        32
#define AD_FORCE_ALIGN "force_align"
#define DEF_AD_FORCE_ALIGN 0
#define AD_CHECK_SIZE  "check_size"
#define DEF_AD_CHECK_SIZE  0
#define AD_CHECK_ALIGN "check_align"
#define DEF_AD_CHECK_ALIGN 0
#define AD_IGNORE_INIT "ignore_init_data"
#define DEF_AD_IGNORE_INIT_DATA 0


#define ADAPTER_BASE_PARAMS_1                                                 \
{AD_VERBOSE,                                                                  \
  "uINT: Level of verbose output",                                            \
  STR(DEF_AD_VERBOSE)},                                                       \
{AD_FW_SC_END,                                                                \
  "BOOL: Should we do something when SystemC send an END_REQ/END_RESP?",      \
  STR(DEF_AD_FW_SC_END)},                                                     \
{AD_CR_SC_END,                                                                \
  "BOOL: Should we create SystemC END_REQ/END_RESP?",                         \
  STR(DEF_AD_CR_SC_END)},                                                     \
{AD_NO_PAYLOAD,                                                               \
  "BOOL: Determines if payloads are copied back and forthor if timing only",  \
  STR(DEF_AD_NO_PAYLOAD)},                                                    \
{AD_FORCE_SIZE,                                                               \
  "BOOL: Determines if we force transactions to a certain size",              \
  STR(DEF_AD_FORCE_SIZE)},                                                    \
{AD_SIZE,                                                                     \
  "UINT: Size in bytes to force transaction size to",                         \
  STR(DEF_AD_SIZE)},                                                          \
{AD_FORCE_ALIGN,                                                              \
  "BOOL: Determines if module forces address to align to [" AD_SIZE "]",      \
  STR(DEF_AD_FORCE_ALIGN)},                                                   \
{AD_CHECK_SIZE,                                                               \
  "BOOL: Determine if we error on transactions != [" AD_SIZE "]",             \
  STR(DEF_AD_CHECK_SIZE)},                                                    \
{AD_CHECK_ALIGN,                                                              \
  "BOOL: Determine if we error on transaction alignment != [" AD_SIZE "]",    \
  STR(DEF_AD_CHECK_ALIGN)}

#define ADAPTER_BASE_PARAMS_2                                                 \
{AD_USE_LINK,                                                                 \
  "BOOL: Determine if we should configure a link or not",                     \
  STR(DEF_AD_USE_LINK)},                                                      \
{AD_LINK_NAME,                                                                \
  "STR: Name of the port to use, match the port name used by incoming link",  \
  STR(DEF_AD_LINK_NAME)},                                                     \
{AD_USE_SW_ID,                                                                \
  "BOOL: Should streaming width be used as an id",                            \
  STR(DEF_AD_USE_SW_ID)},                                                     \
{AD_SW_ID,                                                                    \
  "uINT: Id to use in streaming width if not used for intended purpose",      \
  STR(DEF_AD_SW_ID)},                                                         \
{AD_IGNORE_INIT,                                                              \
  "BOOL: Should we ignore sendInitData or throw error",                       \
  STR(DEF_AD_IGNORE_INIT)}

#define MEMEVENT {"memHierarchy.MemEvent",NULL}                               

#define ADAPTER_BASE_PORTS                                                    \
{"[" AD_LINK_NAME "]",                                                        \
  "Link that the adapter uses for events coming from SST",                    \
  NULL}                                                                   


#endif//COMMON_DOWNSTREMADAPTER_H
