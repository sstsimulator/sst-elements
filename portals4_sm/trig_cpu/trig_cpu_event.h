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

#ifndef _TRIG_CPU_EVENT_H
#define _TRIG_CPU_EVENT_H

#include <sst/core/event.h>

namespace SST {
namespace Portals4_sm {

    class trig_cpu_event : public Event {
    public:

	trig_cpu_event() : Event() {
	    canceled = false;
	    timeout = false;
	}

	trig_cpu_event(bool timeout) : Event(), timeout(timeout) {
	    canceled = false;
    }

    ~trig_cpu_event() {}

	void cancel() {
	    if (timeout) {
		canceled = true;
	    }
	    else {
		printf("Non-timeout trig_cpu_event cannot be canceled\n");
		abort();
	    }
	}

	bool isCanceled() {
	    return canceled;
	}

	bool isTimeout() {
	    return timeout;
	}

    private:
	bool canceled;
	bool timeout;

    };


}
} //namespace SST

#endif
