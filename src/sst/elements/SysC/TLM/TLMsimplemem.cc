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

#include <sst_config.h>
#include "TLMsimplemem.h"

#include "../common/controller.h"
#include "../common/memorymanager.h"

using namespace SST;
using namespace SST::SysC;

TLMSimpleMem::TLMSimpleMem(Component* _component,
                           Params& _params)
: BaseType(_component,_params)
    , proxy("TLMSimpleMem","SimpleMemSocket")
    , parent_component(_component)
    , out("In @p at @f:@l:",1,0,Output::STDOUT)
    , payload_queue(this,&ThisType::callBack)
{
  out.output("Creating TLMSimpleMem\n");
  controller=getPrimaryController();
  if(!controller)
    out.fatal(CALL_INFO,-1,
              "Could not locate controller\n");
  memory_manager = new MemoryManager(&out);
  is_polling = _params.find_integer(TLMSM_IS_POLLING,0);
  if(is_polling)
    response_handler = new RequestHandler_t(this,
                                            &ThisType::pollingResponseHandler);
  else
    response_handler = new RequestHandler_t(this,
                                            &ThisType::defaultResponseHandler);

  acknowledge_request = _params.find_integer(TLMSM_ACKNOWLEDGE,0);
  acknowledge_handler = new RequestHandler_t(this,
                                             &ThisType::defaultResponseHandler);
  track_request_ids =         _params.find_integer(TLMSM_TRACK_ID   ,0 );
  use_streaming_width_as_id = _params.find_integer(TLMSM_USE_SW_ID  ,0 );
  streaming_width_id =        _params.find_integer(TLMSM_SW_ID      ,0 );
  no_payload =                _params.find_integer(TLMSM_NO_PAYLOAD ,1 );
  force_size =                _params.find_integer(TLMSM_FORCE_SIZE ,0 );
  force_align =               _params.find_integer(TLMSM_FORCE_ALIGN,0 );
  enforced_size =             _params.find_integer(TLMSM_SIZE       ,32);
  check_size =                _params.find_integer(TLMSM_CHECK_SIZE ,0 );
  check_align =               _params.find_integer(TLMSM_CHECK_ALIGN,0 );

  //out.output("streaming_width_id = %d\n",streaming_width_id);
  ignore_init_data =          _params.find_integer(TLMSM_IGNORE_INIT,0);
  proxy.socket.register_nb_transport_bw(this, &ThisType::handleBackward);
}

bool TLMSimpleMem::initialize(const std::string &_port_name,
                              BaseType::HandlerBase *_handler){
  if(!is_polling){
    if(NULL == _handler)
      out.fatal(CALL_INFO,-1,
                "Cannot have null handler and not be polling\n");
  }
  else {
    if(NULL != _handler)
      out.fatal(CALL_INFO,-1,
                "Cannot have handler and be polling\n");
  }
  response_handler = _handler;
  return true;
}
void TLMSimpleMem::sendInitData(Request_t *_request){
  if(!ignore_init_data)
    out.fatal(CALL_INFO,-1,
              "Cannot Initialize SystemC components via sendInitData\n");
}
void TLMSimpleMem::sendInitData(SST::Event *_event){
  if(!ignore_init_data)
    out.fatal(CALL_INFO,-1,
              "Cannot Initialize SystemC components via sendInitData\n");
}
SST::Link* TLMSimpleMem::getLink(void) const {
  out.fatal(CALL_INFO,-1,
            "SystemC does not use links\n");
  return NULL;
}
void TLMSimpleMem::sendRequest(Request_t *_request){
  //out.output("TLMSimpleMem::sendRequest()\n");
  Payload_t* trans=makePayload(*_request);
  if(use_streaming_width_as_id){
    //out.output("Using Streaming width as id\n");
    trans->set_streaming_width(streaming_width_id);
  }
  trans->acquire();
  Phase_t phase=tlm::BEGIN_REQ;
  SysCTime_t delay=SC_ZERO_TIME;
  TLMReturn_t status=proxy.socket->nb_transport_fw(*trans,phase,delay);
  switch(status){
  case tlm::TLM_UPDATED:
    //(*response_handler)(updateRequest(*_request,*trans,true));
    //TODO: fix when HMC fixed
    if(track_request_ids)
      id_map[_request->addr]=std::make_pair(_request->id,_request->groupId);
    break;
  case tlm::TLM_ACCEPTED:
  case tlm::TLM_COMPLETED:
    break;
  default:
    out.fatal(CALL_INFO,-1,"Unexpected Status\n");
  }
  controller->requestCrunch(parent_component->getId());
  delete _request;
  //trans->release(); //TODO: fix when HMC is fixed
}
TLMSimpleMem::Request_t* TLMSimpleMem::recvResponse(void){
  if(!is_polling)
    out.fatal(CALL_INFO,-1,
              "Interface is not set to polling\n");
  return nextResponse();
}

TLMSimpleMem::Socket_t* TLMSimpleMem::getSocket(void) {
  return &proxy.socket;
}

void TLMSimpleMem::setAcknowledgeHandler(BaseType::HandlerBase *_handler){
  if(!_handler)
    out.fatal(CALL_INFO,-1,
              "Cannot have null acknowledge handler\n");
  acknowledge_handler = _handler;
}

void TLMSimpleMem::defaultAcknowledgeHandler(Request_t *_request){
  out.fatal(CALL_INFO,-1,
            "No acknowledge handler set via setAcknowledgeHandler()\n");
}

TLMSimpleMem::TLMReturn_t TLMSimpleMem::handleBackward(Payload_t& _trans,
                                                       Phase_t& _phase,
                                                       SysCTime_t& _delay){
  //_trans.acquire(); //TODO: fix when HMC is fixed
  out.output("TLMSimpleMem::handleBackward()\n");
  switch(_phase){
  case tlm::END_REQ:
    if(acknowledge_request)
      payload_queue.notify(_trans,_phase,_delay);
    else
      _trans.release();
    break;
  case tlm::BEGIN_RESP:
    payload_queue.notify(_trans,_phase,_delay);
    break;
  case tlm::BEGIN_REQ:
  case tlm::END_RESP:
  default:
    out.fatal(CALL_INFO,-1,
              "Unrecognized Phase\n");
  }
  return tlm::TLM_ACCEPTED;
}

void TLMSimpleMem::callBack(Payload_t& _trans,
                            const Phase_t& _phase){
  out.output("TLMSimpleMem::callBack()\n");
  Request_t *request=makeRequest(_trans);
  if(NULL == request)
    out.fatal(CALL_INFO,-1,
              "Error creating request\n");
  if(track_request_ids){
   // out.output("Mapping id for address = %x\nMap size = %d\n",request->addr,id_map.size());
    IdPair_t ids=id_map[request->addr];
    request->id=ids.first;
    request->groupId=ids.second;
  }
  if(tlm::END_REQ == _phase){
    (*acknowledge_handler)(request);
  }
  else{
    if(track_request_ids)
      id_map.erase(request->addr);
    _trans.release(); //TODO: fixe when HMC is fixed
    if(Request_t::Read == request->cmd)
      request->cmd = Request_t::ReadResp;
    else
      request->cmd = Request_t::WriteResp;
    (*response_handler)(request);
  }
  //_trans.release(); //TODO: fix when HMC is fixed
}

void TLMSimpleMem::defaultResponseHandler(Request_t *_request){
  out.fatal(CALL_INFO,-1,
            "No responseHandler set via initialize()\n");
}

void TLMSimpleMem::pollingResponseHandler(Request_t *_request){
  request_queue.push_back(_request);
}

TLMSimpleMem::Request_t* TLMSimpleMem::nextResponse(){
  Request_t* next=NULL;
  if(request_queue.size()){
    next=request_queue.front();
    request_queue.pop_front();
  }
  return next;
}

inline TLMSimpleMem::Payload_t* TLMSimpleMem::makePayload(const Request_t& _request) const {
  Payload_t* payload=new Payload_t(memory_manager);
  switch(_request.cmd){
  case Request_t::Read:
  case Request_t::ReadResp:
    payload->set_command(tlm::TLM_READ_COMMAND);
    break;
  case Request_t::Write:
  case Request_t::WriteResp:
    payload->set_command(tlm::TLM_WRITE_COMMAND);
    break;
  default:
    out.fatal(CALL_INFO,-1,
              "Unsupported command type\n");
    break;
  }
  BaseType::Addr new_address=_request.addr;
  if(force_align)
    new_address %= enforced_size;
  else if(check_align)
    if(new_address % enforced_size)
      out.fatal(CALL_INFO,-1,
                "Transaction alignment does not match forced alignment\n");
  payload->set_address(new_address);
  //out.output("\n\n%ld\n%ld\n",_request.size,_request.data.size());
  uint32_t new_size=_request.size;
  if(force_size)
    new_size=enforced_size;
  else if(check_size)
    if(new_size != enforced_size)
      out.fatal(CALL_INFO,-1,
                "Transaction Size '%d' does not match forced size '%d'\n"
                ,new_size,enforced_size);
  payload->set_data_ptr(new uint8_t[new_size]);
  assert(payload->get_data_ptr());
  if(payload->get_command() == tlm::TLM_WRITE_COMMAND){
    if(!no_payload){
      if(_request.data.size() == _request.size){
        if(new_size >= _request.size)
          memcpy(payload->get_data_ptr(),_request.data.data(),_request.size);
        else
          memcpy(payload->get_data_ptr(),_request.data.data(),new_size);
      }
      else{
        out.fatal(CALL_INFO,-1,
                  "_request.data.size() '%ld' != _request.size()'%ld'\n",
                  _request.data.size(),_request.size);
      }
    }
  }
  payload->set_data_length(new_size);
  payload->set_streaming_width(new_size);
  payload->set_response_status(tlm::TLM_INCOMPLETE_RESPONSE);
  return payload;
}

inline TLMSimpleMem::Request_t* TLMSimpleMem::makeRequest(const Payload_t& _payload,bool is_response) const {
  Request_t::Command command=Request_t::Read;
  switch(_payload.get_command()){
  case tlm::TLM_READ_COMMAND:
    if(is_response)
      command = Request_t::ReadResp;
    else
      command = Request_t::Read;
    break;
  case tlm::TLM_WRITE_COMMAND:
    if(is_response)
      command = Request_t::WriteResp;
    else
      command = Request_t::Write;
    break;
  default:
    out.fatal(CALL_INFO,-1,
              "Unsoported command type\n");
    break;
  }
  Request_t* request=new Request_t(command,
                                   _payload.get_address(),
                                   _payload.get_data_length());
  request->data.resize(request->size);
  if(!no_payload)
    memcpy(request->data.data(),_payload.get_data_ptr(),request->size);
  return request;
}

inline TLMSimpleMem::Request_t* TLMSimpleMem::updateRequest(Request_t& _request,
                                                            const Payload_t& _payload,
                                                            bool is_response){
  if(is_response){
    if(Request_t::Read == _request.cmd){
      _request.cmd = Request_t::ReadResp;
      _request.size=_payload.get_data_length();
      _request.data.resize(_request.size);
      memcpy(_request.data.data(),_payload.get_data_ptr(),_request.size);
    }
    else if(Request_t::Write == _request.cmd){
      _request.cmd = Request_t::WriteResp;
    }
  }
  else {
    if(Request_t::Write == _request.cmd){
      _request.size=_payload.get_data_length();
      _request.data.resize(_request.size);
      memcpy(_request.data.data(),_payload.get_data_ptr(),_request.size);
    }
  }
  return &_request;
}
