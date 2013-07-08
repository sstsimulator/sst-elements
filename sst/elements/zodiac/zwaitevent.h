
#ifndef _H_ZODIAC_WAIT_EVENT
#define _H_ZODIAC_WAIT_EVENT

#include "zevent.h"

using namespace SST::Hermes;

namespace SST {
namespace Zodiac {

class ZodiacWaitEvent : public ZodiacEvent {

	public:
		ZodiacWaitEvent(uint64_t reqID); 
		ZodiacEventType getEventType();

		uint64_t getRequestID();

	protected:
		uint64_t reqID;

};

}
}

#endif
