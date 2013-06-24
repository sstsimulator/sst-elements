
#ifndef _H_ZODIAC_EVENT_BASE
#define _H_ZODIAC_EVENT_BASE

#include "sst/elements/hermes/msgapi.h"

namespace SST {
namespace Zodiac {

enum ZodiacEventType {
	SKIP,
	COMPUTE,
	SEND,
	RECV,
	COLLECTIVE
};

class ZodiacEvent {

	public:
		ZodiacEvent();
		virtual ZodiacEventType getEventType() = 0;

};

}
}

#endif
