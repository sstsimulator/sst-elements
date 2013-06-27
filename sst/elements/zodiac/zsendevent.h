
#ifndef _H_ZODIAC_SEND_EVENT_BASE
#define _H_ZODIAC_SEND_EVENT_BASE

#include "zevent.h"

using namespace SST::Hermes;

namespace SST {
namespace Zodiac {

class ZodiacSendEvent : public ZodiacEvent {

	public:
		ZodiacSendEvent(uint32_t dest, uint32_t length, 
			PayloadDataType dataType,
			uint32_t tag, Communicator group);
		ZodiacEventType getEventType();

		uint32_t getDestination();
		uint32_t getLength();
		uint32_t getMessageTag();
		PayloadDataType getDataType();
		Communicator getCommunicatorGroup();

	private:
		uint32_t msgDest;
		uint32_t msgLength;
		uint32_t msgTag;
		PayloadDataType msgType;
		Communicator msgComm;

};

}
}

#endif
