// Copyright 2009-2010 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2010, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include <sst_config.h>
#include "sst/core/serialization/element.h"

#include <sst/core/element.h>

#include <sst/core/configGraph.h>

#include <stdio.h>
#include <stdarg.h>

#include "portals_args.h"
#include <boost/tokenizer.hpp>
#include <boost/foreach.hpp>

using namespace std;
using namespace boost;

static Component* 
create_portals_nic(SST::ComponentId_t id, 
                SST::Component::Params_t& params)
{
    return new portals_nic( id, params );
}


static string str(const char* format, ...) {
    char buffer[256];
    va_list args;
    va_start (args, format);
    vsprintf (buffer,format, args);
    va_end (args);
    string ret(buffer);
    return ret;
}


static void partition(ConfigGraph* graph, int ranks) {
}


static void generate(ConfigGraph* graph, string options, int ranks) {
}




static const ElementInfoComponent components[] = {
    { "portals_nic",
      "NIC with offloaded Portals4 implementation.",
      NULL,
      create_portals_nic,
    },
    { NULL, NULL, NULL, NULL }
};

static const ElementInfoPartitioner partitioners[] = {
    { "partitioner",
      "Partitioner for portals4_sm simulations",
      NULL,
      partition,
    },
    { NULL, NULL, NULL, NULL }
};
      
static const ElementInfoGenerator generators[] = {
    { "generator",
      "Generator for portals4_sm simulations",
      NULL,
      generate,
    },
    { NULL, NULL, NULL, NULL }
};
      
					     

extern "C" {
    ElementLibraryInfo portals4_sm_eli = {
        "portals4_sm",
        "State-machine based processor/nic for Portals 4 research",
        components,
	NULL,
	NULL,
	partitioners,
	generators,
    };
}
