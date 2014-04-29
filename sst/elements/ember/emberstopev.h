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


#ifndef _H_EMBER_STOP_EVENT
#define _H_EMBER_STOP_EVENT

#include "emberevent.h"

namespace SST {
namespace Ember {

class EmberStopEvent : public EmberEvent {

public:
	EmberStopEvent() {}
	~EmberStopEvent() {}
	EmberEventType getEventType() {
		return STOP;
	}
	std::string getPrintableString() {
		return "Stop Event";
	}

};

}
}

#endif
