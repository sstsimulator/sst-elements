
#include "zallredevent.h"

using namespace SST::Hermes;
using namespace SST::Zodiac;
using namespace SST;

ZodiacAllreduceEvent::ZodaicAllreduceEvent(
	uint32_t length,
	PayloadDataType dataType,
	ReductionOperation op,
	Communicator group) {

	msgLength = length;
	msgType = dataType;
	mpiOp = op;
	msgComm = group;
}


ZodiacEventType ZodiacAllreduceEvent::getEventType() {
	return Z_ALLREDUCE;
}

ReductionOperation ZodiacAllreduceEvent::getOp() {
	return mpiOp;
}

uint32_t ZodiacAllreduceEvent::getLength() {
	return msgLength;
}

PayloadDataType ZodiacAllreduceEvent::getDataType() {
	return msgType;
}

Communicator ZodiacAllreduceEvent::getCommunicatorGroup() {
	return msgComm;
}
