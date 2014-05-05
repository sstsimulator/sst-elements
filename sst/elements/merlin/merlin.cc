// Copyright 2009-2014 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2014, Sandia Corporation
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include <sst_config.h>
#include "sst/core/serialization.h"

#include <sst/core/element.h>
#include <sst/core/configGraph.h>

#include "linkControl.h"

#include "hr_router/hr_router.h"
#include "test/nic.h"
#include "test/pt2pt/pt2pt_test.h"

#include "topology/torus.h"
#include "topology/singlerouter.h"
#include "topology/fattree.h"
#include "topology/dragonfly.h"

#include "trafficgen/trafficgen.h"

#include "pymodule.h"

#include <stdio.h>
#include <stdarg.h>

using namespace std;
using namespace SST::Merlin;

static Component*
create_portals_nic(SST::ComponentId_t id,
		   SST::Params& params)
{
    // return new portals_nic( id, params );
    return NULL;
}

static Component*
create_hr_router(SST::ComponentId_t id,
	      SST::Params& params)
{
    return new hr_router( id, params );
}

static Component*
create_test_nic(SST::ComponentId_t id,
		SST::Params& params)
{
    return new nic( id, params );
}

static Component*
create_pt2pt_test(SST::ComponentId_t id,
		  SST::Params& params)
{
    return new pt2pt_test( id, params );
}


static Component*
create_traffic_generator(SST::ComponentId_t id,
        SST::Params& params)
{
    return new TrafficGen( id, params );
}

// static string str(const char* format, ...) {
//     char buffer[256];
//     va_list args;
//     va_start (args, format);
//     vsprintf (buffer,format, args);
//     va_end (args);
//     string ret(buffer);
//     return ret;
// }

static Module*
load_torus_topology(Component* comp, Params& params)
{
    return new topo_torus(comp,params);
}

static Module*
load_singlerouter_topology(Component* comp, Params& params)
{
    return new topo_singlerouter(comp,params);
}

static Module*
load_fattree_topology(Component* comp, Params& params)
{
    return new topo_fattree(comp,params);
}

static Module*
load_dragonfly_topology(Component* comp, Params& params)
{
    return new topo_dragonfly(comp,params);
}

static Module*
load_linkcontrol(Params& params)
{
    return new LinkControl(params);
}

static const ElementInfoComponent components[] = {
    { "portals_nic",
      "NIC with offloaded Portals4 implementation.",
      NULL,
      create_portals_nic,
    },
    { "hr_router",
      "High radix router.",
      NULL,
      create_hr_router,
    },
    { "test_nic",
      "Simple NIC to test base functionality.",
      NULL,
      create_test_nic,
    },
    { "pt2pt_test",
      "Simple NIC to test basic pt2pt performance.",
      NULL,
      create_pt2pt_test,
    },
    { "trafficgen",
      "Pattern-based traffic generator.",
      NULL,
      create_traffic_generator,
    },
    { NULL, NULL, NULL, NULL }
};

static const ElementInfoModule modules[] = {
    { "torus",
      "Torus topology object",
      NULL,
      NULL,
      load_torus_topology,
      NULL,
      "SST::Merlin::Topology"
    },
    { "singlerouter",
      "Simple, single-router topology object",
      NULL,
      NULL,
      load_singlerouter_topology,
      NULL,
      "SST::Merlin::Topology"
    },
    { "fattree",
      "Fattree topology object",
      NULL,
      NULL,
      load_fattree_topology,
      NULL,
      "SST::Merlin::Topology"
    },
    { "dragonfly",
      "Dragonfly topology object",
      NULL,
      NULL,
      load_dragonfly_topology,
      NULL,
      "SST::Merlin::Topology"
    },
    { "linkcontrol",
      "Link Control module for building Merlin-enabled NICs",
      NULL,
      load_linkcontrol,
      NULL,
      NULL,
      "SST::Merlin::LinkControl"
    },
    { NULL, NULL, NULL, NULL, NULL, NULL }
};


extern "C" {
    ElementLibraryInfo merlin_eli = {
        "merlin",
        "Flexible network components",
        components,
        NULL,   // Events
        NULL,   // Introspectors
        modules,
        NULL, // partitioners,
        NULL, // generators,
        genMerlinPyModule  // Python Module Generator
    };
}

BOOST_CLASS_EXPORT(RtrEvent)
BOOST_CLASS_EXPORT(credit_event)
BOOST_CLASS_EXPORT(internal_router_event)

