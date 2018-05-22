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

#include "sst_config.h"

#define SST_ELI_COMPILE_OLD_ELI_WITHOUT_DEPRECATION_WARNINGS

#include "sst/core/element.h"

//#include "HMCwrapper.h"
#include "HMCsimplemem.h"
#include "HMCadapter.h"
#include "../SysC/TLM/TLMsimplemem.h"

#include <sst/core/params.h>

#include <map>
#include <string>

#include <cstdio>
#define NULLPARAM {NULL,NULL,NULL}
using namespace SST;
using namespace SST::SysC;
using namespace SST::SysC::HMC;
extern "C" {
  static Module* create_HMCSimpleMem(SST::Component* _component,
                                     SST::Params& _params){
    //assert(0);
    printf("create_HMCSimpleMem()");
    fflush(stdin);
    return new HMCSimpleMem(_component,_params);
  }
  static const ElementInfoParam HMCSimpleMem_params[] = {
    HMCSIMPLEMEM_PARAMS,
    TLMSIMPLEMEM_PARAMS_1,
    NULLPARAM
  };
  static Module* create_HMCSimpleMemLink(SST::Component* _component,
                                         SST::Params& _params){
    /* static std::map<uint8_t,HMCSimpleMem*> cubes;
       uint8_t cube_id=_params.find_integer("cube_id",0);
       auto it=cubes.find(cube_id);
       if(it == cubes.end())
       cubes[cube_id]=new HMCSimpleMem(_component,_params);
       uint8_t link_id=_params.find_integer("link_id",0);
       return cubes[cube_id]->getLinkInterface(link_id);
       */
    return NULL;
  }
  static const ElementInfoParam HMCSimpleMemLink_params[] = {
    HMCSIMPLEMEM_PARAMS,
    TLMSIMPLEMEM_PARAMS_1,
    {"link_id",
      "Integer specifying which link to use",
      "0"},
    NULLPARAM
  };
  static const ElementInfoModule  modules[]={
    {"HMCSimpleMem",
      "Interface to HMC using the SimpleMem standard interface",
      NULL,
      NULL,
      create_HMCSimpleMem,
      HMCSimpleMem_params,
      "SST::Interfaces::SimpleMem"},
    {"HMCSimpleMemLink",
      "Interface to a specific HMC link using the SimpleMem stanard interface",
      NULL,
      NULL,
      create_HMCSimpleMemLink,
      HMCSimpleMemLink_params,
      "SST::Interfaces::SimpleMem"},
    {NULL,NULL,NULL,NULL,NULL,NULL,NULL}
  };
  static Component* create_HMCAdapter(SST::ComponentId_t _id,
                                      SST::Params& _params){
    return new HMCAdapter(_id,_params);
  }
  static const ElementInfoParam HMCAdapter_params[] = {
    HMC_PARAMS,
    ADAPTER_BASE_PARAMS_1,
    NULLPARAM
  };
  static const ElementInfoPort HMCAdapter_ports[] = {
    HMC_PORTS,
    {NULL,NULL,NULL}
  };
  /*
     static Component* create_HMCWrapper(SST::ComponentId_t _id,
     SST::Params& _params){
     return new HMCWrapper(_id,_params);
     }
     static const ElementInfoParam HMCWrapper_params[] = {
     HMCWRAPPER_PARAMS,
     NULLPARAM
     };
     static const ElementInfoPort HMCWrapper_ports[] = {
     HMCWRAPPER_PORTS,
     {NULL,NULL,NULL}
     };
     */
  static const ElementInfoComponent components[]={
    {"HMCAdapter",
      "Direct MemEvent connection to HMC",
      NULL,
      create_HMCAdapter,
      HMCAdapter_params,
      HMCAdapter_ports,
      0},
    /*    {"HMCWrapper",
          "Interface to HMC",
          NULL,
          create_HMCWrapper,
          HMCWrapper_params,
          HMCWrapper_ports,
          0},
          */
    {NULL,NULL,NULL,NULL,NULL,NULL,0}
  };
  ElementLibraryInfo HMC_eli = {
    "HMC",
    "Wrapper Component for HMC",
    components,
    NULL,
    NULL,
    modules
  };
}
