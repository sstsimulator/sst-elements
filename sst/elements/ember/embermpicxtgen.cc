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

#include <sst_config.h>

#include "embermpicxtgen.h"
#include "emberevent.h"
#include "emberinitev.h"

using namespace SST::Ember;

EmberContextSwitchingMessagePassingGenerator::EmberContextSwitchingMessagePassingGenerator(Component* owner, Params& params) :
	EmberMessagePassingGenerator(owner, params) {

}

EmberContextSwitchingMessagePassingGenerator::~EmberContextSwitchingMessagePassingGenerator() {

}

void EmberContextSwitchingMessagePassingGenerator::suspendGenerator() {
	// Save the generator context
	getcontext(&genContext);
}

void EmberContextSwitchingMessagePassingGenerator::resumeGenerator() {
	// Reload the saved context and continue
	setcontext(&genContext);
}
