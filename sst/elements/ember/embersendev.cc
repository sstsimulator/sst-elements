
#include "embersendev.h"

using namespace SST::Ember;

EmberSendEvent::EmberSendEvent(uint32_t pRank, uint32_t pSendSizeBytes, int pTag, Communicator pComm) {
	rank = pRank;
	sendSizeBytes = pSendSizeBytes;
	sendTag = pTag;
	comm = pComm;
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

Communicator EmberSendEvent::getCommunicator() {
	return comm;
}

std::string EmberSendEvent::getPrintableString() {
	char buffer[256];
	sprintf(buffer, "Send Event (Send to Rank=%" PRIu32 ", Message Size=%" PRIu32 " bytes, Tag=%d, Communicator=%" PRIu32 ")" ,
		rank, sendSizeBytes, sendTag, comm);
	std::string bufferStr = buffer;
	return bufferStr;
}
