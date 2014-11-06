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

#include "proscpu.h"
#include "prostextreader.h"
#include "prosbinaryreader.h"
#include "prosbingzreader.h"


using namespace SST;
using namespace SST::Prospero;

static Component* create_ProsperoCPU(ComponentId_t id, Params& params) {
	return new ProsperoComponent( id, params );
}

static Module* create_TextTraceReader(Component* comp, Params& params) {
	return new ProsperoTextTraceReader(comp, params);
}

static Module* create_BinaryTraceReader(Component* comp, Params& params) {
	return new ProsperoBinaryTraceReader(comp, params);
}

static Module* create_CompressedBinaryTraceReader(Component* comp, Params& params) {
	return new ProsperoCompressedBinaryTraceReader(comp, params);
}

static const ElementInfoParam prospero_params[] = {
    { "verbose", "Verbosity for debugging. Increased numbers for increased verbosity.", "0" },
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

static const ElementInfoParam prosperoCompressedBinaryReader_params[] = {
    { "file", "Sets the file for the trace reader to use", "" },
    { NULL, NULL, NULL }
};

static const ElementInfoPort prospero_ports[] = {
    { "cache_link", "Link to the memHierarchy cache", NULL },
    { NULL, NULL, NULL }
};

static const ElementInfoModule modules[] = {
 	{
		"ProsperoTextTraceReader",
		"Reads a trace from a text file",
		NULL,
		NULL,
		create_TextTraceReader,
		prosperoTextReader_params,
		"SST::Prospero::ProsperoTraceReader"
	},
 	{
		"ProsperoBinaryTraceReader",
		"Reads a trace from a binary file",
		NULL,
		NULL,
		create_BinaryTraceReader,
		prosperoBinaryReader_params,
		"SST::Prospero::ProsperoTraceReader"
	},
 	{
		"ProsperoCompressedBinaryTraceReader",
		"Reads a trace from a compressed binary file",
		NULL,
		NULL,
		create_CompressedBinaryTraceReader,
		prosperoCompressedBinaryReader_params,
		"SST::Prospero::ProsperoTraceReader"
	},
    	{NULL, NULL, NULL, NULL, NULL, NULL}
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
        	NULL, /* Events */
        	NULL, /* Introspectors */
        	modules,
		NULL,
		NULL
	};
}

