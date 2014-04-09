#include <sst_config.h>
#include "emberstartev.h"

using namespace SST::Ember;

EmberStartEvent::EmberStartEvent() {

}

EmberStartEvent::~EmberStartEvent() {

}

EmberEventType EmberStartEvent::getEventType() {
	return START;
}

std::string EmberStartEvent::getPrintableString() {
	return "Start Event";
}
