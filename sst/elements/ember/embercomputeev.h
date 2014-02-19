
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
