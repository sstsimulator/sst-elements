
#ifndef _H_ZODIAC_BARRIER_EVENT
#define _H_ZODIAC_BARRIER_EVENT

#include "zevent.h"

using namespace SST::Hermes;

namespace SST {
namespace Zodiac {

class ZodiacBarrierEvent : public ZodiacEvent {

	public:
		ZodiacBarrierEvent(Communicator group);
		ZodiacEventType getEventType();
		Communicator getCommunicatorGroup();

	private:
		Communicator msgComm;

};

}
}

#endif
