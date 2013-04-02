
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
