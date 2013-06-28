
#ifndef _H_ZODIAC_IRECV_EVENT
#define _H_ZODIAC_IRECV_EVENT

#include "zrecvevent.h"
#include "zevent.h"

using namespace SST::Hermes;

namespace SST {
namespace Zodiac {

class ZodiacIRecvEvent : public ZodiacRecvEvent {

	public:
		ZodiacIRecvEvent(uint32_t src, uint32_t length, 
			PayloadDataType dataType,
			uint32_t tag, Communicator group, uint64_t rqID);
		ZodiacEventType getEventType();
		uint64_t getRequestID();

	protected:
		uint64_t reqID;

};

}
}

#endif
