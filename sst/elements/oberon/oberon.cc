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
	Params& params) {

	char* prefix = (char*) malloc(sizeof(char) * 1024);
	sprintf(prefix, "oberon-%010lld", (long long int) id);

	char* prefixOut = (char*) malloc(sizeof(char) * (1024));
	sprintf(prefixOut, "ObrE-[%10lld] ", (long long int) id);

	string prefixStr = prefix;

	int32_t verbose = (int32_t) params.find_integer("verbose", 1);
	Output::output_location_t goto_stdout = (Output::output_location_t) 1;

	string prefixOutStr = prefixOut;
	output = new Output(prefixOutStr, verbose, 0, goto_stdout);

	int32_t memory_size = (int32_t) params.find_integer("memorysize", 32768);
	string model_exe = params.find_string("model", "");

	// check user has specified a model to be run.
	if(model_exe == "") {
		std::cerr << "Oberon: you did not specify a model to execute." << std::endl;
		exit(-1);
	}

	model = new OberonModel(memory_size, prefix, output);
	engine = new OberonEngine(model, output);

	model->setMemoryContentsFromFile(model_exe.c_str());

	if(params.find_string("dumpatend", "no") == "yes")
		dumpAtEnd = true;

	free(prefix);
	free(prefixOut);

	registerAsPrimaryComponent();
  	primaryComponentDoNotEndSim();

	output->verbose(CALL_INFO, 2, 0, "Registering Oberon clock...\n");
	registerClock( "1GHz", new Clock::Handler<OberonComponent>(this,
                     &OberonComponent::clockTic ) );
}

void OberonComponent::handleEvent( SST::Event *ev ) {

}

bool OberonComponent::clockTic( SST::Cycle_t ) {
	OberonEvent* ev;

	while(! engine->instanceHalted()) {
		ev = engine->generateNextEvent();

		if(ev != NULL) {
			delete ev;
		} else {
			primaryComponentOKToEndSim();
			break;
		}
	}

	return true;
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
    { "dumpatend", "Tells the engine to dump memory at the end of execution (yes=dump)" },
    { "verbose", "Sets the level of verbosity for Oberon interpreter output." },
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
