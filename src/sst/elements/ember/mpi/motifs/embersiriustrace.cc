// Copyright 2009-2021 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2021, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#include <sst_config.h>

#include "embersiriustrace.h"

#include <cstdint>
#include <climits>

using namespace SST::Ember;

EmberSIRIUSTraceGenerator::EmberSIRIUSTraceGenerator(SST::ComponentId_t id,
                                            Params& params) :
	EmberMessagePassingGenerator(id, params, "SIRIUSTrace")
{
	std::string trace_prefix = params.find<std::string>("arg.traceprefix", "");

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

	currentTraceTime = 0;

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

void EmberSIRIUSTraceGenerator::enqueueCompute( std::queue<EmberEvent*>& evQ,
		const double nextStartTime,
		const double nextEndTime) {

	const double currentDiffTime = nextStartTime - currentTraceTime;

	if(currentDiffTime > 0) {
		enQ_compute( evQ, (currentDiffTime * 1.0e9 ) );
	}

	currentTraceTime = std::max(currentTraceTime, nextEndTime);
}

bool EmberSIRIUSTraceGenerator::generate( std::queue<EmberEvent*>& evQ)
{
	const uint32_t sirius_func_type = readUINT32();

	switch(sirius_func_type) {
	case SIRIUS_MPI_SEND:
		readMPISend(evQ);
		break;
	case SIRIUS_MPI_ISEND:
		readMPIIsend(evQ);
		break;
	case SIRIUS_MPI_RECV:
		readMPIRecv(evQ);
		break;
	case SIRIUS_MPI_IRECV:
		readMPIIrecv(evQ);
		break;
	case SIRIUS_MPI_ALLREDUCE:
		readMPIAllreduce(evQ);
		break;
	case SIRIUS_MPI_REDUCE:
		readMPIReduce(evQ);
		break;
	case SIRIUS_MPI_WAIT:
		readMPIWait(evQ);
		break;
	case SIRIUS_MPI_WAITALL:
		readMPIWaitall(evQ);
		break;
	case SIRIUS_MPI_BARRIER:
		readMPIBarrier(evQ);
		break;
	case SIRIUS_MPI_BCAST:
		readMPIBcast(evQ);
		break;
	case SIRIUS_MPI_COMM_SPLIT:
		readMPICommSplit(evQ);
		break;
	case SIRIUS_MPI_COMM_DISCONNECT:
		readMPICommDisconnect(evQ);
		break;
	case SIRIUS_MPI_FINALIZE:
		readMPIFinalize(evQ);
		return true;
	}

    	return false;
}

int32_t EmberSIRIUSTraceGenerator::readTag() const {
	const int32_t tag = readINT32();

	if(INT32_MAX == tag) {
		return AnyTag;
	} else {
		return tag;
	}
}

void EmberSIRIUSTraceGenerator::readMPIInit( std::queue<EmberEvent*>& evQ ) {
	const double startTime  = readTime();
	const double startTime2 = readTime();
	const int32_t result    = readINT32();

	currentTraceTime = startTime2;
}

void EmberSIRIUSTraceGenerator::readMPICommDisconnect( std::queue<EmberEvent*>& evQ ) {
	const double startTime = readTime();
	const Communicator* comm = readCommunicator();

	const double endTime = readTime();
	const int32_t result = readINT32();

	for(auto findComm = communicatorMap.begin(); findComm != communicatorMap.end(); findComm++) {
		if(comm == findComm->second) {
			communicatorMap.erase(findComm);
			break;
		}
	}

	verbose(CALL_INFO, 4, 0, "Enqueue comm disconnect\n");

	enqueueCompute(evQ, startTime, endTime);
	enQ_commDestroy( evQ, *comm );
}

void EmberSIRIUSTraceGenerator::readMPICommSplit( std::queue<EmberEvent*>& evQ ) {
	const double startTime = readTime();
	const Communicator* comm = readCommunicator();
	const int32_t color = readINT32();
	const int32_t key   = readINT32();
	const uint32_t newCommID = readUINT32();
	const double endTime = readTime();
	const int32_t result = readINT32();

	Communicator* newComm = new Communicator(0);

	auto checkCommMapping = communicatorMap.find(newCommID);

	if( checkCommMapping != communicatorMap.end() ) {
		fatal(CALL_INFO, -1, "Error: communicator mapped to %" PRIu32 " already in comm map.\n", newCommID);
	}

	communicatorMap.insert( std::pair<uint32_t, Communicator*>(newCommID, newComm) );

	enqueueCompute(evQ, startTime, endTime);
	enQ_commSplit(evQ, *comm, color, key, newComm );
}

void EmberSIRIUSTraceGenerator::readMPISend( std::queue<EmberEvent*>& evQ ) {
	const double startTime = readTime();
	const uint64_t readBuffer = readUINT64();
	const uint32_t count = readUINT32();
	const PayloadDataType dType = readDataType();
	const int32_t dest = readINT32();
	const int32_t tag = readTag();
	const Communicator* comm = readCommunicator();
	const double endTime = readTime();
	const int32_t result = readINT32();

	verbose(CALL_INFO, 2, 0, "Send to %" PRId32 ", tag=%" PRId32 ", count=%" PRIu32 "\n",
		dest, tag, count);

	void* sendBuffer = memAlloc( count * getTypeElementSize(dType) );

	enqueueCompute(evQ, startTime, endTime);
	enQ_send( evQ, sendBuffer, count, dType, dest, tag, *comm );
}

void EmberSIRIUSTraceGenerator::readMPIIsend( std::queue<EmberEvent*>& evQ ) {
	const double startTime = readTime();
	const uint64_t buffer  = readUINT64();
	const uint32_t count   = readUINT32();
	const PayloadDataType dType = readDataType();
	const int32_t dest = readINT32();
	const int32_t tag  = readTag();
	const Communicator* comm = readCommunicator();
	const uint64_t req = readUINT64();
	const double endTime = readTime();
	const int32_t result = readINT32();

	verbose(CALL_INFO, 2, 0, "Isend to %" PRId32 ", tag=%" PRId32 ", count=%" PRIu32 "\n",
		dest, tag, count);

	MessageRequest* emberReq = new MessageRequest();

        auto checkReq = liveRequests.find(req);
        if( checkReq == liveRequests.end() ) {
                fatal(CALL_INFO, -1, "Error: when issuing an Isend, found an MPI_Request was already active.\n");
        }

        // Add into the map, keep for a WAIT call
        liveRequests.insert( std::pair<uint64_t, MessageRequest*>(req, emberReq) );

	void* sendBuffer = memAlloc( count * getTypeElementSize(dType) );

	enqueueCompute(evQ, startTime, endTime);
	enQ_isend( evQ, sendBuffer, count, dType, dest, tag, *comm, emberReq );
}

void EmberSIRIUSTraceGenerator::readMPIRecv( std::queue<EmberEvent*>& evQ ) {
	const double startTime = readTime();
	const uint64_t readBuffer = readUINT64();
	const uint32_t count = readUINT32();
	const PayloadDataType dType = readDataType();
	int32_t src = readINT32();

	if(INT32_MAX == src) {
		src = AnySrc;
	}

	const int32_t tag = readTag();
	const Communicator* comm = readCommunicator();
	MessageResponse* msgResp = new MessageResponse();
	const double endTime = readTime();
	const int32_t result = readINT32();

	verbose(CALL_INFO, 2, 0, "Recv from %" PRId32 ", tag=%" PRId32 ", count=%" PRIu32 "\n",
		src, tag, count);

	void* recvBuffer = memAlloc( count * getTypeElementSize(dType) );

	enqueueCompute(evQ, startTime, endTime);
	enQ_recv( evQ, recvBuffer, count, dType, src, tag, *comm, msgResp );
}

void EmberSIRIUSTraceGenerator::readMPIBarrier( std::queue<EmberEvent*>& evQ ) {
	const double startTime = readTime();
	const Communicator* comm = readCommunicator();
	const double endTime = readTime();
	const int32_t result = readINT32();

	verbose(CALL_INFO, 2, 0, "Barrier\n");

	enqueueCompute(evQ, startTime, endTime);
	enQ_barrier( evQ, *comm );
}

void EmberSIRIUSTraceGenerator::readMPIReduce( std::queue<EmberEvent*>& evQ ) {
	const double startTime = readTime();
	const uint64_t buffer = readUINT64();
	const uint64_t recvBuffer = readUINT64();
	const uint32_t count = readUINT32();
	const PayloadDataType dType = readDataType();
	const ReductionOperation opType = readReductionOp();
	const int32_t root = readINT32();
	const Communicator* comm = readCommunicator();

	const double endTime = readTime();
	const int32_t result = readINT32();

	void* allocLocalBuffer = memAlloc( count * getTypeElementSize(dType) );
	void* allocRecvBuffer  = memAlloc( count * getTypeElementSize(dType) );

	verbose(CALL_INFO, 2, 0, "Reduce count=%" PRIu32 ", root=%" PRId32 "\n", count, root);

	enqueueCompute(evQ, startTime, endTime);
	enQ_reduce( evQ, allocLocalBuffer, allocRecvBuffer, count, dType, opType, root, *comm );
}

void EmberSIRIUSTraceGenerator::readMPIAllreduce( std::queue<EmberEvent*>& evQ ) {
	const double startTime = readTime();
	const uint64_t buffer = readUINT64();
	const uint64_t recvBuffer = readUINT64();
	const uint32_t count = readUINT32();
	const PayloadDataType dType = readDataType();
	const ReductionOperation opType = readReductionOp();
	const Communicator* comm = readCommunicator();

	const double endTime = readTime();
	const int32_t result = readINT32();

	void* allocLocalBuffer = memAlloc( count * getTypeElementSize(dType) );
	void* allocRecvBuffer  = memAlloc( count * getTypeElementSize(dType) );

	verbose(CALL_INFO, 2, 0, "Allreduce count=%" PRIu32 "\n", count);

	enqueueCompute(evQ, startTime, endTime);
	enQ_allreduce( evQ, allocLocalBuffer, allocRecvBuffer, count, dType, opType, *comm );
}

void EmberSIRIUSTraceGenerator::readMPIIrecv( std::queue<EmberEvent*>& evQ ) {
	const double startTime = readTime();
	const uint64_t buffer = readUINT64();
	const uint32_t count  = readUINT32();
	const PayloadDataType dType = readDataType();

	int32_t src = readINT32();

	if(INT32_MAX == src) {
		src = AnySrc;
	}

	const int32_t tag = readTag();
	const Communicator* comm = readCommunicator();
	const uint64_t req   = readUINT64();
	const double endTime = readTime();
	const int32_t result = readINT32();

	MessageRequest* emberReq = new MessageRequest();
	void* allocBuffer = memAlloc( count * getTypeElementSize(dType) );

	auto checkReq = liveRequests.find(req);
	if( checkReq != liveRequests.end() ) {
		printLiveRequestMap();
		fatal(CALL_INFO, -1, "Error: when issuing an Irecv, found an MPI_Request was already active. (Request=%" PRIu64 ")\n", req);
	}

	verbose(CALL_INFO, 2, 0, "Irecv src=%" PRId32 ", count=%" PRIu32 "\n", src, count);

	// Add into the map, keep for a WAIT call
	liveRequests.insert( std::pair<uint64_t, MessageRequest*>(req, emberReq) );

	enqueueCompute(evQ, startTime, endTime);
	enQ_irecv( evQ, allocBuffer, count, dType, src, tag, *comm, emberReq );
}

void EmberSIRIUSTraceGenerator::readMPIWaitall( std::queue<EmberEvent*>& evQ ) {
	const double startTime = readTime();
	const uint32_t reqCount = readUINT32();

	// Get requests to wait against, remembering we may be given some
	// MPI_REQUEST_NULL in the array, which we need to skip
	std::vector<uint64_t> requestAddr;
	for(uint32_t i = 0 ; i < reqCount; i++) {
		const uint64_t nextReqID = readUINT64();

		if(SIRIUS_MPI_REQUEST_NULL != nextReqID) {
			requestAddr.push_back(nextReqID);
		}
	}

	const double endTime = readTime();
	const int32_t result = readINT32();

	MessageRequest* reqs = (MessageRequest*) malloc( sizeof(MessageRequest*) * requestAddr.size() );
	for(uint32_t i = 0; i < requestAddr.size(); i++) {
		auto findReq = liveRequests.find(requestAddr[i]);

		if( findReq == liveRequests.end() ) {
			fatal(CALL_INFO, -1, "Error: unable to find request at address: %" PRIu64 "\n", requestAddr[i]);
		} else {
			reqs[i] = *(findReq->second);
			liveRequests.erase(findReq);
		}
	}

	verbose(CALL_INFO, 2, 0, "Waitall, count=%" PRIu32 ", found %" PRIu32 " non MPI_REQUEST_NULL requests.\n",
		reqCount, static_cast<const uint32_t>(requestAddr.size()) );

	enqueueCompute(evQ, startTime, endTime);
	enQ_waitall( evQ, requestAddr.size(), reqs, NULL );
}

void EmberSIRIUSTraceGenerator::readMPIWait( std::queue<EmberEvent*>& evQ ) {
	const double startTime = readTime();
	const uint64_t request = readUINT64();
	const uint64_t status  = readUINT64();
	const double endTime   = readTime();
	const int32_t result   = readINT32();

	if(SIRIUS_MPI_REQUEST_NULL != request) {
		MessageRequest* emberReq;
		auto reqLookup = liveRequests.find(request);

		if( reqLookup == liveRequests.end() ) {
			fatal(CALL_INFO, -1, "Error: unable to find a pending matching request for an MPI_Wait event.\n");
		}

		emberReq = reqLookup->second;

		verbose(CALL_INFO, 2, 0, "Wait, request=%" PRIu64 "\n", request);

		enqueueCompute(evQ, startTime, endTime);
		enQ_wait( evQ, emberReq );

		// Remove the request from the map
		liveRequests.erase(reqLookup);
	} else {
		verbose(CALL_INFO, 2, 0, "Wait: did not enqueue request because request entry is MPI_REQUEST_NULL\n");
	}
}

void EmberSIRIUSTraceGenerator::readMPIBcast( std::queue<EmberEvent*>& evQ ) {
	const double startTime = readTime();
	const uint64_t buffer = readUINT64();
	const uint32_t count = readUINT32();
	const PayloadDataType dType = readDataType();
	const int32_t root = readINT32();
	const Communicator* comm = readCommunicator();
	const double endTime = readTime();
	const int32_t result = readINT32();

	void* realBuffer = memAlloc( count * getTypeElementSize(dType) );

	verbose(CALL_INFO, 2, 0, "Bcast: root=%" PRId32 ", count=%" PRIu32 "\n", root, count);

	enqueueCompute(evQ, startTime, endTime);
	enQ_bcast( evQ, realBuffer, count, dType, root, *comm );
}

void EmberSIRIUSTraceGenerator::readMPIFinalize( std::queue<EmberEvent*>& evQ ) {
	const double startTime = readTime();
	const double endTime   = readTime();
	const int32_t result = readINT32();

	// We do NOT issue a Finalize because we may load in additional motifs after us
	// there is a Fini motif for this work
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

const Communicator* EmberSIRIUSTraceGenerator::readCommunicator() const {
	uint32_t comm;

	size_t readLen = fread(&comm, sizeof(comm), 1, trace_file);

        if( 1 != readLen ) {
                fatal(CALL_INFO, -1, "I/O Error reading from SIRIUS trace.\n");
        }

	if( 0 == comm ) {
		return &GroupWorld;
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
	case SIRIUS_MPI_FLOAT:
		return FLOAT;
	case SIRIUS_MPI_LONG:
		return LONG;
	case SIRIUS_MPI_CHAR:
		return CHAR;
	case SIRIUS_MPI_COMPLEX:
		return COMPLEX;
	default:
		fatal(CALL_INFO, -1, "I/O Error reading from SIRIUS trace, unknown data type\n");
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
	case FLOAT:
		return 4;
	case LONG:
		return 8;
	case COMPLEX:
		return 16;
	default:
		fatal(CALL_INFO, -1, "Error: unknown data type provided for conversion.\n");
	}

	return 0;
}

ReductionOperation EmberSIRIUSTraceGenerator::readReductionOp() const {
	uint32_t opType;

	size_t readLen = fread(&opType, sizeof(opType), 1, trace_file);

	if( 1 != readLen ) {
		fatal(CALL_INFO, -1, "I/O Error reading from SIRIUS trace.\n");
	}

	switch(opType) {
	case SIRIUS_MPI_SUM:
		return MP::SUM;
	case SIRIUS_MPI_MAX:
		return MP::MAX;
	case SIRIUS_MPI_MIN:
		return MP::MIN;
	default:
		fatal(CALL_INFO, -1, "I/O Error - unknown MPI operation type\n");
	}

	return MP::SUM;
}
