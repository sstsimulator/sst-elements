
#include "emberinitev.h"

using namespace SST::Ember;

EmberInitEvent::EmberInitEvent() {

}

EmberInitEvent::~EmberInitEvent() {

}

EmberEventType EmberInitEvent::getEventType() {
	return INIT;
}

std::string EmberInitEvent::getPrintableString() {
	return "Init Event";
}
