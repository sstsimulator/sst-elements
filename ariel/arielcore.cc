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

#include "arielcore.h"

#define ARIEL_CORE_VERBOSE(LEVEL, OUTPUT) if(verbosity >= (LEVEL)) OUTPUT

ArielCore::ArielCore(ArielTunnel *tunnel, SimpleMem* coreToCacheLink,
        uint32_t thisCoreID, uint32_t maxPendTrans,
        Output* out, uint32_t maxIssuePerCyc,
        uint32_t maxQLen, uint64_t cacheLineSz, SST::Component* own,
        ArielMemoryManager* memMgr, const uint32_t perform_address_checks, Params& params) :
	output(out), tunnel(tunnel), perform_checks(perform_address_checks),
	verbosity(static_cast<uint32_t>(out->getVerboseLevel()))
{
	output->verbose(CALL_INFO, 2, 0, "Creating core with ID %" PRIu32 ", maximum queue length=%" PRIu32 ", max issue is: %" PRIu32 "\n", thisCoreID, maxQLen, maxIssuePerCyc);
	cacheLink = coreToCacheLink;
        allocLink = 0;
	coreID = thisCoreID;
	maxPendingTransactions = maxPendTrans;
	isHalted = false;
	maxIssuePerCycle = maxIssuePerCyc;
	maxQLength = maxQLen;
	cacheLineSize = cacheLineSz;
	owner = own;
	memmgr = memMgr;

	coreQ = new std::queue<ArielEvent*>();
	pendingTransactions = new std::unordered_map<SimpleMem::Request::id_t, SimpleMem::Request*>();
	pending_transaction_count = 0;

	char* subID = (char*) malloc(sizeof(char) * 32);
	sprintf(subID, "%" PRIu32, thisCoreID);

	statReadRequests  = own->registerStatistic<uint64_t>( "read_requests", subID );
	statWriteRequests = own->registerStatistic<uint64_t>( "write_requests", subID );
	statSplitReadRequests = own->registerStatistic<uint64_t>( "split_read_requests", subID );
	statSplitWriteRequests = own->registerStatistic<uint64_t>( "split_write_requests", subID );
	statNoopCount     = own->registerStatistic<uint64_t>( "no_ops", subID );
	statInstructionCount = own->registerStatistic<uint64_t>( "instruction_count", subID );

	statFPSPIns = own->registerStatistic<uint64_t>("fp_sp_ins", subID);
	statFPDPIns = own->registerStatistic<uint64_t>("fp_dp_ins", subID);

	statFPSPSIMDIns = own->registerStatistic<uint64_t>("fp_sp_simd_ins", subID);
	statFPDPSIMDIns = own->registerStatistic<uint64_t>("fp_dp_simd_ins", subID);

	statFPSPScalarIns = own->registerStatistic<uint64_t>("fp_sp_scalar_ins", subID);
	statFPDPScalarIns = own->registerStatistic<uint64_t>("fp_dp_scalar_ins", subID);

	statFPSPOps = own->registerStatistic<uint64_t>("fp_sp_ops", subID);
	statFPDPOps = own->registerStatistic<uint64_t>("fp_dp_ops", subID);

	free(subID);

	std::string traceGenName = params.find_string("tracegen", "");
	enableTracing = ("" != traceGenName);

	// If we enabled tracing then open up the correct file.
	if(enableTracing) {
		Params interfaceParams = params.find_prefix_params("tracer.");
		traceGen = dynamic_cast<ArielTraceGenerator*>( own->loadModuleWithComponent(traceGenName, own,
			interfaceParams) );

		if(NULL == traceGen) {
			output->fatal(CALL_INFO, -1, "Unable to load tracing module: \"%s\"\n",
				traceGenName.c_str());
		}

		traceGen->setCoreID(coreID);
	}

	currentCycles = 0;
}

ArielCore::~ArielCore() {
//	delete statReadRequests;
//	delete statWriteRequests;
//	delete statSplitReadRequests;
//	delete statSplitWriteRequests;
//	delete statNoopCount;
//	delete statInstructionCount;

	if(NULL != cacheLink) {
		delete cacheLink;
	}

	if(enableTracing && traceGen) {
		delete traceGen;
	}
}

void ArielCore::setCacheLink(SimpleMem* newLink, Link* newAllocLink) {
	cacheLink = newLink;
        allocLink = newAllocLink;
}

void ArielCore::printTraceEntry(const bool isRead,
                       	const uint64_t address, const uint32_t length) {

	if(enableTracing) {
		if(isRead) {
			traceGen->publishEntry(currentCycles, address, length, READ);
		} else {
			traceGen->publishEntry(currentCycles, address, length, WRITE);
		}
	}
}

void ArielCore::commitReadEvent(const uint64_t address,
	const uint64_t virtAddress, const uint32_t length) {

	if(length > 0) {
	        SimpleMem::Request *req = new SimpleMem::Request(SimpleMem::Request::Read, address, length);
		req->setVirtualAddress(virtAddress);

		pending_transaction_count++;
	        pendingTransactions->insert( std::pair<SimpleMem::Request::id_t, SimpleMem::Request*>(req->id, req) );

	        if(enableTracing) {
	        	printTraceEntry(true, (const uint64_t) req->addr, (const uint32_t) length);
	        }

	        // Actually send the event to the cache
	        cacheLink->sendRequest(req);
	}
}

void ArielCore::commitWriteEvent(const uint64_t address,
	const uint64_t virtAddress, const uint32_t length) {

	if(length > 0) {
        	SimpleMem::Request *req = new SimpleMem::Request(SimpleMem::Request::Write, address, length);
		req->setVirtualAddress(virtAddress);

	        // TODO BJM:  DO we need to fill in dummy data?

		pending_transaction_count++;
	        pendingTransactions->insert( std::pair<SimpleMem::Request::id_t, SimpleMem::Request*>(req->id, req) );

	        if(enableTracing) {
	        	printTraceEntry(false, (const uint64_t) req->addr, (const uint32_t) length);
	        }

	        // Actually send the event to the cache
        	cacheLink->sendRequest(req);
	}
}

void ArielCore::handleEvent(SimpleMem::Request* event) {
    ARIEL_CORE_VERBOSE(4, output->verbose(CALL_INFO, 4, 0, "Core %" PRIu32 " handling a memory event.\n", coreID));

    SimpleMem::Request::id_t mev_id = event->id;
    auto find_entry = pendingTransactions->find(mev_id);

    if(find_entry != pendingTransactions->end()) {
        ARIEL_CORE_VERBOSE(4, output->verbose(CALL_INFO, 4, 0, "Correctly identified event in pending transactions, removing from list, before there are: %" PRIu32 " transactions pending.\n",
                (uint32_t) pendingTransactions->size()));

        pendingTransactions->erase(find_entry);
        pending_transaction_count--;
    } else {
        output->fatal(CALL_INFO, -4, "Memory event response to core: %" PRIu32 " was not found in pending list.\n", coreID);
    }
    delete event;
}

void ArielCore::finishCore() {
	// Close the trace file if we did in fact open it.
	if(enableTracing && traceGen) {
		delete traceGen;
        	traceGen = NULL;
	}

}

void ArielCore::halt() {
	isHalted = true;
}


void ArielCore::handleSwitchPoolEvent(ArielSwitchPoolEvent* aSPE) {
	ARIEL_CORE_VERBOSE(2, output->verbose(CALL_INFO, 2, 0, "Core: %" PRIu32 " set default memory pool to: %" PRIu32 "\n", coreID, aSPE->getPool()));
	memmgr->setDefaultPool(aSPE->getPool());
}

void ArielCore::createSwitchPoolEvent(uint32_t newPool) {
	ArielSwitchPoolEvent* ev = new ArielSwitchPoolEvent(newPool);
	coreQ->push(ev);

	ARIEL_CORE_VERBOSE(4, output->verbose(CALL_INFO, 4, 0, "Generated a switch pool event on core %" PRIu32 ", new level is: %" PRIu32 "\n", coreID, newPool));
}

void ArielCore::createNoOpEvent() {
	ArielNoOpEvent* ev = new ArielNoOpEvent();
	coreQ->push(ev);

	ARIEL_CORE_VERBOSE(4, output->verbose(CALL_INFO, 4, 0, "Generated a No Op event on core %" PRIu32 "\n", coreID));
}

void ArielCore::createReadEvent(uint64_t address, uint32_t length) {
	ArielReadEvent* ev = new ArielReadEvent(address, length);
	coreQ->push(ev);

	ARIEL_CORE_VERBOSE(4, output->verbose(CALL_INFO, 4, 0, "Generated a READ event, addr=%" PRIu64 ", length=%" PRIu32 "\n", address, length));
}

void ArielCore::createAllocateEvent(uint64_t vAddr, uint64_t length, uint32_t level, uint64_t instPtr) {
    	ArielAllocateEvent* ev = new ArielAllocateEvent(vAddr, length, level, instPtr);
	coreQ->push(ev);

	ARIEL_CORE_VERBOSE(2, output->verbose(CALL_INFO, 2, 0, "Generated an allocate event, vAddr(map)=%" PRIu64 ", length=%" PRIu64 " in level %" PRIu32 " from IP %" PRIx64 "\n",
                        vAddr, length, level, instPtr));
}

void ArielCore::createFreeEvent(uint64_t vAddr) {
	ArielFreeEvent* ev = new ArielFreeEvent(vAddr);
	coreQ->push(ev);

	ARIEL_CORE_VERBOSE(2, output->verbose(CALL_INFO, 2, 0, "Generated a free event for virtual address=%" PRIu64 "\n", vAddr));
}

void ArielCore::createWriteEvent(uint64_t address, uint32_t length) {
	ArielWriteEvent* ev = new ArielWriteEvent(address, length);
	coreQ->push(ev);

	ARIEL_CORE_VERBOSE(4, output->verbose(CALL_INFO, 4, 0, "Generated a WRITE event, addr=%" PRIu64 ", length=%" PRIu32 "\n", address, length));
}

void ArielCore::createExitEvent() {
	ArielExitEvent* xEv = new ArielExitEvent();
	coreQ->push(xEv);

	ARIEL_CORE_VERBOSE(4, output->verbose(CALL_INFO, 4, 0, "Generated an EXIT event.\n"));
}

bool ArielCore::isCoreHalted() const {
	return isHalted;
}

bool ArielCore::refillQueue() {
    ARIEL_CORE_VERBOSE(16, output->verbose(CALL_INFO, 16, 0, "Refilling event queue for core %" PRIu32 "...\n", coreID));

    while(coreQ->size() < maxQLength) {
        ARIEL_CORE_VERBOSE(16, output->verbose(CALL_INFO, 16, 0, "Attempting to fill events for core: %" PRIu32 " current queue size=%" PRIu32 ", max length=%" PRIu32 "\n",
                coreID, (uint32_t) coreQ->size(), (uint32_t) maxQLength));

        ArielCommand ac;
        const bool avail = tunnel->readMessageNB(coreID, &ac);

        if ( !avail ) {
            ARIEL_CORE_VERBOSE(32, output->verbose(CALL_INFO, 32, 0, "Tunnel claims no data on core: %" PRIu32 "\n", coreID));
            return false;
        }

        ARIEL_CORE_VERBOSE(32, output->verbose(CALL_INFO, 32, 0, "Tunnel reads data on core: %" PRIu32 "\n", coreID));

        // There is data on the pipe
        switch(ac.command) {
        case ARIEL_OUTPUT_STATS:
            fprintf(stdout, "Performing statistics output at simulation time = %" PRIu64 "\n", owner->getCurrentSimTimeNano());
            Simulation::getSimulation()->getStatisticsProcessingEngine()->performGlobalStatisticOutput();
            if (allocLink) {
                // tell the allocate montior to dump stats. We
                // optionally pass a marker number back in the instruction field
                arielAllocTrackEvent *e 
                    = new arielAllocTrackEvent(arielAllocTrackEvent::BUOY,
                                               0, 0, 0, ac.instPtr);
                allocLink->send(e);
            }
            break;

        case ARIEL_START_INSTRUCTION:
	    if(ARIEL_INST_SP_FP == ac.inst.instClass) {
		statFPSPIns->addData(1);

		if(ac.inst.simdElemCount > 1) {
			statFPSPSIMDIns->addData(1);
		} else {
			statFPSPScalarIns->addData(1);
		}

		if(ac.inst.simdElemCount < 32)
			statFPSPOps->addData(ac.inst.simdElemCount);
            } else if(ARIEL_INST_DP_FP == ac.inst.instClass) {
		statFPDPIns->addData(1);

		if(ac.inst.simdElemCount > 1) {
			statFPDPSIMDIns->addData(1);
		} else {
			statFPDPScalarIns->addData(1);
		}

		if(ac.inst.simdElemCount < 16)
			statFPDPOps->addData(ac.inst.simdElemCount);
	    }

            while(ac.command != ARIEL_END_INSTRUCTION) {
                ac = tunnel->readMessage(coreID);

                switch(ac.command) {
                case ARIEL_PERFORM_READ:
                    createReadEvent(ac.inst.addr, ac.inst.size);
                    break;

                case ARIEL_PERFORM_WRITE:
                    createWriteEvent(ac.inst.addr, ac.inst.size);
                    break;

                case ARIEL_END_INSTRUCTION:
                    break;

                default:
                    // Not sure what this is
                    assert(0);
                    break;
                }
            }

            // Add one to our instruction counts
	    statInstructionCount->addData(1);

            break;

        case ARIEL_NOOP:
            createNoOpEvent();
            break;

        case ARIEL_ISSUE_TLM_MAP:
            createAllocateEvent(ac.mlm_map.vaddr, ac.mlm_map.alloc_len, ac.mlm_map.alloc_level, ac.instPtr);
            break;

        case ARIEL_ISSUE_TLM_FREE:
            createFreeEvent(ac.mlm_free.vaddr);
            break;

        case ARIEL_SWITCH_POOL:
            createSwitchPoolEvent(ac.switchPool.pool);
            break;

        case ARIEL_PERFORM_EXIT:
            createExitEvent();
            break;
        default:
            // Not sure what this is
            assert(0);
            break;
        }
    }

	ARIEL_CORE_VERBOSE(16, output->verbose(CALL_INFO, 16, 0, "Refilling event queue for core %" PRIu32 " is complete\n", coreID));
	return true;
}

void ArielCore::handleFreeEvent(ArielFreeEvent* rFE) {
	output->verbose(CALL_INFO, 4, 0, "Core %" PRIu32 " processing a free event (for virtual address=%" PRIu64 ")\n", coreID, rFE->getVirtualAddress());

	memmgr->free(rFE->getVirtualAddress());

        if (allocLink) {
            // tell the allocate montior (e.g. mem sieve that a free has occured)
            arielAllocTrackEvent *e = 
                new arielAllocTrackEvent(arielAllocTrackEvent::FREE,
					 rFE->getVirtualAddress(),
					 0,
                                         0, 
                                         0);
                        
            allocLink->send(e);
        }
}

void ArielCore::handleReadRequest(ArielReadEvent* rEv) {
	ARIEL_CORE_VERBOSE(4, output->verbose(CALL_INFO, 4, 0, "Core %" PRIu32 " processing a read event...\n", coreID));

	const uint64_t readAddress = rEv->getAddress();
	const uint64_t readLength  = (uint64_t) rEv->getLength();

	if(readLength > cacheLineSize) {
		output->verbose(CALL_INFO, 4, 0, "Potential error? request for a read of length=%" PRIu64 " is larger than cache line which is not allowed (coreID=%" PRIu32 ", cache line: %" PRIu64 "\n",
			readLength, coreID, cacheLineSize);
		return;
	}

	const uint64_t addr_offset  = readAddress % ((uint64_t) cacheLineSize);

	if((addr_offset + readLength) <= cacheLineSize) {
		ARIEL_CORE_VERBOSE(4, output->verbose(CALL_INFO, 4, 0, "Core %" PRIu32 " generating a non-split read request: Addr=%" PRIu64 " Length=%" PRIu64 "\n",
			coreID, readAddress, readLength));

		// We do not need to perform a split operation
		const uint64_t physAddr = memmgr->translateAddress(readAddress);

		ARIEL_CORE_VERBOSE(4, output->verbose(CALL_INFO, 4, 0, "Core %" PRIu32 " issuing read, VAddr=%" PRIu64 ", Size=%" PRIu64 ", PhysAddr=%" PRIu64 "\n", 
			coreID, readAddress, readLength, physAddr));

		commitReadEvent(physAddr, readAddress, (uint32_t) readLength);
	} else {
		ARIEL_CORE_VERBOSE(4, output->verbose(CALL_INFO, 4, 0, "Core %" PRIu32 " generating a split read request: Addr=%" PRIu64 " Length=%" PRIu64 "\n",
			coreID, readAddress, readLength));

		// We need to perform a split operation
		const uint64_t leftAddr = readAddress;
		const uint64_t leftSize = cacheLineSize - addr_offset;

		const uint64_t rightAddr = (readAddress - addr_offset) + ((uint64_t) cacheLineSize);
		const uint64_t rightSize = (readAddress + ((uint64_t) readLength)) % ((uint64_t) cacheLineSize);

		const uint64_t physLeftAddr = memmgr->translateAddress(leftAddr);
		const uint64_t physRightAddr = memmgr->translateAddress(rightAddr);

		ARIEL_CORE_VERBOSE(4, output->verbose(CALL_INFO, 4, 0, "Core %" PRIu32 " issuing split-address read, LeftVAddr=%" PRIu64 ", RightVAddr=%" PRIu64 ", LeftSize=%" PRIu64 ", RightSize=%" PRIu64 ", LeftPhysAddr=%" PRIu64 ", RightPhysAddr=%" PRIu64 "\n", 
			coreID, leftAddr, rightAddr, leftSize, rightSize, physLeftAddr, physRightAddr));

		if(perform_checks > 0) {
			if( (leftSize + rightSize) != readLength ) {
				output->fatal(CALL_INFO, -4, "Core %" PRIu32 " read request for address %" PRIu64 ", length=%" PRIu64 ", split into left address=%" PRIu64 ", left size=%" PRIu64 ", right address=%" PRIu64 ", right size=%" PRIu64 " does not equal read length (cache line of length %" PRIu64 ")\n",
					coreID, readAddress, readLength, leftAddr, leftSize, rightAddr, rightSize, cacheLineSize);
			}

			if( ((leftAddr + leftSize) % cacheLineSize) != 0) {
				output->fatal(CALL_INFO, -4, "Error leftAddr=%" PRIu64 " + size=%" PRIu64 " is not a multiple of cache line size: %" PRIu64 "\n",
					leftAddr, leftSize, cacheLineSize);
			}

			if( ((rightAddr + rightSize) % cacheLineSize) > cacheLineSize ) {
				output->fatal(CALL_INFO, -4, "Error rightAddr=%" PRIu64 " + size=%" PRIu64 " is not a multiple of cache line size: %" PRIu64 "\n",
					leftAddr, leftSize, cacheLineSize);
			}
		}

		commitReadEvent(physLeftAddr, leftAddr, (uint32_t) leftSize);
		commitReadEvent(physRightAddr, rightAddr, (uint32_t) rightSize);

		statSplitReadRequests->addData(1);
	}

	statReadRequests->addData(1);
}

void ArielCore::handleWriteRequest(ArielWriteEvent* wEv) {
	ARIEL_CORE_VERBOSE(4, output->verbose(CALL_INFO, 4, 0, "Core %" PRIu32 " processing a write event...\n", coreID));

	const uint64_t writeAddress = wEv->getAddress();
	const uint64_t writeLength  = wEv->getLength();

	if(writeLength > cacheLineSize) {
		output->verbose(CALL_INFO, 4, 0, "Potential error? request for a write of length=%" PRIu64 " is larger than cache line which is not allowed (coreID=%" PRIu32 ", cache line: %" PRIu64 "\n",
			writeLength, coreID, cacheLineSize);
		return;
	}

	const uint64_t addr_offset  = writeAddress % ((uint64_t) cacheLineSize);

	if((addr_offset + writeLength) <= cacheLineSize) {
		ARIEL_CORE_VERBOSE(4, output->verbose(CALL_INFO, 4, 0, "Core %" PRIu32 " generating a non-split write request: Addr=%" PRIu64 " Length=%" PRIu64 "\n",
			coreID, writeAddress, writeLength));
	
		// We do not need to perform a split operation
		const uint64_t physAddr = memmgr->translateAddress(writeAddress);
		
		ARIEL_CORE_VERBOSE(4, output->verbose(CALL_INFO, 4, 0, "Core %" PRIu32 " issuing write, VAddr=%" PRIu64 ", Size=%" PRIu64 ", PhysAddr=%" PRIu64 "\n", 
			coreID, writeAddress, writeLength, physAddr));

		commitWriteEvent(physAddr, writeAddress, (uint32_t) writeLength);
	} else {
		ARIEL_CORE_VERBOSE(4, output->verbose(CALL_INFO, 4, 0, "Core %" PRIu32 " generating a split write request: Addr=%" PRIu64 " Length=%" PRIu64 "\n",
			coreID, writeAddress, writeLength));
	
		// We need to perform a split operation
		const uint64_t leftAddr = writeAddress;
		const uint64_t leftSize = cacheLineSize - addr_offset;

		const uint64_t rightAddr = (writeAddress - addr_offset) + ((uint64_t) cacheLineSize);
		const uint64_t rightSize = (writeAddress + ((uint64_t) writeLength)) % ((uint64_t) cacheLineSize);

		const uint64_t physLeftAddr = memmgr->translateAddress(leftAddr);
		const uint64_t physRightAddr = memmgr->translateAddress(rightAddr);

		ARIEL_CORE_VERBOSE(4, output->verbose(CALL_INFO, 4, 0, "Core %" PRIu32 " issuing split-address write, LeftVAddr=%" PRIu64 ", RightVAddr=%" PRIu64 ", LeftSize=%" PRIu64 ", RightSize=%" PRIu64 ", LeftPhysAddr=%" PRIu64 ", RightPhysAddr=%" PRIu64 "\n", 
			coreID, leftAddr, rightAddr, leftSize, rightSize, physLeftAddr, physRightAddr));

		if(perform_checks > 0) {
			if( (leftSize + rightSize) != writeLength ) {
				output->fatal(CALL_INFO, -4, "Core %" PRIu32 " write request for address %" PRIu64 ", length=%" PRIu64 ", split into left address=%" PRIu64 ", left size=%" PRIu64 ", right address=%" PRIu64 ", right size=%" PRIu64 " does not equal write length (cache line of length %" PRIu64 ")\n",
					coreID, writeAddress, writeLength, leftAddr, leftSize, rightAddr, rightSize, cacheLineSize);
			}

			if( ((leftAddr + leftSize) % cacheLineSize) != 0) {
				output->fatal(CALL_INFO, -4, "Error leftAddr=%" PRIu64 " + size=%" PRIu64 " is not a multiple of cache line size: %" PRIu64 "\n",
					leftAddr, leftSize, cacheLineSize);
			}

			if( ((rightAddr + rightSize) % cacheLineSize) > cacheLineSize ) {
				output->fatal(CALL_INFO, -4, "Error rightAddr=%" PRIu64 " + size=%" PRIu64 " is not a multiple of cache line size: %" PRIu64 "\n",
					leftAddr, leftSize, cacheLineSize);
			}
		}

		commitWriteEvent(physLeftAddr, leftAddr, (uint32_t) leftSize);
		commitWriteEvent(physRightAddr, rightAddr, (uint32_t) rightSize);
		statSplitWriteRequests->addData(1);
	}

	statWriteRequests->addData(1);
}

void ArielCore::handleAllocationEvent(ArielAllocateEvent* aEv) {
	output->verbose(CALL_INFO, 2, 0, "Handling a memory allocation event, vAddr=%" PRIu64 ", length=%" PRIu64 ", at level=%" PRIu32 " from ip=%" PRIx64 "\n",
                        aEv->getVirtualAddress(), aEv->getAllocationLength(), aEv->getAllocationLevel(), aEv->getInstructionPointer());

	memmgr->allocate(aEv->getAllocationLength(), aEv->getAllocationLevel(), aEv->getVirtualAddress());

        if (allocLink) {
	  output->verbose(CALL_INFO, 2, 0, " Sending memory allocation event to allocate monitor\n");
            // tell the allocate montior (e.g. mem sieve that an
            // allocation has occured)
            arielAllocTrackEvent *e 
                = new arielAllocTrackEvent(arielAllocTrackEvent::ALLOC,
                                           aEv->getVirtualAddress(),
					   aEv->getAllocationLength(),
                                           aEv->getAllocationLevel(),
                                           aEv->getInstructionPointer());
            allocLink->send(e);
        }
}

void ArielCore::printCoreStatistics() {
}

bool ArielCore::processNextEvent() {
	// Attempt to refill the queue
	if(coreQ->empty()) {
		bool addedItems = refillQueue();

		ARIEL_CORE_VERBOSE(16, output->verbose(CALL_INFO, 16, 0, "Attempted a queue fill, %s data\n",
			(addedItems ? "added" : "did not add")));

		if(! addedItems) {
			return false;
		}
	}
	
	ARIEL_CORE_VERBOSE(8, output->verbose(CALL_INFO, 8, 0, "Processing next event in core %" PRIu32 "...\n", coreID));

	ArielEvent* nextEvent = coreQ->front();
	bool removeEvent = false;

	switch(nextEvent->getEventType()) {
	case NOOP:
		ARIEL_CORE_VERBOSE(8, output->verbose(CALL_INFO, 8, 0, "Core %" PRIu32 " next event is NOOP\n", coreID));

		statNoopCount->addData(1);
		removeEvent = true;
		break;

	case READ_ADDRESS:
		ARIEL_CORE_VERBOSE(8, output->verbose(CALL_INFO, 8, 0, "Core %" PRIu32 " next event is READ_ADDRESS\n", coreID));

//		if(pendingTransactions->size() < maxPendingTransactions) {
		if(pending_transaction_count < maxPendingTransactions) {
			ARIEL_CORE_VERBOSE(16, output->verbose(CALL_INFO, 16, 0, "Found a read event, fewer pending transactions than permitted so will process...\n"));
			removeEvent = true;
			handleReadRequest(dynamic_cast<ArielReadEvent*>(nextEvent));
		} else {
			ARIEL_CORE_VERBOSE(16, output->verbose(CALL_INFO, 16, 0, "Pending transaction queue is currently full for core %" PRIu32 ", core will stall for new events\n", coreID));
			break;
		}
		break;

	case WRITE_ADDRESS:
		ARIEL_CORE_VERBOSE(8, output->verbose(CALL_INFO, 8, 0, "Core %" PRIu32 " next event is WRITE_ADDRESS\n", coreID));

//		if(pendingTransactions->size() < maxPendingTransactions) {
		if(pending_transaction_count < maxPendingTransactions) {
			ARIEL_CORE_VERBOSE(16, output->verbose(CALL_INFO, 16, 0, "Found a write event, fewer pending transactions than permitted so will process...\n"));
			removeEvent = true;
			handleWriteRequest(dynamic_cast<ArielWriteEvent*>(nextEvent));
		} else {
			ARIEL_CORE_VERBOSE(16, output->verbose(CALL_INFO, 16, 0, "Pending transaction queue is currently full for core %" PRIu32 ", core will stall for new events\n", coreID));
			break;
		}
		break;

	case START_DMA_TRANSFER:
		ARIEL_CORE_VERBOSE(8, output->verbose(CALL_INFO, 8, 0, "Core %" PRIu32 " next event is START_DMA_TRANSFER\n", coreID));
		removeEvent = true;
		break;

	case WAIT_ON_DMA_TRANSFER:
		ARIEL_CORE_VERBOSE(8, output->verbose(CALL_INFO, 8, 0, "Core %" PRIu32 " next event is WAIT_ON_DMA_TRANSFER\n", coreID));
		removeEvent = true;
		break;

	case SWITCH_POOL:
		ARIEL_CORE_VERBOSE(8, output->verbose(CALL_INFO, 8, 0, "Core %" PRIu32 " next event is a SWITCH_POOL\n",
			coreID));
		removeEvent = true;
		handleSwitchPoolEvent(dynamic_cast<ArielSwitchPoolEvent*>(nextEvent));
		break;

	case FREE:
		ARIEL_CORE_VERBOSE(8, output->verbose(CALL_INFO, 8, 0, "Core %" PRIu32 " next event is FREE\n", coreID));
		removeEvent = true;
		handleFreeEvent(dynamic_cast<ArielFreeEvent*>(nextEvent));
		break;

	case MALLOC:
		ARIEL_CORE_VERBOSE(8, output->verbose(CALL_INFO, 8, 0, "Core %" PRIu32 " next event is MALLOC\n", coreID));
		removeEvent = true;
		handleAllocationEvent(dynamic_cast<ArielAllocateEvent*>(nextEvent));
		break;

	case CORE_EXIT:
		ARIEL_CORE_VERBOSE(8, output->verbose(CALL_INFO, 8, 0, "Core %" PRIu32 " next event is CORE_EXIT\n", coreID));
		isHalted = true;
		std::cout << "CORE ID: " << coreID << " PROCESSED AN EXIT EVENT" << std::endl;
		output->verbose(CALL_INFO, 2, 0, "Core %" PRIu32 " has called exit.\n", coreID);
		return true;

	default:
		output->fatal(CALL_INFO, -4, "Unknown event type has arrived on core %" PRIu32 "\n", coreID);
		break;
	}

	// If the event has actually been processed this cycle then remove it from the queue
	if(removeEvent) {
		ARIEL_CORE_VERBOSE(8, output->verbose(CALL_INFO, 8, 0, "Removing event from pending queue, there are %" PRIu32 " events in the queue before deletion.\n", 
			(uint32_t) coreQ->size()));
		coreQ->pop();

		delete nextEvent;
		return true;
	} else {
		ARIEL_CORE_VERBOSE(8, output->verbose(CALL_INFO, 8, 0, "Event removal was not requested, pending transaction queue length=%" PRIu32 ", maximum transactions: %" PRIu32 "\n",
			(uint32_t)pendingTransactions->size(), maxPendingTransactions));
		return false;
	}
}

void ArielCore::tick() {
	if(! isHalted) {
		ARIEL_CORE_VERBOSE(16, output->verbose(CALL_INFO, 16, 0, "Ticking core id %" PRIu32 "\n", coreID));
		for(uint32_t i = 0; i < maxIssuePerCycle; ++i) {
			bool didProcess = processNextEvent();

			// If we didnt process anything in the call or we have halted then
			// we stop the ticking and return
			if( (!didProcess) || isHalted) {
				break;
			}
		}

		currentCycles++;
	}
}
