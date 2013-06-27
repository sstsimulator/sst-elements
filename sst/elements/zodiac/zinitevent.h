
#ifndef _H_ZODIAC_INIT_EVENT
#define _H_ZODIAC_INIT_EVENT

#include "zevent.h"

using namespace SST::Hermes;

namespace SST {
namespace Zodiac {

class ZodiacInitEvent : public ZodiacEvent {

	public:
		ZodiacInitEvent();
		ZodiacEventType getEventType();

};

}
}

#endif
