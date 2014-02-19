
#include "emberfinalizeev.h"

using namespace SST::Ember;

EmberFinalizeEvent::EmberFinalizeEvent() {

}

EmberFinalizeEvent::~EmberFinalizeEvent() {

}

EmberEventType EmberFinalizeEvent::getEventType() {
	return FINALIZE;
}

std::string EmberFinalizeEvent::getPrintableString() {
	return "Finalize Event";
}
