
#include "emberrecvev.h"

using namespace SST::Ember;

EmberRecvEvent::EmberRecvEvent(uint32_t pRank, uint32_t pRecvSizeBytes, int pTag, Communicator pComm) {
	recvSizeBytes = pRecvSizeBytes;
	rank = pRank;
	recvTag = pTag;
	comm = pComm;
}

EmberRecvEvent::~EmberRecvEvent() {

}

Communicator EmberRecvEvent::getCommunicator() {
	return comm;
}

EmberEventType EmberRecvEvent::getEventType() {
	return RECV;
}

uint32_t EmberRecvEvent::getRecvFromRank() {
	return rank;
}

uint32_t EmberRecvEvent::getMessageSize() {
	return recvSizeBytes;
}

int EmberRecvEvent::getTag() {
	return recvTag;
}

std::string EmberRecvEvent::getPrintableString() {
	char buffer[128];
	sprintf(buffer, "Recv Event (Recv from rank=%" PRIu32 ", Size=%" PRIu32 " bytes, tag=%d",
		rank, recvSizeBytes, recvTag);
	std::string bufferStr = buffer;
	return bufferStr;
}
