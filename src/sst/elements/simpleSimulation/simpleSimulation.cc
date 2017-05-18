// Copyright 2009-2017 Sandia Corporation. Under the terms
// of Contract DE-NA0003525 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2017, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include "sst_config.h"

#define SST_ELI_COMPILE_OLD_ELI_WITHOUT_DEPRECATION_WARNINGS

#include "sst/core/element.h"

#include "simpleCarWash.h"

using namespace SST;
using namespace SST::SimpleCarWash;

static Component* create_simpleCarWash(SST::ComponentId_t id, SST::Params& params) 
{
    return new simpleCarWash(id, params);
}

static const ElementInfoParam simpleCarWash_params[] = {
    { "clock", "Clock frequency", "1GHz" },
    { "clockcount", "Number of clock ticks to execute", "100000"},
    { NULL, NULL, NULL }
};


static const ElementInfoComponent simpleSimulationComponents[] = {
    { "simpleCarWash",		                         // Name
      "Car Wash Simulator",                              // Description
      NULL,                                              // PrintHelp
      create_simpleCarWash,	                         // Allocator
      simpleCarWash_params,                     // Parameters
      NULL,                                              // Ports
      COMPONENT_CATEGORY_UNCATEGORIZED,                  // Category
      NULL                                               // Statistics
    },
    { NULL, NULL, NULL, NULL, NULL, NULL, 0, NULL}
};

extern "C" {
    ElementLibraryInfo simpleSimulation_eli = {
        "simpleSimulation",                          // Name
        "A Simple Example Element With Demo Components", // Description
        simpleSimulationComponents,                         // Components
        NULL,                                            // Events 
        NULL,                                            // Introspectors 
        NULL,                                            // Modules 
        NULL,                                            // Subcomponents 
        NULL,                                            // Partitioners
        NULL,                                            // Python Module Generator
        NULL                                             // Generators
    };
}


