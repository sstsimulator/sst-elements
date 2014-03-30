
#include "emberirecvev.h"

using namespace SST::Ember;

EmberIRecvEvent::EmberIRecvEvent(uint32_t pRank, uint32_t pRecvSizeBytes, int pTag, Communicator pComm, MessageRequest* req) {
	recvSizeBytes = pRecvSizeBytes;
	rank = pRank;
	recvTag = pTag;
	comm = pComm;
	request = req;
}

EmberIRecvEvent::~EmberIRecvEvent() {

}

Communicator EmberIRecvEvent::getCommunicator() {
	return comm;
}

MessageRequest* EmberIRecvEvent::getMessageRequestHandle() {
	return request;
}

EmberEventType EmberIRecvEvent::getEventType() {
	return IRECV;
}

uint32_t EmberIRecvEvent::getRecvFromRank() {
	return rank;
}

uint32_t EmberIRecvEvent::getMessageSize() {
	return recvSizeBytes;
}

int EmberIRecvEvent::getTag() {
	return recvTag;
}

std::string EmberIRecvEvent::getPrintableString() {
	char buffer[128];
	sprintf(buffer, "Irecv Event (Recv from rank=%" PRIu32 ", Size=%" PRIu32 " bytes, tag=%d)",
		rank, recvSizeBytes, recvTag);
	std::string bufferStr = buffer;
	return bufferStr;
}
