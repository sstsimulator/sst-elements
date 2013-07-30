
#ifndef _H_ZODIAC_ALLREDUCE_EVENTBASE
#define _H_ZODIAC_ALLREDUCE_EVENTBASE

#include "zcollective.h"

using namespace SST::Hermes;

namespace SST {
namespace Zodiac {

class ZodiacAllreduceEvent : public ZodiacCollectiveEvent {

	public:
		ZodiacAllreduceEvent(uint32_t length,
			PayloadDataType dataType,
			ReductionOperation op, Communicator group);
		ZodiacEventType getEventType();

		uint32_t getLength();
		PayloadDataType getDataType();
		Communicator getCommunicatorGroup();
		ReductionOperation getOp();

	private:
		uint32_t msgLength;
		PayloadDataType msgType;
		Communicator msgComm;
		ReductionOperation mpiOp;

};

}
}

#endif
