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

#include <stdio.h>
#include <stdarg.h>

#include "msgapi.h"

using namespace std;
using namespace SST::Hermes;

/*static Module*
load_torus_topology(Params& params)
{
    return new topo_torus(params);
}

static Module*
load_singlerouter_topology(Params& params)
{
    return new topo_singlerouter(params);
}

static Module*
load_fattree_topology(Params& params)
{
    return new topo_fattree(params);
}

static Module*
load_dragonfly_topology(Params& params)
{
    return new topo_dragonfly(params);
}*/

static const ElementInfoModule modules[] = {
   /* { "torus",
      "Torus topology object",
      NULL,
      load_torus_topology,
      NULL,
    },*/
    { NULL, NULL, NULL, NULL, NULL }
};

extern "C" {
    ElementLibraryInfo hermes_eli = {
        "hermes",
        "Message exchange interfaces",
        NULL, //components,
	NULL,
	NULL,
	modules,
	// partitioners,
	// generators,
	NULL,
	NULL,
    };
}
