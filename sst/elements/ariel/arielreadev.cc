
#include "arielreadev.h"

using namespace SST::ArielComponent;

ArielReadEvent::ArielReadEvent(uint64_t rAddr, uint32_t len) {
	readAddress = rAddr;
	readLength = len;
}

ArielReadEvent::~ArielReadEvent() {

}

ArielEventType ArielReadEvent::getEventType() {
	return READ_ADDRESS;
}

uint64_t ArielReadEvent::getAddress() {
	return readAddress;
}

uint32_t ArielReadEvent::getLength() {
	return readLength;
}
