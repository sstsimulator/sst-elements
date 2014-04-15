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
	{NULL, NULL}
};

static const ElementInfoModule modules[] = {
    {NULL, NULL, NULL, NULL, NULL, NULL}
};


static const ElementInfoComponent components[] = {
	{ "ariel", "PIN-based CPU model",
		NULL, create_Ariel,
		ariel_params
	},
	{ NULL, NULL, NULL, NULL }
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
