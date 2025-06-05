// Copyright 2009-2025 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2025, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// of the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include <sst_config.h>
#include "arielcore.h"
#include "tb_header.h"
#include <iostream>
#include <exception>
#include <stdexcept>

using namespace SST::ArielComponent;

#define ARIEL_CORE_VERBOSE(LEVEL, OUTPUT) if(verbosity >= (LEVEL)) OUTPUT


ArielCore::ArielCore(ComponentId_t id, ArielTunnel *tunnel,
            uint32_t thisCoreID, uint32_t maxPendTrans,
            Output* out, uint32_t maxIssuePerCyc,
            uint32_t maxQLen, uint64_t cacheLineSz,
            ArielMemoryManager* memMgr, const uint32_t perform_address_checks, Params& params,
            TimeConverter *timeconverter) :
            ComponentExtension(id), output(out), tunnel(tunnel),
            perform_checks(perform_address_checks),
            verbosity(static_cast<uint32_t>(out->getVerboseLevel())),
            timeconverter(timeconverter) {

    // set both counters for flushes to 0
    output->verbose(CALL_INFO, 2, 0, "Creating core with ID %" PRIu32 ", maximum queue length=%" PRIu32 ", max issue is: %" PRIu32 "\n", thisCoreID, maxQLen, maxIssuePerCyc);
    inst_count = 0;
    coreID = thisCoreID;
    maxPendingTransactions = maxPendTrans;
    isHalted = false;
    isStalled = false;
    isFenced = false;
    maxIssuePerCycle = maxIssuePerCyc;
    maxQLength = maxQLen;
    cacheLineSize = cacheLineSz;
    memmgr = memMgr;

    writePayloads = params.find<int>("writepayloadtrace") == 0 ? false : true;
    coreQ = new std::queue<ArielEvent*>();
    pendingTransactions = new std::unordered_map<StandardMem::Request::id_t, RequestInfo>();
    pending_transaction_count = 0;

    char* subID = (char*) malloc(sizeof(char) * 32);
    snprintf(subID, sizeof(char)*32, "%" PRIu32, thisCoreID);

    statReadRequests  = registerStatistic<uint64_t>( "read_requests", subID );
    statWriteRequests = registerStatistic<uint64_t>( "write_requests", subID );
    statReadLatency  = registerStatistic<uint64_t>( "read_latency", subID);
    statWriteLatency = registerStatistic<uint64_t>( "write_latency", subID);
    statReadRequestSizes = registerStatistic<uint64_t>( "read_request_sizes", subID );
    statWriteRequestSizes = registerStatistic<uint64_t>( "write_request_sizes", subID );
    statSplitReadRequests = registerStatistic<uint64_t>( "split_read_requests", subID );
    statSplitWriteRequests = registerStatistic<uint64_t>( "split_write_requests", subID );

    statFlushRequests = registerStatistic<uint64_t>( "flush_requests", subID);
    statFenceRequests = registerStatistic<uint64_t>( "fence_requests", subID);
    statNoopCount     = registerStatistic<uint64_t>( "no_ops", subID );
    statInstructionCount = registerStatistic<uint64_t>( "instruction_count", subID );
    statCycles = registerStatistic<uint64_t>( "cycles", subID );
    statActiveCycles = registerStatistic<uint64_t>( "active_cycles", subID );

    statFPSPIns = registerStatistic<uint64_t>("fp_sp_ins", subID);
    statFPDPIns = registerStatistic<uint64_t>("fp_dp_ins", subID);

    statFPSPSIMDIns = registerStatistic<uint64_t>("fp_sp_simd_ins", subID);
    statFPDPSIMDIns = registerStatistic<uint64_t>("fp_dp_simd_ins", subID);

    statFPSPScalarIns = registerStatistic<uint64_t>("fp_sp_scalar_ins", subID);
    statFPDPScalarIns = registerStatistic<uint64_t>("fp_dp_scalar_ins", subID);

    statFPSPOps = registerStatistic<uint64_t>("fp_sp_ops", subID);
    statFPDPOps = registerStatistic<uint64_t>("fp_dp_ops", subID);


    free(subID);

    memmgr->registerInterruptHandler(coreID, new ArielMemoryManager::InterruptHandler<ArielCore>(this, &ArielCore::handleInterrupt));
    stdMemHandlers = new StdMemHandler(this, output);

    std::string traceGenName = params.find<std::string>("tracegen", "");
    enableTracing = ("" != traceGenName);

    // If we enabled tracing then open up the correct file.
    if(enableTracing) {
        Params interfaceParams = params.get_scoped_params("tracer");
        traceGen = loadModule<ArielTraceGenerator>(traceGenName, interfaceParams);

        if(NULL == traceGen) {
            output->fatal(CALL_INFO, -1, "Unable to load tracing module: \"%s\"\n",
                    traceGenName.c_str());
        }

        traceGen->setCoreID(coreID);
    }

    currentCycles = 0;
}

ArielCore::~ArielCore() {
    if(NULL != cacheLink) {
        delete cacheLink;
    }

    if(enableTracing && traceGen) {
        delete traceGen;
    }

    delete stdMemHandlers;
}

void ArielCore::setCacheLink(StandardMem* newLink) {
    cacheLink = newLink;
}

void ArielCore::setRtlLink(Link* rtllink) {

    RtlLink = rtllink;
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
        StandardMem::Read *req = new StandardMem::Read(address, length, 0, virtAddress);

        pending_transaction_count++;
        pendingTransactions->insert( std::pair<StandardMem::Request::id_t, RequestInfo>(req->getID(), {req, getCurrentSimTime(timeconverter)}) );

        if(enableTracing) {
            printTraceEntry(true, (const uint64_t) req->pAddr, (const uint32_t) length);
        }

        // Actually send the event to the cache
        cacheLink->send(req);
    }
}

void ArielCore::commitWriteEvent(const uint64_t address,
        const uint64_t virtAddress, const uint32_t length, const uint8_t* payload) {

    if(length > 0) {
        std::vector<uint8_t> data;

        if( writePayloads ) {
            data.insert(data.end(), &payload[0], &payload[length]);

            if(verbosity >= 16) {
                char* buffer = new char[64];
                std::string payloadString = "";
                for(int i = 0; i < length; ++i) {
                    snprintf(buffer, 64, "0x%X ", payload[i]);
                    payloadString.append(buffer);
                }

                delete[] buffer;

                output->verbose(CALL_INFO, 16, 0, "Write-Payload: Len=%" PRIu32 ", Data={ %s } %" PRIx64 "\n",
                        length, payloadString.c_str(), virtAddress);
            }
        } else {
            data.resize(length, 0);
        }

        StandardMem::Write *req = new StandardMem::Write(address, length, data, false, 0, virtAddress);

        pending_transaction_count++;
        pendingTransactions->insert( std::pair<StandardMem::Request::id_t, RequestInfo>(req->getID(), {req, getCurrentSimTime(timeconverter)}) );

        if(enableTracing) {
            printTraceEntry(false, (const uint64_t) req->pAddr, (const uint32_t) length);
        }

        // Actually send the event to the cache
        cacheLink->send(req);
    }
}

// Although flushes are not technically memory transaction events like writing and reading, they are treated
// as such for the purposes of determining the number of pending transactions in the queue. Thus, this function
// is referred to as a commit

void ArielCore::commitFlushEvent(const uint64_t address,
            const uint64_t virtAddress, const uint32_t length) {

    if(length > 0) {
        /*  Todo: should the request specify the physical address, or the virtual address? */
        StandardMem::Request *req = new StandardMem::FlushAddr( address, length, true, std::numeric_limits<uint32_t>::max());
        pending_transaction_count++;
        pendingTransactions->insert( std::pair<StandardMem::Request::id_t, RequestInfo>(req->getID(), {req, getCurrentSimTime(timeconverter)}) );

        cacheLink->send(req);
        statFlushRequests->addData(1);
    }
}

void ArielCore::handleEvent(StandardMem::Request* event) {
    ARIEL_CORE_VERBOSE(4, output->verbose(CALL_INFO, 4, 0, "Core %" PRIu32 " handling a memory event.\n", coreID));
    StandardMem::Request::id_t mev_id = event->getID();
    auto find_entry = pendingTransactions->find(mev_id);

    if(find_entry != pendingTransactions->end()) {
        if (dynamic_cast<StandardMem::ReadResp*>(event)) {
            statReadLatency->addData(getCurrentSimTime(timeconverter) - find_entry->second.start);
        } else if (dynamic_cast<StandardMem::WriteResp*>(event)) {
            statWriteLatency->addData(getCurrentSimTime(timeconverter) - find_entry->second.start);
        }
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

bool ArielCore::handleInterrupt(ArielMemoryManager::InterruptAction action) {

    ARIEL_CORE_VERBOSE(4, output->verbose(CALL_INFO, 4, 0, "Core %" PRIu32 " received an interrupt.\n", coreID));

    switch (action) {
        case ArielMemoryManager::InterruptAction::STALL:
            isStalled = true;
            break;
        case ArielMemoryManager::InterruptAction::UNSTALL:
            isStalled = false;
            break;
        default:
            output->fatal(CALL_INFO, -4, "Received an unknown interrupt on core %" PRIu32 "\n", coreID);
    }
    return true; // Able to handle
}

void ArielCore::finishCore() {
    // Close the trace file if we did in fact open it.
    if(enableTracing && traceGen) {
        delete traceGen;
        traceGen = NULL;
    }
}

void ArielCore::halt(){
    isHalted = true;
}

void ArielCore::stall(){
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

void ArielCore::createRtlEvent(void* inp_ptr, void* ctrl_ptr, void* updated_params, size_t inp_size, size_t ctrl_size, size_t updated_rtl_params_size) {
    ArielRtlEvent* Ev = new ArielRtlEvent();
    Ev->set_rtl_inp_ptr(inp_ptr);
    Ev->set_rtl_ctrl_ptr(ctrl_ptr);
    Ev->set_updated_rtl_params(updated_params);
    Ev->set_rtl_inp_size(inp_size);
    Ev->set_rtl_ctrl_size(ctrl_size);
    Ev->set_updated_rtl_params_size(updated_rtl_params_size);
    coreQ->push(Ev);

    ARIEL_CORE_VERBOSE(4, output->verbose(CALL_INFO, 4, 0, "Generated a RTL event.\n"));
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
                fprintf(stdout, "Performing statistics output at simulation time = %" PRIu64 " cycles\n", getCurrentSimTimeNano());
                performGlobalStatisticOutput();
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

            case ARIEL_ISSUE_RTL:
                createRtlEvent(ac.shmem.inp_ptr, ac.shmem.ctrl_ptr, ac.shmem.updated_rtl_params, ac.shmem.inp_size, ac.shmem.ctrl_size, ac.shmem.updated_rtl_params_size);
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
    ARIEL_CORE_VERBOSE(4, output->verbose(CALL_INFO, 4, 0, "Core %" PRIu32 " processing a free event (for virtual address=%" PRIu64 ")\n", coreID, rFE->getVirtualAddress()));

    memmgr->freeMalloc(rFE->getVirtualAddress());
}

void ArielCore::handleReadRequest(ArielReadEvent* rEv) {
    ARIEL_CORE_VERBOSE(4, output->verbose(CALL_INFO, 4, 0, "Core %" PRIu32 " processing a read event...\n", coreID));

    const uint64_t readAddress = rEv->getAddress();
    const uint64_t readLength  = std::min((uint64_t) rEv->getLength(), cacheLineSize); // Trim to cacheline size (occurs rarely for instructions such as xsave and fxsave)

    /* No longer neccessary due to trimming above
     * if(readLength > cacheLineSize) {
        output->verbose(CALL_INFO, 4, 0, "Potential error? request for a read of length=%" PRIu64 " is larger than cache line which is not allowed (coreID=%" PRIu32 ", cache line: %" PRIu64 "\n",
                    readLength, coreID, cacheLineSize);
        return;
    }*/

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
    const uint64_t writeLength  = std::min((uint64_t) wEv->getLength(), cacheLineSize); // Trim to cacheline size (occurs rarely for instructions such as xsave and fxsave)

    // No longer neccessary due to trimming above
/*    if(writeLength > cacheLineSize) {
        output->verbose(CALL_INFO, 4, 0, "Potential error? request for a write of length=%" PRIu64 " is larger than cache line which is not allowed (coreID=%" PRIu32 ", cache line: %" PRIu64 "\n",
                    writeLength, coreID, cacheLineSize);
        return;
    }*/

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
    memmgr->allocateMMAP(aEv->getAllocationLength(), aEv->getAllocationLevel(), aEv->getVirtualAddress(),
            aEv->getInstructionPointer(), aEv->getFileID(), coreID);
}

void ArielCore::handleAllocationEvent(ArielAllocateEvent* aEv) {
    output->verbose(CALL_INFO, 2, 0, "Handling a memory allocation event, vAddr=%" PRIu64 ", length=%" PRIu64 ", at level=%" PRIu32 " with malloc ID=%" PRIu64 "\n",
                aEv->getVirtualAddress(), aEv->getAllocationLength(), aEv->getAllocationLevel(), aEv->getInstructionPointer());

    memmgr->allocateMalloc(aEv->getAllocationLength(), aEv->getAllocationLevel(), aEv->getVirtualAddress(), aEv->getInstructionPointer(), coreID);
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

void ArielCore::handleRtlEvent(ArielRtlEvent* RtlEv) {

    RtlEv->set_cachelinesize(cacheLineSize);
    memmgr->get_page_info(RtlEv->RtlData.pageTable, RtlEv->RtlData.freePages, RtlEv->RtlData.pageSize);
    memmgr->get_tlb_info(RtlEv->RtlData.translationCache, RtlEv->RtlData.translationCacheEntries, RtlEv->RtlData.translationEnabled);
    RtlLink->send(RtlEv);
    return;
}

void ArielCore::handleRtlAckEvent(SST::Event* e) {

    output->verbose(CALL_INFO, 16, 0, "\nAriel received Event from RTL\n");
    ArielRtlEvent* ev = dynamic_cast<ArielRtlEvent*>(e);
    if(ev->getEventRecvAck() == true) {
        output->verbose(CALL_INFO, 16, 0, "\nACK Received from RTL. Inp/Ctrl data signal update successful\n");
        ev->setEventRecvAck(false);
    }

    if(ev->getEndSim() == true) {
        output->verbose(CALL_INFO, 16, 0, "\nRTL simulation Complete\n");
        //==========================Calculate Simulation time===================================//
        //                                  xx.xx ns
        //======================================================================================//

        //===================Print Outputs and other stats stored in Shmem======================//
        //                   get the pointers and print the Output: XXXX
        //======================================================================================//
    }
    if(ev->RtlData.rtl_inp_ptr != nullptr) {
        rtl_inp_ptr = ev->RtlData.rtl_inp_ptr;
        ArielReadEvent *are = new ArielReadEvent((uint64_t)ev->RtlData.rtl_inp_ptr, (uint64_t)ev->RtlData.rtl_inp_size);
        output->verbose(CALL_INFO, 1, 0, "\nAriel received Event from RTL. Generating Read Request\n");
        handleReadRequest(are);
    }

    return;
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

        case RTL:
            ARIEL_CORE_VERBOSE(8, output->verbose(CALL_INFO, 8, 0, "Core %" PRIu32 "next event is RTL (RTLEvent call)\n", coreID));
            output->verbose(CALL_INFO, 1, 0, "\nArielRTLEvent is being issued");
            removeEvent = true;
            handleRtlEvent(dynamic_cast<ArielRtlEvent*>(nextEvent));
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

