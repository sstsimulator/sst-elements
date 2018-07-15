// Copyright 2009-2018 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2018, NTESS
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
#include "arielcore.h"

using namespace SST::OpalComponent;

#define ARIEL_CORE_VERBOSE(LEVEL, OUTPUT) if(verbosity >= (LEVEL)) OUTPUT


ArielCore::ArielCore(ArielTunnel *tunnel, SimpleMem* coreToCacheLink,
            uint32_t thisCoreID, uint32_t maxPendTrans,
            Output* out, uint32_t maxIssuePerCyc,
            uint32_t maxQLen, uint64_t cacheLineSz, SST::Component* own,
            ArielMemoryManager* memMgr, const uint32_t perform_address_checks, Params& params) :
            output(out), tunnel(tunnel), perform_checks(perform_address_checks),
            verbosity(static_cast<uint32_t>(out->getVerboseLevel())) {

    // set both counters for flushes to 0
    output->verbose(CALL_INFO, 2, 0, "Creating core with ID %" PRIu32 ", maximum queue length=%" PRIu32 ", max issue is: %" PRIu32 "\n", thisCoreID, maxQLen, maxIssuePerCyc);
    inst_count = 0;
    cacheLink = coreToCacheLink;
    allocLink = 0;
    coreID = thisCoreID;
    maxPendingTransactions = maxPendTrans;
    isHalted = false;
    isStalled = false;
    isFenced = false;
    maxIssuePerCycle = maxIssuePerCyc;
    maxQLength = maxQLen;
    cacheLineSize = cacheLineSz;
    owner = own;
    memmgr = memMgr;

    opal_enabled = false;
    writePayloads = params.find<int>("writepayloadtrace") == 0 ? false : true;

    coreQ = new std::queue<ArielEvent*>();
    pendingTransactions = new std::unordered_map<SimpleMem::Request::id_t, SimpleMem::Request*>();
    pending_transaction_count = 0;

    char* subID = (char*) malloc(sizeof(char) * 32);
    sprintf(subID, "%" PRIu32, thisCoreID);

    statReadRequests  = own->registerStatistic<uint64_t>( "read_requests", subID );
    statWriteRequests = own->registerStatistic<uint64_t>( "write_requests", subID );
    statReadRequestSizes = own->registerStatistic<uint64_t>( "read_request_sizes", subID );
    statWriteRequestSizes = own->registerStatistic<uint64_t>( "write_request_sizes", subID );
    statSplitReadRequests = own->registerStatistic<uint64_t>( "split_read_requests", subID );
    statSplitWriteRequests = own->registerStatistic<uint64_t>( "split_write_requests", subID );
    statFlushRequests = own->registerStatistic<uint64_t>( "flush_requests", subID);
    statFenceRequests = own->registerStatistic<uint64_t>( "fence_requests", subID);
    statNoopCount     = own->registerStatistic<uint64_t>( "no_ops", subID );
    statInstructionCount = own->registerStatistic<uint64_t>( "instruction_count", subID );
    statCycles = own->registerStatistic<uint64_t>( "cycles", subID );
    statActiveCycles = own->registerStatistic<uint64_t>( "active_cycles", subID );

    statFPSPIns = own->registerStatistic<uint64_t>("fp_sp_ins", subID);
    statFPDPIns = own->registerStatistic<uint64_t>("fp_dp_ins", subID);

    statFPSPSIMDIns = own->registerStatistic<uint64_t>("fp_sp_simd_ins", subID);
    statFPDPSIMDIns = own->registerStatistic<uint64_t>("fp_dp_simd_ins", subID);

    statFPSPScalarIns = own->registerStatistic<uint64_t>("fp_sp_scalar_ins", subID);
    statFPDPScalarIns = own->registerStatistic<uint64_t>("fp_dp_scalar_ins", subID);

    statFPSPOps = own->registerStatistic<uint64_t>("fp_sp_ops", subID);
    statFPDPOps = own->registerStatistic<uint64_t>("fp_dp_ops", subID);

    free(subID);

    std::string traceGenName = params.find<std::string>("tracegen", "");
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
    //delete statReadRequests;
    //delete statWriteRequests;
    //delete statSplitReadRequests;
    //delete statSplitWriteRequests;
    //delete statNoopCount;
    //delete statInstructionCount;

    if(NULL != cacheLink) {
        delete cacheLink;
    }

    if(enableTracing && traceGen) {
        delete traceGen;
    }
}

void ArielCore::setOpalLink(Link * opallink) {

    OpalLink = opallink;

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
                printTraceEntry(true, (const uint64_t) req->addrs[0], (const uint32_t) length);
        }

        // Actually send the event to the cache
        cacheLink->sendRequest(req);
    }
}

void ArielCore::commitWriteEvent(const uint64_t address,
        const uint64_t virtAddress, const uint32_t length, const uint8_t* payload) {

    if(length > 0) {
        SimpleMem::Request *req = new SimpleMem::Request(SimpleMem::Request::Write, address, length);
        req->setVirtualAddress(virtAddress);

        if( writePayloads ) {
            if(verbosity >= 16) {
                char* buffer = new char[64];
                std::string payloadString = "";
    
                for(int i = 0; i < length; ++i) {
                    sprintf(buffer, "0x%X ", payload[i]);
                    payloadString.append(buffer);
                }
    
                delete[] buffer;
                
                output->verbose(CALL_INFO, 16, 0, "Write-Payload: Len=%" PRIu32 ", Data={ %s }\n",
                        length, payloadString.c_str());
            }
            req->setPayload( (uint8_t*) payload, length );
        }

        pending_transaction_count++;
        pendingTransactions->insert( std::pair<SimpleMem::Request::id_t, SimpleMem::Request*>(req->id, req) );

        if(enableTracing) {
            printTraceEntry(false, (const uint64_t) req->addrs[0], (const uint32_t) length);
        }

        // Actually send the event to the cache
        cacheLink->sendRequest(req);
    }
}

// Although flushes are not technically memory transaction events like writing and reading, they are treated
// as such for the purposes of determining the number of pending transactions in the queue. Thus, this function
// is referred to as a commit

void ArielCore::commitFlushEvent(const uint64_t address,
            const uint64_t virtAddress, const uint32_t length) {

    if(length > 0) {
        /*  Todo: should the request specify the physical address, or the virtual address? */
        SimpleMem::Request *req = new SimpleMem::Request(SimpleMem::Request::FlushLineInv, address, length);
        req->addAddress(address);
        pending_transaction_count++;
        pendingTransactions->insert( std::pair<SimpleMem::Request::id_t, SimpleMem::Request*>(req->id, req) );

        cacheLink->sendRequest(req);
        statFlushRequests->addData(1);
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
        if(isCoreFenced() && pending_transaction_count == 0)
                unfence();
    } else {
        output->fatal(CALL_INFO, -4, "Memory event response to core: %" PRIu32 " was not found in pending list.\n", coreID);
    }

    delete event;
}

void ArielCore::ISR_Opal(OpalEvent *ev) {

    ARIEL_CORE_VERBOSE(4, output->verbose(CALL_INFO, 4, 0, "Core %" PRIu32 " handling opal event.\n", coreID));

    switch(ev->getType())
    {
        case SST::OpalComponent::EventType::SHOOTDOWN:
            //maxIssuePerCycle=0; // This will stall the core by not executing anything
            isStalled = true;
            break;

        case SST::OpalComponent::EventType::SDACK:
            //maxIssuePerCycle=storeMaxIssuePerCycle; // restore necessary information
            isStalled = false;
            break;

        default:
            output->fatal(CALL_INFO, -4, "Opal event response to core: %" PRIu32 " was not valid.\n", coreID);
    }

}

void ArielCore::handleInterruptEvent(SST::Event *event) {

    ARIEL_CORE_VERBOSE(4, output->verbose(CALL_INFO, 4, 0, "Core %" PRIu32 " is interrupted.\n", coreID));

    // for now only Opal can interrupt
    OpalEvent * ev =  dynamic_cast<OpalComponent::OpalEvent*> (event);
    ISR_Opal(ev);
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

void ArielCore::stall() {
    isStalled = true;
}

void ArielCore::fence(){
    ARIEL_CORE_VERBOSE(4, output->verbose(CALL_INFO, 4, 0, "Core: %" PRIu32 " FENCE:  Current pending transaction count: %" PRIu32 " (%" PRIu32 ")\n", coreID, pending_transaction_count, maxPendingTransactions));

    if( pending_transaction_count > 0 ) {
        isFenced = true;
        isStalled = true;
    }
}

void ArielCore::unfence()
{
    ARIEL_CORE_VERBOSE(4, output->verbose(CALL_INFO, 4, 0, "Core: %" PRIu32 " UNFENCE:  Current pending transaction count: %" PRIu32 " (%" PRIu32 ")\n", coreID, pending_transaction_count, maxPendingTransactions));
    isFenced = false;
    isStalled = false;
    /* Todo:   Register statistics  */
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

void ArielCore::createMmapEvent(uint32_t fileID, uint64_t vAddr, uint64_t length, uint32_t level, uint64_t instPtr) {
    ArielMmapEvent* ev = new ArielMmapEvent(fileID, vAddr, length, level, instPtr);
    coreQ->push(ev);

    ARIEL_CORE_VERBOSE(2, output->verbose(CALL_INFO, 2, 0, "Generated an mmap event, vAddr(map)=%" PRIu64 ", length=%" PRIu64 " in level %" PRIu32 " from IP %" PRIx64 "\n",
                    vAddr, length, level, instPtr));
}

void ArielCore::createFreeEvent(uint64_t vAddr) {
    ArielFreeEvent* ev = new ArielFreeEvent(vAddr);
    coreQ->push(ev);

    ARIEL_CORE_VERBOSE(2, output->verbose(CALL_INFO, 2, 0, "Generated a free event for virtual address=%" PRIu64 "\n", vAddr));
}

void ArielCore::createWriteEvent(uint64_t address, uint32_t length, const uint8_t* payload) {
    ArielWriteEvent* ev = new ArielWriteEvent(address, length, payload);
    coreQ->push(ev);

    ARIEL_CORE_VERBOSE(4, output->verbose(CALL_INFO, 4, 0, "Generated a WRITE event, addr=%" PRIu64 ", length=%" PRIu32 "\n", address, length));
}

void ArielCore::createFlushEvent(uint64_t vAddr){
    ArielFlushEvent *ev = new ArielFlushEvent(vAddr, cacheLineSize);
    coreQ->push(ev);

    ARIEL_CORE_VERBOSE(4, output->verbose(CALL_INFO,4,0, "Generated a FLUSH event.\n"));
}

void ArielCore::createFenceEvent(){
    ArielFenceEvent *ev = new ArielFenceEvent();
    coreQ->push(ev);

    ARIEL_CORE_VERBOSE(4, output->verbose(CALL_INFO, 4, 0, "Generated a FENCE event.\n"));
}

void ArielCore::createExitEvent() {
    ArielExitEvent* xEv = new ArielExitEvent();
    coreQ->push(xEv);

    ARIEL_CORE_VERBOSE(4, output->verbose(CALL_INFO, 4, 0, "Generated an EXIT event.\n"));
}

bool ArielCore::isCoreHalted() const {
    return isHalted;
}

bool ArielCore::isCoreStalled() const {
    return isStalled;
}

bool ArielCore::isCoreFenced() const {
    // returns true iff isFenced is true
    return isFenced;
}

bool ArielCore::hasDrainCompleted() const {
    // Returns true iff pending_transaction_count is 0
    if(pending_transaction_count == 0)
        return true;
    else
        return false;
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
                                    createWriteEvent(ac.inst.addr, ac.inst.size, &ac.inst.payload[0]);
                                    break;

                            case ARIEL_END_INSTRUCTION:
                                    break;

                            default:
                                    // Not sure what this is
                                    output->fatal(CALL_INFO, -1, "Error: Ariel did not understand command (%d) provided during instruction queue refill.\n", (int)(ac.command));
                                    break;
                        }
                }

                // Add one to our instruction counts
                //statInstructionCount->addData(1);

                break;

            case ARIEL_NOOP:
                createNoOpEvent();
                break;

            case ARIEL_FLUSHLINE_INSTRUCTION:
                createFlushEvent(ac.flushline.vaddr);
                break;

            case ARIEL_FENCE_INSTRUCTION:
                createFenceEvent();
                break;

            case ARIEL_ISSUE_TLM_MMAP:
                createMmapEvent(ac.mlm_mmap.fileID, ac.mlm_mmap.vaddr, ac.mlm_mmap.alloc_len, ac.mlm_mmap.alloc_level, ac.instPtr);
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
                output->fatal(CALL_INFO, -1, "Error: Ariel did not understand command (%d) provided during instruction queue refill.\n", (int)(ac.command));
                break;
        }
    }

    ARIEL_CORE_VERBOSE(16, output->verbose(CALL_INFO, 16, 0, "Refilling event queue for core %" PRIu32 " is complete\n", coreID));
    return true;
}

void ArielCore::handleFreeEvent(ArielFreeEvent* rFE) {
    output->verbose(CALL_INFO, 4, 0, "Core %" PRIu32 " processing a free event (for virtual address=%" PRIu64 ")\n", coreID, rFE->getVirtualAddress());

    memmgr->freeMalloc(rFE->getVirtualAddress());

    if (allocLink) {
        // tell the allocate montior (e.g. mem sieve that a free has occured)
        arielAllocTrackEvent *e =  new arielAllocTrackEvent(arielAllocTrackEvent::FREE,
                            rFE->getVirtualAddress(), 0, 0, 0);

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

    // NOTE: Physical and virtual addresses may not be aligned the same w.r.t. line size if map-on-malloc is being used (arielinterceptcalls != 0), so use physical offsets to determine line splits
    // There is a chance that the non-alignment causes an undetected bug if an access spans multiple malloc regions that are contiguous in VA space but non-contiguous in PA space.
    // However, a single access spanning multiple malloc'd regions shouldn't happen...
    // Addresses mapped via first touch are always line/page aligned
    const uint64_t physAddr = memmgr->translateAddress(readAddress);
    const uint64_t addr_offset  = physAddr % ((uint64_t) cacheLineSize);

    if((addr_offset + readLength) <= cacheLineSize) {
        ARIEL_CORE_VERBOSE(4, output->verbose(CALL_INFO, 4, 0, "Core %" PRIu32 " generating a non-split read request: Addr=%" PRIu64 " Length=%" PRIu64 "\n",
                            coreID, readAddress, readLength));

        // We do not need to perform a split operation

        ARIEL_CORE_VERBOSE(4, output->verbose(CALL_INFO, 4, 0, "Core %" PRIu32 " issuing read, VAddr=%" PRIu64 ", Size=%" PRIu64 ", PhysAddr=%" PRIu64 "\n",
                            coreID, readAddress, readLength, physAddr));

        commitReadEvent(physAddr, readAddress, (uint32_t) readLength);
    } else {
        ARIEL_CORE_VERBOSE(4, output->verbose(CALL_INFO, 4, 0, "Core %" PRIu32 " generating a split read request: Addr=%" PRIu64 " Length=%" PRIu64 "\n",
                            coreID, readAddress, readLength));

        // We need to perform a split operation
        const uint64_t leftAddr = readAddress;
        const uint64_t leftSize = cacheLineSize - addr_offset;

        const uint64_t rightAddr = (readAddress + ((uint64_t) cacheLineSize)) - addr_offset;
        const uint64_t rightSize = readLength - leftSize;

        const uint64_t physLeftAddr = physAddr;
        const uint64_t physRightAddr = memmgr->translateAddress(rightAddr);

        ARIEL_CORE_VERBOSE(4, output->verbose(CALL_INFO, 4, 0, "Core %" PRIu32 " issuing split-address read, LeftVAddr=%" PRIu64 ", RightVAddr=%" PRIu64 ", LeftSize=%" PRIu64 ", RightSize=%" PRIu64 ", LeftPhysAddr=%" PRIu64 ", RightPhysAddr=%" PRIu64 "\n",
                            coreID, leftAddr, rightAddr, leftSize, rightSize, physLeftAddr, physRightAddr));

        if(perform_checks > 0) {
        /*  if( (leftSize + rightSize) != readLength ) {
                    output->fatal(CALL_INFO, -4, "Core %" PRIu32 " read request for address %" PRIu64 ", length=%" PRIu64 ", split into left address=%" PRIu64 ", left size=%" PRIu64 ", right address=%" PRIu64 ", right size=%" PRIu64 " does not equal read length (cache line of length %" PRIu64 ")\n",
                                coreID, readAddress, readLength, leftAddr, leftSize, rightAddr, rightSize, cacheLineSize);
                }*/

                if( ((physLeftAddr + leftSize) % cacheLineSize) != 0) {
                    output->fatal(CALL_INFO, -4, "Error leftAddr=%" PRIu64 " + size=%" PRIu64 " is not a multiple of cache line size: %" PRIu64 "\n",
                                leftAddr, leftSize, cacheLineSize);
                }

                /*if( ((physRightAddr + rightSize) % cacheLineSize) > cacheLineSize ) {
                    output->fatal(CALL_INFO, -4, "Error rightAddr=%" PRIu64 " + size=%" PRIu64 " is not a multiple of cache line size: %" PRIu64 "\n",
                                leftAddr, leftSize, cacheLineSize);
                }*/
        }

        commitReadEvent(physLeftAddr, leftAddr, (uint32_t) leftSize);
        commitReadEvent(physRightAddr, rightAddr, (uint32_t) rightSize);

        statSplitReadRequests->addData(1);
    }

    statReadRequests->addData(1);
    statReadRequestSizes->addData(readLength);
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

    // See note in handleReadRequest() on alignment issues
    const uint64_t physAddr = memmgr->translateAddress(writeAddress);
    const uint64_t addr_offset  = physAddr % ((uint64_t) cacheLineSize);

    // We do not need to perform a split operation
    if((addr_offset + writeLength) <= cacheLineSize) {
        ARIEL_CORE_VERBOSE(4, output->verbose(CALL_INFO, 4, 0, "Core %" PRIu32 " generating a non-split write request: Addr=%" PRIu64 " Length=%" PRIu64 "\n",
                            coreID, writeAddress, writeLength));


        ARIEL_CORE_VERBOSE(4, output->verbose(CALL_INFO, 4, 0, "Core %" PRIu32 " issuing write, VAddr=%" PRIu64 ", Size=%" PRIu64 ", PhysAddr=%" PRIu64 "\n",
                            coreID, writeAddress, writeLength, physAddr));

        if( writePayloads ) {
            uint8_t* payloadPtr = wEv->getPayload();
            commitWriteEvent(physAddr, writeAddress, (uint32_t) writeLength, payloadPtr);
        } else {
            commitWriteEvent(physAddr, writeAddress, (uint32_t) writeLength, NULL);
        }
    } else {
        ARIEL_CORE_VERBOSE(4, output->verbose(CALL_INFO, 4, 0, "Core %" PRIu32 " generating a split write request: Addr=%" PRIu64 " Length=%" PRIu64 "\n",
                            coreID, writeAddress, writeLength));

        // We need to perform a split operation
        const uint64_t leftAddr = writeAddress;
        const uint64_t leftSize = cacheLineSize - addr_offset;

        const uint64_t rightAddr = (writeAddress + ((uint64_t) cacheLineSize)) - addr_offset;
        const uint64_t rightSize = writeLength - leftSize;

        const uint64_t physLeftAddr = physAddr;
        const uint64_t physRightAddr = memmgr->translateAddress(rightAddr);

        ARIEL_CORE_VERBOSE(4, output->verbose(CALL_INFO, 4, 0, "Core %" PRIu32 " issuing split-address write, LeftVAddr=%" PRIu64 ", RightVAddr=%" PRIu64 ", LeftSize=%" PRIu64 ", RightSize=%" PRIu64 ", LeftPhysAddr=%" PRIu64 ", RightPhysAddr=%" PRIu64 "\n",
                            coreID, leftAddr, rightAddr, leftSize, rightSize, physLeftAddr, physRightAddr));

        if(perform_checks > 0) {
            /* if( (leftSize + rightSize) != writeLength ) {
                output->fatal(CALL_INFO, -4, "Core %" PRIu32 " write request for address %" PRIu64 ", length=%" PRIu64 ", split into left address=%" PRIu64 ", left size=%" PRIu64 ", right address=%" PRIu64 ", right size=%" PRIu64 " does not equal write length (cache line of length %" PRIu64 ")\n",
                            coreID, writeAddress, writeLength, leftAddr, leftSize, rightAddr, rightSize, cacheLineSize);
            }*/

            if( ((physLeftAddr + leftSize) % cacheLineSize) != 0) {
                output->fatal(CALL_INFO, -4, "Error leftAddr=%" PRIu64 " + size=%" PRIu64 " is not a multiple of cache line size: %" PRIu64 "\n",
                            leftAddr, leftSize, cacheLineSize);
            }

            /*if( ((rightAddr + rightSize) % cacheLineSize) > cacheLineSize ) {
                output->fatal(CALL_INFO, -4, "Error rightAddr=%" PRIu64 " + size=%" PRIu64 " is not a multiple of cache line size: %" PRIu64 "\n",
                            leftAddr, leftSize, cacheLineSize);
            }*/
        }

        if( writePayloads ) {
            uint8_t* payloadPtr = wEv->getPayload();
            
            commitWriteEvent(physLeftAddr, leftAddr, (uint32_t) leftSize, payloadPtr);
            commitWriteEvent(physRightAddr, rightAddr, (uint32_t) rightSize, &payloadPtr[leftSize]);
        } else {
            commitWriteEvent(physLeftAddr, leftAddr, (uint32_t) leftSize, NULL);
            commitWriteEvent(physRightAddr, rightAddr, (uint32_t) rightSize, NULL);
        }
        statSplitWriteRequests->addData(1);
    }

    statWriteRequests->addData(1);
    statWriteRequestSizes->addData(writeLength);
}



void ArielCore::handleMmapEvent(ArielMmapEvent* aEv) {

    if(opal_enabled) {
        OpalEvent * tse = new OpalEvent(OpalComponent::EventType::MMAP);
        tse->setHint(aEv->getAllocationLevel());
        tse->setFileId(aEv->getFileID());
        std::cout<<"Before sending to Opal.. file ID is : "<<tse->getFileId()<<std::endl;
        // length should be in multiple of page size
        tse->setResp(aEv->getVirtualAddress(), 0, aEv->getAllocationLength() );
        OpalLink->send(tse);
    }

}

void ArielCore::handleAllocationEvent(ArielAllocateEvent* aEv) {
    output->verbose(CALL_INFO, 2, 0, "Handling a memory allocation event, vAddr=%" PRIu64 ", length=%" PRIu64 ", at level=%" PRIu32 " with malloc ID=%" PRIu64 "\n",
                aEv->getVirtualAddress(), aEv->getAllocationLength(), aEv->getAllocationLevel(), aEv->getInstructionPointer());

    // If Opal is enabled, make sure you pass these requests to it
    if(opal_enabled) {
        OpalEvent * tse = new OpalEvent(OpalComponent::EventType::HINT);
        tse->setHint(aEv->getAllocationLevel());
        tse->setResp(aEv->getVirtualAddress(), 0, aEv->getAllocationLength() );
        OpalLink->send(tse);
    }

    if (allocLink) {
        output->verbose(CALL_INFO, 2, 0, " Sending memory allocation event to allocate monitor\n");
        // tell the allocate montior (e.g. mem sieve that an
        // allocation has occured)
        arielAllocTrackEvent *e = new arielAllocTrackEvent(arielAllocTrackEvent::ALLOC,
                            aEv->getVirtualAddress(),
                            aEv->getAllocationLength(),
                            aEv->getAllocationLevel(),
                            aEv->getInstructionPointer());
        allocLink->send(e);
    } else {    // As a config convience, we're not supporting allocLink + allocate-on-malloc but there's no real reason not to
        memmgr->allocateMalloc(aEv->getAllocationLength(), aEv->getAllocationLevel(), aEv->getVirtualAddress());
    }
}

void ArielCore::handleFlushEvent(ArielFlushEvent *flEv) {
    const uint64_t virtualAddress = (uint64_t) flEv->getVirtualAddress();
    const uint64_t readLength = (uint64_t) flEv->getLength();

    const uint64_t physAddr = memmgr->translateAddress(virtualAddress);
    commitFlushEvent(physAddr, virtualAddress, (uint32_t) readLength);
}

void ArielCore::handleFenceEvent(ArielFenceEvent *fEv) {
    /*  Todo: Should we treat this like the Flush event, and require that the Fence
    *  be put into a transaction queue?  */
    // Possibility A:
    fence();
    // Possibility B:
    // commitFenceEvent();
    statFenceRequests->addData(1);
}

void ArielCore::printCoreStatistics() {
}

bool ArielCore::processNextEvent() {

    // Upon every call, check if the core is drained and we are fenced. If so, unfence
    // return true; /* Todo: reevaluate if this is needed */

    // Attempt to refill the queue
    if(coreQ->empty()) {
        bool addedItems = refillQueue();

        ARIEL_CORE_VERBOSE(16, output->verbose(CALL_INFO, 16, 0, "Attempted a queue fill, %s data\n",
                            (addedItems ? "added" : "did not add")));

        if(! addedItems) {
                return false;
        }
    }

    updateCycle = true;

    ARIEL_CORE_VERBOSE(8, output->verbose(CALL_INFO, 8, 0, "Processing next event in core %" PRIu32 "...\n", coreID));

    ArielEvent* nextEvent = coreQ->front();
    bool removeEvent = false;

    switch(nextEvent->getEventType()) {
        case NOOP:
                ARIEL_CORE_VERBOSE(8, output->verbose(CALL_INFO, 8, 0, "Core %" PRIu32 " next event is NOOP\n", coreID));
                statInstructionCount->addData(1);
                inst_count++;
                statNoopCount->addData(1);
                removeEvent = true;
                break;

        case READ_ADDRESS:
                ARIEL_CORE_VERBOSE(8, output->verbose(CALL_INFO, 8, 0, "Core %" PRIu32 " next event is READ_ADDRESS\n", coreID));

                //  if(pendingTransactions->size() < maxPendingTransactions) {
                if(pending_transaction_count < maxPendingTransactions) {
                    ARIEL_CORE_VERBOSE(16, output->verbose(CALL_INFO, 16, 0, "Found a read event, fewer pending transactions than permitted so will process...\n"));
                    statInstructionCount->addData(1);
                    inst_count++;
                    removeEvent = true;
                    handleReadRequest(dynamic_cast<ArielReadEvent*>(nextEvent));
                } else {
                    ARIEL_CORE_VERBOSE(16, output->verbose(CALL_INFO, 16, 0, "Pending transaction queue is currently full for core %" PRIu32 ", core will stall for new events\n", coreID));
                    break;
                }
                break;

        case WRITE_ADDRESS:
                ARIEL_CORE_VERBOSE(8, output->verbose(CALL_INFO, 8, 0, "Core %" PRIu32 " next event is WRITE_ADDRESS\n", coreID));

                //  if(pendingTransactions->size() < maxPendingTransactions) {
                if(pending_transaction_count < maxPendingTransactions) {
                    ARIEL_CORE_VERBOSE(16, output->verbose(CALL_INFO, 16, 0, "Found a write event, fewer pending transactions than permitted so will process...\n"));
                    statInstructionCount->addData(1);
                    inst_count++;
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
                ARIEL_CORE_VERBOSE(8, output->verbose(CALL_INFO, 8, 0, "Core %" PRIu32 " next event is a SWITCH_POOL\n", coreID));
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

        case MMAP:
                ARIEL_CORE_VERBOSE(8, output->verbose(CALL_INFO, 8, 0, "Core %" PRIu32 " next event is MMAP\n", coreID));
                removeEvent = true;
                handleMmapEvent(dynamic_cast<ArielMmapEvent*>(nextEvent));
                break;

        case CORE_EXIT:
                ARIEL_CORE_VERBOSE(8, output->verbose(CALL_INFO, 8, 0, "Core %" PRIu32 " next event is CORE_EXIT\n", coreID));
                isHalted = true;
                std::cout << "CORE ID: " << coreID << " PROCESSED AN EXIT EVENT" << std::endl;
                output->verbose(CALL_INFO, 2, 0, "Core %" PRIu32 " has called exit.\n", coreID);
                return true;

        case FLUSH:
                ARIEL_CORE_VERBOSE(8, output->verbose(CALL_INFO, 8, 0, "Core %" PRIu32 " next event is a FLUSH\n", coreID));
                if(pending_transaction_count < maxPendingTransactions) {
                    ARIEL_CORE_VERBOSE(16, output->verbose(CALL_INFO, 16, 0, "Found a FLUSH event, fewer pending transactions than permitted so will process..\n"));
                    statInstructionCount->addData(1);
                    inst_count++;
                    handleFlushEvent(dynamic_cast<ArielFlushEvent*>(nextEvent));
                    removeEvent = true;
                } else {
                    ARIEL_CORE_VERBOSE(16, output->verbose(CALL_INFO, 16, 0, "Pending transaction queue is currently full for core %" PRIu32 ",core will stall for new events\n", coreID));
                }
                break;

        case FENCE:
                ARIEL_CORE_VERBOSE(8, output->verbose(CALL_INFO, 8, 0, "Core %" PRIu32 " next event is a FENCE\n", coreID));
                if(!isCoreFenced()) {// If core is fenced, drop this fence - they can be merged
                    handleFenceEvent(dynamic_cast<ArielFenceEvent*>(nextEvent));
                }
                removeEvent = true;
                break;

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

// Just to mark the starting of the simulation
bool started=false;

void ArielCore::tick() {
    // todo: if the core is fenced, increment the current cycle counter

    if(!isHalted) {
        ARIEL_CORE_VERBOSE(16, output->verbose(CALL_INFO, 16, 0, "Ticking core id %" PRIu32 "\n", coreID));
        updateCycle = false;

        if(!isStalled) {
                for(uint32_t i = 0; i < maxIssuePerCycle; ++i) {
                    bool didProcess = processNextEvent();

                    // If we didnt process anything in the call or we have halted then
                    // we stop the ticking and return
                    if( (!didProcess) || isHalted || isStalled ) {
                            break;
                    }

                    if(didProcess)
                            started = true;
                }
        }

        currentCycles++;
        statCycles->addData(1);

        if( updateCycle ) {
                statActiveCycles->addData(1);
        }
    }

    if(inst_count >= max_insts && (max_insts!=0) && (coreID==0))
        isHalted=true;
}

