// Copyright 2009-2017 Sandia Corporation. Under the terms
// of Contract DE-NA0003525 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2017, Sandia Corporation
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
#include <sst/core/element.h>
#include <sst/core/params.h>
#include <sst/core/simulation.h>

#include "scratchpad.h"
#include "membackend/scratchBackendConvertor.h"
#include "util.h"
#include "memNIC.h"
#include <sst/core/interfaces/stringEvent.h>

using namespace std;
using namespace SST;
using namespace SST::MemHierarchy;

/*
 *
 *  ScratchPad Controller
 *  
 *  Events          Definition                                  Max Req Size
 *  ---------------------------------------------------------------------------------
 *  Scratch Read    Read from scratchpad                        scratchLineSize
 *  Scratch Write   Write to scratchpad                         scratchLineSize
 *  Scratch Get     Read from remote memory into scratchpad
 *  Scratch Put     Write from scratchpad into remote memory    
 *  Remote Read     Read from remote memory                     remoteMemLineSize
 *  Remote Write    Write to remote memory                      remoteMemLineSize
 *
 * Ordering:
 *  - Only concerned with coherence (same address)
 *  - No write acks from scratch or memory
 *  - scratchpad can't reorder reads and writes to the same line or we'd need acks
 *  - network can't reorder or we'll need acks and mshr structures for remote accesses
 *
 *
 * Conflict resolution:
 * * For CPU ordering, CPU is responsible for ensuring conflicting events
 * that the app needs ordered aren't issued to the memory system in parallel
 * Scratchpad will ensure that once CPU receives an ack for the event, a future event
 * will be ordered later
 *
 *  First       Second      Who orders
 *  --------------------------------------------
 *  ScRead      ScRead      No conflict
 *  ScRead      ScWrite     MSHR/scratch
 *  ScRead      Get         MSHR/scratch
 *  ScRead      Put         No conflict
 *  ScWrite     ScRead      MSHR/scratch
 *  ScWrite     ScWrite     MSHR/scratch
 *  ScWrite     Get         MSHR/scratch
 *  ScWrite     Put         MSHR/scratch
 *  RemRead     Get         Network/memory
 *  RemRead     Put         Network/memory
 *  RemRead     RemRead     No conflict
 *  RemRead     RemWrite    Network/memory
 *  RemWrite    Get         Network/memory
 *  RemWrite    Put         Network/memory
 *  RemWrite    RemRead     Network/memory
 *  RemWrite    RemWrite    Network/memory
 *  Get         ScRead      MSHR/scratchpad - always return Get'd data
 *  Get         ScWrite     MSHR/scratchpad - always overwrite Get'd data
 *  Get         Get         MSHR/scratchpad - second Get should always overwrite first, no remote conflict
 *  Get         Put         MSHR/scratchpad - Put should read Get'd data
 *  Get         RemRead     No conflict
 *  Get         RemWrite    Network/memory
 *  Put         ScRead      No conflict
 *  Put         ScWrite     MSHR/scratchpad
 *  Put         Get         CPU - since we aren't ordering remote, get's remote read can bypass put's remote write
 *  Put         Put         CPU - can't ensure second Put's remote write is sent after first Put's write
 *  Put         RemRead     CPU - can't ensure read comes after Put's remote write
 *  Put         RemWrite    CPU - can't ensure write comes after Put's remote write
 *
 */

Scratchpad::Scratchpad(ComponentId_t id, Params &params) : Component(id) {
    // Output 
    int debugLevel = params.find<int>("debug_level", 0);
    dbg.init("", debugLevel, 0, (Output::output_location_t)params.find<int>("debug", 0));
    if (debugLevel < 0 || debugLevel > 10) 
        dbg.fatal(CALL_INFO, -1, "Debugging level must be between 0 and 10\n");
    
    Output out("", 1, 0, Output::STDOUT);
    

    bool found;
    /* Get parameters and check validity */
    // Clock
    string clock_freq = params.find<std::string>("clock", "", found);
    if (!found) out.fatal(CALL_INFO, -1, "Param not specified (%s): clock - clock rate (with units, e.g., GHz)\n", getName().c_str());

    // Size
    UnitAlgebra size = UnitAlgebra(params.find<std::string>("size", "0B", found));
    if (!found) out.fatal(CALL_INFO, -1, "Param not specified (%s): size - size of the scratchpad (with units, e.g., MiB)\n", getName().c_str());
    if (!size.hasUnits("B")) out.fatal(CALL_INFO, -1, "Invalid param (%s): size - units must be bytes ('B'). SI units ok. You specified '%s'\n", getName().c_str(), size.toString().c_str());
    scratchSize_ = size.getRoundedValue();

    // Line size
    scratchLineSize_ = params.find<uint64_t>("scratch_line_size", 64);
    remoteLineSize_ = params.find<uint64_t>("memory_line_size", 64);

    if (scratchSize_ % scratchLineSize_ != 0) {
        out.fatal(CALL_INFO, -1, "Invalid param combination (%s): size, scratch_line_size - size must be divisible by scratch_line_size. You speciified size: %s, scratch_line_size: %" PRIu64 "\n", 
                getName().c_str(), size.toString().c_str(), scratchLineSize_);
    }

    // Throughput limits
    responsesPerCycle_ = params.find<uint32_t>("response_per_cycle",0);

    // To back or not to back
    doNotBack_ = params.find<bool>("do_not_back", false);
    
    // Remote address computation
    remoteAddrOffset_ = params.find<uint64_t>("memory_addr_offset", scratchSize_);

    // Create backend which will handle the timing for scratchpad
    std::string bkName = params.find<std::string>("backendConvertor", "memHierarchy.scratchBackendConvertor");

    // Copy some parameters into the backend
    Params bkParams = params.find_prefix_params("backendConvertor.");
    bkParams.insert("backend.clock", clock_freq);
    bkParams.insert("backend.request_width", std::to_string(scratchLineSize_));
    bkParams.insert("backend.mem_size", size.toString()); 
    
    // Create backend
    scratch_ = dynamic_cast<ScratchBackendConvertor*>(loadSubComponent(bkName, this, bkParams));
    
    // Initialize scratchpad entries
    if (doNotBack_) {
        backing_ = NULL;
    } else {
        std::string memFile = "";
        memFile.clear();
        try {
            backing_ = new Backend::Backing(memFile, scratch_->getMemSize() );
        } catch (int) {
            dbg.fatal(CALL_INFO, -1, "%s, Error - unable to MMAP backing store for scratchpad\n", getName().c_str());
        }
    }

    // Assume no caching, may change during init
    caching_ = false;

    // Create clock
    registerClock(clock_freq, new Clock::Handler<Scratchpad>(this, &Scratchpad::clock));
   
    // Register statistics
    stat_ScratchReadReceived      = registerStatistic<uint64_t>("request_received_scratch_read");
    stat_ScratchWriteReceived     = registerStatistic<uint64_t>("request_received_scratch_write");
    stat_RemoteReadReceived       = registerStatistic<uint64_t>("request_received_remote_read");
    stat_RemoteWriteReceived      = registerStatistic<uint64_t>("request_received_remote_write");
    stat_ScratchGetReceived       = registerStatistic<uint64_t>("request_received_scratch_get");
    stat_ScratchPutReceived       = registerStatistic<uint64_t>("request_received_scratch_put");
    stat_ScratchReadIssued        = registerStatistic<uint64_t>("request_issued_scratch_read");
    stat_ScratchWriteIssued       = registerStatistic<uint64_t>("request_issued_scratch_write");

    std::string cpuLinkLatency = params.find<std::string>("cpu_link_latency", "50ps");
    std::string memLinkLatency = params.find<std::string>("mem_link_latency", "1ns");

    // Figure out port connections and set up links
    // Options: cpu and network; or cpu and memory;
    // cpu is a MoveEvent interface, memory & network are MemEvent interfaces (memory is a direct connect while network uses SimpleNetwork)
    bool memoryDirect = isPortConnected("memory");
    bool memoryNetwork = isPortConnected("network");
    if (!isPortConnected("cpu")) {
        out.fatal(CALL_INFO, -1, "Invalid port configuration (%s): Scratchpad requires direct connection to CPU on 'cpu' port.\n", getName().c_str());
    } else if (!memoryDirect && !memoryNetwork) {
        out.fatal(CALL_INFO, -1, "Invalid port configuration (%s): Did not detect port for memory-side events. Connect either 'memory' or 'network'\n", getName().c_str());
    }
    
    linkUp_ = configureLink( "cpu", cpuLinkLatency, new Event::Handler<Scratchpad>(this, &Scratchpad::processIncomingCPUEvent));

    MemNIC::ComponentInfo myInfo;
    MemNIC::ComponentTypeInfo typeInfo;
    UnitAlgebra packet;
    if (memoryNetwork) {
        myInfo.link_port = "network";
        myInfo.link_bandwidth = params.find<std::string>("network_bw", "80GiB/s");
	myInfo.num_vcs = 1;
        myInfo.name = getName();
        myInfo.network_addr = params.find<int>("network_address");
        myInfo.link_inbuf_size = params.find<std::string>("network_input_buffer_size", "1KiB");
        myInfo.link_outbuf_size = params.find<std::string>("network_output_buffer_size", "1KiB");
        
        typeInfo.rangeStart = 0;
        typeInfo.rangeEnd   = (uint64_t)-1;
        typeInfo.interleaveSize = 0;
        typeInfo.interleaveStep = 0;
        typeInfo.blocksize = scratchLineSize_;
        typeInfo.coherenceProtocol = CoherenceProtocol::NONE;
        typeInfo.cacheType = ""; // Unused?
        // Get min packet size
        packet = UnitAlgebra(params.find<std::string>("min_packet_size", "8B"));
        if (!packet.hasUnits("B")) out.fatal(CALL_INFO, -1, "Invalid param (%s): min_packet_size - must have units of bytes (B). Ex: '8B'. SI units are ok. You specified '%s'\n", getName().c_str(), packet.toString().c_str());
        
        myInfo.type = MemNIC::TypeScratch;
        linkNet_ = new MemNIC(this, &dbg, -1, myInfo, new Event::Handler<Scratchpad>(this, &Scratchpad::processIncomingRemoteEvent));
        linkNet_->addTypeInfo(typeInfo);
        linkDown_ = nullptr;
    } else {
        linkDown_ = configureLink( "memory", memLinkLatency, new Event::Handler<Scratchpad>(this, &Scratchpad::processIncomingRemoteEvent));
        linkNet_ = nullptr;
    }


    // Initialize local variables
    timestamp_ = 0;
}

/* Init - forward memory initialization events to memory; detect presence of caches */
void Scratchpad::init(unsigned int phase) {
    //Init MemNIC if we have one - must be done before attempting to send anything on this link
    if (linkNet_) linkNet_->init(phase);

    // Send initial info out
    if (!phase) {
        if (linkNet_) linkNet_->sendInitData(new MemEventInit(getName(), Command::NULLCMD, Endpoint::Scratchpad, true, scratchLineSize_));
        else linkDown_->sendInitData(new MemEventInit(getName(), Command::NULLCMD, Endpoint::Scratchpad, true, scratchLineSize_));
        linkUp_->sendInitData(new MemEventInit(getName(), Command::NULLCMD, Endpoint::Scratchpad, true, scratchLineSize_));
    }

    // Handle incoming events
    while (SST::Event *ev = linkUp_->recvInitData()) {
        MemEventInit *sEv = dynamic_cast<MemEventInit*>(ev);
        if (sEv) {
            if (sEv->getCmd() == Command::NULLCMD) {
                dbg.debug(_L10_, "%s received init event: %s\n", getName().c_str(), sEv->getBriefString().c_str());
                if (sEv->getType() == Endpoint::Cache) {
                    caching_ = true;
                    cacheStatus_.resize(scratchSize_/scratchLineSize_, false);
                } else if (sEv->getType() == Endpoint::Directory) {
                    dbg.fatal(CALL_INFO, -1, "Error, scratchpad does not currently support link ot a directory...working on this\n");
                }
            } else { // Not a NULLCMD
                MemEventInit * memRequest = new MemEventInit(getName(), sEv->getCmd(), sEv->getAddr() - remoteAddrOffset_, sEv->getPayload());
                if (linkNet_) {
                    memRequest->setDst(linkNet_->findTargetDestination(memRequest->getAddr() - remoteAddrOffset_));
                    memRequest->setAddr(linkNet_->convertToDestinationAddress(memRequest->getDst(), memRequest->getAddr() - remoteAddrOffset_));
                    dbg.debug(_L10_, "%s forwarding init event: %s\n", getName().c_str(), memRequest->getBriefString().c_str());
                    linkNet_->sendInitData(memRequest);
                } else {
                    dbg.debug(_L10_, "%s forwarding init event: %s\n", getName().c_str(), memRequest->getBriefString().c_str());
                    linkDown_->sendInitData(memRequest);

                }
            }
        } // Not a MemEventInit
        delete ev;
    }
}


/* Events received from processor */
void Scratchpad::processIncomingCPUEvent(SST::Event* event) {
    MemEventBase * ev = static_cast<MemEventBase*>(event);
    
    dbg.debug(_L3_, "%" PRIu64 " (%s) Received: %s\n", timestamp_, getName().c_str(), ev->getBriefString().c_str());
    
    Command cmd = ev->getCmd();
    
    
    switch(cmd) {    
        case Command::GetS:
            handleRead(ev);
            break;
        case Command::GetX:
        case Command::PutM:
        case Command::PutE:
        case Command::PutS:
            handleWrite(ev);
            break;
        case Command::Get:
            handleScratchGet(ev);
            break;
        case Command::Put:
            handleScratchPut(ev);
            break;
        /* The rest of the cases are for hierarchies with caches only */
        case Command::AckInv:
            handleAckInv(ev);
            break;
        case Command::FetchResp:
            handleFetchResp(ev);
            break;
        default:
            dbg.fatal(CALL_INFO, -1, "Scratchpad (%s) received unhandled event: cmd = %s, cycles = %" PRIu64 ".\n", getName().c_str(), CommandString[(int)cmd], timestamp_);
    }
}

/* Events received from memory - must be read response but could be response to a memory read or a ScratchGet */
void Scratchpad::processIncomingRemoteEvent(SST::Event * event) {
    MemEvent * ev = static_cast<MemEvent*>(event);

    dbg.debug(_L3_, "%" PRIu64 " (%s) Received: %s\n", timestamp_, getName().c_str(), ev->getBriefString().c_str());

    // Determine what kind of event spawned this and pass off to handler
    std::map<SST::Event::id_type,SST::Event::id_type>::iterator it = responseIDMap_.find(ev->getResponseToID());

    if (it == responseIDMap_.end()) {
        dbg.fatal(CALL_INFO, -1, "(%s) Received data response from remote but no matching request in responseIDMap_, id is (%" PRIu64 ", %" PRIu32 "), timestamp is %" PRIu64 "\n",
                getName().c_str(), ev->getResponseToID().first, ev->getResponseToID().second, timestamp_);
    }

    SST::Event::id_type requestID = it->second;
    responseIDMap_.erase(it);

    MemEventBase * requestBase = outstandingEventList_.find(requestID)->second.request;

    if (requestBase->getCmd() == Command::Get) handleRemoteGetResponse(ev, requestID);
    else handleRemoteReadResponse(ev, requestID);
}


/* Clock handler */
bool Scratchpad::clock(Cycle_t cycle) {
    timestamp_++;

    // issue ready events
    uint32_t responseThisCycle = (responsesPerCycle_ == 0) ? 1 : 0;
    while (!procMsgQueue_.empty() && procMsgQueue_.begin()->first < timestamp_) {
        MemEventBase * sendEv = procMsgQueue_.begin()->second;
        dbg.debug(_L4_, "%" PRIu64 " (%s) Sending event to processor: %s\n", timestamp_, getName().c_str(), sendEv->getBriefString().c_str());
        linkUp_->send(sendEv);
        procMsgQueue_.erase(procMsgQueue_.begin());
        responseThisCycle++;
        if (responseThisCycle == responsesPerCycle_) break;
    }

    while (!memMsgQueue_.empty() && memMsgQueue_.begin()->first < timestamp_) {
        MemEvent * sendEv = memMsgQueue_.begin()->second;
        if (linkDown_) {
            dbg.debug(_L4_, "%" PRIu64 " (%s) Sending event to memory: %s\n", timestamp_, getName().c_str(), sendEv->getBriefString().c_str());
            linkDown_->send(sendEv);
        } else {
            sendEv->setDst(linkNet_->findTargetDestination(sendEv->getBaseAddr()));
            sendEv->setAddr(linkNet_->convertToDestinationAddress(sendEv->getDst(), sendEv->getAddr()));
            sendEv->setBaseAddr(linkNet_->convertToDestinationAddress(sendEv->getDst(), sendEv->getBaseAddr()));
            dbg.debug(_L4_, "%" PRIu64 " (%s) Sending event to memory: %s\n", timestamp_, getName().c_str(), sendEv->getBriefString().c_str());
            linkNet_->send(sendEv);
        }
        memMsgQueue_.erase(memMsgQueue_.begin());
    }

    if (linkNet_) linkNet_->clock();
    scratch_->clock(cycle); // Clock backend

    return false;
}

/***************** request and response handlers ***********************/
void Scratchpad::handleRead(MemEventBase * event) {
    MemEvent * ev = static_cast<MemEvent*>(event);

    if (ev->getAddr() < scratchSize_) {
        handleScratchRead(ev);
    } else {
        handleRemoteRead(ev);
    }
}

void Scratchpad::handleWrite(MemEventBase * event) {
    MemEvent * ev = static_cast<MemEvent*>(event);
    if (ev->getAddr() < scratchSize_) {
        if (ev->isWriteback() || ev->queryFlag(MemEvent::F_NONCACHEABLE)) {
            handleScratchWrite(ev);
        } else {
            handleScratchRead(ev);
        }
    } else {
        handleRemoteWrite(ev);
    }
}

void Scratchpad::handleScratchRead(MemEvent * ev) {

    stat_ScratchReadReceived->addData(1);

    MemEvent * response = ev->makeResponse();

    MemEvent * read = new MemEvent(this, ev->getAddr(), ev->getBaseAddr(), Command::GetS, ev->getSize());
    read->setRqstr(ev->getRqstr());
    read->setVirtualAddress(ev->getVirtualAddress());
    read->setInstructionPointer(ev->getInstructionPointer());

    responseIDMap_.insert(std::make_pair(read->getID(),ev->getID()));
    responseIDAddrMap_.insert(std::make_pair(read->getID(),ev->getBaseAddr()));
    outstandingEventList_.insert(std::make_pair(ev->getID(),OutstandingEvent(ev,response)));

    if (caching_ && !ev->queryFlag(MemEvent::F_NONCACHEABLE)) {
        cacheStatus_[ev->getBaseAddr()/scratchLineSize_] = true;
    }

    if (mshr_.find(ev->getBaseAddr()) == mshr_.end()) {
        std::vector<uint8_t> data = doScratchRead(read);
        response->setPayload(data);
        mshr_.insert(std::make_pair(ev->getBaseAddr(), std::list<MSHREntry>(1,MSHREntry(ev->getID(), Command::GetS, true))));
    } else {
        mshr_.find(ev->getBaseAddr())->second.push_back(MSHREntry(ev->getID(), Command::GetS, read));
    }
    dbg.debug(_L5_, "\tInserting in mshr. %s\n", mshr_.find(ev->getBaseAddr())->second.back().getString().c_str());
}

void Scratchpad::handleScratchWrite(MemEvent * ev) {
    /* Update cache state */
    if (caching_ && !ev->queryFlag(MemEvent::F_NONCACHEABLE))
        cacheStatus_[ev->getBaseAddr()/scratchLineSize_] = false;

    /* Drop clean writebacks */
    if (ev->isWriteback() && !ev->getDirty()) {
        delete ev;
        return;
    }

    stat_ScratchWriteReceived->addData(1);

    MemEvent * response;
    if (!ev->isWriteback()) {
        response = ev->makeResponse();
    }

    MemEvent * write = new MemEvent(this, ev->getAddr(), ev->getBaseAddr(), Command::GetX, ev->getPayload());
    write->setRqstr(ev->getRqstr());
    write->setVirtualAddress(ev->getVirtualAddress());
    write->setInstructionPointer(ev->getInstructionPointer());


    if (mshr_.find(ev->getBaseAddr()) == mshr_.end()) {
        doScratchWrite(write);
        if (response)
            sendResponse(response);
        delete ev;
    } else {
        outstandingEventList_.insert(std::make_pair(ev->getID(),OutstandingEvent(ev,response)));
        mshr_.find(ev->getBaseAddr())->second.push_back(MSHREntry(ev->getID(), Command::GetX, write));
        dbg.debug(_L5_, "\tInserting in mshr. %s\n", mshr_.find(ev->getBaseAddr())->second.back().getString().c_str());
    }
}

void Scratchpad::handleScratchGet(MemEventBase * event) {
    MoveEvent * ev = static_cast<MoveEvent*>(event);

    stat_ScratchGetReceived->addData(1);

    MoveEvent * response = ev->makeResponse();
    outstandingEventList_.insert(std::make_pair(ev->getID(),OutstandingEvent(ev,response)));

    // Issue remote read
    ev->setSrcBaseAddr((ev->getSrcAddr() - remoteAddrOffset_) & ~(remoteLineSize_ - 1));
    MemEvent * remoteRead = new MemEvent(this, ev->getSrcAddr() - remoteAddrOffset_, ev->getSrcBaseAddr(), Command::GetS, ev->getSize());
    remoteRead->setFlag(MemEvent::F_NONCACHEABLE);
    remoteRead->setRqstr(ev->getRqstr());
    remoteRead->setVirtualAddress(ev->getSrcVirtualAddress());
    remoteRead->setInstructionPointer(ev->getInstructionPointer());
    responseIDMap_.insert(std::make_pair(remoteRead->getID(), ev->getID()));
    
    dbg.debug(_L5_, "\tInserting event in memory queue. %s\n", remoteRead->getBriefString().c_str());
    memMsgQueue_.insert(std::make_pair(timestamp_, remoteRead));

    // Insert into mshr and send inv if needed
    // start base addr -> end base addr
    // start base addr + size
    uint32_t lineCount = 1 + (ev->getDstAddr() + ev->getSize() - ev->getDstBaseAddr() - 1)/ scratchLineSize_;
    for (uint32_t i = 0; i < lineCount; i++) {
        Addr baseAddr = ev->getDstBaseAddr() + i*scratchLineSize_;
        if (mshr_.find(baseAddr) == mshr_.end()) {
            bool needDataAndAck = startGet(baseAddr, ev);
            mshr_.insert(std::make_pair(baseAddr, std::list<MSHREntry>(1, MSHREntry(ev->getID(), Command::Get, true, needDataAndAck))));
        } else {
            mshr_.find(baseAddr)->second.push_back(MSHREntry(ev->getID(), Command::Get, false));
        }
        dbg.debug(_L5_, "\tInserting in mshr. %s\n", mshr_.find(baseAddr)->second.back().getString().c_str());
        outstandingEventList_.find(ev->getID())->second.incrementCount();
    }
}

void Scratchpad::handleScratchPut(MemEventBase * event) {
    MoveEvent *ev = static_cast<MoveEvent*>(event);

    stat_ScratchPutReceived->addData(1);

    MoveEvent * response = ev->makeResponse();
    ev->setDstBaseAddr((ev->getDstBaseAddr() - remoteAddrOffset_) & ~(remoteLineSize_ - 1));
    MemEvent * remoteWrite = new MemEvent(this, ev->getDstAddr() - remoteAddrOffset_, ev->getDstBaseAddr(), Command::GetX, ev->getSize());
    remoteWrite->setZeroPayload(ev->getSize());
    outstandingEventList_.insert(std::make_pair(ev->getID(), OutstandingEvent(ev, response, remoteWrite)));
    
    Addr addr = ev->getSrcAddr();
    Addr baseAddr = ev->getSrcBaseAddr();
    uint32_t bytesLeft = ev->getSize();
    while (bytesLeft != 0) {
        uint32_t size = (baseAddr + scratchLineSize_) - addr;
        if (size > bytesLeft) size = bytesLeft;

        if (mshr_.find(baseAddr) == mshr_.end()) {
            startPut(baseAddr, ev);
            mshr_.insert(std::make_pair(baseAddr, std::list<MSHREntry>(1, MSHREntry(ev->getID(), Command::Put, true))));
        } else {
            mshr_.find(baseAddr)->second.push_back(MSHREntry(ev->getID(), Command::Put, false));
        }
        dbg.debug(_L5_, "\tInserting in mshr. %s\n", mshr_.find(baseAddr)->second.back().getString().c_str());

        bytesLeft -= size;
        baseAddr += scratchLineSize_;
        addr = baseAddr;
        
        outstandingEventList_.find(ev->getID())->second.incrementCount();
    }
}

void Scratchpad::handleScratchResponse(SST::Event::id_type responseID) {
    dbg.debug(_L3_, "%" PRIu64 " (%s) Received scratch response with ID <%" PRIu64 ", %" PRIu32 ">\n", timestamp_, getName().c_str(), responseID.first, responseID.second);
    SST::Event::id_type requestID = responseIDMap_.find(responseID)->second;
    responseIDMap_.erase(responseID);
    
    Addr baseAddr = responseIDAddrMap_.find(responseID)->second;
    responseIDAddrMap_.erase(responseID);

    if (outstandingEventList_.find(requestID)->second.request->getCmd() == Command::Put) {
        updatePut(requestID);
    } else { // Anything else - GetS, GetX, etc.
        finishRequest(requestID);
    }
    updateMSHR(baseAddr);
}

void Scratchpad::handleAckInv(MemEventBase * event) {
    MemEvent *response = static_cast<MemEvent*>(event);
    
    SST::Event::id_type requestID = responseIDMap_.find(response->getResponseToID())->second;
    responseIDMap_.erase(response->getResponseToID());

    Addr baseAddr = response->getBaseAddr();

    cacheStatus_[baseAddr/scratchLineSize_] = false;

    MoveEvent * request = static_cast<MoveEvent*>(outstandingEventList_.find(requestID)->second.request);

    // Find matching mshr entry
    MSHREntry * entry = &(mshr_.find(baseAddr)->second.front());
    if (entry->cmd == Command::Get) {
        if (entry->needDataAndAck) {
            entry->needDataAndAck = false;
            dbg.debug(_L5_, "\tUpdated mshr entry. %s\n", mshr_.find(baseAddr)->second.front().getString().c_str());
        } else {
            updateGet(entry->id);
            updateMSHR(baseAddr);
        }
    } else { // Command::Put
        // Send a read since we didn't get data
        // Determine address and size for the read
        Addr addr = baseAddr;
        if (addr == request->getSrcBaseAddr())
            addr = request->getSrcAddr();

        uint32_t size = deriveSize(addr, baseAddr, request->getSrcAddr(), request->getSize());
        
        MemEvent * read = new MemEvent(this, addr, baseAddr, Command::GetS, size);
        read->setRqstr(response->getRqstr());
        read->setVirtualAddress(request->getSrcVirtualAddress());
        read->setInstructionPointer(response->getInstructionPointer());
        std::vector<uint8_t> data = doScratchRead(read);
        std::vector<uint8_t> payload = outstandingEventList_.find(requestID)->second.remoteWrite->getPayload();
        uint32_t offset = addr - request->getSrcAddr();
        for (uint32_t i = 0; i < size; i++) {
            payload[i+offset] = data[i];
        }
        outstandingEventList_.find(requestID)->second.remoteWrite->setPayload(payload);
    }
    delete event;
}

void Scratchpad::handleFetchResp(MemEventBase * event) {
    MemEvent * response = static_cast<MemEvent*>(event);

    SST::Event::id_type requestID = responseIDMap_.find(response->getResponseToID())->second;
    responseIDMap_.erase(response->getResponseToID());

    Addr baseAddr = response->getBaseAddr();
    
    cacheStatus_[baseAddr/scratchLineSize_] = false;

    // Send a write to scratch since we forcefully invalidated
    MemEvent * write = new MemEvent(this, response->getAddr(), baseAddr, Command::GetX, response->getPayload());
    write->setRqstr(response->getRqstr());
    write->setVirtualAddress(response->getVirtualAddress());
    write->setInstructionPointer(response->getInstructionPointer());
    doScratchWrite(write);

    // Locate original request (put)
    MoveEvent * put = static_cast<MoveEvent*>(outstandingEventList_.find(requestID)->second.request);

    // Determine the target address and size for updating the remote write payload
    Addr addr = baseAddr;
    if (addr == put->getSrcBaseAddr())
        addr = put->getSrcAddr();
    
    uint32_t size = deriveSize(addr, baseAddr, put->getSrcAddr(), put->getSize());

    // Update write payload
    std::vector<uint8_t> payload = outstandingEventList_.find(requestID)->second.remoteWrite->getPayload();
    uint32_t offset = addr - put->getSrcAddr();
    for (uint32_t i = 0; i < size; i++) {
        payload[i+offset] = response->getPayload()[i];
    }
    outstandingEventList_.find(requestID)->second.remoteWrite->setPayload(payload);

    // Clear this mshr entry
    updatePut(requestID);
    updateMSHR(baseAddr);   // Delete mshr entry
    delete response;        // Delete response
}

void Scratchpad::handleRemoteRead(MemEvent * event) {
    stat_RemoteReadReceived->addData(1);

    event->setBaseAddr((event->getAddr() - remoteAddrOffset_) & ~(remoteLineSize_ - 1));
    MemEvent * request = new MemEvent(this, event->getAddr() - remoteAddrOffset_, event->getBaseAddr(), Command::GetS, event->getSize());
    request->setFlag(MemEvent::F_NONCACHEABLE);
    request->setRqstr(event->getRqstr());
    request->setVirtualAddress(event->getVirtualAddress());
    request->setInstructionPointer(event->getInstructionPointer());
    
    MemEvent * response = event->makeResponse();
    outstandingEventList_.insert(std::make_pair(event->getID(), OutstandingEvent(event, response)));
    responseIDMap_.insert(std::make_pair(request->getID(), event->getID()));
    dbg.debug(_L5_, "\tInserting event in memory queue. %s\n", request->getBriefString().c_str());
    memMsgQueue_.insert(std::make_pair(timestamp_, request));
}

void Scratchpad::handleRemoteWrite(MemEvent * event) {
    stat_RemoteWriteReceived->addData(1);

    event->setBaseAddr((event->getAddr() - remoteAddrOffset_) & ~(remoteLineSize_ - 1));
    MemEvent * request = new MemEvent(this, event->getAddr() - remoteAddrOffset_, event->getBaseAddr(), Command::GetX, event->getPayload());
    request->setFlag(MemEvent::F_NONCACHEABLE);
    request->setFlag(MemEvent::F_NORESPONSE);
    request->setRqstr(event->getRqstr());
    request->setVirtualAddress(event->getVirtualAddress());
    request->setInstructionPointer(event->getInstructionPointer());

    dbg.debug(_L5_, "\tInserting event in memory queue. %s\n", request->getBriefString().c_str());
    memMsgQueue_.insert(std::make_pair(timestamp_, request));

    MemEvent * response = event->makeResponse();
    procMsgQueue_.insert(std::make_pair(timestamp_, response));

    delete event;
}

void Scratchpad::handleRemoteGetResponse(MemEvent * response, SST::Event::id_type requestID) {
    
    MoveEvent * request = static_cast<MoveEvent*>(outstandingEventList_.find(requestID)->second.request);
    
    uint32_t bytesLeft = request->getSize();
    Addr addr = request->getDstAddr();
    Addr baseAddr = request->getDstBaseAddr();
    uint32_t payloadOffset = 0;

    while (bytesLeft != 0) {
        // Create write
        uint32_t size = (baseAddr + scratchLineSize_) - addr;
        if (size > bytesLeft) size = bytesLeft;
        std::vector<uint8_t> data(response->getPayload()[payloadOffset],response->getPayload()[payloadOffset+size]);
        MemEvent * write = new MemEvent(this,addr,baseAddr,Command::GetX,data);
        write->setRqstr(request->getRqstr());
        write->setVirtualAddress(request->getDstVirtualAddress());
        write->setInstructionPointer(request->getInstructionPointer());
        
        if (mshr_.find(baseAddr)->second.front().id == requestID) {
            doScratchWrite(write);
            if (mshr_.find(baseAddr)->second.front().needDataAndAck) {
                mshr_.find(baseAddr)->second.front().needDataAndAck = false;
                dbg.debug(_L5_, "\tUpdated mshr entry. %s\n", mshr_.find(baseAddr)->second.front().getString().c_str());
            } else {
                updateGet(requestID);
                updateMSHR(baseAddr);
            }
        } else {
            // Find it
            if (mshr_.find(baseAddr) == mshr_.end()) {
                dbg.fatal(CALL_INFO, -1, "ERROR: remoteGetResponse but no matching entry in mshr for address %" PRIu64 "\n", baseAddr);
            }
            for (std::list<MSHREntry>::iterator it = mshr_.find(baseAddr)->second.begin(); it != mshr_.find(baseAddr)->second.end(); it++) {
                if (it->id == requestID) {
                    it->scratch = write;
                    dbg.debug(_L5_, "\tUpdated mshr entry. %s\n", it->getString().c_str());
                }
            }
        }
        payloadOffset += size;
        bytesLeft -= size;
        baseAddr += scratchLineSize_;
        addr += size;
    }
    delete response;
}

void Scratchpad::handleRemoteReadResponse(MemEvent * response, SST::Event::id_type requestID) {
    // Update response with payload and finish request
    MemEvent * fwdResponse = static_cast<MemEvent*>(outstandingEventList_.find(requestID)->second.response);
    fwdResponse->setPayload(response->getPayload());
    
    finishRequest(requestID);

    delete response;
}

// Update MSHR
void Scratchpad::updateMSHR(Addr baseAddr) {
    // Remove top event
    mshr_.find(baseAddr)->second.pop_front();

    // Start next event
    while (!mshr_.find(baseAddr)->second.empty()) {
        MSHREntry * entry = &(mshr_.find(baseAddr)->second.front());
        dbg.debug(_L5_, "\tProcessing MSHR entry. %s\n", entry->getString().c_str());

        if (entry->cmd == Command::GetS) {
            std::vector<uint8_t> readData = doScratchRead(entry->scratch);
            static_cast<MemEvent*>(outstandingEventList_.find(entry->id)->second.response)->setPayload(readData);
            entry->started = true;
            dbg.debug(_L5_, "\t\tUpdated. %s\n", entry->getString().c_str());
            break;
        } else if (entry->cmd == Command::GetX) {
            doScratchWrite(entry->scratch);
            finishRequest(entry->id);
            mshr_.find(baseAddr)->second.pop_front();
            dbg.debug(_L5_, "\t\tRemoved\n");
        } else if (entry->cmd == Command::Get) {
            if (!(entry->started)) {
                entry->started = true;
                entry->needDataAndAck = startGet(baseAddr, static_cast<MoveEvent*>(outstandingEventList_.find(entry->id)->second.request));
                dbg.debug(_L5_, "\t\tUpdated. %s\n", entry->getString().c_str());
            }
            if (entry->scratch != nullptr) { // Data arrived, issue write
                doScratchWrite(entry->scratch);
                if (entry->needDataAndAck) { // Were waiting to send write & get Ack, now just waiting for Ack
                    entry->needDataAndAck = false;
                    dbg.debug(_L5_, "\t\tUpdated. %s\n", entry->getString().c_str());
                    break;
                } else { // we were only waiting on the write so we're good
                    updateGet(entry->id);
                    mshr_.find(baseAddr)->second.pop_front();
                    dbg.debug(_L5_, "\t\tRemoved.\n");
                }
            } else {
                dbg.fatal(CALL_INFO, -1, "\t\tUNHANDLED CASE\n");
                break;
            }
        } else if (entry->cmd == Command::Put) {
            startPut(baseAddr, static_cast<MoveEvent*>(outstandingEventList_.find(entry->id)->second.request));
            entry->started = true;
            dbg.debug(_L5_, "\t\tUpdated. %s\n", entry->getString().c_str());
        } else {
            // Error
        }
    }

    // Clear mshr entry if list is empty
    if (mshr_.find(baseAddr)->second.empty()) {
        mshr_.erase(baseAddr);
        dbg.debug(_L5_, "\tRemoved MSHR address %" PRIu64 "\n", baseAddr);
    }
}

// Helper methods
std::vector<uint8_t> Scratchpad::doScratchRead(MemEvent * event) {
    stat_ScratchReadIssued->addData(1);

    std::vector<uint8_t> data;
    if (backing_) {
        for (size_t i = 0; i < event->getSize(); i++) {
            data.push_back(backing_->get(event->getAddr() + i));
            dbg.debug(_L5_, "\tUpdating payload. (addr, data): (%" PRIu64 ", %u)\n", event->getAddr() + i, data.at(i));
        }
    } else {
        data.resize(event->getSize(), 0);
    }
    dbg.debug(_L4_, "\tSending request to scratch: %s\n", event->getBriefString().c_str());
    scratch_->handleMemEvent(event);
    return data;
}

void Scratchpad::doScratchWrite(MemEvent * event) {
    stat_ScratchWriteIssued->addData(1);

    if (backing_) {
        for (size_t i = 0; i < event->getSize(); i++) {
            dbg.debug(_L5_, "\tUpdating backing store. (addr, new data): (%" PRIu64 ", %u)\n", event->getAddr() + i, event->getPayload()[i]);
            backing_->set(event->getAddr() + i, event->getPayload()[i]);
        }
    }

    dbg.debug(_L4_, "\tSending request to scratch: %s\n", event->getBriefString().c_str());
    scratch_->handleMemEvent(event);
}

void Scratchpad::sendResponse(MemEventBase * event) {
    procMsgQueue_.insert(std::make_pair(timestamp_, event));
}

bool Scratchpad::startGet(Addr baseAddr, MoveEvent * get) {
    if (caching_ && cacheStatus_[baseAddr/scratchLineSize_]) {
        MemEvent * inv = new MemEvent(this, baseAddr, baseAddr, Command::ForceInv, scratchLineSize_);
        inv->setRqstr(get->getRqstr());
        inv->setVirtualAddress(get->getDstVirtualAddress());
        inv->setInstructionPointer(get->getInstructionPointer());
        procMsgQueue_.insert(std::make_pair(timestamp_, inv));
        responseIDMap_.insert(std::make_pair(inv->getID(),get->getID()));
        return true;
    }
    return false;
}

bool Scratchpad::startPut(Addr baseAddr, MoveEvent * put) {
    if (caching_ && cacheStatus_[baseAddr/scratchLineSize_]) {
        MemEvent * inv = new MemEvent(this, baseAddr, baseAddr, Command::ForceFetchInv, scratchLineSize_);
        inv->setRqstr(put->getRqstr());
        inv->setVirtualAddress(put->getSrcVirtualAddress());
        inv->setInstructionPointer(put->getInstructionPointer());
        procMsgQueue_.insert(std::make_pair(timestamp_, inv));
        responseIDMap_.insert(std::make_pair(inv->getID(),put->getID()));
        return false;
    } else {
        // Derive addr and size from baseAddr and the put request
        Addr addr = baseAddr;
        if (baseAddr == put->getSrcBaseAddr())
            addr = put->getSrcAddr();
        uint32_t size = deriveSize(addr, baseAddr, put->getSrcAddr(), put->getSize());

        MemEvent * read = new MemEvent(this, addr, baseAddr, Command::GetS, size);
        read->setRqstr(put->getRqstr());
        read->setVirtualAddress(put->getSrcVirtualAddress());
        read->setInstructionPointer(put->getInstructionPointer());
        responseIDMap_.insert(std::make_pair(read->getID(), put->getID()));
        responseIDAddrMap_.insert(std::make_pair(read->getID(), baseAddr));
        
        std::vector<uint8_t> data = doScratchRead(read);

        std::vector<uint8_t> payload = outstandingEventList_.find(put->getID())->second.remoteWrite->getPayload();
        uint32_t offset = addr - put->getSrcAddr();
        for (uint32_t i = 0; i < size; i++) {
            payload[i+offset] = data[i];
        }
        outstandingEventList_.find(put->getID())->second.remoteWrite->setPayload(payload);
        return false;
    }
}

void Scratchpad::updatePut(SST::Event::id_type putID) {
    uint32_t count = outstandingEventList_.find(putID)->second.decrementCount();
    if (count == 0) {
        dbg.debug(_L5_, "\tInserting event in memory queue. %s\n", outstandingEventList_.find(putID)->second.remoteWrite->getBriefString().c_str());
        memMsgQueue_.insert(std::make_pair(timestamp_, outstandingEventList_.find(putID)->second.remoteWrite));
        sendResponse(outstandingEventList_.find(putID)->second.response);
        delete outstandingEventList_.find(putID)->second.request;
        outstandingEventList_.erase(putID);
    }

}

void Scratchpad::updateGet(SST::Event::id_type getID) {
    uint32_t count = outstandingEventList_.find(getID)->second.decrementCount();
    if (count == 0) {
        sendResponse(outstandingEventList_.find(getID)->second.response);
        delete outstandingEventList_.find(getID)->second.request;
        outstandingEventList_.erase(getID);
    }
}

void Scratchpad::finishRequest(SST::Event::id_type requestID) {
    if (outstandingEventList_.find(requestID)->second.response)
        sendResponse(outstandingEventList_.find(requestID)->second.response);
    delete outstandingEventList_.find(requestID)->second.request;
    outstandingEventList_.erase(requestID);
}

uint32_t Scratchpad::deriveSize(Addr addr, Addr baseAddr, Addr requestAddr, uint32_t requestSize) {
    uint32_t size = baseAddr + scratchLineSize_ - addr;
    if (addr + size > requestAddr + requestSize) {
        size = requestAddr + requestSize - addr;
    }
    return size;
}
