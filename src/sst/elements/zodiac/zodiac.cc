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

#include <sst/core/element.h>
#include <sst/core/configGraph.h>

#include <stdio.h>
#include <stdarg.h>

#include "ztrace.h"
#include "zsirius.h"

#ifdef HAVE_ZODIAC_DUMPI
#include "zdumpi.h"
#endif

#ifdef HAVE_ZODIAC_OTF
#include "zotf.h"
#endif

using namespace std;
using namespace SST;
using namespace SST::Zodiac;

//Disabled for current release and development
#if 0
static Component*
create_ZodiacTraceReader(SST::ComponentId_t id,
                  SST::Params& params)
{
    return new ZodiacTraceReader( id, params );
}
#endif

static Component*
create_ZodiacSiriusTraceReader(SST::ComponentId_t id,
                  SST::Params& params)
{
    return new ZodiacSiriusTraceReader( id, params );
}

static const ElementInfoParam sirius_params[] = {
	{ "trace", "Set the trace file to be read in for this end point." },
	{ "os.module", "Sets the messaging API to use for generation and handling of the message protocol" },
	{ "scalecompute", "Scale compute event times by a double precision value (allows dilation of times in traces), default is 1.0", "1.0"},
	{ "verbose", "Sets the verbosity level for the component to output debug/information messages", "0"},
	{ "buffer", "Sets the size of the buffer to use for message data backing, default is 4096 bytes", "4096"},
    { "name","used internally",""},
    { "module","used internally",""},
    	{ NULL, NULL, NULL }
};

static const ElementInfoPort sirius_ports[] = {
    {"nic", "Network Interface port", NULL},
    {"loop", "Firefly Loopback port", NULL},
    {NULL, NULL, NULL}
};

#ifdef HAVE_ZODIAC_DUMPI
static Component*
create_ZodiacDUMPITraceReader(SST::ComponentId_t id,
                  SST::Params& params)
{
    return new ZodiacDUMPITraceReader( id, params );
}
#endif

#ifdef HAVE_ZODIAC_OTF
static Component*
create_ZodiacOTFTraceReader(SST::ComponentId_t id,
                  SST::Params& params)
{
    std::cout << "Constructing a Zodiac OTF Reader..." << std::endl;
    return new ZodiacOTFTraceReader( id, params );
}
#endif

static const ElementInfoComponent components[] = {
// Disabled for current release and development builds
#if 0
    {
	"ZodiacTraceReader",
	"Application Communication Trace Reader",
	NULL,
	create_ZodiacTraceReader,
    },
#endif
    {
	"ZodiacSiriusTraceReader",
	"Application Communication Sirius Trace Reader",
	NULL,
	create_ZodiacSiriusTraceReader,
	sirius_params,
	sirius_ports,
	COMPONENT_CATEGORY_NETWORK
    },
#ifdef HAVE_ZODIAC_DUMPI
    {
	"ZodiacDUMPITraceReader",
	"Application Communication DUMPI Trace Reader",
	NULL,
	create_ZodiacDUMPITraceReader,
    },
#endif
#ifdef HAVE_ZODIAC_OTF
    {
	"ZodiacOTFTraceReader",
	"Application Communication OTF Trace Reader",
	NULL,
	create_ZodiacOTFTraceReader,
    },
#endif
    { NULL, NULL, NULL, NULL }
};

extern "C" {
    ElementLibraryInfo zodiac_eli = {
        "zodiac",
        "Application Communication Tracing Component",
        components,
	NULL,
	NULL,
	NULL, //modules,
	// partitioners,
	// generators,
	NULL,
	NULL,
    };
}
