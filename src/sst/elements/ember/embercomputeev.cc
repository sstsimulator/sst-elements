// Copyright 2009-2018 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2018, NTESS
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
#include "embercomputeev.h"

using namespace SST::Ember;

#if 0
EmberComputeEvent::EmberComputeEvent(uint32_t nsDelay) {
	nanoSecDelay = nsDelay;
}

EmberComputeEvent::~EmberComputeEvent() {

}

EmberEventType EmberComputeEvent::getEventType() {
	return COMPUTE;
}

uint32_t EmberComputeEvent::getNanoSecondDelay() {
	return nanoSecDelay;
}

std::string EmberComputeEvent::getPrintableString() {
	char buffer[64];
	sprintf(buffer, "Compute Event (Delay=%" PRIu32 "ns)", nanoSecDelay);
	std::string bufferStr = buffer;

	return bufferStr;
}
#endif
