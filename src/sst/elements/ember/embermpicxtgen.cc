// Copyright 2009-2019 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2019, NTESS
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

#include "embermpicxtgen.h"
#include "emberevent.h"
#include "emberinitev.h"

using namespace SST::Ember;

#ifndef SST_ENABLE_PREVIEW_BUILD  // inserted by script
EmberContextSwitchingMessagePassingGenerator::EmberContextSwitchingMessagePassingGenerator(Component* owner, Params& params) :
	EmberMessagePassingGenerator(owner, params) {

}
#endif  // inserted by script
EmberContextSwitchingMessagePassingGenerator::EmberContextSwitchingMessagePassingGenerator(ComponentId_t id, Params& params) :
	EmberMessagePassingGenerator(id, params) {


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
