
#include <sst_config.h>
#include "zwaitevent.h"

using namespace SST::Hermes;
using namespace SST::Zodiac;
using namespace SST;

ZodiacWaitEvent::ZodiacWaitEvent(uint64_t rID) {
	reqID = rID;
}

ZodiacEventType ZodiacWaitEvent::getEventType() {
	return Z_WAIT;
}

uint64_t ZodiacWaitEvent::getRequestID() {
	return reqID;
}
