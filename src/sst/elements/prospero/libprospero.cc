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

#include <sst_config.h>

#define SST_ELI_COMPILE_OLD_ELI_WITHOUT_DEPRECATION_WARNINGS

#include "sst/core/element.h"
#include "sst/core/component.h"

#include "proscpu.h"
#include "prostextreader.h"
#include "prosbinaryreader.h"
#ifdef HAVE_LIBZ
#include "prosbingzreader.h"
#endif

using namespace SST;
using namespace SST::Prospero;

static Component* create_ProsperoCPU(ComponentId_t id, Params& params) {
	return new ProsperoComponent( id, params );
}

static SubComponent* create_TextTraceReader(Component* comp, Params& params) {
	return new ProsperoTextTraceReader(comp, params);
}

static SubComponent* create_BinaryTraceReader(Component* comp, Params& params) {
	return new ProsperoBinaryTraceReader(comp, params);
}

#ifdef HAVE_LIBZ
static SubComponent* create_CompressedBinaryTraceReader(Component* comp, Params& params) {
	return new ProsperoCompressedBinaryTraceReader(comp, params);
}
#endif

static const ElementInfoParam prospero_params[] = {
    { "verbose", "Verbosity for debugging. Increased numbers for increased verbosity.", "0" },
    { "cache_line_size", "Sets the length of the cache line in bytes, this should match the L1 cache", "64" },
    { "reader",  "The trace reader module to load", "prospero.ProsperoTextTraceReader" },
    { "pagesize", "Sets the page size for the Prospero simple virtual memory manager", "4096"},
    { "clock", "Sets the clock of the core", "2GHz"} ,
    { "max_outstanding", "Sets the maximum number of outstanding transactions that the memory system will allow", "16"},
    { "max_issue_per_cycle", "Sets the maximum number of new transactions that the system can issue per cycle", "2"},
    { NULL, NULL, NULL }
};

static const ElementInfoParam prosperoTextReader_params[] = {
    { "file", "Sets the file for the trace reader to use", "" },
    { NULL, NULL, NULL }
};

static const ElementInfoParam prosperoBinaryReader_params[] = {
    { "file", "Sets the file for the trace reader to use", "" },
    { NULL, NULL, NULL }
};

#ifdef HAVE_LIBZ
static const ElementInfoParam prosperoCompressedBinaryReader_params[] = {
    { "file", "Sets the file for the trace reader to use", "" },
    { NULL, NULL, NULL }
};
#endif

static const ElementInfoPort prospero_ports[] = {
    { "cache_link", "Link to the memHierarchy cache", NULL },
    { NULL, NULL, NULL }
};

static const ElementInfoSubComponent subcomponents[] = {
 	{
		"ProsperoTextTraceReader",
		"Reads a trace from a text file",
		NULL,
		create_TextTraceReader,
		prosperoTextReader_params,
		NULL,
		"SST::Prospero::ProsperoTraceReader"
	},
 	{
		"ProsperoBinaryTraceReader",
		"Reads a trace from a binary file",
		NULL,
		create_BinaryTraceReader,
		prosperoBinaryReader_params,
		NULL,
		"SST::Prospero::ProsperoTraceReader"
	},
#ifdef HAVE_LIBZ
	{
		"ProsperoCompressedBinaryTraceReader",
		"Reads a trace from a compressed binary file",
		NULL,
		create_CompressedBinaryTraceReader,
		prosperoCompressedBinaryReader_params,
		NULL,
		"SST::Prospero::ProsperoTraceReader"
	},
#endif
    	{ NULL, NULL, NULL, NULL, NULL, NULL }
};

static const ElementInfoComponent components[] = {
	{
		"prosperoCPU",
        	"Trace-based CPU model",
		NULL,
        	create_ProsperoCPU,
		prospero_params,
        	prospero_ports,
        	COMPONENT_CATEGORY_PROCESSOR
	},
	{ NULL, NULL, NULL, NULL, NULL, NULL, 0 }
};

extern "C" {
	ElementLibraryInfo prospero_eli = {
		"prospero",
		"Trace-based CPU models",
		components,
        	NULL, // Events
        	NULL, // Introspectors
        	NULL, // Modules
		subcomponents,
		NULL,
		NULL
	};
}

