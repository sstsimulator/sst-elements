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
#include "sst/core/element.h"
#include "../common/MEgenerator.h"
#include "../common/MEanouncer.h"
#include "../common/MEmemtest.h"
#include "tests/DownStreamAdapterTest.h"
#include <cstdio>
using namespace SST;
using namespace SST::SysC;
extern "C" {
  static Component* create_MEMemTest(SST::ComponentId_t _id,
                                     SST::Params& _params){
    return new MEMemTest(_id,_params);
  }

  static const ElementInfoParam MEMemTest_params[]={
    {"num_cycles",
      "Number of memEvents to generate",
      "100"},
    {"data_size",
      "size in bytes of generators memory",
      "64"},
    {"payload_size",
      "size in bytes of the payload to be sent",
      "32"},
    {"clock",
      "clock rate in frequency or period",
      "1MHz"},
    {NULL, NULL, NULL}

  };

  static Component* create_Controller(SST::ComponentId_t _id,
                                      SST::Params& _params){
    return new Controller(_id,_params);
  }
  static const ElementInfoParam controller_params[]={
    {"cycle_period",
      "Number of [cycle_units] between automatic catchups",
      "1"},
    {"cycle_units",
      "time unit to express cycle period in",
      "ns"},
    {"crunch_threshold",
      "Delay in [crunch_threshold_units] before initiating crunch after first request comes in",
      "1"},
    {"crunch_threshold_units",
      "time unit to express [crunch_threshold] in",
      "ns"},
    {NULL,NULL,NULL}
  };
  static Component* create_TLMAnnouncerWrapper(SST::ComponentId_t _id,
                                              SST::Params& _params){
    return new TLMAnnouncerWrapper(_id,_params);
  }
  static const ElementInfoParam adapter_params[] = {
    ADAPTER_BASE_PARAMS,
    {NULL,NULL,NULL}
  };
  static const ElementInfoPort adapter_ports[] = {
    ADAPTER_BASE_PORTS,
    {NULL,NULL,NULL}
  };

  static Component* create_MEGenerator(SST::ComponentId_t _id,
                                       SST::Params& _params) {
    printf("create_meGenerator\n");
    return new MEGenerator(_id,_params);
  }

  static const ElementInfoParam MEgenerator_params[] = {
    MEGENERATOR_PARAMS,
    {NULL, NULL, NULL}
  };

  static const ElementInfoPort MEgenerator_ports[] = {
    MEGENERATOR_PORTS,
    {NULL,NULL,NULL}
  };

  static Component* create_MEAnnouncer(SST::ComponentId_t _id,
                                      SST::Params& _params) {
    return new MEAnnouncer(_id,_params);
  }
  static const ElementInfoParam MEanouncer_params[] = {
    MEANOUNCER_PARAMS,
    {NULL, NULL, NULL}
  };
  static const ElementInfoPort MEanouncer_ports[] = {
    MEANOUNCER_PORTS,
    {NULL,NULL,NULL}
  };

  static const ElementInfoComponent components[]={
    { "MEMemTest",
      "Compares reads to writes to test memory function",
      NULL,
      create_MEMemTest,
      MEMemTest_params,
      MEgenerator_ports,
      0
    },
    { "MEGenerator",
      "memEvent Generator Component",
      NULL,
      create_MEGenerator,
      MEgenerator_params,
      MEgenerator_ports,
      0
    },
    { "MEAnnouncer",
      "memEvent Announcer(reciever) Component",
      NULL,
      create_MEAnnouncer,
      MEanouncer_params,
      MEanouncer_ports,
      0
    },
    { "TLMAnnouncerWrapper",
      "A test of the DownStreamAdapter<memEvent>",
      NULL,
      create_TLMAnnouncerWrapper,
      adapter_params,
      adapter_ports,
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
    {NULL,NULL,NULL,NULL,NULL,NULL,0}
  };

  ElementLibraryInfo SysCTests_eli = {
    "SysCTests",
    "Components for testing SystemC",
    components,
  };
}
