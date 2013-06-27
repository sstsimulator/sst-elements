
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
