
#include "zevent.h"

ZodaicEvent::ZodiacEvent(ZodiacEventType t) {
	evType = t;
}

ZodiacEventType getEventType() {
	return evType;
}
