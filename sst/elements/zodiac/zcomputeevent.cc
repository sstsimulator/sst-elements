
#include "zcomputeevent.h"

using namespace SST::Hermes;
using namespace SST::Zodiac;
using namespace SST;

ZodiacComputeEvent::ZodiacComputeEvent(double time) {
	computeTime = time;
}

ZodiacEventType ZodiacComputeEvent::getEventType() {
	return COMPUTE;
}

double ZodiacComputeEvent::getComputeDuration() {
	return computeTime;
}

double ZodiacComputeEvent::getComputeDurationNano() {
	return computeTime * 1000000000.0;
}
