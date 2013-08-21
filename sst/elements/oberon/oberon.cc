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

#include "oberon.h"
#include "oberonengine.h"

using namespace SST;
using namespace SST::Oberon;

void OberonComponent::handleEvent( SST::Event *ev ) {

}

bool OberonComponent::clockTic( SST::Cycle_t ) {
	OberonEvent ev;

	do {
		ev = engine.generateNextEvent();
	} while(ev.getEventType() != HALT);

}

BOOST_CLASS_EXPORT(OberonComponent)

static Component*
create_oberon(SST::ComponentId_t id,
                  SST::Component::Params_t& params)
{
    return new OberonComponent( id, params );
}

static const ElementInfoParam component_params[] = {
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
