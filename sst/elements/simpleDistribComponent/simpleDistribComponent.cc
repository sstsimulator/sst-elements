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
#include "simpleDistribComponent.h"

#include <assert.h>

#include "sst/core/element.h"
#include "sst/core/params.h"

using namespace SST;
using namespace SST::RNG;
using namespace SST::SimpleDistribComponent;

SimpleDistribComponent::SimpleDistribComponent(ComponentId_t id, Params& params) :
  Component(id) {

  // tell the simulator not to end without us
  registerAsPrimaryComponent();
  primaryComponentDoNotEndSim();

  //set our clock
  registerClock( "1GHz", new Clock::Handler<simpleDistribComponent>(this,
			&SimpleDistribComponent::tick ) );
}

SimpleDistribComponent::SimpleDistribComponent() :
    Component(-1)
{
    // for serialization only
}

bool SimpleDistribComponent::tick( Cycle_t ) {
      	primaryComponentOKToEndSim();
	return true;
}

// Element Library / Serialization stuff

BOOST_CLASS_EXPORT(SimpleDistribComponent)

static Component*
create_simpleDistribComponent(SST::ComponentId_t id,
                  SST::Params& params)
{
    return new SimpleDistribComponent( id, params );
}

static const ElementInfoParam component_params[] = {
    { NULL, NULL}
};

static const ElementInfoComponent components[] = {
    { "SimpleDistribComponent",
      "Random number generation component",
      NULL,
      create_simpleDistribComponent,
      component_params
    },
    { NULL, NULL, NULL, NULL }
};

extern "C" {
    ElementLibraryInfo simpleDistribComponent_eli = {
        "SimpleDistribComponent",
        "Random number distribution component",
        components,
    };
}
