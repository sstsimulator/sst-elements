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


#ifndef _H_EMBER_COMPUTE_EVENT
#define _H_EMBER_COMPUTE_EVENT

#include "emberevent.h"

namespace SST {
namespace Ember {

class EmberComputeEvent : public EmberEvent {

public:
	EmberComputeEvent(uint32_t nanoSecondsDelay);
	~EmberComputeEvent();
	EmberEventType getEventType();
	uint32_t getNanoSecondDelay();
	std::string getPrintableString();

protected:
	uint32_t nanoSecDelay;

};

}
}

#endif
