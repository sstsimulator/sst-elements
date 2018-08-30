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

#include "TLM/memeventadapterfunctions.h"
#include "common/controller.h"
#include "common/simplewedge.h"
#include "TLM/TLMsimplemem.h"
#include "TLM/adapter.h"
//#include <sst/elements/SysC/TLM/adapter.h>
#include "common/util.h"

#include "sst/core/element.h"
using namespace SST;
using namespace SST::SysC;
extern "C" {
  static Module* create_ME_DS_Adapter(SST::Component* _comp,
                                  SST::Params& _params){
    return new DownStreamAdapter<SST::MemHierarchy::MemEvent>(_comp,_params);
  }
  static Module* create_ME_US_Adapter(SST::Component* _comp,
                                      SST::Params& _params){
    return new UpStreamAdapter<SST::MemHierarchy::MemEvent>(_comp,_params);
  }
  static const ElementInfoParam MEAdapter_params[] = {
    ADAPTER_BASE_PARAMS_1,
    ADAPTER_BASE_PARAMS_2,
    NULL_PARAM
  };
  static const ElementInfoPort MEAdapter_ports[] = {
    ADAPTER_BASE_PORTS,
    NULL_PORT
  };
  static Module* create_TLMSimpleMem(SST::Component* _comp,
                                     SST::Params& _params){
    return new TLMSimpleMem(_comp,_params);
  }
  static const ElementInfoParam TLMSimpleMem_params[] = {
    TLMSIMPLEMEM_PARAMS_1,
    TLMSIMPLEMEM_PARAMS_2,
    NULL_PARAM
  };
  static const ElementInfoModule modules[] = {
    {"ME_DS_Adapter",
      "MemEvent Cpu -> SystemC TLM Memory",
      NULL,
      NULL,
      create_ME_DS_Adapter,
      MEAdapter_params,
      "SST::SysC::DownStreamAdapter"},
    {"ME_US_Adapter",
      "SystemC TLM CPU -> MemEvent Memory",
      NULL,
      NULL,
      create_ME_US_Adapter,
      MEAdapter_params,
      "SST::SysC::UpStreamAdapter"
    },
    {"TLMSimpleMem",
      "Interface to SystemC using the SimpleMem standard interface",
      NULL,
      NULL,
      create_TLMSimpleMem,
      TLMSimpleMem_params,
      "SST::Interface::SimpleMem"},
    NULL_MODULE
  };

  static Component* create_MemEventWedge(SST::ComponentId_t _id,
                                       SST::Params& _params){
    return new SimpleWedge<SST::MemHierarchy::MemEvent>(_id,_params);
  }
  static const ElementInfoParam MemEventWedge_params[] = {
    SW_PARAMS,
    NULL_PARAM
  };
  static const ElementInfoPort Wedge_ports[]={
    {"MemoryPort","Port that faces the memory model",NULL},
    {"UpstreamPort","Port that faces upstream (cpu/cache)",NULL},
    NULL_PORT
  };
  static Component* create_Controller(SST::ComponentId_t _id,
                                      SST::Params& _params){
    return new Controller(_id,_params);
  }
  static const ElementInfoParam controller_params[]={
    CONTROLLER_PARAMS,
    NULL_PARAM
  };     
  static const ElementInfoComponent components[]={
    {"MemEventWedge",
      "A simplistic memory store to provide backing along a memEvent Path",
      NULL,
      create_MemEventWedge,
      MemEventWedge_params,
      Wedge_ports,
      0
    },
    {"Controller",
      "Control structure for SystemC interface",
      NULL,
      create_Controller,
      controller_params,
      NULL,
      0
    },
    NULL_COMPONENT
  };
  ElementLibraryInfo SysC_eli = {
    "SysC",
    "Components for interfacing with SystemC",
    components,
    NULL,
    NULL,
    modules
  };
}// extern 'C'

