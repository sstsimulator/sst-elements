
#include "DUMPIReader.h"

using namespace std;
using namespace SST::Zodiac;

DUMPIReader::DUMPIReader(string file, uint32_t focusOnRank, uint32_t maxQLen, std::queue<ZodiacEvent>*>* evQ) {
	rank = focusOnRank;
	eventQ = evQ;
	qLimit = maxQLen;
	foundFinalize = false;
}

void DUMPIReader::close() {
}

uint32_t DUMPIReader::generateNextEvents() {
	return (uint32_t) eventQ->size();
}

uint32_t DUMPIReader::getQueueLimit() {
	return qLimit;
}

uint32_t DUMPIReader::getCurrentQueueSize() {
	return eventQ->size();
}

void DUMPIReader::enqueueEvent(ZodiacEvent* ev) {
	assert(eventQ->size() < qLimit);
	eventQ->push(ev);
}
