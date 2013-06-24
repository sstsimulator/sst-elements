
#include "DUMPIReader.h"

using namespace std;
using namespace SST::Zodiac;

DUMPIReader::DUMPIReader(string file, uint32_t focusOnRank, uint32_t maxQLen, std::queue<ZodiacEvent>*>* evQ) {
	rank = focusOnRank;
	eventQ = evQ;
	qLimit = maxQLen;
	foundFinalize = false;

	trace = undumpi_open(file.c_str());
	if(NULL == trace) {
		std::cerr << "Error: unable to open DUMPI trace: " << file << std::endl;
	}
}

void DUMPIReader::close() {
	undumpi_close(trace);
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
