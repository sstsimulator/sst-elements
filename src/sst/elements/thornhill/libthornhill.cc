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


#include <assert.h>

#include "sst_config.h"

#include "sst/core/subcomponent.h"
#include "sst/core/element.h"

#include "singleThread.h"

using namespace SST;
using namespace SST::Thornhill;

static SubComponent*
load_SingleThread( Component* comp, Params& params ) {
    return new SingleThread(comp, params);
}

static const ElementInfoParam singleThread_params[] = {
//    {   "arg.waitall",          "Sets the wait mode",   "1"},
    {   NULL,   NULL,   NULL    }
};


static const ElementInfoSubComponent subcomponents[] = {
    { 	"SingleThread",
    	"One one worker thread",
    	NULL,
    	load_SingleThread,
    	singleThread_params,
		NULL, // Statistics
    	"SST::Thornhill::SingleThread"
    },
	{   NULL, NULL, NULL, NULL, NULL, NULL, NULL  }
};

extern "C" {
    ElementLibraryInfo thornhill_eli = {
    "Thornhill",
    "Detailed Models Library",
	NULL,		// Components
    NULL,       // Events
    NULL,       // Introspector
	NULL,		// Modules
    subcomponents,
    NULL,
    NULL
    };
}
