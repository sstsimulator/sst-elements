
#ifndef _H_ZODIAC_RECV_EVENT_BASE
#define _H_ZODIAC_RECV_EVENT_BASE

#include "zevent.h"

using namespace SST::Hermes;

namespace SST {
namespace Zodiac {

class ZodiacRecvEvent : public ZodiacEvent {

	public:
		ZodiacRecvEvent(uint32_t src, uint32_t length, 
			PayloadDataType dataType,
			uint32_t tag, HermesCommunicator group);
		ZodiacEventType getEventType();

		uint32_t getSource();
		uint32_t getLength();
		uint32_t getMessageTag();
		PayloadDataType getDataType();
		HermesCommunicator getCommunicatorGroup();

	private:
		uint32_t msgSrc;
		uint32_t msgLength;
		uint32_t msgTag;
		PayloadDataType msgType;
		HermesCommunicator msgComm;

};

}
}

#endif
