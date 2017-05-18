// Copyright 2009-2017 Sandia Corporation. Under the terms
// of Contract DE-NA0003525 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2017, Sandia Corporation
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

#include <assert.h>

#define SST_ELI_COMPILE_OLD_ELI_WITHOUT_DEPRECATION_WARNINGS

#include "sst/core/element.h"
#include "sst/core/component.h"
#include "sst/core/subcomponent.h"

#include "singleThread.h"
#include "memoryHeapLink.h"
#include "memoryHeap.h"

using namespace SST;
using namespace SST::Thornhill;

static Component*
create_MemoryHeap( ComponentId_t id, Params& params ) {
    return new MemoryHeap( id, params);
}

static SubComponent*
load_SingleThread( Component* comp, Params& params ) {
    return new SingleThread(comp, params);
}

static SubComponent*
load_MemoryHeapLink( Component* comp, Params& params ) {
    return new MemoryHeapLink(comp, params);
}

static const ElementInfoParam singleThread_params[] = {
    {   NULL,   NULL,   NULL    }
};

static const ElementInfoParam component_params[] = {
    {   NULL,   NULL,   NULL    }
};

static const char * port_events[] = { "MemoryHeapEvent", NULL };

static const ElementInfoPort component_ports[] = {
    {"detailed%(num_ports)d", "Port connected to Memory Heap client", port_events },
    {NULL, NULL, NULL}
};

static const ElementInfoComponent components[] = {
	{
        "MemoryHeap",
        "Memory Heap.",
        NULL,
        create_MemoryHeap,
        component_params,
        component_ports,
        COMPONENT_CATEGORY_UNCATEGORIZED,
        NULL
    },
    { NULL, NULL, NULL, NULL, NULL, NULL, 0, NULL}
};

static const ElementInfoSubComponent subcomponents[] = {
    { 	"SingleThread",
    	"One worker thread",
    	NULL,
    	load_SingleThread,
    	singleThread_params,
		NULL, // Statistics
    	"SST::Thornhill::SingleThread"
    },

    { 	"MemoryHeapLink",
    	"Link to Memory Heap",
    	NULL,
    	load_MemoryHeapLink,
    	NULL, // Params,
		NULL, // Statistics
    	"SST::Thornhill::SingleThread"
    },

	{   NULL, NULL, NULL, NULL, NULL, NULL, NULL  }
};

extern "C" {
    ElementLibraryInfo thornhill_eli = {
    "Thornhill",
    "Detailed Models Library",
	components,
    NULL,       // Events
    NULL,       // Introspector
	NULL,		// Modules
    subcomponents,
    NULL,
    NULL
    };
}
