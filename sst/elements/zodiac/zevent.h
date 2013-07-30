
#ifndef _H_ZODIAC_EVENT_BASE
#define _H_ZODIAC_EVENT_BASE

#include "sst/elements/hermes/msgapi.h"

namespace SST {
namespace Zodiac {

enum ZodiacEventType {
	Z_SKIP,
	Z_COMPUTE,
	Z_SEND,
	Z_IRECV,
	Z_RECV,
	Z_BARRIER,
	Z_ALLREDUCE,
	Z_COLLECTIVE,
	Z_INIT,
	Z_FINALIZE,
	Z_WAIT
};

class ZodiacEvent : public SST::Event {

	public:
		ZodiacEvent();
		virtual ZodiacEventType getEventType() = 0;

};

}
}

#endif
