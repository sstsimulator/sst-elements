// Copyright 2009-2015 Sandia Coporation.  Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2015, Sandia Corporation
// All rights reserved.
//
// This file is part of the SST software package.  For lucense
// information, see the LICENSE file in the top level directory of the
// distribution.

#include <sst_config.h>

#include <sst/core/serialization.h>
#include <sst/core/element.h>
#include <sst/core/component.h>

#include "gem5.h"


static SST::Component* create_Gem5(SST::ComponentId_t id, SST::Params &params)
{
    return new SST::gem5::Gem5Comp(id, params);
}


static const SST::ElementInfoParam gem5_params[] = {
    {"comp_debug", "Debug information from the component: 0 (off), 1 (stdout), 2 (stderr), 3(file)"},
    {"frequency", "Frequency with which to call into Gem5"},
    {"cmd", "Gem5 command to execute."},
    {"connectors", "list of port-connectors between Gem5 and SST."},
    {"init_mem_port", "What port (if any) on which to send initialized memory data."},
    {"init_mem_conn", "What Port-connector to use between Gem5 & SST for initial memory."},
    {"system_name", "Name of Gem5 system object (default:  system)"},
    {NULL, NULL}
};

static const SST::ElementInfoComponent components[] = {
    { "Gem5",
        "Gem5 simulation component",
        NULL,
        create_Gem5,
        gem5_params
    },
    { NULL, NULL, NULL, NULL, NULL }
};


extern "C" {
    SST::ElementLibraryInfo gem5_eli = {
        "Gem5",
        "Gem5 Simulation",
        components,
    };
}
