
#include "embersendev.h"

EmberSendEvent::EmberSendEvent(uint32_t pRank, uint32_t pSendSizeBytes, int pTag) {
	rank = pRank;
	sendSizeBytes = pSendSizeBytes;
	sendTag = pTag;
}

EmberSendEvent::~EmberSendEvent() {

}

EmberEventType EmberSendEvent::getEventType() {
	return SEND;
}

uint32_t EmberSendEvent::getSendToRank() {
	return rank;
}

uint32_t EmberSendEvent::getMessageSize() {
	return sendSizeBytes;
}

int EmberSendEvent::getTag() {
	return sendTag;
}
