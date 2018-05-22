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

#ifndef HMCSIMPLEMEM_H
#define HMCSIMPLEMEM_H
#define SC_INCLUDE_DYNAMIC_PROCESSES
#include <sst/core/sst_types.h>
#include <systemc.h>
#include "tlm.h"
#include "tlm_utils/simple_initiator_socket.h"
#include "tlm_utils/simple_target_socket.h"

#include "HMCSim.h"
#include <../SysC/TLM/TLMsimplemem.h>

#include <sst/core/interfaces/simpleMem.h>
#include <sst/core/output.h>
#include <vector>
namespace SST{
class Component;
class Event;
class Link;
class Params;
namespace SysC{
namespace HMC{

#define INVALID_ID -1
#define NO_STRING "inValiD"

class HMCSimpleMem 
    : public SST::Interfaces::SimpleMem
{
 private:
  typedef HMCSimpleMem                ThisType;
  typedef SST::Interfaces::SimpleMem  BaseType;
 protected:
  typedef BaseType::Request           Request_t;
  typedef SST::SysC::TLMSimpleMem     Interface_t;
  typedef BaseType::Handler<ThisType> ResponseHandler_t;

  typedef Micron::Internal::HMC::Config         Config_t;
  typedef Micron::Internal::HMC::HMCWrapper     HMCWrapper_t;
  typedef Micron::Internal::HMC::HMCController  HMCController_t;
  typedef tlm_utils::simple_target_socket<HMCController_t> TargetSocket_t;
  typedef std::vector<TargetSocket_t*>          TargetSocketVector_t;
  
  
 public:
  HMCSimpleMem(SST::Component* _component,SST::Params& _params);
  ~HMCSimpleMem();
  virtual bool initialize(const std::string& _port_name,
             BaseType::HandlerBase *_handler=NULL);
  virtual void sendInitData(Request_t *_request);
  virtual void sendInitData(SST::Event *_event);
  //virtual SST::Event* recvInitData(); //Use parent
  virtual SST::Link* getLink(void) const;
  virtual void sendRequest(Request_t *_request);
  virtual Request_t* recvResponse();

  Interface_t* getLinkInterface(uint8_t _index);
  void setAcknowledgeHandler(BaseType::HandlerBase *_handler);

 private:
  BaseType::HandlerBase  *response_handler;
  BaseType::HandlerBase  *acknowledge_handler;
  HMCWrapper_t           *hmc;
  SST::Component         *parent_component;
  std::ofstream          *log_file;
  Interface_t*            hmc_simplemem_interfaces[4];
  TargetSocketVector_t    target_sockets;
  Output                  out;
  int32_t                 cube_id;
  uint8_t                 num_sockets;
  bool                    ignore_init_data;
};
#define STR_EXPAND(tok) #tok
#define STR(tok) STR_EXPAND(tok)

#define HMCSM_CUBE_ID         "cube_id"          
#define DEF_HMCSM_CUBE_ID     0
#define HMCSM_CONFIG_FILE     "config_file"      
#define DEF_HMCSM_CONFIG_FILE config.def
#define HMCSM_OUTPUT_FILE     "output_file"
#define DEF_HMCSM_OUTPUT_FILE powerOutputLog.cdf
#define HMCSM_IGNORE_INIT     "ignore_init_data" 
#define DEF_HMCSM_IGNORE_INIT 0

#define HMCSIMPLEMEM_PARAMS                                                   \
{HMCSM_CUBE_ID,                                                               \
  "Integer id of this particular cube",                                       \
  STR(DEF_HMCSM_CUBE_ID)},                                                    \
{HMCSM_CONFIG_FILE,                                                           \
  "file name to load HMC config from",                                        \
  STR(DEF_HMCSM_CONFIG_FILE)},                                                \
{HMCSM_OUTPUT_FILE,                                                           \
  "file name to save power info to",                                          \
  STR(DEF_HMCSM_OUTPUT_FILE)},                                                \
{HMCSM_IGNORE_INIT,                                                           \
  "Determine if we ignore init data or error on it"                           \
  STR(DEF_HMCSM_IGNORE_INIT)}



}//namespace HMC
}//namespace SysC
}//namespace SST
#endif//HMCSIMPLEMEM_H
