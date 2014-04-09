
#include <sst_config.h>
#include "arielallocev.h"

using namespace SST::ArielComponent;

ArielAllocateEvent::ArielAllocateEvent(uint64_t vAddr, uint64_t allocLen, uint32_t lev) {
	virtualAddress = vAddr;
	allocateLength = allocLen;
	level = lev;
}

ArielAllocateEvent::~ArielAllocateEvent() {

}

ArielEventType ArielAllocateEvent::getEventType() {
	return MALLOC;
}

uint64_t ArielAllocateEvent::getVirtualAddress() {
	return virtualAddress;
}

uint64_t ArielAllocateEvent::getAllocationLength() {
	return allocateLength;
}

uint32_t ArielAllocateEvent::getAllocationLevel() {
	return level;
}
