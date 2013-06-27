
#include "siriusreader.h"

using namespace std;
using namespace SST::Zodiac;

SiriusReader::SiriusReader(string file, uint32_t focusOnRank, uint32_t maxQLen, std::queue<ZodiacEvent*>* evQ) {
	rank = focusOnRank;
	eventQ = evQ;
	qLimit = maxQLen;
	foundFinalize = false;

	trace = fopen(file.c_str(), "rb");
	if(NULL == trace) {
		std::cerr << "Error opening the Sirius trace file: " << file << std::endl;
		exit(-1);
	}

	prevEventTime = readTime();
}

void SiriusReader::close() {
	if(NULL == trace) {
		std::cerr << "Error: trace file is NULL" << std::endl;
	} else {
		std::cout << "Closing trace file" << std::endl;
	}
	fclose(trace);
}

uint32_t SiriusReader::generateNextEvents() {
	int finalized_reached = 0;

	while((foundFinalize == false) && (eventQ->size() < qLimit)) {
		generateNextEvent();
	}

	return (uint32_t) eventQ->size();
}

uint32_t SiriusReader::getQueueLimit() {
	return qLimit;
}

uint32_t SiriusReader::getCurrentQueueSize() {
	return eventQ->size();
}

void SiriusReader::generateNextEvent() {
	uint32_t call_type = readUINT32();
	double callTime = readTime();
	double evTimeDiff = callTime - prevEventTime;

	if(evTimeDiff > 0) {
		ZodiacComputeEvent* ev = new ZodiacComputeEvent(evTimeDiff);
		eventQ->push(ev);
	}

	switch(call_type) {
	case SIRIUS_MPI_INIT:
		readInit();
		break;

	case SIRIUS_MPI_FINALIZE:
		readFinalize();
		break;

	case SIRIUS_MPI_SEND:
		readSend();
		break;

	case SIRIUS_MPI_RECV:
		readRecv();
		break;

	case SIRIUS_MPI_BARRIER:
		readBarrier();
		break;

	default:
		std::cout << "Unknown MPI command in trace (" << call_type << ") position: " <<
			ftell(trace) << std::endl;
		exit(-1);
		break;
	}

	prevEventTime = callTime;

	// Read the profiled MPI time
	readTime();
	// read the MPI function result
	readINT32();
}

void SiriusReader::readSend() {
	uint64_t buffer = readUINT64();
	uint32_t count  = readUINT32();
	uint32_t dtype  = readUINT32();
	int32_t  dest   = readINT32();
	int32_t  tag    = readINT32();
	uint32_t comm   = readUINT32();

	ZodiacSendEvent* ev = new ZodiacSendEvent((uint32_t) dest, count,
		convertToHermesType(dtype), tag, comm);
	eventQ->push(ev);
}

void SiriusReader::readRecv() {
	uint64_t buffer = readUINT64();
	uint32_t count  = readUINT32();
	uint32_t dtype  = readUINT32();
	int32_t  src   = readINT32();
	int32_t  tag    = readINT32();
	uint32_t comm   = readUINT32();

	ZodiacRecvEvent* ev = new ZodiacRecvEvent((uint32_t) src, count,
		convertToHermesType(dtype), tag, comm);
	eventQ->push(ev);
}

void SiriusReader::readInit() {

}

void SiriusReader::readFinalize() {
	foundFinalize = true;
}

void SiriusReader::readBarrier() {
	uint32_t comm = readUINT32();

	ZodiacBarrierEvent* ev = new ZodiacBarrierEvent(comm);
	eventQ->push(ev);
}

uint32_t SiriusReader::readUINT32() {
	uint32_t temp;
	fread(&temp, 1, sizeof(uint32_t), trace);
	return temp;
}

uint64_t SiriusReader::readUINT64() {
	uint64_t temp;
	fread(&temp, 1, sizeof(uint64_t), trace);
	return temp;
}

double SiriusReader::readTime() {
	double temp;
	fread(&temp, 1, sizeof(double), trace);
	return temp;
}

int32_t SiriusReader::readINT32() {
	int32_t temp;
	fread(&temp, 1, sizeof(int32_t), trace);
	return temp;
}

int64_t SiriusReader::readINT64() {
	int64_t temp;
	fread(&temp, 1, sizeof(int64_t), trace);
	return temp;
}

PayloadDataType SiriusReader::convertToHermesType(uint32_t dtype) {
	PayloadDataType hType = CHAR;

	if(dtype == SIRIUS_MPI_INTEGER) {
		hType = INT;
	} else if(dtype == SIRIUS_MPI_DOUBLE) {
		hType = DOUBLE;
	}

	return hType;
}

bool SiriusReader::hasReachedFinalize() {
	return foundFinalize;
}
