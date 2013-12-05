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
using namespace SST::SimpleRandomDistribComponent;

SimpleDistribComponent::SimpleDistribComponent(ComponentId_t id, Params& params) :
  Component(id) {

  // tell the simulator not to end without us
  registerAsPrimaryComponent();
  primaryComponentDoNotEndSim();

  rng_max_count = params.find_integer("count", 1000);
  rng_count = 0;

  std::string distrib_type = params.find_string("distrib", "gaussian");
  if("gaussian" == distrib_type || "normal" == distrib_type) {
	double mean = params.find_floating("mean", 1.0);
	double stddev = params.find_floating("stddev", 0.2);

	comp_distrib = new SSTGaussianDistribution(mean, stddev);
  } else if("exponential" == distrib_type) {
	double lambda = params.find_floating("lambda", 1.0);
	comp_distrib = new SSTExponentialDistribution(lambda);
  } else {
	std::cerr << "Unknown distribution type." << std::endl;
	exit(-1);
  }

  //set our clock
  registerClock( "1GHz", new Clock::Handler<SimpleDistribComponent>(this,
			&SimpleDistribComponent::tick ) );
}

SimpleDistribComponent::SimpleDistribComponent() :
    Component(-1)
{
    // for serialization only
}

bool SimpleDistribComponent::tick( Cycle_t cyc ) {

	std::cout << "Tick " << cyc << " " << comp_distrib->getNextDouble() << std::endl;

	rng_count++;

	if(rng_max_count == rng_count) {
      		primaryComponentOKToEndSim();
		return true;
	}

	return false;
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
