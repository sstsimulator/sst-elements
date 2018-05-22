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
#include "HMCsimplemem.h"
#include <fstream>
#include <sst/core/component.h>
#include <sst/core/params.h>

using namespace SST;
using namespace SST::SysC;
using namespace SST::SysC::HMC;

HMCSimpleMem::HMCSimpleMem(Component* _component,
                           Params& _params)
: BaseType(_component,_params)
    , parent_component(_component)
    , out("In @p at @f:@l:",0,0,Output::STDOUT)
{
  out.output("Creating HMCSimpleMem\n");
  for(uint8_t i=0;i<4;++i)
    hmc_simplemem_interfaces[i]=NULL;
  log_file=new std::ofstream();
  cube_id = _params.find_integer(HMCSM_CUBE_ID,DEF_HMCSM_CUBE_ID);
  /*if(INVALID_ID == cube_id)
    out.fatal(CALL_INFO,-1,
              "Invalid 'cube_id' in sdl file\n");*/
  std::string config_file=_params.find_string(HMCSM_CONFIG_FILE,STR(DEF_HMCSM_CONFIG_FILE));
  /*if(NO_STRING == config_file)
    out.fatal(CALL_INFO,-1,
              "No HMC config file specified\n");*/
  std::string file_name = _params.find_string(HMCSM_OUTPUT_FILE,STR(DEF_HMCSM_OUTPUT_FILE));
  log_file->open(file_name.c_str());
  if(!log_file->is_open())
    out.fatal(CALL_INFO,-1,
              "Could not open power log file");
  Config_t *config = Micron::Internal::HMC::GetConfig(config_file);
  if(NULL == config)
    out.fatal(CALL_INFO,-1,
              "Error Loading config\n");
  hmc = Micron::Internal::HMC::getHMCWrapper(*config,*log_file);
  if(NULL == hmc)
    out.fatal(CALL_INFO,-1,
              "Error getting Micron HMC Wrapper\n");
  target_sockets = hmc->GetControllerSockets();
  num_sockets = target_sockets.size();
  out.output("Number if sockets = %d\n",num_sockets);
  ignore_init_data = _params.find_integer(HMCSM_IGNORE_INIT,DEF_HMCSM_IGNORE_INIT);
  _params[TLMSM_USE_SW_ID]=std::to_string(1);
  for(uint8_t i=0;i<num_sockets;++i){
    _params[TLMSM_SW_ID] = std::to_string(i);
    //sc_core::sc_module_name* temp=new sc_core::sc_module_name("TLMSimpleMem");
    hmc_simplemem_interfaces[i] = dynamic_cast<Interface_t*> ( 
        parent_component->loadModuleWithComponent("SysC.TLMSimpleMem",
                                                  parent_component,
                                                  _params)
        );
    if(NULL == hmc_simplemem_interfaces[i])
      out.fatal(CALL_INFO,-1,
                "Error loading module\n");
    (hmc_simplemem_interfaces[i]->getSocket())->bind(*(target_sockets[i]));
  }
}

bool HMCSimpleMem::initialize(const std::string& _port_name,
                              BaseType::HandlerBase *_handler){
  response_handler = _handler;
  if(NULL == response_handler)
    out.fatal(CALL_INFO,-1,
              "Must have a response handler\n");
  //out.output("num_sockets=%d",num_sockets);
  for(uint8_t i=0;i<num_sockets;++i)
    if(NULL==hmc_simplemem_interfaces[i])
      out.fatal(CALL_INFO,-1,"hmc_simplemem_interfaces[%d]==NULL",i);
    else
      hmc_simplemem_interfaces[i]->initialize("",response_handler);
  return (NULL != response_handler);
}

void HMCSimpleMem::sendInitData(Request_t *_request){
  if(!ignore_init_data)
    out.fatal(CALL_INFO,-1,
              "Cannot Initialize SystemC components via sendInitData\n");
}

void HMCSimpleMem::sendInitData(SST::Event *_event){
  if(!ignore_init_data)
    out.fatal(CALL_INFO,-1,
              "Cannot Initialize SystemC components via sendInitData\n");
}
SST::Link* HMCSimpleMem::getLink(void) const {
  out.fatal(CALL_INFO,-1,
            "SystemC does not use links\n");
  return NULL;
}
void HMCSimpleMem::sendRequest(Request_t *_request){
  static uint32_t last;
  //out.output("HMCSimpleMem::sendRequest()\n");
  //out.output("ID = %d\n",_request->id);
  ++last;
  if(last >= num_sockets)
    last=0;
  hmc_simplemem_interfaces[last]->sendRequest(_request);
}
HMCSimpleMem::Request_t* HMCSimpleMem::recvResponse(){
  out.fatal(CALL_INFO,-1,
            "This method should not be called, use getLinkInterface()\n");
  return NULL;
}
HMCSimpleMem::Interface_t* HMCSimpleMem::getLinkInterface(uint8_t _index){
  if(3 < _index)
    out.fatal(CALL_INFO,-1,
              "HMC Link index out of range[0-3]\n");
  return hmc_simplemem_interfaces[_index];
}
void HMCSimpleMem::setAcknowledgeHandler(BaseType::HandlerBase *_handler){
  if(!_handler)
    out.fatal(CALL_INFO,-1,"Cannot have a null acknowdlege handler\n");
  acknowledge_handler = _handler;
  for(uint8_t i=0;hmc_simplemem_interfaces[i];++i)
    hmc_simplemem_interfaces[i]->setAcknowledgeHandler(acknowledge_handler);
}
HMCSimpleMem::~HMCSimpleMem(){
  log_file->close();
  hmc->DumpStats();
}
