#include <sst_config.h>
#include "embercomputeev.h"

using namespace SST::Ember;

EmberComputeEvent::EmberComputeEvent(uint32_t nsDelay) {
	nanoSecDelay = nsDelay;
}

EmberComputeEvent::~EmberComputeEvent() {

}

EmberEventType EmberComputeEvent::getEventType() {
	return COMPUTE;
}

uint32_t EmberComputeEvent::getNanoSecondDelay() {
	return nanoSecDelay;
}

std::string EmberComputeEvent::getPrintableString() {
	char buffer[64];
	sprintf(buffer, "Compute Event (Delay=%" PRIu32 "ns)", nanoSecDelay);
	std::string bufferStr = buffer;

	return bufferStr;
}
