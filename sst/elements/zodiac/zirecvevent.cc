
#include "zirecvevent.h"

using namespace SST::Hermes;
using namespace SST::Zodiac;
using namespace SST;

ZodiacIRecvEvent::ZodiacIRecvEvent(uint32_t src, uint32_t length,
                        PayloadDataType dataType,
                        uint32_t tag, Communicator group, uint64_t rID) :
ZodiacRecvEvent(src, length, dataType, tag, group) {

	reqID = rID;
}

ZodiacEventType ZodiacIRecvEvent::getEventType() {
	return Z_IRECV;
}

uint64_t ZodiacIRecvEvent::getRequestID() {
	return reqID;
}
