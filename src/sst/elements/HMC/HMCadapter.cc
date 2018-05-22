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
#include "HMCadapter.h"
#include <string>
using namespace SST;
using namespace SST::SysC;
using namespace SST::SysC::HMC;
HMCAdapter::HMCAdapter(ComponentId_t _id,
                       Params& _params)
  : BaseType(_id)
    , out("In @p at @f:@l:",0,0,Output::STDOUT)
{
  out.verbose(CALL_INFO,1,0,
              "Constructing HMCAdapter\n");
  std::string time_base=_params.find_string(HMC_TIME_BASE,STR(DEF_HMC_TIME_BASE));
  out.verbose(CALL_INFO,2,0,
              STR(HMC_TIME_BASE) " = %s",time_base.c_str());
  registerTimeBase(time_base);
  log_file=new std::ofstream();
  cube_id = _params.find_integer(HMC_CUBE_ID,DEF_HMC_CUBE_ID);
  out.verbose(CALL_INFO,2,0,
              STR(HMC_CUBE_ID) " = %d\n",cube_id);
  std::string file_name
      = _params.find_string(HMC_OUTPUT_FILE,STR(DEF_HMC_OUTPUT_FILE));
  out.verbose(CALL_INFO,2,0,
              STR(HMC_OUTPUT_FILE) " = %s\n",file_name.c_str());
  log_file->open(file_name.c_str());
  if(!log_file->is_open())
    out.fatal(CALL_INFO,-1,"Could not open power log file\n");
  else
    out.verbose(CALL_INFO,2,0,"Succesfully opened power log file\n");
  std::string config_file
      = _params.find_string(HMC_CONFIG_FILE,STR(DEF_HMC_CONFIG_FILE));
  out.verbose(CALL_INFO,2,0,
              STR(HMC_CONFIG_FILE) " = %s\n",config_file.c_str());
  Config_t *config = Micron::Internal::HMC::GetConfig(config_file);
  if(NULL == config)
    out.fatal(CALL_INFO,-1,"Could not load config\n");
  else
    out.verbose(CALL_INFO,2,0,"Successfully loaded config\n");
  hmc = Micron::Internal::HMC::getHMCWrapper(*config,*log_file);
  if(NULL == hmc)
    out.fatal(CALL_INFO,-1, "Could not get Micron HMC Wrapper\n");
  else
    out.verbose(CALL_INFO,2,0, "Successfully create HMC Wrapper\n");
  target_sockets = hmc->GetControllerSockets();
  uint8_t num_sockets = target_sockets.size();
  if(4 != num_sockets)
    out.fatal(CALL_INFO,-1,"Incorrect link count '%d'\n",num_sockets);
  //TODO ignore_init_data;
  uint8_t id=0;
  _params[AD_USE_SW_ID] = std::to_string(1);
  _params[AD_USE_LINK] = std::to_string(1);
  for(auto N : adapters){
    std::string link_name= HMC_LINK_PREFIX;
    link_name.append(std::to_string(id));
    _params[AD_LINK_NAME] = link_name;
    _params[AD_SW_ID] = std::to_string(id);
    N = dynamic_cast<Adapter_t*>(
        loadModuleWithComponent("SysC.ME_DS_Adapter",
                                this,
                                _params)
        );
    if(NULL == N)
      out.fatal(CALL_INFO,-1,"Could not load module\n");
    links[id] = N->getLink();
    (N->getSocket())->bind(*(target_sockets[id]));
    ++id;
    }
}
HMCAdapter::~HMCAdapter(){
  hmc->DumpStats();
  delete hmc;
  for(int i=0;i<4;++i)
    delete adapters[i];
  log_file->close();
}
