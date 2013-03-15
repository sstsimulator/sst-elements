// Copyright 2012 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2012, Sandia Corporation
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#include "sst/core/serialization/element.h"
#include "sst/core/element.h"
#include "sst/core/component.h"

#include "cache.h"
#include "bus.h"
#include "trivialCPU.h"
#include "memController.h"

using namespace SST;
using namespace SST::MemHierarchy;

static Component*
create_Cache(SST::ComponentId_t id,
		SST::Component::Params_t& params)
{
	return new Cache( id, params );
}


static Component*
create_Bus(SST::ComponentId_t id,
		SST::Component::Params_t& params)
{
	return new Bus( id, params );
}


static Component*
create_trivialCPU(SST::ComponentId_t id,
		SST::Component::Params_t& params)
{
	return new trivialCPU( id, params );
}


static Component*
create_MemController(SST::ComponentId_t id,
		SST::Component::Params_t& params)
{
	return new MemController( id, params );
}


static const ElementInfoComponent components[] = {
	{ "Cache",
		"Simple Cache Component",
		NULL,
		create_Cache
	},
	{ "Bus",
		"Mem Hierarchy Bus Component",
		NULL,
		create_Bus
	},
	{"trivialCPU",
		"Simple Demo Component",
		NULL,
		create_trivialCPU
	},
	{"trivialMem",
		"Simple Demo Component",
		NULL,
		create_trivialMemory
	},
	{"MemController",
		"Simple Demo Component",
		NULL,
		create_MemController
	},
	{ NULL, NULL, NULL, NULL }
};


extern "C" {
	ElementLibraryInfo memHierarchy_eli = {
		"memHierarchy",
		"Simple Memory Hierarchy",
		components,
	};
}
