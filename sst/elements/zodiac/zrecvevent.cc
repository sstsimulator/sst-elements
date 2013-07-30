
#include "zrecvevent.h"

using namespace SST::Hermes;
using namespace SST::Zodiac;
using namespace SST;

ZodiacRecvEvent::ZodiacRecvEvent(uint32_t src, uint32_t length,
                        PayloadDataType dataType,
                        uint32_t tag, Communicator group) {

	msgSrc = src;
	msgLength = length;
	msgTag = tag;
	msgType = dataType;
	msgComm = group;
}

ZodiacEventType ZodiacRecvEvent::getEventType() {
	return Z_RECV;
}

uint32_t ZodiacRecvEvent::getSource() {
	return msgSrc;
}

uint32_t ZodiacRecvEvent::getLength() {
	return msgLength;
}

uint32_t ZodiacRecvEvent::getMessageTag() {
	return msgTag;
}

PayloadDataType ZodiacRecvEvent::getDataType() {
	return msgType;
}

Communicator ZodiacRecvEvent::getCommunicatorGroup() {
	return msgComm;
}
