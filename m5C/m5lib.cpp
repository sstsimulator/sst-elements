// Copyright 2009-2015 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2015, Sandia Corporation
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.



#include <sst_config.h>

#include <sst/core/element.h>

#include <bounce.h>
#include <m5.h>
#include <rawEvent.h>
#include <memEvent.h>

using namespace SST;

static Component*
create_M5(ComponentId_t id, Params& params)
{
    return new SST::M5::M5( id, params );
}

static Component*
create_Bounce(ComponentId_t id, Params& params)
{
    return new SST::M5::Bounce( id, params );
}

DeclareSerializable(SST::M5::RawEvent);
DeclareSerializable(SST::M5::MemEvent);



static const ElementInfoParam m5_params[] = {
    {"debug", "Debug flag for the m5c wrapper component", ""},
    {"M5debug", "Debug flags to pass to Gem5", ""},
    {"info", "Info-enable flag to pass to Gem5", "0"},
    {"configFile", "Path to Gem5 Configuration file.", NULL},
    {"statFile", "File into which Gem5 statistics should be written.", NULL},
    {"mem_initializer_port", "Port on which any memory-initialization should occur.", ""},
    {"frequency", "Frequency with wich Gem5 should be called to update.", "1 GHz"},
    {"numBarrier", "Number of Barrier objects to be used.  [NOT RECOMMENDED]", "0"},
    {"memory_trace", "Level of tracing of memory operations", "0"},
    {"registerExit", "Set component as a Primary Component.", "yes"},
    {NULL, NULL, NULL}
};

static const ElementInfoPort m5_ports[] = {
    {"core%d-dcache", "D-Cache ports", NULL},
    {"core%d-icache", "I-Cache ports", NULL},
    {"*", "The Everything Else Port", NULL},
    {NULL, NULL, NULL}
};

static const ElementInfoParam bounce_params[] = {
    {"debug", "Debug flag for the bounce test component", ""},
    {NULL, NULL, NULL}
};


static const ElementInfoPort bounce_ports[] = {
    {"port0", "Port 0", NULL},
    {"port1", "Port 1", NULL},
    {NULL, NULL, NULL}
};


static const ElementInfoComponent components[] = {
    { "M5",
      "M5",
      NULL,
      create_M5,
      m5_params,
      m5_ports,
      COMPONENT_CATEGORY_PROCESSOR|COMPONENT_CATEGORY_MEMORY|COMPONENT_CATEGORY_NETWORK|COMPONENT_CATEGORY_SYSTEM,
    },
    { "Bounce",
      "Bounce test component",
      NULL,
      create_Bounce,
      bounce_params,
      bounce_ports,
      COMPONENT_CATEGORY_UNCATEGORIZED
    },
    { NULL, NULL, NULL, NULL, NULL, NULL, NULL}
};

extern "C" {
    ElementLibraryInfo m5C_eli = {
        "M5",
        "GEM5 Wrapper component",
        components,
    };
}
