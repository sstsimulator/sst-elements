// Copyright 2009-2013 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2013, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include "sst_config.h"
#include "sst/core/serialization.h"
#include <assert.h>

#include "sst/core/element.h"
#include <sst/core/params.h>

#include "oberon.h"
#include "oberonev.h"
#include "oberonengine.h"

using namespace SST;
using namespace SST::Oberon;

OberonComponent::OberonComponent(SST::ComponentId_t id,
	SST::Component::Params_t& params) {

	uint32_t memory_size = params.find_integer("memorysize", 32768);
	string model_exe = params.find_string("model", "");

	// check user has specified a model to be run.
	if(model_exe == "") {
		std::cerr << "Oberon: you did not specify a model to execute." << std::endl;
		exit(-1);
	}

	model = new OberonModel(memory_size);

	char* prefix = (char*) malloc(sizeof(char) * 1024);
	sprintf(prefix, "oberon-%lld", (long long int) id);
	string prefixStr = prefix;

	engine = new OberonEngine(prefixStr, model);

	free(prefix);
}

void OberonComponent::handleEvent( SST::Event *ev ) {

}

bool OberonComponent::clockTic( SST::Cycle_t ) {
	OberonEvent* ev;

	while(! engine->instanceHalted()) {
		ev = engine->generateNextEvent();

		delete ev;
	}
}

BOOST_CLASS_EXPORT(OberonComponent)

static Component*
create_oberon(SST::ComponentId_t id,
                  SST::Params& params)
{
    return new OberonComponent( id, params );
}

static const ElementInfoParam component_params[] = {
    { "memorysize", "Size of memory for this instance." },
    { "model", "Model code to be interpreted." },
    { NULL, NULL}
};

static const ElementInfoComponent components[] = {
    { "OberonComponent",
      "Oberon state machine component",
      NULL,
      create_oberon,
      component_params
    },
    { NULL, NULL, NULL, NULL }
};

extern "C" {
    ElementLibraryInfo oberon_eli = {
        "OberonComponent",
        "State machine Component",
        components,
    };
}
