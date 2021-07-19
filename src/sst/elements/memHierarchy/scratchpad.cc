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
#include <sst/core/params.h>
#include <sst/core/simulation.h>

#include "scratchpad.h"
#include "membackend/scratchBackendConvertor.h"
#include "util.h"
#include <sst/core/interfaces/stringEvent.h>
#include "memLink.h"
#include "memNIC.h"

using namespace std;
using namespace SST;
using namespace SST::MemHierarchy;

/* Debug macros */
#ifdef __SST_DEBUG_OUTPUT__ /* From sst-core, enable with --enable-debug */
#define is_debug_addr(addr) (DEBUG_ADDR.empty() || DEBUG_ADDR.find(addr) != DEBUG_ADDR.end())
#define is_debug_event(ev) (DEBUG_ADDR.empty() || ev->doDebug(DEBUG_ADDR))
#else
#define is_debug_addr(addr) false
#define is_debug_event(ev) false
#endif

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
 */

Scratchpad::Scratchpad(ComponentId_t id, Params &params) : Component(id) {
    // Output
    int debugLevel = params.find<int>("debug_level", 0);
    dbg.init("", debugLevel, 0, (Output::output_location_t)params.find<int>("debug", 0));
    if (debugLevel < 0 || debugLevel > 10)
        dbg.fatal(CALL_INFO, -1, "Debugging level must be between 0 and 10\n");

    out.init("", 1, 0, Output::STDOUT);

    std::vector<Addr> addrArr;
    params.find_array<Addr>("debug_addr", addrArr);
    for (std::vector<Addr>::iterator it = addrArr.begin(); it != addrArr.end(); it++)
        DEBUG_ADDR.insert(*it);

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

    // Remote address computation
    remoteAddrOffset_ = params.find<uint64_t>("memory_addr_offset", scratchSize_);

    // Create backend
    scratch_ = loadUserSubComponent<ScratchBackendConvertor>("backendConvertor");

    if (!scratch_) {
        // Create backend which will handle the timing for scratchpad
        std::string bkName = params.find<std::string>("backendConvertor", "memHierarchy.scratchBackendConvertor");

        // Copy some parameters into the backend
        Params bkParams = params.get_scoped_params("backendConvertor");
        bkParams.insert("backend.clock", clock_freq);
        bkParams.insert("backend.request_width", std::to_string(scratchLineSize_));
        bkParams.insert("backend.mem_size", size.toString());

        scratch_ = loadAnonymousSubComponent<ScratchBackendConvertor>(bkName, "backendConvertor", 0, ComponentInfo::SHARE_PORTS | ComponentInfo::INSERT_STATS, bkParams);
    }

    using std::placeholders::_1;
    scratch_->setCallbackHandler(std::bind( &Scratchpad::handleScratchResponse, this, _1 ));

    // Initialize scratchpad entries
    // Set up backing store if needed
    std::string backingType = params.find<std::string>("backing", "mmap", found); /* Default to using an mmap backing store, fall back on malloc */
    backing_ = nullptr;
    if (!found) {
        bool oldBackVal = params.find<bool>("do_not_back", false, found);
        if (found) {
            out.output("%s, ** Found deprecated parameter: do_not_back ** Use 'backing' parameter instead and specify 'none', 'malloc', or 'mmap'. Remove this parameter from your input deck to eliminate this message.\n",
                    getName().c_str());
        }
        if (oldBackVal) backingType = "none";
    }

    if (backingType != "none" && backingType != "mmap" && backingType != "malloc") {
        out.fatal(CALL_INFO, -1, "%s, Error - Invalid param: backing. Must be one of 'none', 'malloc', or 'mmap'. You specified: %s\n",
                getName().c_str(), backingType.c_str());
    }

    std::string mallocSize = params.find<std::string>("backing_size_unit", "1MiB");
    UnitAlgebra size_ua(mallocSize);
    if (!size_ua.hasUnits("B")) {
        out.fatal(CALL_INFO, -1, "%s, Error - Invalid param: backing_size_unit. Must have units of bytes (B). SI ok. You specified: %s\n",
                getName().c_str(), mallocSize.c_str());
    }
    size_t sizeBytes = size_ua.getRoundedValue();
    if (sizeBytes > scratch_->getMemSize()) {
        // Since getMemSize() might not be a power of 2, but malloc store needs it....get a reasonably close power of 2
        sizeBytes = 1 << log2Of(scratch_->getMemSize());
    }

    backing_ = nullptr;
    if (backingType == "mmap") {
        std::string memoryFile = params.find<std::string>("memory_file", "");

        if ( 0 == memoryFile.compare( "" ) ) {
            memoryFile.clear();
        }
        try {
            backing_ = new Backend::BackingMMAP( memoryFile, scratch_->getMemSize() );
        }
        catch ( int e) {
            if (e == 1)
                dbg.fatal(CALL_INFO, -1, "%s, Error - unable to open memory_file. You specified '%s'.\n", getName().c_str(), memoryFile.c_str());
            else if (e == 2) {
                out.output("%s, Could not MMAP backing store (likely, simulated memory exceeds real memory). Creating malloc based store instead.\n", getName().c_str());
                backing_ = new Backend::BackingMalloc(sizeBytes);
            } else
                dbg.fatal(CALL_INFO, -1, "%s, Error - unable to create backing store. Exception thrown is %d.\n", getName().c_str(), e);
        }
    } else if (backingType == "malloc") {
        backing_ = new Backend::BackingMalloc(sizeBytes);
    }

    // Assume no caching, may change during init
    caching_ = false;
    directory_ = false;

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

    // Figure out port connections and set up links
    // Options: cpu and network; or cpu and memory;
    // cpu is a MoveEvent interface, memory & network are MemEvent interfaces (memory is a direct connect while network uses SimpleNetwork)
    bool memoryDirect = isPortConnected("memory");
    bool scratchNetwork = isPortConnected("network");
    bool cpuDirect = isPortConnected("cpu");
    if (!cpuDirect && !scratchNetwork) {
        out.fatal(CALL_INFO, -1, "Invalid port configuration (%s): Did not detect port for cpu-side events. Connect either 'cpu' or 'network'\n", getName().c_str());
    } else if (!memoryDirect && !scratchNetwork) {
        out.fatal(CALL_INFO, -1, "Invalid port configuration (%s): Did not detect port for memory-side events. Connect either 'memory' or 'network'\n", getName().c_str());
    } else if (cpuDirect && scratchNetwork && memoryDirect) {
        out.fatal(CALL_INFO, -1, "Invalid port configuration (%s): Too many connected ports. Connect either 'cpu' or 'network' for cpu-side events and either 'memory' or 'network' for memory-side events\n",
                getName().c_str());
    }

    if (cpuDirect) {
        Params cpulink = params.get_scoped_params("cpulink");
        cpulink.insert("port", "cpu");
        linkUp_ = loadAnonymousSubComponent<MemLinkBase>("memHierarchy.MemLink", "cpulink", 0, ComponentInfo::SHARE_PORTS | ComponentInfo::INSERT_STATS, cpulink);
        linkUp_->setRecvHandler(new Event::Handler<Scratchpad>(this, &Scratchpad::processIncomingCPUEvent));
    }
    if (memoryDirect) {
        Params memlink = params.get_scoped_params("memlink");
        memlink.insert("port", "memory");
        linkDown_ = loadAnonymousSubComponent<MemLinkBase>("memHierarchy.MemLink", "memlink", 0, ComponentInfo::SHARE_PORTS | ComponentInfo::INSERT_STATS, memlink);
        linkDown_->setRecvHandler(new Event::Handler<Scratchpad>(this, &Scratchpad::processIncomingRemoteEvent));
    }

    if (scratchNetwork) {
        // Fix up parameters for nic params & warn that we're doing it
        if (fixupParam(params, "network_bw", "memNIC.network_bw"))
            out.output(CALL_INFO, "Note (%s): Changed 'network_bw' to 'memNIC.network_bw' in params. Change your input file to remove this notice.\n", getName().c_str());
        if (fixupParam(params, "network_input_buffer_size", "memNIC.network_input_buffer_size"))
            out.output(CALL_INFO, "Note (%s): Changed 'network_input_buffer_size' to 'memNIC.network_input_buffer_size' in params. Change your input file to remove this notice.\n", getName().c_str());
        if (fixupParam(params, "network_output_buffer_size", "memNIC.network_output_buffer_size"))
            out.output(CALL_INFO, "Note (%s): Changed 'network_output_buffer_size' to 'memNIC.network_output_buffer_size' in params. Change your input file to remove this notice.\n", getName().c_str());
        if (fixupParam(params, "min_packet_size", "memNIC.min_packet_size"))
            out.output(CALL_INFO, "Note (%s): Changed 'min_packet_size' to 'memNIC.min_packet_size' in params. Change your input file to remove this notice.\n", getName().c_str());

        // These are defaults and will not overwrite user provided
        Params nicParams = params.get_scoped_params("memNIC");
        nicParams.insert("addr_range_start", "0", false);
        nicParams.insert("addr_range_end", std::to_string((uint64_t) - 1), false);
        nicParams.insert("interleave_size", "0B", false);
        nicParams.insert("interleave_step", "0B", false);
        nicParams.insert("port", "network");
        nicParams.insert("group", "3", false); // 3 is the default for anything that talks to memory but this can be set by user too so don't overwrite

        if (!memoryDirect) { /* Connect mem side to network */
            linkDown_ = loadAnonymousSubComponent<MemLinkBase>("memHierarchy.MemNIC", "memlink", 0, ComponentInfo::SHARE_PORTS | ComponentInfo::INSERT_STATS, nicParams);
            linkDown_->setRecvHandler(new Event::Handler<Scratchpad>(this, &Scratchpad::processIncomingRemoteEvent));
            if (!cpuDirect) {
                linkUp_ = linkDown_; /* Connect cpu side to same network */
                linkDown_->setRecvHandler(new Event::Handler<Scratchpad>(this, &Scratchpad::processIncomingNetworkEvent));
            }
        } else {
            linkUp_ = loadAnonymousSubComponent<MemLinkBase>("memHierarchy.MemNIC", "cpulink", 0, ComponentInfo::SHARE_PORTS | ComponentInfo::INSERT_STATS, nicParams);
            linkUp_->setRecvHandler(new Event::Handler<Scratchpad>(this, &Scratchpad::processIncomingCPUEvent));
        }
    }

    // Initialize local variables
    timestamp_ = 0;
}

/* Init
 * 1. Initialize network if present
 * 2. Inform our neighbors who we are (line size, endpoint type, whether to expect writeback acks, etc.)
 * 3. Discover who our neighbors are, especially if any are caches and we need to be sending shoot downs
 * 4. Forward any memory initialization events to memory
 */
void Scratchpad::init(unsigned int phase) {

    //Init MemNIC if we have one - must be done before attempting to send anything on this link
    linkUp_->init(phase);
    if (linkUp_ != linkDown_) linkDown_->init(phase);

    // Send initial info out
    if (!phase) {
        linkDown_->sendInitData(new MemEventInitCoherence(getName(), Endpoint::Scratchpad, true, true, scratchLineSize_, true));
        if (linkUp_ != linkDown_) linkUp_->sendInitData(new MemEventInitCoherence(getName(), Endpoint::Scratchpad, true, true, scratchLineSize_, true));
    }

    // Handle incoming events
    while (MemEventInit *initEv = linkUp_->recvInitData()) {
        if (initEv->getCmd() == Command::NULLCMD) {
#ifdef __SST_DEBUG_OUTPUT__
            dbg.debug(_L10_, "I: %-20s   Event:Init      (%s)\n",
                    getName().c_str(), initEv->getVerboseString().c_str());
#endif
            if (initEv->getInitCmd() == MemEventInit::InitCommand::Coherence) {
                MemEventInitCoherence * initEvC = static_cast<MemEventInitCoherence*>(initEv);
                if (initEvC->getType() == Endpoint::Cache) {
                    caching_ = true;
                    cacheStatus_.resize(scratchSize_/scratchLineSize_, false);
                } else if (initEvC->getType() == Endpoint::Directory) {
                    caching_ = true;
                    directory_ = true;
                    cacheStatus_.resize(scratchSize_/scratchLineSize_, false);
                }
            }
        } else { // Not a NULLCMD
            MemEventInit * memRequest = new MemEventInit(getName(), initEv->getCmd(), initEv->getAddr() - remoteAddrOffset_, initEv->getPayload());
            memRequest->setDst(linkDown_->findTargetDestination(memRequest->getAddr()));
            linkDown_->sendInitData(memRequest);
        }
        delete initEv;
    }
    // Handle incoming events
    while (MemEventInit *initEv = linkDown_->recvInitData()) {
        if (initEv->getCmd() == Command::NULLCMD) {
            dbg.debug(_L10_, "I: %-20s   Event:Init      (%s)\n",
                    getName().c_str(), initEv->getVerboseString().c_str());
        }
        delete initEv;
    }
}


/* setup. Empty for now */
void Scratchpad::setup() { }


/*
 * Events received from a network connection.
 * May be processor-side and/or remote memory
 * Sort into process function according to source
 */
void Scratchpad::processIncomingNetworkEvent(SST::Event* event) {
    MemEventBase * ev = static_cast<MemEventBase*>(event);
    if (ev->getCmd() == Command::GetSResp || ev->getCmd() == Command::GetXResp)
        processIncomingRemoteEvent(event);
    else
        processIncomingCPUEvent(event);
}


/*
 * Handle events received from a processor
 * - Read/write requests - remote and scratch
 * - Put/get requests
 * - (TODO) Flush requests
 * - Shootdown acknowledgements
 * - Acks
 */
void Scratchpad::processIncomingCPUEvent(SST::Event* event) {
    MemEventBase * ev = static_cast<MemEventBase*>(event);

    if (is_debug_event(ev))
        dbg.debug(_L3_, "E: %-20" PRIu64 " %-20" PRIu64 " %-20s Event:New     (%s)\n",
                Simulation::getSimulation()->getCurrentSimCycle(), timestamp_, getName().c_str(), ev->getVerboseString().c_str());

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
        case Command::NACK:
            handleNack(ev);
            break;
        default:
            dbg.fatal(CALL_INFO, -1, "Scratchpad (%s) received unhandled event: cmd = %s, cycles = %" PRIu64 ".\n", getName().c_str(), CommandString[(int)cmd], timestamp_);
    }
}


/*
 * Handle events received from remote memory
 * Writes are not ack'd so must be read responses, could be response
 * to a memory read or a ScratchGet
 */
void Scratchpad::processIncomingRemoteEvent(SST::Event * event) {
    MemEvent * ev = static_cast<MemEvent*>(event);

    if (is_debug_event(ev))
        dbg.debug(_L3_, "E: %-20" PRIu64 " %-20" PRIu64 " %-20s Event:New     (%s)\n",
                Simulation::getSimulation()->getCurrentSimCycle(), timestamp_, getName().c_str(), ev->getVerboseString().c_str());

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


/*
 * Clock handler
 * TODO turn off clock when idle
 */
bool Scratchpad::clock(Cycle_t cycle) {
    timestamp_++;

    bool debug = false;

    // issue ready events
    uint32_t responseThisCycle = (responsesPerCycle_ == 0) ? 1 : 0;
    while (!procMsgQueue_.empty() && procMsgQueue_.begin()->first < timestamp_) {
        MemEventBase * sendEv = procMsgQueue_.begin()->second;

        if (is_debug_event(sendEv)) {
            debug = true;
            dbg.debug(_L4_, "E: %-20" PRIu64 " %-20" PRIu64 " %-20s Event:Send    (%s)\n",
                    Simulation::getSimulation()->getCurrentSimCycle(), timestamp_, getName().c_str(), sendEv->getBriefString().c_str());
        }

        linkUp_->send(sendEv);
        procMsgQueue_.erase(procMsgQueue_.begin());
        responseThisCycle++;
        if (responseThisCycle == responsesPerCycle_) break;
    }

    while (!memMsgQueue_.empty() && memMsgQueue_.begin()->first < timestamp_) {
        MemEvent * sendEv = memMsgQueue_.begin()->second;
        sendEv->setDst(linkDown_->findTargetDestination(sendEv->getBaseAddr()));

        if (is_debug_event(sendEv)) {
            debug = true;
            dbg.debug(_L4_, "E: %-20" PRIu64 " %-20" PRIu64 " %-20s Event:Send    (%s)\n",
                    Simulation::getSimulation()->getCurrentSimCycle(), timestamp_, getName().c_str(), sendEv->getBriefString().c_str());
        }

        linkDown_->send(sendEv);

        memMsgQueue_.erase(memMsgQueue_.begin());
    }

    linkDown_->clock();
    if (linkUp_ != linkDown_) linkUp_->clock();
    scratch_->clock(cycle); // Clock backend

    return false;
}


/***************** request and response handlers ***********************/
/*
 *  Sort read requests into local (scratch) and remote
 */
void Scratchpad::handleRead(MemEventBase * event) {
    MemEvent * ev = static_cast<MemEvent*>(event);

    if (ev->getAddr() < scratchSize_) {
        handleScratchRead(ev);
    } else {
        handleRemoteRead(ev);
    }
}

/*
 *  Sort write requests into local and remote
 *  Also send coherence reads that correspond to processor writes to the read function
 */
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


/*
 *  Handle a scratch read
 *  Commands:
 *      - GetS
 *      - GetSX
 *      - GetX, cacheable
 *  Put request in mshr, if no conflict, send to scratchpad and wait for response
 */
void Scratchpad::handleScratchRead(MemEvent * ev) {

    Addr addr = ev->getBaseAddr();
    stat_ScratchReadReceived->addData(1);

    if (is_debug_addr(addr))
        eventDI.prefill(ev->getID(), ev->getCmd(), addr);

    MemEvent * response = ev->makeResponse();
    if (caching_ && !ev->queryFlag(MemEvent::F_NONCACHEABLE)) // Send data in exclusive state to let caches decide what to do with it
        response->setCmd(Command::GetXResp);

    MemEvent * read = new MemEvent(getName(), ev->getAddr(), ev->getBaseAddr(), Command::GetS, ev->getSize());
    read->setRqstr(ev->getRqstr());
    read->setVirtualAddress(ev->getVirtualAddress());
    read->setInstructionPointer(ev->getInstructionPointer());

    responseIDMap_.insert(std::make_pair(read->getID(),ev->getID()));
    responseIDAddrMap_.insert(std::make_pair(read->getID(),ev->getBaseAddr()));
    outstandingEventList_.insert(std::make_pair(ev->getID(),OutstandingEvent(ev,response)));

    if (mshr_.find(ev->getBaseAddr()) == mshr_.end()) {
        std::vector<uint8_t> data = doScratchRead(read);
        response->setPayload(data);
        mshr_.insert(std::make_pair(ev->getBaseAddr(), std::list<MSHREntry>(1,MSHREntry(ev->getID(), Command::GetS, true, false))));
        if (caching_ && !ev->queryFlag(MemEvent::F_NONCACHEABLE)) {
            cacheStatus_.at(ev->getBaseAddr()/scratchLineSize_) = true;
        }
        if (is_debug_addr(addr))
            eventDI.action = "ScrRead";
    } else {
        mshr_.find(ev->getBaseAddr())->second.push_back(MSHREntry(ev->getID(), Command::GetS, read));
        if (is_debug_addr(addr)) {
            eventDI.action = "stall";
            eventDI.reason = "MSHR conflict";
        }
    }

    if (is_debug_event(ev)) {
        dbg.debug(_L10_, "M: %-20" PRIu64 " %-20" PRIu64 " %-20s MSHR:InsEv    0x%-16" PRIx64 " %s\n",
                Simulation::getSimulation()->getCurrentSimCycle(), timestamp_, getName().c_str(), ev->getBaseAddr(), mshr_.find(ev->getBaseAddr())->second.back().getString().c_str());
    }
}


/*
 * Handle a scratch write
 * Commands:
 *      - GetX, noncacheable
 *      - PutM/E/S
 * Check for races with invalidations and handle as needed
 * Send AckPut for clean writebacks (PutE/PutS) and drop them
 * Send write to scratchpad after checking for conflicts in MSHR
 *
 * If this is a replacement from cache, mark as uncached
 * If from directory, replacements don't neccessarily mean that block is no longer cached,
 * so don't clear cached state.
 */
void Scratchpad::handleScratchWrite(MemEvent * ev) {
    Addr addr = ev->getBaseAddr();

    if (is_debug_addr(addr))
        eventDI.prefill(ev->getID(), ev->getCmd(), addr);

    bool doWrite = false; // Decide whether to handle this write immediately EVEN if a conflict
    bool inserted = false;
    /* Check for writeback/invalidation races */
    if (!directory_ && ev->isWriteback() && mshr_.find(ev->getBaseAddr()) != mshr_.end()) {
        MSHREntry * entry = &(mshr_.find(ev->getBaseAddr())->second.front());
        if (outstandingEventList_.find(entry->id)->second.request->getCmd() == Command::Get) {
            handleAckInv(ev);
            return;
            // TODO handle corner cases where Get only writes partial line
        } else if (outstandingEventList_.find(entry->id)->second.request->getCmd() == Command::Put) {
            if (ev->getPayload().empty()) {
                handleAckInv(ev);
            } else {
                handleFetchResp(ev);
            }
            return;
        }
    }

    /* Drop clean writebacks after sending AckPut */
    if (ev->isWriteback() && !ev->getDirty()) {
        if (caching_) {
            cacheStatus_.at(ev->getBaseAddr()/scratchLineSize_) = directory_;
        }
        MemEvent * response = ev->makeResponse();
        sendResponse(response);
        delete ev;
        return;
    }

    stat_ScratchWriteReceived->addData(1);

    MemEvent * response = nullptr;
    response = ev->makeResponse();

    MemEvent * write = new MemEvent(getName(), ev->getAddr(), ev->getBaseAddr(), Command::PutM, ev->getPayload());
    write->setRqstr(ev->getRqstr());
    write->setVirtualAddress(ev->getVirtualAddress());
    write->setInstructionPointer(ev->getInstructionPointer());
    write->setFlag(MemEvent::F_NORESPONSE);

    if (directory_ && ev->isWriteback() && mshr_.find(ev->getBaseAddr()) != mshr_.end()) {
        /* For directory - jump write ahead of a Put so we have correct data but otherwise
         * do not resolve race by treating writeback as ackinv since it may not actually signal that
         * the block is not present in caches */
        std::list<MSHREntry>* entry = &(mshr_.find(ev->getBaseAddr())->second);
        for (std::list<MSHREntry>::iterator it = entry->begin(); it != entry->end(); it++) {
            if (it->cmd == Command::Put) {
                if (it == entry->begin()) {
                    doScratchWrite(write);
                    sendResponse(response); /* Send response when request is sent to scratch, since scratch doesn't respond */
                    delete ev;
                } else {
                    outstandingEventList_.insert(std::make_pair(ev->getID(),OutstandingEvent(ev,response)));
                    it = entry->insert(it, MSHREntry(ev->getID(), Command::GetX, write));

                    if (is_debug_event(ev))
                        dbg.debug(_L10_, "M: %-20" PRIu64 " %-20" PRIu64 " %-20s MSHR:InsEv    0x%-16" PRIx64 " %s\n",
                                Simulation::getSimulation()->getCurrentSimCycle(), timestamp_, getName().c_str(), ev->getBaseAddr(), it->getString().c_str());
                }
                return;
            }
        }
    }

    if (mshr_.find(ev->getBaseAddr()) == mshr_.end()) {
        doScratchWrite(write);
        sendResponse(response); /* Send response when request is sent to scratch since scratch doesn't respond */
        delete ev;
        /* Update cache state */
        if (caching_ && !ev->queryFlag(MemEvent::F_NONCACHEABLE)) {
            cacheStatus_.at(ev->getBaseAddr()/scratchLineSize_) = directory_;
        }
    } else {
        outstandingEventList_.insert(std::make_pair(ev->getID(),OutstandingEvent(ev,response)));
        mshr_.find(ev->getBaseAddr())->second.push_back(MSHREntry(ev->getID(), Command::GetX, write));

        if (is_debug_event(ev))
            dbg.debug(_L10_, "M: %-20" PRIu64 " %-20" PRIu64 " %-20s MSHR:InsEv    0x%-16" PRIx64 " %s\n",
                        Simulation::getSimulation()->getCurrentSimCycle(), timestamp_, getName().c_str(), ev->getBaseAddr(), mshr_.find(ev->getBaseAddr())->second.back().getString().c_str());
    }
}


/*
 * Handle scratch Get by copying 'size' bytes from remote address
 * srcAddr to scratch address dstAddr. 'Size' may exceed the scratch
 * line size.
 *
 * 1. Issue read to remote for 'size' bytes from srcAddr
 * 2. If caching, send shootdowns for any cached blocks between
 *    dstAddr & dstAddr+size. All dirty data is discarded.
 * 3. Once the remote read returns, issue writes to the local scratch
 *    starting at address dstAddr (may mean writing multiple blocks).
 * 4. Once all writes are sent and all shootdown responses received,
 *    send AckMove to processor. At this point, any scratch reads sent
 *    by the processor are guaranteed to return new data.
 */
void Scratchpad::handleScratchGet(MemEventBase * event) {
    MoveEvent * ev = static_cast<MoveEvent*>(event);
    Addr saddr = ev->getSrcBaseAddr();
    Addr daddr = ev->getDstBaseAddr();

    stat_ScratchGetReceived->addData(1);

    MoveEvent * response = ev->makeResponse();
    outstandingEventList_.insert(std::make_pair(ev->getID(),OutstandingEvent(ev,response)));

    // Issue remote read
    ev->setSrcBaseAddr((ev->getSrcAddr() - remoteAddrOffset_) & ~(remoteLineSize_ - 1));
    MemEvent * remoteRead = new MemEvent(getName(), ev->getSrcAddr() - remoteAddrOffset_, ev->getSrcBaseAddr(), Command::GetS, ev->getSize());
    remoteRead->setFlag(MemEvent::F_NONCACHEABLE);
    remoteRead->setRqstr(ev->getRqstr());
    remoteRead->setVirtualAddress(ev->getSrcVirtualAddress());
    remoteRead->setInstructionPointer(ev->getInstructionPointer());
    responseIDMap_.insert(std::make_pair(remoteRead->getID(), ev->getID()));

    if (is_debug_event(remoteRead)) {
        dbg.debug(_L10_, "C: %-20" PRIu64 " %-20" PRIu64 " %-20s Get           0x%-16" PRIx64 " 0x%-16" PRIx64 " Remote Read (<%" PRIu64 ", %" PRIu32 ">, 0x%" PRIx64 ")\n",
                Simulation::getSimulation()->getCurrentSimCycle(), timestamp_, getName().c_str(), saddr, daddr, remoteRead->getID().first, remoteRead->getID().second, remoteRead->getBaseAddr());
    }

    memMsgQueue_.insert(std::make_pair(timestamp_, remoteRead));

    // Insert into mshr and send inv if needed
    // start base addr -> end base addr
    // start base addr + size
    uint32_t lineCount = 1 + (ev->getDstAddr() + ev->getSize() - ev->getDstBaseAddr() - 1)/ scratchLineSize_;
    for (uint32_t i = 0; i < lineCount; i++) {
        Addr baseAddr = ev->getDstBaseAddr() + i*scratchLineSize_;
        if (mshr_.find(baseAddr) == mshr_.end()) {
            bool needAck = startGet(baseAddr, ev);
            mshr_.insert(std::make_pair(baseAddr, std::list<MSHREntry>(1, MSHREntry(ev->getID(), Command::Get, true, needAck))));
        } else {
            mshr_.find(baseAddr)->second.push_back(MSHREntry(ev->getID(), Command::Get, true));
        }

        if (is_debug_addr(baseAddr))
            dbg.debug(_L10_, "M: %-20" PRIu64 " %-20" PRIu64 " %-20s MSHR:InsEv    0x%-16" PRIx64 " %s\n",
                    Simulation::getSimulation()->getCurrentSimCycle(), timestamp_, getName().c_str(), baseAddr, mshr_.find(baseAddr)->second.back().getString().c_str());

        outstandingEventList_.find(ev->getID())->second.incrementCount();
    }
}


/*
 * Handle scratch Put by copying 'size' bytes from scratch address
 * srcAddr to remote address dstAddr. 'Size' may exceed the scratch line size.
 *
 * 1. If caching, send shootdowns for any cached blocks between
 *    srcAddr & srcAddr+size.
 * 2. Send scratch read requests for any uncached blocks between
 *    srcAddr & srcAddr+size.
 * 3. Collect data scratch & shootdown responses in the payload of a write event.
 *    If a shootdown response arrives without data (i.e., was clean or uncached),
 *    send a scratch read.
 * 4. Once all data is received, send write to remote and AckMove to processor
 */
void Scratchpad::handleScratchPut(MemEventBase * event) {
    MoveEvent *ev = static_cast<MoveEvent*>(event);

    stat_ScratchPutReceived->addData(1);

    MoveEvent * response = ev->makeResponse();
    ev->setDstBaseAddr((ev->getDstBaseAddr() - remoteAddrOffset_) & ~(remoteLineSize_ - 1));

    MemEvent * remoteWrite = new MemEvent(getName(), ev->getDstAddr() - remoteAddrOffset_, ev->getDstBaseAddr(), Command::GetX, ev->getSize());
    remoteWrite->setZeroPayload(ev->getSize());
    remoteWrite->setFlag(MemEvent::F_NONCACHEABLE);
    remoteWrite->setFlag(MemEvent::F_NORESPONSE);

    outstandingEventList_.insert(std::make_pair(ev->getID(), OutstandingEvent(ev, response, remoteWrite)));

    Addr addr = ev->getSrcAddr();
    Addr baseAddr = ev->getSrcBaseAddr();
    uint32_t bytesLeft = ev->getSize();
    while (bytesLeft != 0) {
        uint32_t size = (baseAddr + scratchLineSize_) - addr;
        if (size > bytesLeft) size = bytesLeft;

        if (mshr_.find(baseAddr) == mshr_.end()) {
            bool needAck = startPut(baseAddr, ev);
            mshr_.insert(std::make_pair(baseAddr, std::list<MSHREntry>(1, MSHREntry(ev->getID(), Command::Put, !needAck, needAck))));
        } else {
            mshr_.find(baseAddr)->second.push_back(MSHREntry(ev->getID(), Command::Put));
        }

        if (is_debug_addr(baseAddr))
            dbg.debug(_L10_, "M: %-20" PRIu64 " %-20" PRIu64 " %-20s MSHR:InsEv    0x%-16" PRIx64 " %s\n",
                    Simulation::getSimulation()->getCurrentSimCycle(), timestamp_, getName().c_str(),
                    baseAddr, mshr_.find(baseAddr)->second.back().getString().c_str());

        bytesLeft -= size;
        baseAddr += scratchLineSize_;
        addr = baseAddr;

        outstandingEventList_.find(ev->getID())->second.incrementCount();
    }
}


/*
 * Handle a scratchpad response. Called by scratch backend.
 * Responses:
 *  Response to a ScratchPut's read request: call updatePut() to record response
 *      in remote write's payload
 *  All others (regular read responses): call finishRequest()
 */
void Scratchpad::handleScratchResponse(SST::Event::id_type responseID) {
    SST::Event::id_type requestID = responseIDMap_.find(responseID)->second;
    responseIDMap_.erase(responseID);

    Addr baseAddr = responseIDAddrMap_.find(responseID)->second;
    responseIDAddrMap_.erase(responseID);

    if (is_debug_addr(baseAddr))
        dbg.debug(_L5_, "C: %-20" PRIu64 " %-20" PRIu64 " %-20s Scratch:Recv  0x%-16" PRIx64 " <%" PRIu64 ", %" PRIu32 ">\n",
                Simulation::getSimulation()->getCurrentSimCycle(), timestamp_, getName().c_str(), baseAddr, responseID.first, responseID.second);

    if (outstandingEventList_.find(requestID)->second.request->getCmd() == Command::Put) {
        updatePut(requestID);
    } else { // Anything else - GetS, GetX, etc.
        finishRequest(requestID);
    }
    updateMSHR(baseAddr);
}


/*
 * Handle an AckInv response to a shootdown (Inv)
 *
 * 1. Set cached status to false
 * 2. Set state for the Get or Put to indicate a shootdown response was received
 * 3. If we needed data (for a Put), send a read request to scratch
 * 4. If this was the last shootdown response and we're not waiting for a scratch response,
 *    finish the request
 */
void Scratchpad::handleAckInv(MemEventBase * event) {
    MemEvent *response = static_cast<MemEvent*>(event);
    Addr baseAddr = response->getBaseAddr();

    /* Look up request in mshr */
    MSHREntry * entry = &(mshr_.find(baseAddr)->second.front());
    SST::Event::id_type requestID = entry->id;
    MoveEvent * request = static_cast<MoveEvent*>(outstandingEventList_.find(requestID)->second.request);

    /* Update cache status */
    if (is_debug_addr(baseAddr))
        dbg.debug(_L9_, "C: %-20" PRIu64 " %-20" PRIu64 " %-20s UpdCache   0x%-16" PRIx64 "  Uncached\n",
                Simulation::getSimulation()->getCurrentSimCycle(), timestamp_, getName().c_str(), baseAddr);

    cacheStatus_.at(baseAddr/scratchLineSize_) = false;

    if (entry->cmd == Command::Get) {
        entry->needAck = false;

        if (is_debug_addr(baseAddr))
            dbg.debug(_L10_, "M: %-20" PRIu64 " %-20" PRIu64 " %-20s MSHR:Update   0x%-16" PRIx64 " %s\n",
                    Simulation::getSimulation()->getCurrentSimCycle(), timestamp_, getName().c_str(), baseAddr, entry->getString().c_str());

        if (!entry->needData) {
            updateGet(entry->id);
            updateMSHR(baseAddr);
        }
    } else if (entry->cmd == Command::Put) { // Command::Put
        // Send a read since we didn't get data
        // Determine address and size for the read
        entry->needAck = false;
        entry->needData = true;

        if (is_debug_addr(baseAddr)) {
            dbg.debug(_L10_, "M: %-20" PRIu64 " %-20" PRIu64 " %-20s MSHR:Update   0x%-16" PRIx64 " %s\n",
                    Simulation::getSimulation()->getCurrentSimCycle(), timestamp_, getName().c_str(), baseAddr, entry->getString().c_str());
        }

        Addr addr = baseAddr;
        if (addr == request->getSrcBaseAddr())
            addr = request->getSrcAddr();

        uint32_t size = deriveSize(addr, baseAddr, request->getSrcAddr(), request->getSize());

        MemEvent * read = new MemEvent(getName(), addr, baseAddr, Command::GetS, size);
        read->setRqstr(request->getRqstr());
        read->setVirtualAddress(request->getSrcVirtualAddress());
        read->setInstructionPointer(request->getInstructionPointer());
        responseIDMap_.insert(std::make_pair(read->getID(),requestID));
        responseIDAddrMap_.insert(std::make_pair(read->getID(), baseAddr));

        std::vector<uint8_t> data = doScratchRead(read);
        std::vector<uint8_t> payload = outstandingEventList_.find(requestID)->second.remoteWrite->getPayload();
        uint32_t offset = addr - request->getSrcAddr();
        for (uint32_t i = 0; i < size; i++) {
            payload[i+offset] = data[i];
        }
        outstandingEventList_.find(requestID)->second.remoteWrite->setPayload(payload);
    } else {
        dbg.fatal(CALL_INFO, -1, "%s, Error: unhandled case in handleAckInv. Time = %" PRIu64 ", Event = (%s).\n",
                getName().c_str(), timestamp_, event->getVerboseString().c_str());
    }
    delete event;
}


/*
 * Handle a FetchResp response to a shootdown.
 * Can only be in response to a Put since we don't request data for Get shootdowns.
 *
 * 1. Set cached state to false
 * 2. If data is dirty, send scratch write
 * 3. Update payload for remote write
 * 4. If there are no more scratch or shootdown responses to wait for, finish the put (updatePut())
 */
void Scratchpad::handleFetchResp(MemEventBase * event) {
    MemEvent * response = static_cast<MemEvent*>(event);
    Addr baseAddr = response->getBaseAddr();

    /* Look up request in mshr */
    MSHREntry * entry = &(mshr_.find(baseAddr)->second.front());
    SST::Event::id_type requestID = entry->id;
    MoveEvent * put = static_cast<MoveEvent*>(outstandingEventList_.find(requestID)->second.request);

    /* Update cache status */
    cacheStatus_.at(baseAddr/scratchLineSize_) = false;

    // Send a write to scratch if the line was dirty since we forcefully invalidated
    if (response->getDirty()) {
        MemEvent * write = new MemEvent(getName(), response->getAddr(), baseAddr, Command::PutM, response->getPayload());
        write->setRqstr(put->getRqstr());
        write->setVirtualAddress(put->getSrcVirtualAddress());
        write->setInstructionPointer(put->getInstructionPointer());
        write->setFlag(MemEvent::F_NORESPONSE);
        doScratchWrite(write);
    }
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


/**
 * Handle NACKs
 * Only invalidations will be NACKed since they are the
 * only type of requests a scratchpad sends to caches
 * (responses are never NACKed).
 */
void Scratchpad::handleNack(MemEventBase * event) {
    MemEvent * nack = static_cast<MemEvent*>(event);

    /* The only events that can be nacked are invalidations
     * Check if we need to resend or if the inv has already
     * been resolved.
     */
    MemEvent * nackedEvent = nack->getNACKedEvent();
    if (mshr_.find(nackedEvent->getBaseAddr()) == mshr_.end()) {
        delete nackedEvent;
        delete nack;
        return;
    }

    MSHREntry * entry = &(mshr_.find(nackedEvent->getBaseAddr())->second.front());
    if (entry->needAck) {
        // Determine whether nackedEvent actually matches request -> if not, don't resend
        // resend inv

        /* Compute backoff to avoid excessive NACKing */
        int retries = nackedEvent->getRetries();
        if (retries > 10) retries = 10;
        uint64_t backoff = (0x1 << retries);
        nackedEvent->incrementRetries();

        procMsgQueue_.insert(std::make_pair(timestamp_ + backoff, nackedEvent));

    } else {
        delete nackedEvent;
    }

    delete nack;
}


/*
 * Handle a read request to a remote address.
 * This bypasses the scratchpad completely.
 */
void Scratchpad::handleRemoteRead(MemEvent * event) {
    stat_RemoteReadReceived->addData(1);

    event->setBaseAddr((event->getAddr() - remoteAddrOffset_) & ~(remoteLineSize_ - 1));
    MemEvent * request = new MemEvent(getName(), event->getAddr() - remoteAddrOffset_, event->getBaseAddr(), Command::GetS, event->getSize());
    request->setFlag(MemEvent::F_NONCACHEABLE); // Use byte not line address
    request->setRqstr(event->getRqstr());
    request->setVirtualAddress(event->getVirtualAddress());
    request->setInstructionPointer(event->getInstructionPointer());

    MemEvent * response = event->makeResponse();
    outstandingEventList_.insert(std::make_pair(event->getID(), OutstandingEvent(event, response)));
    responseIDMap_.insert(std::make_pair(request->getID(), event->getID()));

    memMsgQueue_.insert(std::make_pair(timestamp_, request));
}


/*
 * Handle a write request to a remote address.
 * This bypasses the scratchpad completely.
 * The remote will not Ack a write so we send a
 * response to the processor immediately
 */
void Scratchpad::handleRemoteWrite(MemEvent * event) {
    stat_RemoteWriteReceived->addData(1);

    event->setBaseAddr((event->getAddr() - remoteAddrOffset_) & ~(remoteLineSize_ - 1));
    MemEvent * request = new MemEvent(getName(), event->getAddr() - remoteAddrOffset_, event->getBaseAddr(), Command::GetX, event->getPayload());
    request->setFlag(MemEvent::F_NORESPONSE);
    request->setFlag(MemEvent::F_NONCACHEABLE);
    request->setRqstr(event->getRqstr());
    request->setVirtualAddress(event->getVirtualAddress());
    request->setInstructionPointer(event->getInstructionPointer());

    memMsgQueue_.insert(std::make_pair(timestamp_, request));

    MemEvent * response = event->makeResponse();

    procMsgQueue_.insert(std::make_pair(timestamp_, response));

    delete event;
}


/*
 * Handle a read response from remote memory in response to a ScratchGet
 * If from a ScratchGet, write data to scratchpad and send a response
 * to the processor once all data is written.
 */
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
        std::vector<uint8_t> data((response->getPayload()).begin() + payloadOffset, (response->getPayload()).begin() + payloadOffset + size);
        MemEvent * write = new MemEvent(getName(), addr, baseAddr, Command::PutM, data);
        write->setRqstr(request->getRqstr());
        write->setVirtualAddress(request->getDstVirtualAddress());
        write->setInstructionPointer(request->getInstructionPointer());
        write->setFlag(MemEvent::F_NORESPONSE);

        if (mshr_.find(baseAddr)->second.front().id == requestID) {
            doScratchWrite(write);
            mshr_.find(baseAddr)->second.front().needData = false;

            if (is_debug_addr(baseAddr))
                dbg.debug(_L10_, "M: %-20" PRIu64 " %-20" PRIu64 " %-20s MSHR:Update   0x%-16" PRIx64 " %s\n",
                        Simulation::getSimulation()->getCurrentSimCycle(), timestamp_, getName().c_str(), baseAddr, mshr_.find(baseAddr)->second.front().getString().c_str());

            if (!mshr_.find(baseAddr)->second.front().needAck) {
                updateGet(requestID);
                updateMSHR(baseAddr);
            }
        } else {
            // Find it
            if (mshr_.find(baseAddr) == mshr_.end()) {
                dbg.fatal(CALL_INFO, -1, "ERROR: remoteGetResponse but no matching entry in mshr for address 0x%" PRIx64 "\n", baseAddr);
            }
            for (std::list<MSHREntry>::iterator it = mshr_.find(baseAddr)->second.begin(); it != mshr_.find(baseAddr)->second.end(); it++) {
                if (it->id == requestID) {
                    it->scratch = write;
                    it->needData = false;

                    if (is_debug_addr(baseAddr))
                        dbg.debug(_L10_, "M: %-20" PRIu64 " %-20" PRIu64 " %-20s MSHR:Update   0x%-16" PRIx64 " %s\n",
                                Simulation::getSimulation()->getCurrentSimCycle(), timestamp_, getName().c_str(), baseAddr, mshr_.find(baseAddr)->second.front().getString().c_str());
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

        if (entry->cmd == Command::GetS) {
            std::vector<uint8_t> readData = doScratchRead(entry->scratch);
            static_cast<MemEvent*>(outstandingEventList_.find(entry->id)->second.response)->setPayload(readData);

            if (is_debug_addr(baseAddr))
                dbg.debug(_L10_, "M: %-20" PRIu64 " %-20" PRIu64 " %-20s MSHR:Update   0x%-16" PRIx64 " %s\n",
                        Simulation::getSimulation()->getCurrentSimCycle(), timestamp_, getName().c_str(), baseAddr, entry->getString().c_str());

            if (caching_ && (outstandingEventList_.find(entry->id)->second.request->queryFlag(MemEvent::F_NONCACHEABLE))) {
                cacheStatus_.at(baseAddr/scratchLineSize_) = true;
            }
            break;
        } else if (entry->cmd == Command::GetX) {
            doScratchWrite(entry->scratch);
            finishRequest(entry->id);
            mshr_.find(baseAddr)->second.pop_front();

            if (is_debug_addr(baseAddr))
                dbg.debug(_L10_, "M: %-20" PRIu64 " %-20" PRIu64 " %-20s MSHR:Remove   0x%-16" PRIx64 "\n",
                        Simulation::getSimulation()->getCurrentSimCycle(), timestamp_, getName().c_str(), baseAddr);

        } else if (entry->cmd == Command::Get) {
            entry->needAck = startGet(baseAddr, static_cast<MoveEvent*>(outstandingEventList_.find(entry->id)->second.request));
            if (!entry->needData) {
                doScratchWrite(entry->scratch);
                entry->scratch = nullptr;
            }
            if (!entry->needAck && !entry->needData) {
                updateGet(entry->id);
                mshr_.find(baseAddr)->second.pop_front();

                if (is_debug_addr(baseAddr))
                    dbg.debug(_L10_, "M: %-20" PRIu64 " %-20" PRIu64 " %-20s MSHR:Remove   0x%-16" PRIx64 "\n",
                            Simulation::getSimulation()->getCurrentSimCycle(), timestamp_, getName().c_str(), baseAddr);
            } else {
                if (is_debug_addr(baseAddr))
                    dbg.debug(_L10_, "M: %-20" PRIu64 " %-20" PRIu64 " %-20s MSHR:Remove   0x%-16" PRIx64 "\n",
                            Simulation::getSimulation()->getCurrentSimCycle(), timestamp_, getName().c_str(), baseAddr);

                break; // Still waiting on something
            }
        } else if (entry->cmd == Command::Put) {
            entry->needAck = startPut(baseAddr, static_cast<MoveEvent*>(outstandingEventList_.find(entry->id)->second.request));
            entry->needData = !entry->needAck;

            if (is_debug_addr(baseAddr))
                dbg.debug(_L10_, "M: %-20" PRIu64 " %-20" PRIu64 " %-20s MSHR:Remove   0x%-16" PRIx64 "\n",
                        Simulation::getSimulation()->getCurrentSimCycle(), timestamp_, getName().c_str(), baseAddr);

            break;
        } else {
            dbg.fatal(CALL_INFO, -1, "(%s) Updating MSHR but do not handle this case. Addr: 0x%" PRIx64 "\n", getName().c_str(), baseAddr);
        }
    }

    // Clear mshr entry if list is empty
    if (mshr_.find(baseAddr)->second.empty()) {
        mshr_.erase(baseAddr);

        if (is_debug_addr(baseAddr))
            dbg.debug(_L10_, "M: %-20" PRIu64 " %-20" PRIu64 " %-20s MSHR:Erase    0x%-16" PRIx64 "\n", Simulation::getSimulation()->getCurrentSimCycle(), timestamp_, getName().c_str(), baseAddr);
    }
}

// Helper methods
std::vector<uint8_t> Scratchpad::doScratchRead(MemEvent * event) {
    stat_ScratchReadIssued->addData(1);

    std::vector<uint8_t> data;
    data.resize(event->getSize(), 0);
    if (backing_) {
        backing_->get(event->getAddr(), event->getSize(), data);
    }
    dbg.debug(_L5_, "C: %-20" PRIu64 " %-20" PRIu64 " %-20s Scratch:Send  0x%-16" PRIx64 " (%s)\n",
            Simulation::getSimulation()->getCurrentSimCycle(), timestamp_, getName().c_str(), event->getAddr(), event->getBriefString().c_str());
    scratch_->handleMemEvent(event);
    return data;
}

void Scratchpad::doScratchWrite(MemEvent * event) {
    stat_ScratchWriteIssued->addData(1);

    if (backing_) {
        backing_->set(event->getAddr(), event->getSize(), event->getPayload());
    }

    dbg.debug(_L5_, "C: %-20" PRIu64 " %-20" PRIu64 " %-20s Scratch:Send  0x%-16" PRIx64 " (%s)\n",
            Simulation::getSimulation()->getCurrentSimCycle(), timestamp_, getName().c_str(), event->getAddr(), event->getBriefString().c_str());
    scratch_->handleMemEvent(event);
}

void Scratchpad::sendResponse(MemEventBase * event) {
    procMsgQueue_.insert(std::make_pair(timestamp_, event));
}


/* Start a Get request for a particular line by
 * determining whether an inv for that line is needed
 * Return whether inv was sent or not
 */
bool Scratchpad::startGet(Addr baseAddr, MoveEvent * get) {
    if (caching_ && cacheStatus_.at(baseAddr/scratchLineSize_) == true) {
        MemEvent * inv = new MemEvent(getName(), baseAddr, baseAddr, Command::ForceInv, scratchLineSize_);
        inv->setRqstr(get->getRqstr());
        inv->setDst(linkUp_->getSources()->begin()->name);
        inv->setVirtualAddress(get->getDstVirtualAddress());
        inv->setInstructionPointer(get->getInstructionPointer());
        dbg.debug(_L10_, "C: %-20" PRIu64 " %-20" PRIu64 " %-20s Get            0x%-16" PRIx64 " 0x%-16" PRIx64 " Inv         (<%" PRIu64 ", %" PRIu32 ">, 0x%" PRIx64 ")\n",
                Simulation::getSimulation()->getCurrentSimCycle(), timestamp_, getName().c_str(), get->getSrcBaseAddr(), get->getDstBaseAddr(), inv->getID().first, inv->getID().second, inv->getBaseAddr());
        procMsgQueue_.insert(std::make_pair(timestamp_, inv));
        return true;
    }
    return false;
}

/* Start a Put request for a particular line by
 * determining whether a fetch/inv for that line
 * is needed
 * Return whether fetch was sent or not
 */
bool Scratchpad::startPut(Addr baseAddr, MoveEvent * put) {
    if (caching_ && cacheStatus_.at(baseAddr/scratchLineSize_) == true) {
        MemEvent * inv = new MemEvent(getName(), baseAddr, baseAddr, Command::FetchInv, scratchLineSize_);
        inv->setRqstr(put->getRqstr());
        inv->setDst(put->getSrc());
        inv->setVirtualAddress(put->getSrcVirtualAddress());
        inv->setInstructionPointer(put->getInstructionPointer());
        dbg.debug(_L10_, "C: %-20" PRIu64 " %-20" PRIu64 " %-20s Put            0x%-16" PRIx64 " 0x%-16" PRIx64 " Inv         (<%" PRIu64 ", %" PRIu32 ">, 0x%" PRIx64 ")\n",
                Simulation::getSimulation()->getCurrentSimCycle(), timestamp_, getName().c_str(), put->getSrcBaseAddr(), put->getDstBaseAddr(), inv->getID().first, inv->getID().second, inv->getBaseAddr());
        procMsgQueue_.insert(std::make_pair(timestamp_, inv));
        return true;
    } else {
        // Derive addr and size from baseAddr and the put request
        Addr addr = baseAddr;
        if (baseAddr == put->getSrcBaseAddr())
            addr = put->getSrcAddr();
        uint32_t size = deriveSize(addr, baseAddr, put->getSrcAddr(), put->getSize());

        MemEvent * read = new MemEvent(getName(), addr, baseAddr, Command::GetS, size);
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
        MoveEvent * put = static_cast<MoveEvent*>(outstandingEventList_.find(putID)->second.request);
        dbg.debug(_L10_, "C: %-20" PRIu64 " %-20" PRIu64 " %-20s Put            0x%-16" PRIx64 " 0x%-16" PRIx64 " Scratch Done (<%" PRIu64 ", %" PRIu32 ">, 0x%" PRIx64 ")\n",
                Simulation::getSimulation()->getCurrentSimCycle(), timestamp_, getName().c_str(),
                put->getSrcBaseAddr(),
                put->getDstBaseAddr(),
                outstandingEventList_.find(putID)->second.remoteWrite->getID().first,
                outstandingEventList_.find(putID)->second.remoteWrite->getID().second,
                outstandingEventList_.find(putID)->second.remoteWrite->getBaseAddr());
//        dbg.debug(_L5_, "C: %-20" PRIu64 " %-20" PRIu64 " %-20s Finish        0x%-16" PRIx64 " <%" PRIu64 ", %" PRIu32 ">\n",
//                Simulation::getSimulation()->getCurrentSimCycle(), timestamp_, getName().c_str(), outstandingEventList_.find(putID)->second.remoteWrite->getBaseAddr(), baseAddr, responseID.first, responseID.second);
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
    if (outstandingEventList_.find(requestID)->second.response != nullptr)
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
