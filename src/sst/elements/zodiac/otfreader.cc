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
#include "otfreader.h"

using namespace std;
using namespace SST::Zodiac;

int handleOTFDefineProcess(void *userData, uint32_t stream, uint32_t process, const char *name, uint32_t parent) {
	std::cout << "OTF: Define Process: " << process << " process name: " << name << std::endl;

	return OTF_RETURN_OK;
}

int handleOTFSendMsg(void *userData, uint64_t time, uint32_t sender, uint32_t receiver, uint32_t group, uint32_t type,
	uint32_t length, uint32_t source, OTF_KeyValueList *list) {

	OTFReader* reader = (OTFReader*) userData;
	std::cout << "OTF: Send message from " << sender << " to " << receiver << " qSize=" << reader->getCurrentQueueSize() << std::endl;

	ZodiacSendEvent* sendEv = new ZodiacSendEvent(receiver, length, HERMES_DOUBLE, 0, group);
	reader->enqueueEvent(sendEv);

	return OTF_RETURN_OK;
}

int handleOTFRecvMsg(void *userData, uint64_t time, uint32_t recvProc, uint32_t sendProc, uint32_t group, uint32_t type,
	uint32_t length, uint32_t source, OTF_KeyValueList *list) {

	OTFReader* reader = (OTFReader*) userData;
	std::cout << "OTF: Recv message from " << sendProc << " to " << recvProc << " qSize=" << reader->getCurrentQueueSize() << std::endl;

	ZodiacRecvEvent* recvEv = new ZodiacRecvEvent(recvProc, length, HERMES_DOUBLE, 0, group);
	reader->enqueueEvent(recvEv);

	return OTF_RETURN_OK;
}

int handleOTFEnter(void* data, uint64_t time, uint32_t func, uint32_t proc, uint32_t src) {
	std::cout << "OTF: Entered function: " << func << " on process: " << proc << " at time: " << time << std::endl;

	return OTF_RETURN_OK;
}

int handleOTFExit(void* data, uint64_t time, uint32_t func, uint32_t proc, uint32_t src) {
	std::cout << "OTF: Exited function: " << func << " on process: " << proc << " at time: " << time << std::endl;

	return OTF_RETURN_OK;
}

int handleOTFCollectiveOperation(void *userData, uint64_t time, uint32_t process, uint32_t collective, uint32_t procGroup,
	uint32_t rootProc, uint32_t sent, uint32_t received, uint64_t duration, uint32_t source, OTF_KeyValueList *list) {

	std::cout << "OTF: Performed a collective operation on process: " << process << ", collective: " <<
		collective << " at time: " << time << std::endl;

	return OTF_RETURN_OK;
}

int handleOTFBeginCollective(void *userData, uint64_t time, uint32_t process, uint32_t collOp, uint64_t matchingId, uint32_t procGroup,
	uint32_t rootProc, uint64_t sent, uint64_t received, uint32_t scltoken, OTF_KeyValueList *list) {

	std::cout << "OTF: Began a collective on process: " << process << " collective: " << collOp
		<< " at time: " << time << std::endl;

	return OTF_RETURN_OK;
}

int handleOTFEndCollective(void *userData, uint64_t time, uint32_t process, uint64_t matchingId, OTF_KeyValueList *list) {
	std::cout << "OTF: Ended a collective on process: " << process << std::endl;


	return OTF_RETURN_OK;
}

OTFReader::OTFReader(string file, uint32_t focusRank, uint32_t maxQLength, std::queue<ZodiacEvent*>* evQ) {
	// Open a maximum of 5 files concurrently
	fileMgr = OTF_FileManager_open(5);

	// Set the rank we are going to focus on.
	rank = focusRank;

	if(NULL == fileMgr) {
		std::cerr << "Unable to successfully create an OFT file manager." << std::endl;
		exit(-1);
	}

	reader = OTF_Reader_open(file.c_str(), fileMgr);

	if(NULL == reader) {
		std::cerr << "Unable to successfully launch an OTF reader." << std::endl;
		exit(-1);
	}

	handlers = OTF_HandlerArray_open();

	if(NULL == handlers) {
		std::cerr << "Unable to create an OTF handler array." << std::endl;
		exit(-1);
	}

	OTF_HandlerArray_setHandler(handlers, (OTF_FunctionPointer*) handleOTFEnter, OTF_ENTER_RECORD);
	OTF_HandlerArray_setFirstHandlerArg(handlers, this, OTF_ENTER_RECORD);
	OTF_HandlerArray_setHandler(handlers, (OTF_FunctionPointer*) handleOTFExit, OTF_LEAVE_RECORD);
	OTF_HandlerArray_setFirstHandlerArg(handlers, this, OTF_LEAVE_RECORD);
	OTF_HandlerArray_setHandler(handlers, (OTF_FunctionPointer*) handleOTFDefineProcess, OTF_DEFPROCESS_RECORD);
	OTF_HandlerArray_setFirstHandlerArg(handlers, this, OTF_DEFPROCESS_RECORD);
	OTF_HandlerArray_setHandler(handlers, (OTF_FunctionPointer*) handleOTFRecvMsg, OTF_RECEIVE_RECORD);
	OTF_HandlerArray_setFirstHandlerArg(handlers, this, OTF_RECEIVE_RECORD);
	OTF_HandlerArray_setHandler(handlers, (OTF_FunctionPointer*) handleOTFSendMsg, OTF_SEND_RECORD);
	OTF_HandlerArray_setFirstHandlerArg(handlers, this, OTF_SEND_RECORD);
	OTF_HandlerArray_setHandler(handlers, (OTF_FunctionPointer*) handleOTFCollectiveOperation, OTF_COLLOP_RECORD);
	OTF_HandlerArray_setFirstHandlerArg(handlers, this, OTF_COLLOP_RECORD);
	OTF_HandlerArray_setHandler(handlers, (OTF_FunctionPointer*) handleOTFBeginCollective, OTF_BEGINCOLLOP_RECORD);
	OTF_HandlerArray_setFirstHandlerArg(handlers, this, OTF_BEGINCOLLOP_RECORD);
	OTF_HandlerArray_setHandler(handlers, (OTF_FunctionPointer*) handleOTFEndCollective, OTF_ENDCOLLOP_RECORD);
	OTF_HandlerArray_setFirstHandlerArg(handlers, this, OTF_ENDCOLLOP_RECORD);


	// Read in the definitions from the trace(s)
	OTF_Reader_readDefinitions(reader, handlers);

	// Focus the OTF to a specific rank (disable all ranks, then enable the focus)
	std::cout << "Disabling all ranks, then re-enabling rank: " << rank << std::endl;
	OTF_Reader_setProcessStatusAll(reader, 0);
	OTF_Reader_setProcessStatus(reader, rank, 1);

	// These initialize the reading logic
	OTF_Reader_setRecordLimit(reader, 0);
	OTF_Reader_readEvents(reader, handlers);

	std::cout << "Constructing an OTF Reader, setting event count to 1..." << std::endl;
        OTF_Reader_setRecordLimit(reader, 1);

	eventQ = evQ;
	qLimit = maxQLength;
	foundFinalize = false;
}

void OTFReader::close() {
	OTF_HandlerArray_close(handlers);
	OTF_FileManager_close(fileMgr);
	OTF_Reader_close(reader);
}

uint32_t OTFReader::generateNextEvents() {
	std::cout << "Generating next event set..." << std::endl;

	while((eventQ->size() < qLimit) && (!foundFinalize)) {
		std::cout << "Reading next event, queue size: " << eventQ->size() << std::endl;
		if(OTF_READ_ERROR == OTF_Reader_readEvents(reader, handlers)) {
			// We read zero events from the file, exit the loop
			break;
		}
	}

	// Return the size of the queue back to the caller, they
	// may want to know that we consumed fewer events than
	// the queue limit - this happens if we encounter an MPI
	// finalize statement
	return (uint32_t) eventQ->size();
}

uint32_t OTFReader::getQueueLimit() {
	return qLimit;
}

uint32_t OTFReader::getCurrentQueueSize() {
	return eventQ->size();
}

void OTFReader::enqueueEvent(ZodiacEvent* ev) {
	assert(eventQ->size() < qLimit);
	eventQ->push(ev);
}
