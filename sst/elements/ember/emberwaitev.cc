#include <sst_config.h>
#include "emberwaitev.h"

using namespace SST::Ember;

EmberWaitEvent::EmberWaitEvent(MessageRequest* req) {
	request = req;
}

EmberWaitEvent::~EmberWaitEvent() {

}

MessageRequest* EmberWaitEvent::getMessageRequestHandle() {
	return request;
}

EmberEventType EmberWaitEvent::getEventType() {
	return WAIT;
}

std::string EmberWaitEvent::getPrintableString() {
	char buffer[128];
        sprintf(buffer, "Wait Event");
        std::string bufferStr = buffer;
        return bufferStr;
}

