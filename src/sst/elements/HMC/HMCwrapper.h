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

#ifndef HMCWRAPPER_H
#define HMCWRAPPER_H
#define SC_INCLUDE_DYNAMIC_PROCESSES
#include <systemc.h>
#include "tlm.h"

#include <sst/core/sst_types.h>

#include "sst/elements/SysC/TLM/memEventAdapterFunctions.h"
#include "sst/elements/SysC/TLM/adapter.h"
#include <fstream>

#include "HMCSim.h"

#include <vector>
#include <iostream>
#include <string>
using std::cout;
using std::endl;
namespace SST{
class Params;
namespace SysC{
namespace HMC{

typedef Micron::Internal::HMC::HMCWrapper                 HMCWrapper_t;
typedef Micron::Internal::HMC::HMCController              HMCController_t;
typedef tlm_utils::simple_target_socket<HMCController_t>  TargetSocket_t;
typedef std::vector<TargetSocket_t*>                      TargetSocketVector_t;
typedef Micron::Internal::HMC::Config                     Config_t;
typedef SST::MemHierarchy::MemEvent                       Event_t;

class HMCWrapper
    : public DownStreamAdapter<Event_t>
{
 private:
  typedef HMCWrapper                      ThisType;
  typedef DownStreamAdapter<Event_t>      BaseType;
 public:
  bool Status(){return BaseType::Status();}
  void setup(){BaseType::setup();}
  void finish(){BaseType::finish();}
  void init(uint32_t _p){BaseType::init(_p);}
  HMCWrapper(SST::ComponentId_t _id,SST::Params& _params)
      : DownStreamAdapter<Event_t>(_id,_params)
  {
    std::string config_file=_params.find_string("config_file","");
    if(""==config_file){
      cout<<"no config file specified"<<endl;
      assert(""!=config_file);
    }
    std::ofstream log_file;
    log_file.open("powerOutpurLog.cdf");
    Config_t *config = Micron::Internal::HMC::GetConfig(config_file);
    assert(config);
    HMCWrapper_t *hmc=Micron::Internal::HMC::getHMCWrapper(*config,log_file);
    assert(hmc);
    TargetSocketVector_t sockets=hmc->GetControllerSockets();
    TargetSocket_t *target_socket=sockets.front();
    sysc_adapter->getSocket().bind(*target_socket);
  }
};
#define HMCWRAPPER_PARAMS                                                     \
    ADAPTER_BASE_PARAMS
#define HMCWRAPPER_PORTS                                                      \
    ADAPTER_BASE_PORTS
}//namespace HMC
}//namespace SysC
}//namespace SST
#endif //HMCWRAPPER_H
