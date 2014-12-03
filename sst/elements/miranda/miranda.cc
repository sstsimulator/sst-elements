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
#include <sst/core/serialization.h>
#include <sst/core/element.h>

#include "mirandaCPU.h"
#include "generators/singlestream.h"

using namespace SST;
using namespace SST::Miranda;

static Module* load_SingleStreamGenerator(Component* owner, Params& params) {
	return new SingleStreamGenerator(owner, params);
}

static Component* load_MirandaBaseCPU(ComponentId_t id, Params& params) {
	printf("ALLOCATING A MIRANDA CPU...\n");
	return new RequestGenCPU(id, params);
}

static const ElementInfoParam singleStreamGen_params[] = {
    {"cache_line_size",             "The size of the cache line that this prefetcher is attached to, default is 64-bytes", "64"},
    {NULL, NULL, NULL}
};

static const ElementInfoModule modules[] = {
	{
		"SingleStreamGenerator",
		"Creates a single stream of accesses to/from memory",
		NULL,
		NULL,
		load_SingleStreamGenerator,
		singleStreamGen_params,
		"SST::Miranda::RequestGenerator"
	},
	{ NULL, NULL, NULL, NULL, NULL, NULL }
};

static const ElementInfoParam basecpu_params[] = {
	{ "verbose", 		"Sets the verbosity of output produced by the CPU", 	"0" },
	{ NULL,	NULL, NULL }
};

static const ElementInfoPort basecpu_ports[] = {
    	{ "cache_link",      "Link to Memory Controller", NULL },
    	{ NULL, NULL, NULL }
};

static const ElementInfoComponent components[] = {
	{
		"BaseCPU",
		"Creates a base Miranda CPU ready to load an address generator",
		NULL,
		load_MirandaBaseCPU,
		basecpu_params,
		basecpu_ports,
		COMPONENT_CATEGORY_PROCESSOR
	},
	{ NULL, NULL, NULL, NULL, NULL, NULL, 0 }
};

extern "C" {
    ElementLibraryInfo miranda_eli = {
        "Miranda",
        "Address generator compatible with SST MemHierarchy",
        components,
        NULL,
        NULL,
        modules,
        NULL,
        NULL
    };
}
