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
#include "sst/core/serialization.h"
#include "sst/core/element.h"
#include "sst/core/params.h"

#include "simpleClockerComponent.h"

using namespace SST;
using namespace SST::SimpleClockerComponent;

simpleClockerComponent::simpleClockerComponent(ComponentId_t id, Params& params) :
  Component(id) {

  clock_frequency_str = params.find_string("clock", "1GHz");
  clock_count = params.find_integer("clockcount", 1000);

  std::cout << "Clock is configured for: " << clock_frequency_str << std::endl;

  // tell the simulator not to end without us
  registerAsPrimaryComponent();
  primaryComponentDoNotEndSim();

  //set our clock
  registerClock( clock_frequency_str,
		 new Clock::Handler<simpleClockerComponent>(this,
			&simpleClockerComponent::tick ) );
}

simpleClockerComponent::simpleClockerComponent() :
    Component(-1)
{
    // for serialization only
}

bool simpleClockerComponent::tick( Cycle_t ) {
	clock_count--;

	// return false so we keep going
	if(clock_count == 0) {
    primaryComponentOKToEndSim();
		return true;
	} else {
		return false;
	}
}

// Element Libarary / Serialization stuff

BOOST_CLASS_EXPORT(simpleClockerComponent)


