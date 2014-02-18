
#include "emberrecvev.h"

EmberRecvEvent::EmberRecvEvent(uint32_t pRank, uint32_t pRecvSizeBytes, int pTag) {
	recvSizeBytes = pRecvSizeBytes;
	rank = pRank;
	recvTag = pTag;
}

EmberRecvEvent::~EmberRecvEvent() {

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
	return pTag;
}
