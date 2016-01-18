// Copyright 2009-2015 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2015, Sandia Corporation
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#include <sst_config.h>
#include "emberSIRIUSTrace.h"

using namespace SST::Ember;

EmberSIRIUSTraceGenerator::EmberSIRIUSTraceGenerator(SST::Component* owner,
                                            Params& params) :
	EmberMessagePassingGenerator(owner, params, "SIRIUSTrace")
{
	std::string trace_prefix = params.find_string("arg.traceprefix", "");

	if( "" == trace_prefix ) {
		fatal(CALL_INFO, -1, "Error: trace prefix is empty, no way to load a trace!\n");
	} else {
		char* full_trace = (char*) malloc( sizeof(char) * PATH_MAX );
		sprintf(full_trace, "%s.%d", trace_prefix.c_str(), rank());

		trace_file = fopen(full_trace, "rb");

		if( NULL == trace_file ) {
			fatal(CALL_INFO, -1, "Error: unable to open SIRIUS trace: %s\n", full_trace);
		} else {
			verbose(CALL_INFO, 1, 0, "Successfully opened SIRIUS trace: %s\n", full_trace);
		}
	}

	// Start by reading in the MPI_init event
	const uint32_t sirius_init = readUINT32();
	if(sirius_init != SIRIUS_MPI_INIT) {
		fatal(CALL_INFO, -1, "Error: trace does not start with an MPI init event. Correct file?\n");
	}

	std::queue<EmberEvent*> initQueue;
	readMPIInit(initQueue);
}

EmberSIRIUSTraceGenerator::~EmberSIRIUSTraceGenerator() {
	if( NULL != trace_file ) {
		fclose(trace_file);
	}
}

bool EmberSIRIUSTraceGenerator::generate( std::queue<EmberEvent*>& evQ)
{
	const uint32_t sirius_func_type = readUINT32();

	switch(sirius_func_type) {
	case SIRIUS_MPI_ISEND:
		readMPIIsend(evQ);
		break;
	case SIRIUS_MPI_SEND:
		readMPISend(evQ);
		break;
	case SIRIUS_MPI_IRECV:
		readMPIIrecv(evQ);
		break;
	case SIRIUS_MPI_RECV:
		readMPIRecv(evQ);
		break;
	case SIRIUS_MPI_BARRIER:
		break;
	case SIRIUS_MPI_ALLREDUCE:
		break;
	case SIRIUS_MPI_WAIT:
		break;
	case SIRIUS_MPI_FINALIZE:
		readMPIFinalize(evQ);
		return true;
	}

    	return false;
}

void EmberSIRIUSTraceGenerator::readMPIInit( std::queue<EmberEvent*>& evQ ) {
	const double startTime  = readTime();
	const double startTime2 = readTime();
	const int32_t result    = readINT32();
}

void EmberSIRIUSTraceGenerator::readMPISend( std::queue<EmberEvent*>& evQ ) {
	const double startTime = readTime();
	const uint64_t readBuffer = readUINT64();
	const uint32_t count = readUINT32();
	const PayloadDataType dType = readDataType();
	const int32_t dest = readINT32();
	const int32_t tag = readINT32();
	const Communicator& comm = readCommunicator();
	const double endTime = readTime();
	const int32_t result = readINT32();

	verbose(CALL_INFO, 2, 0, "Send to %" PRId32 ", tag=%" PRId32 ", count=%" PRIu32 "\n",
		dest, tag, count);

	void* sendBuffer = memAlloc( count * getTypeElementSize(dType) );
	enQ_send( evQ, sendBuffer, count, dType, dest, tag, comm );
}

void EmberSIRIUSTraceGenerator::readMPIIsend( std::queue<EmberEvent*>& evQ ) {

}

void EmberSIRIUSTraceGenerator::readMPIRecv( std::queue<EmberEvent*>& evQ ) {
	const double startTime = readTime();
	const uint64_t readBuffer = readUINT64();
	const uint32_t count = readUINT32();
	const PayloadDataType dType = readDataType();
	const int32_t src = readINT32();
	const int32_t tag = readINT32();
	const Communicator& comm = readCommunicator();
	MessageResponse* msgResp = new MessageResponse();
	const double endTime = readTime();
	const int32_t result = readINT32();

	verbose(CALL_INFO, 2, 0, "Recv from %" PRId32 ", tag=%" PRId32 ", count=%" PRIu32 "\n",
		src, tag, count);

	void* recvBuffer = memAlloc( count * getTypeElementSize(dType) );
	enQ_recv( evQ, recvBuffer, count, dType, src, tag, comm, msgResp );
}

void EmberSIRIUSTraceGenerator::readMPIIrecv( std::queue<EmberEvent*>& evQ ) {

}

void EmberSIRIUSTraceGenerator::readMPIFinalize( std::queue<EmberEvent*>& evQ ) {
	const double startTime = readTime();
	const double endTime   = readTime();
	const int32_t result = readINT32();
}

double EmberSIRIUSTraceGenerator::readTime() const {
	double tmp = 0;
	size_t readLen = fread(&tmp, sizeof(tmp), 1, trace_file);

	if( 1 != readLen ) {
		fatal(CALL_INFO, -1, "I/O Error reading from SIRIUS trace, size read %" PRIu64 " != %" PRIu64 "\n",
			(uint64_t) readLen, (uint64_t) sizeof(tmp));
	}

	return tmp;
}

uint32_t EmberSIRIUSTraceGenerator::readUINT32() const {
	uint32_t tmp = 0;
	size_t readLen = fread(&tmp, sizeof(tmp), 1, trace_file);

	if( 1 != readLen ) {
		fatal(CALL_INFO, -1, "I/O Error reading from SIRIUS trace, size read %" PRIu64 " != %" PRIu64 "\n",
			(uint64_t) readLen, (uint64_t) sizeof(tmp));
	}

	return tmp;
}

uint64_t EmberSIRIUSTraceGenerator::readUINT64() const {
	uint64_t tmp = 0;
	size_t readLen = fread(&tmp, sizeof(tmp), 1, trace_file);

	if( 1 != readLen ) {
		fatal(CALL_INFO, -1, "I/O Error reading from SIRIUS trace, size read %" PRIu64 " != %" PRIu64 "\n",
			(uint64_t) readLen, (uint64_t) sizeof(tmp));
	}

	return tmp;
}

int32_t EmberSIRIUSTraceGenerator::readINT32() const {
	int32_t tmp = 0;
	size_t readLen = fread(&tmp, sizeof(tmp), 1, trace_file);

	if( 1 != readLen ) {
		fatal(CALL_INFO, -1, "I/O Error reading from SIRIUS trace, size read %" PRIu64 " != %" PRIu64 "\n",
			(uint64_t) readLen, (uint64_t) sizeof(tmp));
	}

	return tmp;
}

Communicator EmberSIRIUSTraceGenerator::readCommunicator() const {
	uint32_t comm;

	size_t readLen = fread(&comm, sizeof(comm), 1, trace_file);

        if( 1 != readLen ) {
                fatal(CALL_INFO, -1, "I/O Error reading from SIRIUS trace.\n");
        }

	if( 0 == comm ) {
		return GroupWorld;
	} else {
		auto findComm = communicatorMap.find(comm);

		if( communicatorMap.end() == findComm ) {
			// Unknown communicator
			fatal(CALL_INFO, -1, "Unknown communicator found in trace (comm=%" PRIu32 ")\n",
				comm);
		}

		return findComm->second;
	}
}

PayloadDataType EmberSIRIUSTraceGenerator::readDataType() const {
	uint32_t dType;

	size_t readLen = fread(&dType, sizeof(dType), 1, trace_file);

	if( 1 != readLen ) {
		fatal(CALL_INFO, -1, "I/O Error reading from SIRIUS trace.\n");
	}

	switch(dType) {
	case SIRIUS_MPI_INTEGER:
		return INT;
	case SIRIUS_MPI_DOUBLE:
		return DOUBLE;
	case SIRIUS_MPI_CHAR:
		return CHAR;
	default:
		fatal(CALL_INFO, -1, "I/O Error reading from SIRIUS trace.\n");
	}

	// Catch all, shouldn't reach here
	return INT;
}

size_t EmberSIRIUSTraceGenerator::getTypeElementSize(const PayloadDataType dType) const {
	switch(dType) {
	case INT:
		return 4;
	case DOUBLE:
		return 8;
	case CHAR:
		return 1;
	default:
		fatal(CALL_INFO, -1, "Error: unknown data type provided for conversion.\n");
	}

	return 0;
}
