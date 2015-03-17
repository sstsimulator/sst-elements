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

#include "sst/core/serialization.h"
#include "sst/core/element.h"
#include "sst/core/component.h"

//#include "ariel.h"
#include "arielcpu.h"
#include "arieltexttracegen.h"

#ifdef HAVE_LIBZ
#include "arielgzbintracegen.h"
#endif

using namespace SST;
using namespace SST::ArielComponent;

static Component* create_Ariel(ComponentId_t id, Params& params)
{
	return new ArielCPU( id, params );
};

static Module* load_TextTrace( Component* comp, Params& params) {
	return new ArielTextTraceGenerator(comp, params);
};

static const ElementInfoStatisticEnable ariel_statistics[] = {
    { "read_requests",        "Stat read_requests", 1},   // Name, Desc, Enable Level 
    { "write_requests",       "Stat write_requests", 1}, 
    { "split_read_requests",  "Stat split_read_requests", 1}, 
    { "split_write_requests", "Stat split_write_requests", 1},
    { "no_ops",               "Stat no_ops", 1},
    { "instruction_count",    "Statistic for counting instructions", 1 },
    { NULL, NULL, 0 }
};

static const ElementInfoParam ariel_text_trace_params[] = {
    { "trace_prefix", "Sets the prefix for the trace file", "ariel-core-" },
    { NULL, NULL, NULL }
};

#ifdef HAVE_LIBZ
static Module* load_CompressedBinaryTrace( Component* comp, Params& params) {
	return new ArielCompressedBinaryTraceGenerator(comp, params);
};

static const ElementInfoParam ariel_gzbinary_trace_params[] = {
    { "trace_prefix", "Sets the prefix for the trace file", "ariel-core-" },
    { NULL, NULL, NULL }
};
#endif

static const ElementInfoParam ariel_params[] = {
    {"verbose", "Verbosity for debugging. Increased numbers for increased verbosity.", "0"},
    {"corecount", "Number of CPU cores to emulate", "1"},
    {"checkaddresses", "Verify that addresses are valid with respect to cache lines", "0"},
    {"translatecacheentries", "Keep a translation cache of this many entries to improve emulated core performance", "4096"},
    {"memorylevels", "Number of memory levels in the system", "1"},
    {"pagesize%(memorylevels)d", "Page size for memory Level x", "4096"},
    {"pagecount%(memorylevels)d", "Page count for memory Level x", "131072"},
    {"page_populate_%(memorylevels)d", "Pre-populate/partially pre-populate a page table for a level in memory, this is the file to read in.", ""},
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
    {"arielmode", "Tool interception mode, set to 1 to trace entire program (default), set to 0 to delay tracing until ariel_enable() call.", "1"},
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
    {
	"TextTraceGenerator",
	"Provides tracing to text file capabilities",
	NULL,
	NULL,
	load_TextTrace,
	ariel_text_trace_params,
	"SST::ArielComponent::ArielTraceGenerator"
    },
#ifdef HAVE_LIBZ
    {
	"CompressedBinaryTraceGenerator",
	"Provides tracing to compressed file capabilities",
	NULL,
	NULL,
	load_CompressedBinaryTrace,
	ariel_gzbinary_trace_params,
	"SST::ArielComponent::ArielTraceGenerator"
    },
#endif
    { NULL, NULL, NULL, NULL, NULL, NULL }
};


static const ElementInfoComponent components[] = {
	{ "ariel",
        "PIN-based CPU model",
		NULL,
        create_Ariel,
		ariel_params,
        ariel_ports,
        COMPONENT_CATEGORY_PROCESSOR,
        ariel_statistics
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
