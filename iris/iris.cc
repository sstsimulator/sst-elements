/*
 * =====================================================================================
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
 * =====================================================================================
 */

#ifndef  IRIS_SST_LIB_CC_INC
#define  IRIS_SST_LIB_CC_INC
#include <sst_config.h>
#include "sst/core/serialization.h"

#include <sst/core/element.h>
#include "router.h"
#include "ninterface.h"
#include "trig_nic/trig_nic.h"

static SST::Component* 
create_router(SST::ComponentId_t id, 
                SST::Params& params)
{
    return new SST::Iris::Router( id, params );
}

static SST::Component* 
create_ninterface(SST::ComponentId_t id, 
                SST::Params& params)
{
    return new SST::Iris::NInterface( id, params );
}

static SST::Component* 
create_trig_nic(SST::ComponentId_t id, 
                SST::Params& params)
{
    return new SST::Iris::iris_trig_nic( id, params );
}

static const SST::ElementInfoParam routerParams[] = {
  {"id", "network node ID for this router", "Required"},
  {"ports", "number of ports for this router", "5"},
  {"vcs", "Number of Virtual Channels for this Router", "6"},
  {"credits","Starting number of flow control credits", "Required"},
  {"Rc_scheme", "Routing Scheme. Options: \"TWONODE_ROUTING\", \"XY\", \"TORUS_ROUTING\", \"RING_ROUTING\"", "RING_ROUTING"},
  {"no_nodes", "Number of nodes in the network", "64"},
  {"grid_size", "Size of grid", "8"},
  {"buffer_size", "Router buffer size in flits", "Required"},
  {NULL, NULL, NULL}
};

const char *ire[] = {
  "irisRtrEvent",
  NULL
};

static const SST::ElementInfoPort routerPorts[] = {
  {"nic","Connection to the IRIS NIC",ire},
  {"xPos","Postitive X direction",ire},
  {"xNeg","Negative X direction",ire},
  {"yPos","Positive Y direction",ire},
  {"yNeg","Negative Y direction",ire},
  {"zPos","Positive Z direction",ire},
  {"zNeg","Negative Z",ire},
  {NULL, NULL, NULL}
};

static const SST::ElementInfoParam niParams[] = {
  {"id", "network node ID for this NIC", "Required"},
  {"vcs", "Number of Virtual Channels", "6"},
  {"terminal_credits","Number of terminal (e.g. towards the cpu) control credits", "Required"},
  {"rtr_credits","Number of router flow control credits", "Required"},
  {"buffer_size", "Router buffer size in flits", "Required"},
  {NULL, NULL, NULL}
};

static const SST::ElementInfoPort niPorts[] = {
  {"cpu","Terminal link from CPU/Node to NIC",ire},
  {"rtr","Link to the router",ire},
  {NULL, NULL, NULL}
};


static const SST::ElementInfoParam trigParams[] = {
  {"latency"," Through NIC latency","Required"},
  {"timing_set","Three options for sets of timing parameters. See tric_nic/tric_nic.cc setTimingParams() for details.","Required"},
  {NULL, NULL, NULL}
};

static const SST::ElementInfoPort trigPorts[] = {
  {"cpu","Link to the router",ire},
  {NULL, NULL, NULL}
};

static const SST::ElementInfoComponent components[] = {
    { "router",
      "Iris Router",
      NULL,
      create_router,
      routerParams, 
      routerPorts
    },
    { "ninterface",
      "Interface for iris terminal and router",
      NULL,
      create_ninterface,
      niParams,
      niPorts
    },
    { "trig_nic",
      "Triggered operation NIC",
      NULL,
      create_trig_nic,
      trigParams,
      trigPorts
    },
    { NULL, NULL, NULL, NULL, NULL, NULL }
};

static const SST::ElementInfoEvent events[] = {
  { "irisRtrEvent", "Router to Router Event. Contains an iris packet.", NULL, NULL },
  { "irisFlitEvent", "Event containing a single flit.", NULL, NULL },
  { NULL, NULL, NULL, NULL }
};

extern "C" {
    SST::ElementLibraryInfo iris_eli = {
        "iris",
        "IRIS Network Router",
        components,
	events, // Events
	NULL, // Introspectors
	NULL, // Modules
	NULL, // partitioners
	NULL, // generators
	NULL, // Python Module generator
    };
}
#endif   /* ----- #ifndef IRIS_SST_LIB_CC_INC  ----- */
