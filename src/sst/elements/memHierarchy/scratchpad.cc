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
#include "scratchEvent.h"
#include "memEvent.h"
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
 *  ScratchGet      Read from remote memory into scratchpad
 *  ScratchPut      Write from scratchpad into remote memory    
 *  Remote Read     Read from remote memory                     remoteMemLineSize
 *  Remote Write    Write to remote memory                      remoteMemLineSize
 *
 *  Ordering:
 *  - Only concerned with requests for same address
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
    // cpu is a ScratchEvent interface, memory & network are MemEvent interfaces (memory is a direct connect while network uses SimpleNetwork)
    bool memoryDirect = isPortConnected("memory");
    bool memoryNetwork = isPortConnected("network");
    if (!isPortConnected("cpu")) {
        out.fatal(CALL_INFO, -1, "Invalid port configuration (%s): Scratchpad requires direct connection to CPU on 'cpu' port.\n", getName().c_str());
    } else if (!memoryDirect && !memoryNetwork) {
        out.fatal(CALL_INFO, -1, "Invalid port configuration (%s): Did not detect port for memory-side events. Connect either 'memory' or 'network'\n", getName().c_str());
    }
    
    linkUp_ = configureLink( "cpu", cpuLinkLatency, new Event::Handler<Scratchpad>(this, &Scratchpad::processIncomingScratchEvent));

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
        linkNet_ = new MemNIC(this, &dbg, -1, myInfo, new Event::Handler<Scratchpad>(this, &Scratchpad::processIncomingMemEvent));
        linkNet_->addTypeInfo(typeInfo);
        linkDown_ = nullptr;
    } else {
        linkDown_ = configureLink( "memory", memLinkLatency, new Event::Handler<Scratchpad>(this, &Scratchpad::processIncomingMemEvent));
        linkNet_ = nullptr;
    }


    // Initialize local variables
    timestamp_ = 0;
}


/* Events received from processor */
void Scratchpad::processIncomingScratchEvent(SST::Event* event) {
    ScratchEvent * ev = static_cast<ScratchEvent*>(event);
    
    ScratchCommand cmd = ev->getCmd();
    
    
    switch(cmd) {    
        case Read:
            if (ev->getAddr() < scratchSize_) handleScratchRead(ev);
            else handleRemoteRead(ev);
            break;
        case Write:
            if (ev->getAddr() < scratchSize_) handleScratchWrite(ev);
            else handleRemoteWrite(ev);
            break;
        case ScratchGet:
            handleScratchGet(ev);
            break;
        case ScratchPut:
            handleScratchPut(ev);
            break;
        default:
            dbg.fatal(CALL_INFO, -1, "Scratchpad (%s) received unhandled event: cmd = %s, cycles = %" PRIu64 ".\n", getName().c_str(), ScratchCommandString[cmd], timestamp_);
    }
}

/* Events received from memory - must be read response but could be response to a memory read or a ScratchGet */
void Scratchpad::processIncomingMemEvent(SST::Event * event) {
    MemEvent * ev = static_cast<MemEvent*>(event);
    std::map<SST::Event::id_type, SST::Event::id_type>::iterator idItr = remoteIDMap_.find(ev->getResponseToID());

    if (idItr == remoteIDMap_.end()) {
        dbg.fatal(CALL_INFO, -1, "(%s) Received data response from remote but no matching request in remoteIDMap_, id is (%" PRIu64 ", %" PRIu32 "), timestamp is %" PRIu64 "\n",
                getName().c_str(), ev->getResponseToID().first, ev->getResponseToID().second, timestamp_);
    }

    std::map<SST::Event::id_type, ScratchPair >::iterator reqItr = pendingPool_.find(idItr->second);

    if (reqItr == pendingPool_.end()) {
        dbg.fatal(CALL_INFO, -1, "(%s) Received data response from remote but no matching request in pendingPool, id is (%" PRIu64 ", %" PRIu32 "), timestamp is %" PRIu64 "\n",
                getName().c_str(), idItr->second.first, idItr->second.second, timestamp_);
    }
    
    ScratchEvent * reqEv = reqItr->second.request;

    // handle according to whether this is a read or a get
    if (reqEv->getCmd() == ScratchGet) {
    
        dbg.debug(_L3_, "%" PRIu64 "  (%s)  MemEvent received: cmd: %s, addr: %" PRIu64 ", src: %s, size: %" PRIu32 "\n\tMatches request: cmd: %s, src addr: %" PRIu64 ", dst addr: %" PRIu64 "\n", 
                timestamp_, getName().c_str(), CommandString[ev->getCmd()], reqEv->getAddr(), ev->getSrc().c_str(), ev->getSize(), ScratchCommandString[reqEv->getCmd()], reqEv->getAddr(), reqEv->getSrcAddr());


        // update backing
        if (backing_) {
            for (size_t i = 0; i < ev->getSize(); i++) {
                dbg.debug(_L5_, "\tUpdating backing store. (addr, new data): (%" PRIu64 ", %u)\n", reqEv->getAddr() + i, ev->getPayload()[i]);
                backing_->set(reqEv->getAddr() + i, ev->getPayload()[i] );
            }
        }

        // create scratch write(s)
        uint32_t bytesLeft = reqEv->getSize();
        Addr startAddr = reqEv->getAddr();
        int payloadOffset = 0;
        while (bytesLeft != 0) {
            Addr baseAddr = startAddr & ~(scratchLineSize_ - 1);
            uint32_t size = (baseAddr + scratchLineSize_) - startAddr;
            if (size > bytesLeft) size = bytesLeft;
            std::vector<uint8_t> data(ev->getPayload()[payloadOffset], ev->getPayload()[payloadOffset + size]); 

            ScratchEvent * write = new ScratchEvent(getName().c_str(), startAddr, baseAddr, Write, data);
            write->setRqstr(reqEv->getRqstr());
            write->setVirtualAddress(reqEv->getVirtualAddress());
            write->setInstructionPointer(reqEv->getInstructionPointer());
            bytesLeft -= size;
        
            dbg.debug(_L4_, "\tCreated scratch request. Cmd = %s, Addr = %" PRIu64 ", BaseAddr = %" PRIu64 ", Size = %" PRIu32 "\n", 
                    ScratchCommandString[write->getCmd()], write->getAddr(), write->getBaseAddr(), write->getSize());
        
            // Check basic ordering -> don't issue repeat requests for the same line - if we change this, we can't send ack to proc immediately since
            // the write(s) may not have been issued to the scratch when we send the ack
            if (scratchMSHR_.find(write->getBaseAddr()) == scratchMSHR_.end()) {
                dbg.debug(_L4_, "\tSending to scratch backend\n");
                stat_ScratchWriteIssued->addData(1);
                scratch_->handleScratchEvent(write);
            } else {
                dbg.debug(_L4_, "\tStalling in MSHR for conflicting access\n");
                scratchMSHR_.find(write->getBaseAddr())->second.push(write);
            }
        }

        // send ack to processor - if protocol is changed to not order reads/writes to same line @scratch then this ack should be delayed until all writes have been issued
        // TODO add timing
        ScratchEvent * respEv = reqEv->makeResponse();
        respEv->setSrc(getName());
        dbg.debug(_L4_, "\tSending processor response\n");
        procMsgQueue_.insert(std::make_pair(timestamp_, respEv));

    } else {
        dbg.debug(_L3_, "%" PRIu64 "  (%s)  MemEvent received: cmd: %s, addr: %" PRIu64 ", src: %s, size: %" PRIu32 "\n\tMatches request: cmd: %s, addr: %" PRIu64 "\n", 
                timestamp_, getName().c_str(), CommandString[ev->getCmd()], reqEv->getAddr(), ev->getSrc().c_str(), ev->getSize(), ScratchCommandString[reqEv->getCmd()], reqEv->getAddr());
        ScratchEvent * respEv = reqEv->makeResponse();
        respEv->setPayload(ev->getPayload());
        respEv->setSrc(getName());

        // send response to processor
        dbg.debug(_L4_, "\tSending processor response\n");
        procMsgQueue_.insert(std::make_pair(timestamp_, respEv));
    } 
        
    // clean up
    delete ev;
    delete reqEv;
    pendingPool_.erase(reqItr);
    remoteIDMap_.erase(idItr);
    
    dbg.debug(_L3_, "\n");
}
    

/***************** request and response handlers ***********************/

/*
 *  
 */
void Scratchpad::handleScratchRead(ScratchEvent * event) {
    dbg.debug(_L3_, "%" PRIu64 "  (%s)  Read (scratch) received: addr: %" PRIu64 ", src: %s, size: %" PRIu32 "\n",
            timestamp_, getName().c_str(), event->getAddr(), event->getSrc().c_str(), event->getSize());
    
    // set baseaddr
    event->setBaseAddr(event->getAddr() & ~(scratchLineSize_ - 1));
    
    // record statistics
    stat_ScratchReadReceived->addData(1);

    // create response, we'll do this as events arrive to avoid mis-ordering if we're backing the scratchpad
    ScratchEvent * respEvent = event->makeResponse();

    for (size_t i = 0; i < event->getSize(); i++) {
        respEvent->getPayload()[i] = !backing_ ? 0 : backing_->get(respEvent->getAddr() + i);
        if (backing_) dbg.debug(_L5_, "\tSetting response data payload from backing store. (addr, data): (%" PRIu64 ", %u)\n", event->getAddr() + i, event->getPayload()[i]);
    }

    // record request in global pending q so we can find the response later
    pendingPool_.insert(std::make_pair(event->getID(), ScratchPair(event, respEvent)));
    
    // check basic ordering -> don't issue repeat requests for the same line?
    if (scratchMSHR_.find(event->getBaseAddr()) == scratchMSHR_.end()) {
        dbg.debug(_L4_, "\tInserting into scratchMSHR, sending to scratch backend\n");
        stat_ScratchReadIssued->addData(1);
        scratch_->handleScratchEvent( event );
        std::queue<ScratchEvent*> newq;
        newq.push(event);
        scratchMSHR_.insert(std::make_pair(event->getBaseAddr(), newq));
    } else {
        dbg.debug(_L4_, "\tStalling in MSHR for conflicting access\n");
        scratchMSHR_.find(event->getBaseAddr())->second.push(event);
    }
    dbg.debug(_L3_, "\n");
}

/*
 *  Do not expect an ack for a write from the scratchpad so if we
 *  can issue this write immediately, also respond immediately
 */
void Scratchpad::handleScratchWrite(ScratchEvent * event) {
    dbg.debug(_L3_, "%" PRIu64 "  (%s)  Write (scratch) received: addr: %" PRIu64 ", src: %s, size: %" PRIu32 "\n",
            timestamp_, getName().c_str(), event->getAddr(), event->getSrc().c_str(), event->getSize());
    
    // set baseaddr
    event->setBaseAddr(event->getAddr() & ~(scratchLineSize_ - 1));
    
    // record statistics
    stat_ScratchWriteReceived->addData(1);

    // create response, we'll do this as events arrive to avoid mis-ordering if we're backing the scratchpad
    ScratchEvent * respEvent = event->makeResponse();
    respEvent->setSrc(getName());

    if (backing_) {
        for (size_t i = 0; i < event->getSize(); i++) {
            dbg.debug(_L5_, "\tUpdating backing store. (addr, new data): (%" PRIu64 ", %u)\n", event->getAddr() + i, event->getPayload()[i]);
            backing_->set (event->getAddr() + i, event->getPayload()[i] );
        }
    }
    
    // check basic ordering -> don't issue repeat requests for the same line
    // TODO add timing
    if (scratchMSHR_.find(event->getBaseAddr()) == scratchMSHR_.end()) {
        dbg.debug(_L4_, "\tSending to scratch backend\n");
        stat_ScratchWriteIssued->addData(1);
        scratch_->handleScratchEvent(event);
    } else {
        dbg.debug(_L4_, "\tStalling in MSHR for conflicting access\n");
        scratchMSHR_.find(event->getBaseAddr())->second.push(event);
    }
    
    dbg.debug(_L4_, "\tSending processor response\n");
    procMsgQueue_.insert(std::make_pair(timestamp_, respEvent));
    dbg.debug(_L3_, "\n");
}

/*
 * Handle ScratchGet
 *  Send memEvent to memory
 */
void Scratchpad::handleScratchGet(ScratchEvent * event) {
    dbg.debug(_L3_, "%" PRIu64 "  (%s)  ScratchGet received: src addr: %" PRIu64 ", dst addr: %" PRIu64 ", src: %s, size: %" PRIu32 "\n",
            timestamp_, getName().c_str(), event->getSrcAddr(), event->getAddr(), event->getSrc().c_str(), event->getSize());
    
    // set baseaddrs
    event->setBaseAddr(event->getAddr() & ~(scratchLineSize_ - 1));
    event->setSrcBaseAddr(event->getSrcAddr() & ~(remoteLineSize_ - 1));

    // record statistics
    stat_ScratchGetReceived->addData(1);
    
    // create memory request -> MemEvent so it's compatible with current memcontroller
    MemEvent * memRequest = new MemEvent(this, event->getSrcAddr() - remoteAddrOffset_, event->getSrcBaseAddr() - remoteAddrOffset_, GetS, event->getSize());
    memRequest->setFlag(MemEvent::F_NONCACHEABLE); // ensure this GetS treated like a non-cacheable read (so we want to use addr not baseaddr)
    memRequest->setRqstr(event->getRqstr());
    memRequest->setVirtualAddress(event->getVirtualAddress());
    memRequest->setInstructionPointer(event->getInstructionPointer());

    // record requests
    pendingPool_.insert(std::make_pair(event->getID(), ScratchPair(event, (ScratchEvent*)nullptr)));
    remoteIDMap_.insert(std::make_pair(memRequest->getID(), event->getID()));

    // Assume the network orders requests from the same src so I don't have to worry about a prior write coming after this read
    // TODO make sure this is a valid assumption!
    uint64_t deliveryTime = timestamp_;
    dbg.debug(_L4_, "\tSending memory request. Cmd = %s, Addr = %" PRIu64 ", BaseAddr = %" PRIu64 ", Size = %" PRIu32 ", send at cycle %" PRIu64 "\n", 
            CommandString[memRequest->getCmd()], memRequest->getAddr(), memRequest->getBaseAddr(), memRequest->getSize(), deliveryTime);
    memMsgQueue_.insert(std::make_pair(deliveryTime, memRequest));
    dbg.debug(_L3_, "\n");
}


void Scratchpad::handleScratchPut(ScratchEvent * event) {
    dbg.debug(_L3_, "%" PRIu64 "  (%s)  ScratchPut received: src addr: %" PRIu64 ", dst addr: %" PRIu64 ", src: %s, size: %" PRIu32 "\n",
            timestamp_, getName().c_str(), event->getSrcAddr(), event->getAddr(), event->getSrc().c_str(), event->getSize());
    
    // set baseaddrs
    event->setBaseAddr(event->getAddr() &~(scratchLineSize_ - 1));
    event->setSrcBaseAddr(event->getSrcAddr() &~(remoteLineSize_ -1));

    // record statistics
    stat_ScratchPutReceived->addData(1);

    // create memory request
    MemEvent * memRequest = new MemEvent(this, event->getAddr() - remoteAddrOffset_, event->getBaseAddr() - remoteAddrOffset_, GetX, event->getSize());
    memRequest->setFlag(MemEvent::F_NONCACHEABLE);
    memRequest->setFlag(MemEvent::F_NORESPONSE); // tell memory not to ack this
    memRequest->setRqstr(event->getRqstr());
    memRequest->setVirtualAddress(event->getVirtualAddress());
    memRequest->setInstructionPointer(event->getInstructionPointer());

    for (size_t i = 0; i < event->getSize(); i++) {
        memRequest->getPayload()[i] = !backing_ ? 0 : backing_->get(event->getSrcAddr() + i);
        if (backing_) dbg.debug(_L5_, "\tSetting memory write data payload from backing store. (addr, data): (%" PRIu64 ", %u)\n", event->getSrcAddr() + i, memRequest->getPayload()[i]);
    }

    pendingPool_.insert(std::make_pair(event->getID(), ScratchPair(event, memRequest)));

    // create scratch request(s)
    // keep track of which scratch requests map to a put and how many requests we're waiting for
    uint32_t bytesLeft = event->getSize();
    Addr startAddr = event->getSrcAddr();
    uint64_t reqCount = 0;
    while (bytesLeft != 0) {
        Addr baseAddr = startAddr & ~(scratchLineSize_ - 1);
        uint32_t size = (baseAddr + scratchLineSize_) - startAddr;
        if (size > bytesLeft) size = bytesLeft;
        ScratchEvent * read = new ScratchEvent(getName().c_str(), startAddr, baseAddr, Read, size);
        read->setRqstr(event->getRqstr());
        read->setVirtualAddress(event->getVirtualAddress());
        read->setInstructionPointer(event->getInstructionPointer());
        bytesLeft -= size;
        
        dbg.debug(_L4_, "\tCreated scratch request. Cmd = %s, Addr = %" PRIu64 ", BaseAddr = %" PRIu64 ", Size = %" PRIu32 "\n", 
                ScratchCommandString[read->getCmd()], read->getAddr(), read->getBaseAddr(), read->getSize());
        // check basic ordering -> don't issue repeat requests for the same line
        if (scratchMSHR_.find(baseAddr) == scratchMSHR_.end()) {
            dbg.debug(_L4_, "\tInserting into scratchMSHR, sending to scratch backend\n");
            stat_ScratchReadIssued->addData(1);
            scratch_->handleScratchEvent(read);
            std::queue<ScratchEvent*> newq;
            newq.push(read);
            scratchMSHR_.insert(std::make_pair(baseAddr, newq));
        } else {
            dbg.debug(_L4_, "\tStalling in MSHR for conflicting access\n");
            scratchMSHR_.find(baseAddr)->second.push(read);
        }
        scratchIDMap_.insert(std::make_pair(read->getID(), std::make_pair(event->getID(), read->getBaseAddr()))); // so we can map it back
        reqCount++;
    }
    
    // record how many reads the ScratchPut is waiting for
    scratchCounters_.insert(std::make_pair(event->getID(), reqCount));
    dbg.debug(_L3_, "\n");
    
}

void Scratchpad::handleRemoteRead(ScratchEvent * event) {
    dbg.debug(_L3_, "%" PRIu64 "  (%s)  Read (remote) received: addr: %" PRIu64 ", src: %s, size: %" PRIu32 "\n",
            timestamp_, getName().c_str(), event->getAddr(), event->getSrc().c_str(), event->getSize());
    
    // record statistics
    stat_RemoteReadReceived->addData(1);
    
    // convert scratch to noncacheable MemEvent
    MemEvent * memRequest = new MemEvent(this, event->getAddr() - remoteAddrOffset_, event->getBaseAddr() - remoteAddrOffset_, GetS, event->getSize());
    memRequest->setFlag(MemEvent::F_NONCACHEABLE);
    memRequest->setRqstr(event->getRqstr());
    memRequest->setVirtualAddress(event->getVirtualAddress());
    memRequest->setInstructionPointer(event->getInstructionPointer());

    // record request
    remoteIDMap_.insert(std::make_pair(memRequest->getID(), event->getID()));
    pendingPool_.insert(std::make_pair(event->getID(), ScratchPair(event, (MemEvent*)nullptr)));
    
    uint64_t deliveryTime = timestamp_;
    dbg.debug(_L4_, "\tSending memory request. Cmd = %s, Addr = %" PRIu64 ", BaseAddr = %" PRIu64 ", Size = %" PRIu32 ", send at cycle %" PRIu64 "\n", 
            CommandString[memRequest->getCmd()], memRequest->getAddr(), memRequest->getBaseAddr(), memRequest->getSize(), deliveryTime);
    memMsgQueue_.insert(std::make_pair(deliveryTime, memRequest));
    
    dbg.debug(_L3_, "\n");
}

void Scratchpad::handleRemoteWrite(ScratchEvent * event) {
    dbg.debug(_L3_, "%" PRIu64 "  (%s)  Write (remote) received: addr: %" PRIu64 ", src: %s, size: %" PRIu32 "\n",
            timestamp_, getName().c_str(), event->getAddr(), event->getSrc().c_str(), event->getSize());
    
    // record statistics
    stat_RemoteWriteReceived->addData(1);
    
    // convert scratch to noncacheable MemEvent
    MemEvent * memRequest = new MemEvent(this, event->getAddr() - remoteAddrOffset_, event->getBaseAddr() - remoteAddrOffset_, GetX, event->getSize());
    memRequest->setFlag(MemEvent::F_NONCACHEABLE);
    memRequest->setFlag(MemEvent::F_NORESPONSE);
    memRequest->setRqstr(event->getRqstr());
    memRequest->setVirtualAddress(event->getVirtualAddress());
    memRequest->setInstructionPointer(event->getInstructionPointer());

    uint64_t deliveryTime = timestamp_;
    dbg.debug(_L4_, "\tSending memory request. Cmd = %s, Addr = %" PRIu64 ", BaseAddr = %" PRIu64 ", Size = %" PRIu32 ", send at cycle %" PRIu64 "\n", 
            CommandString[memRequest->getCmd()], memRequest->getAddr(), memRequest->getBaseAddr(), memRequest->getSize(), deliveryTime);
    memMsgQueue_.insert(std::make_pair(deliveryTime, memRequest));

    ScratchEvent * respEv = event->makeResponse();
    respEv->setSrc(getName());
    dbg.debug(_L4_, "\tSending processor response\n");
    procMsgQueue_.insert(std::make_pair(timestamp_, respEv));
    dbg.debug(_L3_, "\n");
}


void Scratchpad::handleScratchResponse(SST::Event::id_type id) {
    std::map<SST::Event::id_type, ScratchPair>::iterator respItr = pendingPool_.find(id);
    
    
    // response to reads generated by a put request
    if (respItr == pendingPool_.end()) { 
        std::map<SST::Event::id_type, std::pair<SST::Event::id_type, Addr> >::iterator it = scratchIDMap_.find(id);
        if (it == scratchIDMap_.end()) {
            dbg.fatal(CALL_INFO, -1, "Received data response from scratch but no matching request in scratchIDMap_\n");
        }  
    
        SST::Event::id_type putID = it->second.first;
        scratchCounters_.find(putID)->second--; // decrement count of reads we are waiting for
        
        ScratchEvent * debugEv = pendingPool_.find(putID)->second.request;
        dbg.debug(_L3_, "%" PRIu64 "  (%s)  Scratch response received: request cmd: %s, addr: %" PRIu64 ", src: addr %" PRIu64 ", size: %" PRIu32 "\n",
            timestamp_, getName().c_str(), ScratchCommandString[debugEv->getCmd()], debugEv->getAddr(), debugEv->getSrcAddr(), debugEv->getSize());
        
        // Clean up - remove from scratchMSHR & scratchIDMap
        Addr bAddr = it->second.second;
        removeFromMSHR(bAddr);
        scratchIDMap_.erase(it);

        // check if count has reached 0, if so, complete put
        if (scratchCounters_.find(putID)->second == 0) {
            ScratchEvent * reqEv = pendingPool_.find(putID)->second.request;
            MemEvent * memRequest = pendingPool_.find(putID)->second.memRequest;
            dbg.debug(_L4_, "\tScratchPut complete, send memory write and processor response\n");


            // send write to memory
            dbg.debug(_L4_, "\tSending memory write. Cmd = %s, Addr = %" PRIu64 ", BaseAddr = %" PRIu64 ", Size = %" PRIu32 ", send at cycle %" PRIu64 "\n", 
                CommandString[memRequest->getCmd()], memRequest->getAddr(), memRequest->getBaseAddr(), memRequest->getSize(), timestamp_);
            memMsgQueue_.insert(std::make_pair(timestamp_, memRequest));

            // send ack to processor
            ScratchEvent * respEv = reqEv->makeResponse();
            respEv->setSrc(getName());
            dbg.debug(_L4_, "\tSending processor response\n");
            procMsgQueue_.insert(std::make_pair(timestamp_, respEv));

            // clean up
            scratchCounters_.erase(putID);
            pendingPool_.erase(putID);
            delete reqEv;
        }

    } else { // response to a read request
        ScratchEvent * reqEv = respItr->second.request;
        ScratchEvent * respEv = respItr->second.response;
        respEv->setSrc(getName());
        dbg.debug(_L3_, "%" PRIu64 "  (%s)  Scratch response received: request cmd: %s, addr: %" PRIu64 ", src: %s, size: %" PRIu32 "\n",
            timestamp_, getName().c_str(), ScratchCommandString[respEv->getCmd()], respEv->getAddr(), respEv->getDst().c_str(), respEv->getSize());
        
        // return response from pending queue
        dbg.debug(_L4_, "\tSending processor response\n");
        procMsgQueue_.insert(std::make_pair(timestamp_, respEv));
        
        // remove request from mshr 
        removeFromMSHR(respEv->getBaseAddr());

        // clean up
        pendingPool_.erase(respItr);
    }
    dbg.debug(_L3_, "\n");
}

// clock
bool Scratchpad::clock(Cycle_t cycle) {
    timestamp_++;
    bool sent = false; // For debugging pretty printing only

    // issue ready events
    uint32_t responseThisCycle = (responsesPerCycle_ == 0) ? 1 : 0;
    while (!procMsgQueue_.empty() && procMsgQueue_.begin()->first < timestamp_) {
        ScratchEvent * sendEv = procMsgQueue_.begin()->second;
        dbg.debug(_L4_, "%" PRIu64 "  (%s)  Clock. Sending event from procMsgQueue: cmd: %s, addr: %" PRIu64 ", size: %" PRIu32 "\n", 
                timestamp_, getName().c_str(), ScratchCommandString[sendEv->getCmd()], sendEv->getAddr(), sendEv->getSize());
        linkUp_->send(sendEv);
        procMsgQueue_.erase(procMsgQueue_.begin());
        sent = true;
        responseThisCycle++;
        if (responseThisCycle == responsesPerCycle_) break;
    }

    while (!memMsgQueue_.empty() && memMsgQueue_.begin()->first < timestamp_) {
        MemEvent * sendEv = memMsgQueue_.begin()->second;
        dbg.debug(_L4_, "%" PRIu64 "  (%s)  Clock. Sending event from memMsgQueue: cmd: %s, addr: %" PRIu64 ", size: %" PRIu32 "\n", 
                timestamp_, getName().c_str(), CommandString[sendEv->getCmd()], sendEv->getAddr(), sendEv->getSize());
        if (linkDown_) {
            linkDown_->send(sendEv);
        } else {
            sendEv->setDst(linkNet_->findTargetDestination(sendEv->getBaseAddr()));
            sendEv->setAddr(linkNet_->convertToDestinationAddress(sendEv->getDst(), sendEv->getAddr()));
            sendEv->setBaseAddr(linkNet_->convertToDestinationAddress(sendEv->getDst(), sendEv->getBaseAddr()));
            dbg.debug(_L4_, "\t\tSet destination/addresses to: dst: %s, addr: %" PRIu64 ", baseAddr: %" PRIu64 "\n",
                    sendEv->getDst().c_str(), sendEv->getAddr(), sendEv->getBaseAddr());
            linkNet_->send(sendEv);
        }
        memMsgQueue_.erase(memMsgQueue_.begin());
        sent = true;
    }
    if (sent) dbg.debug(_L4_, "\n");
    

    // clock nic
    if (linkNet_) linkNet_->clock();

    // clock backend
    scratch_->clock(cycle);
    
    return false;
}

void Scratchpad::removeFromMSHR(Addr addr) {
    
    // Locate entry
    std::unordered_map<Addr, std::queue<ScratchEvent*> >::iterator mshrItr = scratchMSHR_.find(addr);

    // Remove top event from MSHR
    mshrItr->second.pop();
    
    // Issue waiting request(s)
    while (!mshrItr->second.empty()) {
        ScratchEvent * reqEvent = mshrItr->second.front();
        dbg.debug(_L4_, "\tSending stalled event to scratch backend. Cmd = %s, Addr = %" PRIu64 ", BaseAddr = %" PRIu64 ", Size = %" PRIu32 "\n",
                ScratchCommandString[reqEvent->getCmd()], reqEvent->getAddr(), reqEvent->getBaseAddr(), reqEvent->getSize());
        reqEvent->getCmd() == Read ? stat_ScratchReadIssued->addData(1) : stat_ScratchWriteIssued->addData(1);
        scratch_->handleScratchEvent(reqEvent);
        if (reqEvent->getCmd() == Read) break;
        mshrItr->second.pop(); 
    }

    // Finally, erase entry if queue is empty
    if (mshrItr->second.empty()) scratchMSHR_.erase(mshrItr);
}

// SST component functions
void Scratchpad::init(unsigned int phase) {
    // Init MemNIC if we have one
    if (linkNet_) linkNet_->init(phase);

    // Send initial info out
    if (!phase) {
        linkUp_->sendInitData(new SST::Interfaces::StringEvent("SST::MemHierarchy::ScratchEvent"));
    }

    // Handle incoming requests
    while (SST::Event *ev = linkUp_->recvInitData()) {
        ScratchEvent *sEv = dynamic_cast<ScratchEvent*>(ev);
        if (sEv && sEv->getCmd() != ScratchNullCmd) {
            MemEvent * memRequest = new MemEvent(this, sEv->getAddr(), sEv->getBaseAddr(), sEv->getCmd() == Read ? GetS : GetX, sEv->getSize());
            // debug.dbg(_L3_, "%s received init event: Cmd: %s, Addr: %" PRIu64 ", BaseAddr: %" PRIu64 ", Size: %lu\n", 
            //          CommandString[memRequest->getCmd()], memRequest->getAddr(), memRequest->getBaseAddr, memRequest->getSize());
            //  debug.dbg(_L5_, "Payload: ");
            //  for (std::vector<uint8_t>::iterator it = memRequest->getPayload().begin(); it != memRequest->getPayload().end()) {
            //      debug.dbg(_L5_, "%u ", *it);
            //  }
            //  debug.dbg(_L5_, "\n");
            if (linkNet_) {
                memRequest->setDst(linkNet_->findTargetDestination(memRequest->getBaseAddr() - remoteAddrOffset_));
                memRequest->setAddr(linkNet_->convertToDestinationAddress(memRequest->getDst(), memRequest->getAddr() - remoteAddrOffset_));
                memRequest->setBaseAddr(linkNet_->convertToDestinationAddress(memRequest->getDst(), memRequest->getBaseAddr() - remoteAddrOffset_));
                // debug.dbg(_L4_, "%s sending init event: Cmd: %s, Addr: %" PRIu64 ", BaseAddr: %" PRIu64 ", Size: %lu, Destination: %s\n",
                //          CommandString[memRequest->getCmd()], memRequest->getAddr(), memRequest->getBaseAddr, memRequest->getSize(), memRequest->getDst().c_str());
                linkNet_->sendInitData(memRequest);
            } else {
                // debug.dbg(_L4_, "%s sending init event: Cmd: %s, Addr: %" PRIu64 ", BaseAddr: %" PRIu64 ", Size: %lu, Destination: %s\n",
                //          CommandString[memRequest->getCmd()], memRequest->getAddr(), memRequest->getBaseAddr, memRequest->getSize(), memRequest->getDst().c_str());
                linkDown_->sendInitData(memRequest);
            }
        }
        delete sEv;
    }
}

void Scratchpad::setup() {}

void Scratchpad::finish() {}
