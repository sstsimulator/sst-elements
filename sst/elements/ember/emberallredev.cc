
#include <sst_config.h>
#include "emberallredev.h"

using namespace SST::Ember;

EmberAllreduceEvent::EmberAllreduceEvent(uint32_t elementCount, EmberDataType elementType,
	EmberReductionOperation op, Communicator communicator) :

	comm(communicator),
	elemCount(elementCount),
	elemType(elementType),
	reduceOp(op)
	{

}

EmberAllreduceEvent::~EmberAllreduceEvent() {

}

uint32_t EmberAllreduceEvent::getElementCount() {
	return elemCount;
}

EmberDataType EmberAllreduceEvent::getElementType() {
	return elemType;
}

EmberReductionOperation EmberAllreduceEvent::getReductionOperation() {
	return reduceOp;
}

Communicator EmberAllreduceEvent::getCommunicator() {
	return comm;
}
