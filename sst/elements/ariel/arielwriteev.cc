
#include "sst_config.h"
#include "sst/core/serialization.h"
#include "sst/core/element.h"
#include <sst/core/params.h>
#include <sst/core/simulation.h>

#include "arielwriteev.h"

using namespace SST::ArielComponent;

ArielWriteEvent::ArielWriteEvent(uint64_t wAddr, uint32_t len) {
	writeAddress = wAddr;
	writeLength = len;
}

ArielWriteEvent::~ArielWriteEvent() {

}

ArielEventType ArielWriteEvent::getEventType() {
	return WRITE_ADDRESS;
}

uint64_t ArielWriteEvent::getAddress() {
	return writeAddress;
}

uint32_t ArielWriteEvent::getLength() {
	return writeLength;
}
