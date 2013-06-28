
#ifndef _H_ZODIAC_EVENT_BASE
#define _H_ZODIAC_EVENT_BASE

#include "sst/elements/hermes/msgapi.h"

namespace SST {
namespace Zodiac {

enum ZodiacEventType {
	SKIP,
	COMPUTE,
	SEND,
	IRECV,
	RECV,
	BARRIER,
	COLLECTIVE,
	INIT,
	FINALIZE
};

class ZodiacEvent : public SST::Event {

	public:
		ZodiacEvent();
		virtual ZodiacEventType getEventType() = 0;

};

}
}

#endif
