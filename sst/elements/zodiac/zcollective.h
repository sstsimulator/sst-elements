
#ifndef _H_ZODIAC_COLLECTIVE_EVENT_BASE
#define _H_ZODIAC_COLLECTIVE_EVENT_BASE

#include "sst/elements/hermes/msgapi.h"
#include "zevent.h"

namespace SST {
namespace Zodiac {

class ZodiacCollectiveEvent : public ZodiacEvent {

	public:
		ZodiacCollectiveEvent();
		virtual ZodiacEventType getEventType() = 0;

};

}
}

#endif
