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
#include "arielcore.h"

#ifdef HAVE_CUDA
#include <../balar/balar_event.h>
using namespace SST::BalarComponent;
#endif

using namespace SST::ArielComponent;

#define ARIEL_CORE_VERBOSE(LEVEL, OUTPUT) if(verbosity >= (LEVEL)) OUTPUT


ArielCore::ArielCore(ComponentId_t id, ArielTunnel *tunnel,
#ifdef HAVE_CUDA
            GpuReturnTunnel *tunnelR, GpuDataTunnel *tunnelD,
#endif
            uint32_t thisCoreID, uint32_t maxPendTrans,
            Output* out, uint32_t maxIssuePerCyc,
            uint32_t maxQLen, uint64_t cacheLineSz,
            ArielMemoryManager* memMgr, const uint32_t perform_address_checks, Params& params) :
            ComponentExtension(id), output(out), tunnel(tunnel),
#ifdef HAVE_CUDA
            tunnelR(tunnelR), tunnelD(tunnelD),
#endif
            perform_checks(perform_address_checks),
            verbosity(static_cast<uint32_t>(out->getVerboseLevel())) {

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
    pendingTransactions = new std::unordered_map<SimpleMem::Request::id_t, SimpleMem::Request*>();
    pending_transaction_count = 0;

#ifdef HAVE_CUDA
    midTransfer = false;
    remainingTransfer = 0;
    remainingPageTransfer = 0;
    pageTransfer = 0;
    ackTransfer = 0;
    pageAckTransfer = 0;
    totalTransfer = 0;

    pendingGpuTransactions = new std::unordered_map<SimpleMem::Request::id_t, SimpleMem::Request*>();
    pending_gpu_transaction_count = 0;
#endif

    char* subID = (char*) malloc(sizeof(char) * 32);
    sprintf(subID, "%" PRIu32, thisCoreID);

    statReadRequests  = registerStatistic<uint64_t>( "read_requests", subID );
    statWriteRequests = registerStatistic<uint64_t>( "write_requests", subID );
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

    std::string traceGenName = params.find<std::string>("tracegen", "");
    enableTracing = ("" != traceGenName);

    // If we enabled tracing then open up the correct file.
    if(enableTracing) {
        Params interfaceParams = params.get_scoped_params("tracer");
        traceGen = dynamic_cast<ArielTraceGenerator*>( loadModule(traceGenName, interfaceParams) );

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
}

void ArielCore::setCacheLink(SimpleMem* newLink) {
    cacheLink = newLink;
}

#ifdef HAVE_CUDA
void ArielCore::setGpuLink(Link* gpulink) {

   GpuLink = gpulink;
}
#endif

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
#ifdef HAVE_CUDA
        if(isGpuEx()){
            pending_transaction_count++;
            pendingGpuTransactions->insert( std::pair<SimpleMem::Request::id_t, SimpleMem::Request*>(req->id, req) );
        }else {
#endif
            pending_transaction_count++;
            pendingTransactions->insert( std::pair<SimpleMem::Request::id_t, SimpleMem::Request*>(req->id, req) );
#ifdef HAVE_CUDA
        }
#endif
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

                output->verbose(CALL_INFO, 16, 0, "Write-Payload: Len=%" PRIu32 ", Data={ %s } %" PRIx64 "\n",
                        length, payloadString.c_str(), virtAddress);
            }
            req->setPayload( (uint8_t*) payload, length );
        }
#ifdef HAVE_CUDA
        if(isGpuEx()){
            pending_transaction_count++;
            pendingGpuTransactions->insert( std::pair<SimpleMem::Request::id_t, SimpleMem::Request*>(req->id, req) );
        } else{
#endif
            pending_transaction_count++;
            pendingTransactions->insert( std::pair<SimpleMem::Request::id_t, SimpleMem::Request*>(req->id, req) );
#ifdef HAVE_CUDA
        }
#endif
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

#ifdef HAVE_CUDA
    if(pendingGpuTransactions->find(mev_id) != pendingGpuTransactions->end()){
        // Update total ACK and Page ACK
        setAckTransfer(getAckTransfer() + event->data.size());
        setPageAckTransfer(getPageAckTransfer() + event->data.size());
        output->verbose(CALL_INFO, 16, 0, "CUDA: Total ACK %" PRIu32 ", Page ACK %" PRIu32"\n",
                        getAckTransfer(), getPageAckTransfer());
        if(getKind() == cudaMemcpyHostToDevice){
            if((getPageAckTransfer() == getPageTransfer())){
                if(getAckTransfer() == getTotalTransfer()){
                    // Received all the data for this cudaMemcpy
                    setBaseAddress(getCurrentAddress() - getTotalTransfer());
                    output->verbose(CALL_INFO, 16, 0, "CUDA: Moving to GPU stage %" PRIu32 "\n",
                                    getBaseAddress());
                    BalarEvent * tse = new BalarEvent(BalarComponent::EventType::REQUEST);
                    tse->API = GPU_MEMCPY;
                    tse->payload = physicalAddresses;
                    tse->CA.cuda_memcpy.dst = getBaseAddress();
                    tse->CA.cuda_memcpy.src = event->addr;
                    tse->CA.cuda_memcpy.count = getTotalTransfer();
                    tse->CA.cuda_memcpy.kind = getKind();

                    // Continue fesimple to wait for ACK from GPU now
                    GpuCommand gc;
                    gc.API_Return.name = GPU_MEMCPY_RET;
                    output->verbose(CALL_INFO, 16, 0, "CUDA: Ariel sent ACK\n");
                    tunnelR->writeMessage(coreID, gc);

                    // Send the message to the GPU to start reading out the data
                    GpuLink->send(tse);

                    // Clear local data
                    physicalAddresses.clear();
                    free(getDataAddress());
                }else {
                    // Received ACK for an entire page, but there is still pending data
                    output->verbose(CALL_INFO, 16, 0, "CUDA: page data:\n");
                    if( verbosity >= 16) {
	                for(int i = 0; i < getPageTransfer(); i++)
                            output->verbose(CALL_INFO, 16, 0, "%" PRIu32 ", ",
                                getDataAddress()[i]);
                        output->verbose(CALL_INFO, 16, 0, "\n");
                    }

                    // Update variables and clear local data we've committed
                    setRemainingTransfer(getRemainingTransfer() - getPageTransfer());
                    setBaseAddress(getCurrentAddress());
                    setPageAckTransfer(0);
                    free(getDataAddress());

                    // Send an ACK to get fesimple to continue
                    uint8_t *data;
                    GpuCommand gc;
                    GpuDataCommand gd;
                    gc.API_Return.name = GPU_MEMCPY_RET;
                    output->verbose(CALL_INFO, 16, 0, "CUDA: Ariel sent ACK\n");
                    tunnelR->writeMessage(coreID, gc);
                    bool avail = false;

                    // Wait for new page from fesimple
                    do {
                        avail = tunnelD->readMessageNB(coreID, &gd);
                    } while(!avail);

                    // Allocate only the amount of data needed
                    size_t current_transfer = gd.count;;
                    data = (uint8_t *) malloc(sizeof(uint8_t) * current_transfer);
                    memcpy(data, gd.page_4k, current_transfer);

                    output->verbose(CALL_INFO, 16, 0, "CUDA: New page data with total size %" PRIu32 ":", current_transfer);
                    if( verbosity >= 16) {
                        for(int i = 0; i < current_transfer; i++)
                            output->verbose(CALL_INFO, 16, 0, "%" PRIu32 ", ",
                                data[i]);
                        output->verbose(CALL_INFO, 16, 0, "\n");
                    }

                    // Updated Page related variables, remove from tunnel, point to new data
                    setPageTransfer(gd.count);
                    setRemainingPageTransfer(gd.count);
                    tunnelD->clearBuffer(coreID);
                    setDataAddress(data);
                }
            }
            // Remove transaction from map
            pendingGpuTransactions->erase(pendingGpuTransactions->find(mev_id));
            pending_transaction_count--;

            // We have pending transactions, so start sending
            if(getRemainingPageTransfer() != 0){
                int index;
                size_t current_transfer;
                while((getOpenTransactions() > 0) && (getRemainingPageTransfer() > 0)){
                    index = getCurrentAddress() - getBaseAddress();
                    current_transfer = (getRemainingPageTransfer() > 64) ? 64 : getRemainingPageTransfer();

                    output->verbose(CALL_INFO, 16, 0, "CUDA: Transfer of  %" PRIu32 " to index %" PRIu64, current_transfer, index);
                    if( verbosity >= 16) {
                        for(int i = 0; i < current_transfer; i++)
                            output->verbose(CALL_INFO, 16, 0, "%" PRIu32 ", ",
                                getDataAddress()[index + i]);
                        output->verbose(CALL_INFO, 16, 0, "\n");
                    }

                    ArielWriteEvent* awe;
                    awe = new ArielWriteEvent(getCurrentAddress(), current_transfer, &getDataAddress()[index]);
                    handleWriteRequest(awe);
                    setCurrentAddress(getCurrentAddress() + current_transfer);
                    setRemainingPageTransfer(getRemainingPageTransfer() - current_transfer);
                }
            }else{
                // Nothing to send, still need more ACKs
            }
        } else if(getKind() == cudaMemcpyDeviceToHost){
            int index = event->getVirtualAddress() - getBaseAddress();
            if((getAckTransfer() == getTotalTransfer())) {
                output->verbose(CALL_INFO, 16, 0, "CUDA: Progress to fesimple (D2H)\n");
                // Add the data at the correct address
                for(int i = 0; i < event->data.size(); i++)
                    getDataAddress()[index+i] = event->data[i];

                output->verbose(CALL_INFO, 16, 0, "CUDA: Data returned to fesimple of size %" PRIu32 ":", getTotalTransfer());
                if( verbosity >= 16) {
                    for(int i = 0; i < getTotalTransfer(); i++)
                        output->verbose(CALL_INFO, 16, 0, "%" PRIu32 ", ",
                            getDataAddress()[i]);
                    output->verbose(CALL_INFO, 16, 0, "\n");
                }

                output->verbose(CALL_INFO, 16, 0, "CUDA: Data returned to fesimple of size %" PRIu32 ":", getTotalTransfer());
                if( verbosity >= 16) {
                    for(int i = 0; i < getTotalTransfer(); i++)
                        output->verbose(CALL_INFO, 16, 0, "%" PRIu32 ", ",
                               getDataAddress()[i]);
                    output->verbose(CALL_INFO, 16, 0, "\n");
                }

                GpuDataCommand gd;
                GpuCommand gc;

                if(getTotalTransfer() <= (1<<12)) {
                    // Small data can be sent back in one chunk
                    memcpy(gd.page_4k, getDataAddress(), getTotalTransfer());
                    tunnelD->writeMessage(coreID, gd);
                    gc.API_Return.name = GPU_MEMCPY_RET;
                    output->verbose(CALL_INFO, 16, 0, "CUDA: Ariel sent ACK\n");
                    tunnelR->writeMessage(coreID, gc);
                } else {
                    // Larger data, and must be sent back in chunks
                    size_t remainder = getTotalTransfer() % (1<<12);
                    size_t pages = getTotalTransfer() - remainder;
                    uint64_t offset = 0;
                    bool avail = false;

                    // Sending if there are full pages or small trailing data
                    while((pages != 0) || (remainder != 0)){
                        if( pages != 0 ) {
                            output->verbose(CALL_INFO, 16, 0, "CUDA: Ariel");
                            if( verbosity >= 16 ) {
                                for(int i = 0; i < (1 << 12); i++)
                                    output->verbose(CALL_INFO, 16, 0, "%" PRIu64 " ",
                                            getDataAddress()[i + offset]);
                                output->verbose(CALL_INFO, 16, 0, "\n");
                            }

                            memcpy(gd.page_4k, &(getDataAddress()[offset]), (1<<12));
                            tunnelD->writeMessage(coreID, gd);
                            pages -= (1<<12);
                            offset += (1<<12);
                        } else {
                            output->verbose(CALL_INFO, 16, 0, "CUDA: Ariel");
                            if( verbosity >= 16 ) {
                                for(int i = 0; i < remainder; i++)
                                    output->verbose(CALL_INFO, 16, 0, "%" PRIu64 " ",
                                            getDataAddress()[i + offset]);
                                output->verbose(CALL_INFO, 16, 0, "\n");
                            }

                            memcpy(gd.page_4k, &(getDataAddress()[offset]), remainder);
                            tunnelD->writeMessage(coreID, gd);
                            remainder = 0;
                        }

                        // Wait for fesimple to ACK it got a page back
                        do {
                            avail = tunnelR->readMessageNB(coreID, &gc);
                        } while (!avail);
                        output->verbose(CALL_INFO, 16, 0, "CUDA: fesimple sent page ACK\n");
                        tunnelR->clearBuffer(coreID);
                        avail = false;
                    }
                    // Sent all data, allow fesimple to proceed
                    //gc.API_Return.name = GPU_MEMCPY_RET;
                    //tunnelR->writeMessage(coreID, gc);
                }
                // Sent all data, allow fesimple to proceed
                gc.API_Return.name = GPU_MEMCPY_RET;
                tunnelR->writeMessage(coreID, gc);

                // Un-stall the GPU, Remove the current transaction
                isStalled = false;
                isGpu = false;
                pendingGpuTransactions->erase(pendingGpuTransactions->find(mev_id));
                pending_transaction_count--;
            }else {
                // Still data left to read
                for(int i = 0; i < event->data.size(); i++){
                    getDataAddress()[index+i] = event->data[i];
                }
                pendingGpuTransactions->erase(pendingGpuTransactions->find(mev_id));
                pending_transaction_count--;
                ArielReadEvent *are;
                while((getOpenTransactions() > 0) && (getRemainingTransfer() > 0)){
                    if(getRemainingTransfer() <= 64) {
                        are = new ArielReadEvent(getCurrentAddress(), getRemainingTransfer());
                        setRemainingTransfer(0);
                    }else {
                        are = new ArielReadEvent(getCurrentAddress(), 64);
                        setRemainingTransfer(getRemainingTransfer()-64);
                        setCurrentAddress(getCurrentAddress() + 64);
                    }
                    handleReadRequest(are);
                }
            }
        }
    }else if(find_entry != pendingTransactions->end()) {
#else
    if(find_entry != pendingTransactions->end()) {
#endif
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

#ifdef HAVE_CUDA
void ArielCore::setKind(cudaMemcpyKind memcpyKind){
    kind = memcpyKind;
}

void ArielCore::gpu(){
    isGpu = true;
}

// Function to generate physical addresses for GPGPU-Sim Component
// Handles split-write/read
void ArielCore::setPhysicalAddresses(SST::Event *ev){
    BalarEvent * gEv =  dynamic_cast<BalarComponent::BalarEvent*> (ev);
    uint64_t phy_addr;
    uint64_t addr_offset;
    uint64_t current_transfer;
    current_transfer = (getRemainingTransfer() > 64) ? 64 : getRemainingTransfer();
    phy_addr = memmgr->translateAddress(getCurrentAddress());
    addr_offset = phy_addr % ((uint64_t) cacheLineSize);
    if((addr_offset + current_transfer <= cacheLineSize)){
        physicalAddresses.push_back(phy_addr);
    } else{
        uint64_t leftAddr = getCurrentAddress();
        uint64_t leftSize = cacheLineSize - addr_offset;
        uint64_t rightAddr = (getCurrentAddress() + ((uint64_t) cacheLineSize)) - addr_offset;
        uint64_t rightSize = current_transfer - leftSize;
        uint64_t physLeftAddr = phy_addr;
        uint64_t physRightAddr = memmgr->translateAddress(rightAddr);
        physicalAddresses.push_back(physLeftAddr);
    }
}


void ArielCore::setMidTransfer(bool midTx){
    midTransfer = midTx;
}

void ArielCore::setRemainingPageTransfer(size_t tx){
    remainingPageTransfer = tx;
}

void ArielCore::setPageTransfer(size_t tx){
    pageTransfer = tx;
}

void ArielCore::setTotalTransfer(size_t tx){
    totalTransfer = tx;
}

void ArielCore::setPageAckTransfer(size_t tx){
    pageAckTransfer = tx;
}

void ArielCore::setAckTransfer(size_t tx){
    ackTransfer = tx;
}

void ArielCore::setRemainingTransfer(size_t tx){
    remainingTransfer = tx;
}

void ArielCore::setBaseAddress(uint64_t virtAddress){
    baseAddress = virtAddress;
}

void ArielCore::setCurrentAddress(uint64_t virtAddress){
    currentAddress = virtAddress;
}

void ArielCore::setDataAddress(uint8_t* virtAddress){
    dataAddress = virtAddress;
}

void ArielCore::setBaseDataAddress(uint8_t* virtAddress){
    baseDataAddress = virtAddress;
}
#endif

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

#ifdef HAVE_CUDA
void ArielCore::createGpuEvent(GpuApi_t API, CudaArguments CA) {
    ArielGpuEvent* gEv = new ArielGpuEvent(API, CA);
    coreQ->push(gEv);

    ARIEL_CORE_VERBOSE(4, output->verbose(CALL_INFO, 4, 0, "Generated a CUDA event.\n"));
}

cudaMemcpyKind ArielCore::getKind() const {
    return kind;
}

bool ArielCore::isGpuEx() const {
    return isGpu;
}

bool ArielCore::getMidTransfer() const {
    return midTransfer;
}

size_t ArielCore::getPageTransfer() const {
    return pageTransfer;
}

size_t ArielCore::getTotalTransfer() const {
    return totalTransfer;
}

size_t ArielCore::getPageAckTransfer() const {
    return pageAckTransfer;
}

size_t ArielCore::getAckTransfer() const {
    return ackTransfer;
}

size_t ArielCore::getRemainingPageTransfer() const {
    return remainingPageTransfer;
}

size_t ArielCore::getRemainingTransfer() const {
    return remainingTransfer;
}

uint64_t ArielCore::getBaseAddress() const {
    return baseAddress;
}

uint64_t ArielCore::getCurrentAddress() const {
    return currentAddress;
}

uint8_t* ArielCore::getDataAddress() const {
    return dataAddress;
}

uint8_t* ArielCore::getBaseDataAddress() const {
    return baseDataAddress;
}

int ArielCore::getOpenTransactions() const {
    return maxPendingTransactions - pendingTransactions->size() - pendingGpuTransactions->size();
}
#endif

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
#ifdef HAVE_CUDA
            case ARIEL_ISSUE_CUDA:
                createGpuEvent(ac.API.name, ac.API.CA);
                break;
#endif
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
#ifdef HAVE_CUDA
        if(isGpuEx()){
            // Save only the first physical address. Assume contiguous physical memory.
            if((getTotalTransfer()) == (getRemainingTransfer()))
                physicalAddresses.push_back(physAddr);
        }
#endif
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

#ifdef HAVE_CUDA
// Create an event to send to the GPU Component
void ArielCore::handleGpuEvent(ArielGpuEvent* gEv){
    if(gpu_enabled) {
        BalarEvent * tse = new BalarEvent(BalarComponent::EventType::REQUEST);
        std::cout << "Add payload.." << std::endl;
        switch(gEv->getGpuApi()){
            case GPU_REG_FAT_BINARY:
                tse->API = GPU_REG_FAT_BINARY;
                strncpy(tse->CA.file_name, file_path, 256);
                break;
            case GPU_REG_FUNCTION:
                tse->API = GPU_REG_FUNCTION;
                tse->CA.register_function.fat_cubin_handle = gEv->getFatCubinHandle();
                tse->CA.register_function.host_fun = gEv->getHostFun();
                strncpy(tse->CA.register_function.device_fun, gEv->getDeviceFun(), 512);
                break;
            case GPU_MEMCPY:
                {
                    tse->API = GPU_MEMCPY;
                    setTotalTransfer(gEv->get_count());
                    setRemainingTransfer(gEv->get_count());
                    setAckTransfer(0);
                    setPageAckTransfer(0);
                    setKind(gEv->get_kind());
                    uint8_t *data;
                    if(getKind() == cudaMemcpyHostToDevice){
                        setBaseAddress(gEv->get_dst());
                        setCurrentAddress(gEv->get_dst());
                        // Read in a 4k page at a time
                        GpuDataCommand gd;
                        bool avail = false;
                        do {
                            avail = tunnelD->readMessageNB(coreID, &gd);
                        } while (!avail);
                        output->verbose(CALL_INFO, 16, 0, "CUDA: D2H received first page from tunnel\n");

                        // Take 4k (max) from tunnelD at a time
                        size_t page_transfer = (getTotalTransfer() <= (1<<12)) ? getTotalTransfer() : (1<<12);
                        data = (uint8_t *) malloc(sizeof(uint8_t) * page_transfer);
                        memcpy(data, gd.page_4k, page_transfer);
                        tunnelD->clearBuffer(coreID);

                        setPageTransfer(page_transfer);
                        setRemainingPageTransfer(page_transfer);
                        setDataAddress(data);

                        // Send transaction until we run out of data or open transactions
                        int index;
                        size_t current_transfer;
                        while((getOpenTransactions() > 0) && (getRemainingPageTransfer() > 0)){
                            index = getCurrentAddress() - getBaseAddress();
                            current_transfer = (getRemainingPageTransfer() > 64) ? 64 : getRemainingPageTransfer();

                            output->verbose(CALL_INFO, 16, 0, "CUDA: Transfer of %" PRIu32 " to index %" PRIu64 ":", current_transfer, index);
                            if( verbosity >= 16) {
                                for(int i = 0; i < current_transfer; i++)
                                    output->verbose(CALL_INFO, 16, 0, "%" PRIu64 " ",
                                            getDataAddress()[index + i]);
                                output->verbose(CALL_INFO, 16, 0, "\n");
                            }

                            ArielWriteEvent* awe;
                            awe = new ArielWriteEvent(getCurrentAddress(), current_transfer, &getDataAddress()[index]);
                            handleWriteRequest(awe);
                            setCurrentAddress(getCurrentAddress() + current_transfer);
                            setRemainingPageTransfer(getRemainingPageTransfer() - current_transfer);
                        }
                    } else if(kind == cudaMemcpyDeviceToHost){
                        setBaseAddress(gEv->get_src());
                        setCurrentAddress(gEv->get_src());

                        // Allocate only what is required
                        data = (uint8_t *) malloc(getTotalTransfer());
                        setDataAddress(data);
                        setPhysicalAddresses(tse);
                        tse->payload = physicalAddresses;
                        tse->CA.cuda_memcpy.dst = physicalAddresses[0];
                        tse->CA.cuda_memcpy.src = gEv->get_src();
                        tse->CA.cuda_memcpy.count = gEv->get_count();
                        tse->CA.cuda_memcpy.kind = gEv->get_kind();
                    } else if (kind == cudaMemcpyDeviceToDevice) {
                        tse->CA.cuda_memcpy.dst = gEv->get_dst();;
                        tse->CA.cuda_memcpy.src = gEv->get_src();
                        tse->CA.cuda_memcpy.count = gEv->get_count();
                        tse->CA.cuda_memcpy.kind = gEv->get_kind();
                    }
                }
                break;
            case GPU_CONFIG_CALL:
                tse->API = GPU_CONFIG_CALL;
                tse->CA.cfg_call.gdx = gEv->get_gridDimx();
                tse->CA.cfg_call.gdy = gEv->get_gridDimy();
                tse->CA.cfg_call.gdz = gEv->get_gridDimz();

                tse->CA.cfg_call.bdx = gEv->get_blockDimx();
                tse->CA.cfg_call.bdy = gEv->get_blockDimy();
                tse->CA.cfg_call.bdz = gEv->get_blockDimz();
                break;
            case GPU_SET_ARG:
                tse->API = GPU_SET_ARG;
                tse->CA.set_arg.address = gEv->get_address();
                memcpy(tse->CA.set_arg.value, gEv->get_value(), gEv->get_size());
                tse->CA.set_arg.size = gEv->get_size();
                tse->CA.set_arg.offset = gEv->get_offset();
                break;
            case GPU_LAUNCH:
                tse->CA.cuda_launch.func = gEv->get_func();
                tse->API = GPU_LAUNCH;
                break;
            case GPU_FREE:
                tse->API = GPU_FREE;
                tse->CA.free_address = gEv->get_free_addr();
                break;
            case GPU_GET_LAST_ERROR:
                tse->API = GPU_GET_LAST_ERROR;
                break;
            case GPU_MALLOC:
                tse->API = GPU_MALLOC;
                tse->CA.cuda_malloc.dev_ptr = gEv->getDevPtr();
                tse->CA.cuda_malloc.size = gEv->getSize();
                break;
            case GPU_REG_VAR:
                tse->API = GPU_REG_VAR;
                tse->CA.register_var.fatCubinHandle = gEv->getVarFatCubinHandle();
                tse->CA.register_var.hostVar = gEv->getVarHostVar();
                strncpy(tse->CA.register_var.deviceName, gEv->getVarDeviceName(), 256);
                tse->CA.register_var.ext = gEv->getVarExt();
                tse->CA.register_var.size = gEv->getVarSize();
                tse->CA.register_var.constant = gEv->getVarConstant();
                tse->CA.register_var.global = gEv->getVarGlobal();
                break;
            case GPU_MAX_BLOCK:
                tse->API = GPU_MAX_BLOCK;
                tse->CA.max_active_block.hostFunc = gEv->getMaxBlockHostFunc();
                tse->CA.max_active_block.blockSize = gEv->getMaxBlockBlockSize();
                tse->CA.max_active_block.dynamicSMemSize = gEv->getMaxBlockSMemSize();
                tse->CA.max_active_block.flags = gEv->getMaxBlockFlag();
                break;
            default:
                //TODO actually fail here
                break;
        }
        std::cout << "Add API " << tse->API << std::endl;
        if(tse->API == GPU_MEMCPY){
            if(getKind() == cudaMemcpyHostToDevice){
            } else if(getKind() == cudaMemcpyDeviceToHost) {
                GpuLink->send(tse);
                std::cout << "Payload sent.. DtH" << std::endl;
            } else if (getKind() == cudaMemcpyDeviceToDevice) {
                GpuLink->send(tse);
                std::cout << "Payload sent..DtD" << std::endl;
            }
        } else{
            GpuLink->send(tse);
            std::cout << "Payload sent.." << std::endl;
        }
    }
}

void ArielCore::handleGpuAckEvent(SST::Event* e){
    BalarEvent * ev = dynamic_cast<BalarComponent::BalarEvent*>(e);
    if (ev->getType() == BalarComponent::EventType::RESPONSE){
        if((ev->API == GPU_MEMCPY_RET)&&(ev->CA.cuda_memcpy.kind == cudaMemcpyDeviceToHost)){
            // Device to Host still needs us to get the data for fesimple
            ArielReadEvent *are;
            while((getOpenTransactions() > 0) && (getRemainingTransfer() > 0)){
                if(getRemainingTransfer() <= 64) {
                    are = new ArielReadEvent(getCurrentAddress(), getRemainingTransfer());
                    setRemainingTransfer(0);
                }else {
                    are = new ArielReadEvent(getCurrentAddress(), 64);
                    setRemainingTransfer(getRemainingTransfer()-64);
                    setCurrentAddress(getCurrentAddress() + 64);
                }
                handleReadRequest(are);
            }
        } else {
            output->verbose(CALL_INFO, 16, 0, "CUDA: Ariel recieved ACK\n");
            std::cout << "Received ACK response" << std::endl;
            isStalled = false;
            isGpu = false;
            GpuCommand gc;
            gc.API_Return.name = ev->API;
            if(ev->API == GPU_MALLOC_RET){
                // cudaMalloc needs the address of GPU memory
                gc.ptr_address = ev->CA.cuda_malloc.ptr_address;
                output->verbose(CALL_INFO, 16, 0, "CUDA: GPU address %" PRIu64 "\n", gc.ptr_address);
            } else if (ev->API == GPU_REG_FAT_BINARY_RET) {
                // cudaRegisterFatBinary needs the handler
                gc.fat_cubin_handle = ev->CA.register_fatbin.fat_cubin_handle;
                std::cout << "Received ACK fatbin handle response " << gc.fat_cubin_handle << std::endl;
            } else if (ev->API == GPU_MAX_BLOCK_RET) {
                gc.num_block = ev->CA.max_active_block.numBlock;
                std::cout << "Received ACK numblock response " << gc.num_block << std::endl;
            }
            output->verbose(CALL_INFO, 16, 0, "CUDA: Ariel sent ACK\n");
            tunnelR->writeMessage(coreID, gc);
        }
    }
}
#endif

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
#ifdef HAVE_CUDA
        case GPU:
            ARIEL_CORE_VERBOSE(8, output->verbose(CALL_INFO, 8, 0, "Core %" PRIu32 "next event is GPU (CUDA call)\n", coreID));
            removeEvent = true;
            stall();
            gpu();
            handleGpuEvent(dynamic_cast<ArielGpuEvent*>(nextEvent));
            break;
#endif
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

