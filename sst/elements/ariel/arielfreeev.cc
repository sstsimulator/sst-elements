
#include "arielfreeev.h"

using namespace SST::ArielComponent;

ArielFreeEvent::ArielFreeEvent(uint64_t vAddr) {
	virtualAddress = vAddr;
}

ArielFreeEvent::~ArielFreeEvent() {

}

uint64_t ArielFreeEvent::getVirtualAddress() {
	return virtualAddress;
}

ArielEventType ArielFreeEvent::getEventType() {
	return FREE;
}
