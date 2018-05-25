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

#ifndef SST_ELEMENTS_SYSC_COMMON_LESSSIMPLEMEM_H
#define SST_ELEMENTS_SYSC_COMMON_LESSSIMPLEMEM_H


namespace SST{
namespace Interfaces{

/** upgrades simplmem to handle 4phase transactions via accepted and acknowledge
 * commands.
 * Allows for encoding a a response status in requests
 * provides for recording 'initial request id'
 * provides conenient default behaviors for certain simplemem interfaces
 * User must specificy translate() behavior to and from events.
 */
class LessSimpleMem:public SimpleMem{
  typedef BaseType_t::Addr    Address_t;
  /** Subclass for encapsulating all information for a LessSimple Memory 
   * request/response */
  class LessSimpleRequest:public SimpleMem::Request{
   protected:
    //LessSimpleRequest Typedefs
    typedef SimpleMem::Request                BaseType_t;
    typedef BaseType_t::Command               Command_t;
    typedef BaseType_t::size                  Size_t;
    typedef uint8_t                           Data_t;
    typedef std::vector<Data_t>               DataVector_t;
    typedef BaseType_t::Flags                 Flags_t;
    typedef BaseType_t::id_t                  ID_t;
    typedef GenericMemEvent::ResponseStatus_t ResponseStatus_t;
   public:
    inline ResponseStatus_t getResponse() const {return resp;}
    inline void setResponse(ResponseStatus_t _response) {resp=_response;}
    inline ID_t getOriginalID() const {return original_request_id;}
    inline void setOriginalId(ID_t _id) {original_request_id=_id;}
   private:
    ID_t              original_request_id;
    ResponseStatus_t  resp;
   public:
    /** Constructor */
    LessSimpleRequest(Command _command, 
                      Addr _address, 
                      size_t _size, 
                      dataVec &_data, 
                      flags_t _flags = 0) 
        : BaseType_t(_command,_address,_size,_data,_flags)
    {
      resp=RESPONSE_INCOMPLETE;
    }
    /** Constructor */
    LessSimpleRequest(Command _command, 
                      Addr _address, 
                      size_t _size, 
                      flags_t _flags = 0) 
        : BaseType_t(_command,_address,_size,_flags)
    {
      resp=RESPONSE_INCOMPLETE;
    }
  }; //class LessSimpleResponse

  LessSimpleMem(SST::Component  *_component,
                Params          &_params)
      : BaseType_t(_component,_params)
        , component(_component)
  { }

  virtual bool initialize(const std::string &_linkName,
                          HandlerBase *_handler = NULL){

    link = component->configureLink(_linkName,
                                    new Event::Handler<LessSimpleMem>(this,&LessSimpleMem::handleEvent) 
                                   );
    return link;
  }

  virtual SST::Link* getLink() const {return link;}
  virtual void sendInitData(LessSimpleRequest *_request){
    getLink()->sendInitData(translate(_request));
  }

  virtual void handleEvent(Event *_event){
    handler(translate(_event));
  }

  virtual void sendRequest(LessSimpleRequest *_request){
    getLink()->send(translate(_request));
  }

  virtual LessSimpleRequest* recvResponse(){
    return translate(getLink()->recv());
  }

  virtual LessSimpleRequest* translate(Event *_event) = 0;
  virtual Event* translate(LessSimpleRequest *_request) = 0;

  Component*    component;
  Link*         link;
  HandlerBase*  handler;
};

}//namespace interfaces
}//namespace SST
#endif // SST_ELEMENTS_SYSC_COMMON_LESSSIMPLEMEM_H
