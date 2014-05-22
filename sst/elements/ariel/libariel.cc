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
#include "sst/core/element.h"
#include "sst/core/component.h"

//#include "ariel.h"
#include "arielcpu.h"

using namespace SST;
using namespace SST::ArielComponent;

static Component* create_Ariel(ComponentId_t id, Params& params)
{
	return new ArielCPU( id, params );
}

static const ElementInfoParam ariel_params[] = {
    {"verbose", "Verbosity for debugging. Increased numbers for increased verbosity.", "0"},
    {"corecount", "Number of CPU cores to emulate", "1"},
    {"checkaddresses", "Verify that addresses are valid with respect to cache lines", "0"},
    {"memorylevels", "Number of memory levels in the system", "1"},
    {"pagesize%(memorylevels)d", "Page size for memory Level x", "4096"},
    {"pagecount%(memorylevels)d", "Page count for memory Level x", "131072"},
    {"defaultlevel", "Default memory level", "0"},
    {"maxissuepercycle", "Maximum number of requests to issue per cycle, per core", "1"},
    {"maxcorequeue", "Maximum queue depth per core", "64"},
    {"maxtranscore", "Maximum number of pending transactions", "16"},
    {"pipetimeout", "Read timeout between Ariel and traced application", "10"},
    {"cachelinesize", "Line size of the attached caching strucutre", "64"},
    {"arieltool", "Path to the Ariel PIN-tool shared library", NULL},
    {"executable", "Executable to trace", NULL},
    {"appargcount", "Number of arguments to the traced executable", "0"},
    {"apparg%(appargcount)d", "Arguments for the traced executable", NULL},
    {"arielmode", "PIN tool startup mode", "1"},
    {"arielinterceptcalls", "Toggle intercepting library calls", "1"},
    {"tracePrefix", "Prefix when tracing is enable", ""},
    {"clock", "Clock rate at which events are generated and processed", "1GHz"},
	{NULL, NULL, NULL}
};

static const ElementInfoPort ariel_ports[] = {
    {"cache_link_%(corecount)d", "Each core's link to its cache", NULL},
    {NULL, NULL, NULL}
};

static const ElementInfoModule modules[] = {
    {NULL, NULL, NULL, NULL, NULL, NULL}
};


static const ElementInfoComponent components[] = {
	{ "ariel",
        "PIN-based CPU model",
		NULL,
        create_Ariel,
		ariel_params,
        ariel_ports,
        COMPONENT_CATEGORY_PROCESSOR
	},
	{ NULL, NULL, NULL, NULL, NULL, NULL, 0 }
};


extern "C" {
	ElementLibraryInfo ariel_eli = {
		"ariel",
		"PIN-based CPU models",
		components,
        	NULL, /* Events */
        	NULL, /* Introspectors */
        	modules
	};
}
