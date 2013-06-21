
#include "zevent.h"

using namespace SST;
using namespace SST::Zodiac;

ZodiacEvent::ZodiacEvent(ZodiacEventType t) {
	evType = t;
}

ZodiacEventType ZodiacEvent::getEventType() {
	return evType;
}
