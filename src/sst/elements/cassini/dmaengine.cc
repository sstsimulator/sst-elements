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

#include <sst/core/output.h>
#include <sst/core/component.h>

#include <dmaengine.h>
#include <dmacmd.h>
#include <dmastate.h>
#include <dmamemop.h>

using namespace SST;
using namespace SST::Interfaces;
using namespace SST::Statistics;

DMAEngine::DMAEngine(SST::ComponentId_t id, SST::Params& params) :
	Component(id) {

	const uint32_t verbose = params.find<uint32_t>("verbose", 0);
	output = new Output("DMAEngine[@p:@l]: ", verbose, 0, SST::Output::STDOUT);

	maxInFlight   = params.find<uint32_t>("max_ops_in_flight", 32);
	opsInFlight = 0;

	cacheLineSize = params.find<uint64_t>("cache_line_size", 64);

	std::string memInterfaceName = params.find<std::string>("memoryinterface", "memHierarchy.memInterface");
	output->verbose(CALL_INFO, 2, 0, "Memory interface to be loaded: %s\n", memInterfaceName.c_str());

	Params interfaceParams = params.find_prefix_params("memoryinterfaceparams.");
	cache_link = dynamic_cast<SimpleMem*>( loadModuleWithComponent(interfaceName,
                this, interfaceParams) );

	if(NULL == cache_link) {
		output->fatal(CALL_INFO, -1, "Error loading memory interface module (%s)\n", memInterfaceName.c_str());
	} else {
		output->verbose(CALL_INFO, 2, 0, "Memory interface loaded successfully.\n");
	}

	cpuLinkCount = (uint32_t) params.find<uint32_t>("cpu_link_count", 1);

	output->verbose(CALL_INFO, 2, 0, "Loading CPU links (total of %" PRIu32 " links requested).\n", cpuLinkCount);
	char* linkNameBuffer = (char*) malloc(sizeof(char) * 256);
	cpuSideLinks = (SST::Link*) malloc(sizeof(SST::Link) * cpuLinkCount);

	for(uint32_t i = 0; i < cpuLinkCount; ++i) {
		sprintf(linkNameBuffer, "cpu_link_%" PRIu32, i);
		cpuSideLinks[i] = configureLink(linkNameBuffer);

		if(NULL == cpuSideLinks[i]) {
			output->fatal(CALL_INFO, -1, "Unable to configure DMA-to-CPU link %" PRIu32 "\n", i);
		} else {
			output->verbose(CALL_INFO, 2, 0, "DMA-to-CPU link %" PRIu32 " configured successfully.\n", i);
		}
	}

	free(linkNameBuffer);

	output->verbose(CALL_INFO, 1, 0, "=======================================================\n");
	output->verbose(CALL_INFO, 1, 0, "DMA Engine Configuration: (%s)\n", getName().c_str());
	output->verbose(CALL_INFO, 1, 0, "\n");
	output->verbose(CALL_INFO, 1, 0, "Maximum memory ops in flight:   %" PRIu32 "\n", maxInFlight);
	output->verbose(CALL_INFO, 1, 0, "Cache Line Size (bytes):        %" PRIu64 "\n", cacheLineSize);
	output->verbose(CALL_INFO, 1, 0, "CPU Links:                      %" PRIu32 "\n", cpuLinkCount);
}

DMAEngine::~DMAEngine() {
	delete output;
	free(cpuSideLinks);
}

void DMAEngine::init(unsigned int phase) {

}

void DMAEngine::finish() {

}

void DMAEngine::issueNextCommand() {
	if(NULL != currentState) {
		if(currentState->issueCompleted() && pendingReadReqs->size() == 0) {
			// Return to the CPU links so they know this is done
		} else {
			// Operation is still pending, cannot issue the next command
			// until this one completed.
			return;
		}
	}

	if(cmdQ->size() > 0) {
		// Create a new DMA engine state record
		DMACommand* nextCmd = cmdQ->pop_front();
		DMAEngineState* newState = new DMAEngineState(nextCmd);
		currentState = newState;

		// Now create the initial read requests
		uint64_t startAddr = nextCmd->getSrcAddr();

		// Check if we have aligned start
		uint64_t alignDiff = startAddr % cacheLineSize;

		if(alignDiff > 0) {
			// Check to see if the DMA request is for a short byte
			// transfer, this would be horribly inefficient but we do
			// want to be functionally correct
			alignDiff = std::min(alignDiff, nextCmd->getCommandLength());
		} else {
			// Ensure we do not over transfer, start with one cache line
			alignDiff = std::min(cacheLineSize, nextCmd->getCommandLength());
		}

		// Issue read into the memory system
		SimpleMem::Request* alignRead = new SimpleMem::Request(SimpleMem::Request::Read,
			nextCmd->getSrcAddr(), alignDiff);
		pendingRequests->insert( std::pair<SimpleMem::Request::id_t, DMAMemoryOperation*>(
			alignRead->getId(), new DMAMemoryOperation(0, alignDiff)) );

		currentState->addIssuedBytes(alignDiff);
		opsInFlight++;

		// Keep issuing until we can't do any more
		while(issueNextReadOperation());
	}
}

bool DMAEngine::issueNextReadOperation() {
	if( ! currentState->allReadsIssued() ) {
		if(opsInFlight < maxInFlight) {
			uint64_t nextReqLength = std::min(cacheLineSize,
                                              (currentState->getCommandLength() - currentState->getIssuedBytes()));

			SimpleMem::Request* nextReq = new SimpleMem::Request(SimpleMem::Request::Read,
				currentState->getSrcAddr() + currentState->getIssuedBytes(),
				nextReqLength);
			pendingRequests->insert(std::pair<SimpleMem::Request::id_t, DMAMemoryOperation*>(
				nextReq->getId(), new DMAMemoryOperation(0, nextReqLength)) );

			currentState->addIssuedBytes(nextReqLength);
			opsInFlight++;

			// We did issue, so return true to let caller know
			return true;
		} else {
			return false;
		}
	} else {
		return false;
	}
}

void DMAEngine::handleDMACommandIssue(SST::Event* ev) {
	DMACommand* dmaEv = dynamic_cast<DMACommand*>(ev);

	if(NULL == dmaEv) {
		output->fatal(CALL_INFO, -1, "DMA Engine recv event which did not cast to DMACommand.\n");
	}

	output->verbose(CALL_INFO, 4, 0, "Recv DMACommand: ID=(%" PRIu64 ", %" PRIu64 "), Src=%" PRIu64 ", Dest=%" PRIu64 ", Len=%" PRIu64 " bytes\n",
		dmaEv->getCommandID().first, dmaEv->getCommand().second,
		dmaEv->getSrcAddr(), dmaEv->getDestAddr(), dmaEv->getLength());
	output->verbose(CALL_INFO, 4, 0, "Enqueuing DMA command in pending queue, current queue length is: %" PRIu32 "\n",
		(uint32_t) cmdQ->size());

	cmdQ->push_back(dmaEv);
}

void DMAEngine::issueWriteRequest(const uint64_t writeAddr,
	const uint64_t length, char* payload) {

}

void DMAEngine::handleMemorySystemEvent(Interfaces::SimpleMem::Request* ev) {

	std::map<SimpleMem::Request::id_t, DMAMemoryOperation*>::iterator findEv;
	findEv = pendingReadReqs.find(ev->getId());

	if(findEv == pendingReadReqs.end()) {
		output->fatal(CALL_INFO, -1, "Recv event but unable to find ID in table.\n");
	}

	DMAOperation* completedOp = findEv->second;
	issueWriteRequest(currentState->getDestAddr() + completedOp->getByteOffset(),
		completedOp->getLength(), NULL);
}


