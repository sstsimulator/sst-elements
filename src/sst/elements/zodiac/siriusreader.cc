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

#include "siriusreader.h"

using namespace std;
using namespace SST::Zodiac;

#if ((__GNUC__ > 4) || (__GNUC__ == 4 && __GNUC_MINOR__ >= 6))
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-variable"
#pragma GCC diagnostic ignored "-Wunused-result"
#endif

#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-variable"
#endif


SiriusReader::SiriusReader(char* file, uint32_t focusOnRank, uint32_t maxQLen, std::queue<ZodiacEvent*>* evQ, int verbose)
{

	rank = focusOnRank;
	eventQ = evQ;
	qLimit = maxQLen;
	foundFinalize = false;

	trace = fopen(file, "rb");
	if(NULL == trace) {
		std::cerr << "Error opening the Sirius trace file: " << file << std::endl;
		exit(-1);
	}

	prevEventTime = 0;
	output = new Output("SiriusReader", verbose, 0, Output::STDOUT);
	readInit();
}

void SiriusReader::close() {
	if(NULL == trace) {
		output->fatal(CALL_INFO, -1, "Error: trace file is NULL when being closed, has an error occured in SIRIUS?\n");
	} else {
		output->verbose(CALL_INFO, 4, 0, "Closing trace file.\n");
	}

	fclose(trace);
}

uint32_t SiriusReader::generateNextEvents() {
//	int finalized_reached = 0;

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
		output->verbose(__LINE__, __FILE__, "generateNextEvent", 8, 0, "Generated a compute event (length=%f)\n", evTimeDiff);
		ZodiacComputeEvent* ev = new ZodiacComputeEvent(evTimeDiff);
		eventQ->push(ev);
	} else {
		output->verbose(__LINE__, __FILE__, "generateNextEvent", 8, 0,
			"Did not generate next event timing prevTime=%f, callTime=%f, diff=%f\n",
			prevEventTime, callTime, evTimeDiff);
	}

	switch(call_type) {
	case SIRIUS_MPI_SEND:
		readSend();
		break;

	case SIRIUS_MPI_RECV:
		readRecv();
		break;

	case SIRIUS_MPI_IRECV:
		readIrecv();
		break;

	case SIRIUS_MPI_ALLREDUCE:
		readAllreduce();
		break;

	case SIRIUS_MPI_BARRIER:
		readBarrier();
		break;

	case SIRIUS_MPI_WAIT:
		readWait();
		break;

	case SIRIUS_MPI_INIT:
		readInit();
		break;

	case SIRIUS_MPI_FINALIZE:
		readFinalize();
		break;

	default:
		std::cout << "Unknown MPI command in trace (" << call_type << ") position: " <<
			ftell(trace) << std::endl;
		exit(-1);
		break;
	}


	// Read the profiled MPI time
	prevEventTime = readTime();
	// read the MPI function result
	readINT32();
}

void SiriusReader::readAllreduce() {
	uint64_t sbuff = readUINT64();
	uint64_t rbuff = readUINT64();
	uint32_t length = readUINT32();
	uint32_t dtype = readUINT32();
	uint32_t op    = readUINT32();
	uint32_t comm  = readUINT32();

	output->verbose(__LINE__, __FILE__, "readAllreduce", 8, 0, "Read an MPI_Allreduce\n");

	ZodiacAllreduceEvent* ev = new ZodiacAllreduceEvent(
			length,
			convertToHermesType(dtype),
			convertToHermesOp(op),
			comm);
	eventQ->push(ev);
}

void SiriusReader::readSend() {
	uint64_t buffer = readUINT64();
	uint32_t count  = readUINT32();
	uint32_t dtype  = readUINT32();
	int32_t  dest   = readINT32();
	int32_t  tag    = readINT32();
	uint32_t comm   = readUINT32();

	output->verbose(__LINE__, __FILE__, "readSend", 8, 0, "Read an MPI_Send\n");

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

	output->verbose(__LINE__, __FILE__, "readRecv", 8, 0, "Read an MPI_Recv\n");

	ZodiacRecvEvent* ev = new ZodiacRecvEvent((uint32_t) src, count,
		convertToHermesType(dtype), tag, comm);
	eventQ->push(ev);
}

void SiriusReader::readIrecv() {
	uint64_t buffer = readUINT64();
	uint32_t count  = readUINT32();
	uint32_t dtype  = readUINT32();
	int32_t  src   = readINT32();
	int32_t  tag    = readINT32();
	uint32_t comm   = readUINT32();
	uint64_t req    = readUINT64();

	output->verbose(__LINE__, __FILE__, "readIrecv", 8, 0, "Read an MPI_Irecv\n");

	ZodiacIRecvEvent* ev = new ZodiacIRecvEvent((uint32_t) src, count,
		convertToHermesType(dtype), tag, comm, req);
	eventQ->push(ev);
}

void SiriusReader::readWait() {
	uint64_t reqID = readUINT64();
	uint64_t status = readUINT64();

	output->verbose(__LINE__, __FILE__, "readWait", 8, 0, "Read an MPI_Wait\n");

	ZodiacWaitEvent* ev = new ZodiacWaitEvent(reqID);
	eventQ->push(ev);
}

void SiriusReader::readInit() {
	output->verbose(__LINE__, __FILE__, "readInit", 8, 0, "Read an MPI_Init\n");
	ZodiacInitEvent* ev = new ZodiacInitEvent();
	eventQ->push(ev);
}

void SiriusReader::readFinalize() {
	output->verbose(__LINE__, __FILE__, "readFinalize", 8, 0, "Read an MPI_Finalize\n");

	ZodiacFinalizeEvent* ev = new ZodiacFinalizeEvent();
	eventQ->push(ev);

	foundFinalize = true;
}

void SiriusReader::readBarrier() {
	uint32_t comm = readUINT32();

	output->verbose(__LINE__, __FILE__, "readRecv", 8, 0, "Read an MPI_Barrier\n");

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

ReductionOperation SiriusReader::convertToHermesOp(uint32_t op) {
	ReductionOperation h_op = SUM;

	switch(op) {
	case SIRIUS_MPI_SUM:
		h_op = SUM;
		break;
	case SIRIUS_MPI_MAX:
		h_op = MAX;
		break;
	case SIRIUS_MPI_MIN:
		h_op = MIN;
		break;
	default:
		std::cout << "Unknown MPI operation, cannot convert to Hermes." << std::endl;
		exit(-1);
	}

	return h_op;
}

bool SiriusReader::hasReachedFinalize() {
	return foundFinalize;
}

void SiriusReader::setOutput(Output* oput) {
	if(output != NULL)
		delete output;

	output = oput;
}

#if defined(__clang__)
#pragma clang diagnostic pop
#endif

#if ((__GNUC__ > 4) || (__GNUC__ == 4 && __GNUC_MINOR__ >= 6))
#pragma GCC diagnostic pop
#endif

