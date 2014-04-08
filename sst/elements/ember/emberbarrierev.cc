
#include <sst_config.h>
#include "emberbarrierev.h"

using namespace SST::Ember;

EmberBarrierEvent::EmberBarrierEvent(Communicator communicator) {
	comm = communicator;
}

EmberBarrierEvent::~EmberBarrierEvent() {

}

Communicator EmberBarrierEvent::getCommunicator() {
	return comm;
}

EmberEventType EmberBarrierEvent::getEventType() {
	return BARRIER;
}

std::string EmberBarrierEvent::getPrintableString() {
	return "Barrier Event";
}
