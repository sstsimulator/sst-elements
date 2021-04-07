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

#include "memoryCacheController.h"
#include "util.h"

#include "membackend/memBackendConvertor.h"
#include "membackend/memBackend.h"
#include "memEventBase.h"
#include "memEvent.h"
#include "bus.h"
#include "cacheListener.h"
#include "memNIC.h"
#include "memLink.h"

#define NO_STRING_DEFINED "N/A"

using namespace std;
using namespace SST;
using namespace SST::MemHierarchy;

// Debug macros
#ifdef __SST_DEBUG_OUTPUT__
#define is_debug_addr(addr) (DEBUG_ADDR.empty() || DEBUG_ADDR.find(addr) != DEBUG_ADDR.end())
#define is_debug_event(ev) (DEBUG_ADDR.empty() || ev->doDebug(DEBUG_ADDR))
#define Debug(level, fmt, ... ) dbg.debug( level, fmt, ##__VA_ARGS__  )
#else
#define is_debug_addr(addr) false
#define is_debug_event(ev) false
#define Debug(level, fmt, ... )
#endif
/*
 *  Debug levels:
 *  3  - event receive/response
 *  4  - backing store
 *  9  - init()
 *  10 - address translation
 */

/*************************** Memory Controller ********************/
MemCacheController::MemCacheController(ComponentId_t id, Params &params) : Component(id), backing_(NULL) {

    int debugLevel = params.find<int>("debug_level", 0);

    lineSize_ = params.find<uint64_t>("cache_line_size", 64);

    // Output for debug
    dbg.init("", debugLevel, 0, (Output::output_location_t)params.find<int>("debug", 0));
    if (debugLevel < 0 || debugLevel > 10)
        out.fatal(CALL_INFO, -1, "Debugging level must be between 0 and 10. \n");

    // Debug address
    std::vector<Addr> addrArr;
    params.find_array<Addr>("debug_addr", addrArr);
    for (std::vector<Addr>::iterator it = addrArr.begin(); it != addrArr.end(); it++) {
        DEBUG_ADDR.insert(*it);
    }

    // Output for warnings
    out.init("", params.find<int>("verbose", 1), 0, Output::STDOUT);

    /* MemCacheController only supports new way of loading backend:
     *  -> Fill backend slot with backend and memcontroller loads the compatible convertor */
    MemBackend * memory = loadUserSubComponent<MemBackend>("backend");
    if (!memory) {  /* Try to load from our parameters (legacy mode 1) */
        out.fatal(CALL_INFO, -1, "%s, ERROR: This component does not support loading backend/backendConvertor from parameters. Load as user (python-defined) subcomponents instead.", getName().c_str());
    }

    std::string convertortype = memory->getBackendConvertorType();
    Params tmpParams;
    memBackendConvertor_ = loadAnonymousSubComponent<MemBackendConvertor>(convertortype, "backendConvertor", 0, ComponentInfo::INSERT_STATS, tmpParams, memory, lineSize_);

    if (memBackendConvertor_ == nullptr) {
        out.fatal(CALL_INFO, -1, "%s, Error - unable to load MemBackendConvertor.", getName().c_str());
    }

    using std::placeholders::_1;
    using std::placeholders::_2;
    memBackendConvertor_->setCallbackHandlers(std::bind(&MemCacheController::handleLocalMemResponse, this, _1, _2), std::bind(&MemCacheController::turnClockOn, this));
    memSize_ = memBackendConvertor_->getMemSize();
    if (memSize_ == 0)
        out.fatal(CALL_INFO, -1, "%s, Error - tried to get memory size from backend but size is 0B. Either backend is missing 'mem_size' parameter or value is invalid.\n", getName().c_str());

    // Load listeners (profilers/tracers/etc.)
    SubComponentSlotInfo* lists = getSubComponentSlotInfo("listener"); // Find all listeners specified in the configuration
    if (lists) {
        for (int i = 0; i < lists->getMaxPopulatedSlotNumber(); i++) {
            if (lists->isPopulated(i))
                listeners_.push_back(lists->create<CacheListener>(i, ComponentInfo::SHARE_NONE));
        }
    }

    // Memory region
    lineOffset_ = log2Of(lineSize_);
    uint64_t addrStart = params.find<uint64_t>("cache_num", 0);
    uint64_t interleaveStep = params.find<uint64_t>("num_caches", 1);
    region_.start = addrStart * lineSize_;
    region_.end = (uint64_t) - 1;
    region_.interleaveSize = lineSize_;
    region_.interleaveStep = lineSize_ * interleaveStep; //(lineSize_ * interleaveStep) >> lineOffset_;
   // if (region_.interleaveSize == 0) region_.interleaveSize = lineSize_;
   // if (region_.interleaveStep == 0) region_.interleaveStep = ;

    bool found;

    link_ = loadUserSubComponent<MemLinkBase>("cpulink");

    if (!link_) {
        out.fatal(CALL_INFO,-1,"%s, Error: No link handler loaded into 'cpulink' subcomponent slot.\n", getName().c_str());
    }

    clockLink_ = link_->isClocked();
    link_->setRecvHandler( new Event::Handler<MemCacheController>(this, &MemCacheController::handleEvent));

    link_->setRegion(region_);

    // Set up backing store if needed
    std::string backingType = params.find<std::string>("backing", "mmap", found); /* Default to using an mmap backing store, fall back on malloc */
    backing_ = nullptr;

    if (backingType != "none" && backingType != "mmap" && backingType != "malloc") {
        out.fatal(CALL_INFO, -1, "%s, Error - Invalid param: backing. Must be one of 'none', 'malloc', or 'mmap'. You specified: %s\n",
                getName().c_str(), backingType.c_str());
    }

    std::string size = params.find<std::string>("backing_size_unit", "1MiB");
    UnitAlgebra size_ua(size);
    if (!size_ua.hasUnits("B")) {
        out.fatal(CALL_INFO, -1, "%s, Error - Invalid param: backing_size_unit. Must have units of bytes (B). SI ok. You specified: %s\n",
                getName().c_str(), size.c_str());
    }
    size_t sizeBytes = size_ua.getRoundedValue();

    if (sizeBytes > memBackendConvertor_->getMemSize()) {
        sizeBytes = memBackendConvertor_->getMemSize();
        // Since getMemSize() might not be a power of 2, but malloc store needs it....get a reasonably close power of 2
        sizeBytes = 1 << log2Of(memBackendConvertor_->getMemSize());
    }

    if (backingType == "mmap") {
        std::string memoryFile = params.find<std::string>("memory_file", NO_STRING_DEFINED );

        if ( 0 == memoryFile.compare( NO_STRING_DEFINED ) ) {
            memoryFile.clear();
        }
        try {
            backing_ = new Backend::BackingMMAP( memoryFile, memBackendConvertor_->getMemSize() );
        }
        catch ( int e) {
            if (e == 1)
                out.fatal(CALL_INFO, -1, "%s, Error - unable to open memory_file. You specified '%s'.\n", getName().c_str(), memoryFile.c_str());
            else if (e == 2) {
                out.verbose(CALL_INFO, 1, 0, "%s, Could not MMAP backing store (likely, simulated memory exceeds real memory). Creating malloc based store instead.\n", getName().c_str());
                backing_ = new Backend::BackingMalloc(sizeBytes);
            } else
                out.fatal(CALL_INFO, -1, "%s, Error - unable to create backing store. Exception thrown is %d.\n", getName().c_str(), e);
        }
    } else if (backingType == "malloc") {
        backing_ = new Backend::BackingMalloc(sizeBytes);
    }

    /* Clock Handler */
    std::string clockfreq = params.find<std::string>("clock");
    UnitAlgebra clock_ua(clockfreq);
    if (!(clock_ua.hasUnits("Hz") || clock_ua.hasUnits("s")) || clock_ua.getRoundedValue() <= 0) {
        out.fatal(CALL_INFO, -1, "%s, Error - Invalid param: clock. Must have units of Hz or s and be > 0. (SI prefixes ok). You specified '%s'\n", getName().c_str(), clockfreq.c_str());
    }
    clockHandler_ = new Clock::Handler<MemCacheController>(this, &MemCacheController::clock);
    clockTimeBase_ = registerClock(clockfreq, clockHandler_);
    clockOn_ = true;

    /* Initialize cache */
    uint64_t cachesize = memSize_ / lineSize_;
    if (memSize_ % lineSize_ != 0)
        out.fatal(CALL_INFO, -1, "%s, Error - memory size must be a multiple of line size. Memory size is %zu bytes and line size is %" PRIu64 " bytes\n",
                getName().c_str(), memSize_, lineSize_);
    cache_.resize(cachesize, CacheState(0,I));

    /* Statistics */
    statReadHit = registerStatistic<uint64_t>("CacheHits_Read");
    statReadMiss = registerStatistic<uint64_t>("CacheMisses_Read");
    statWriteHit = registerStatistic<uint64_t>("CacheHits_Write");
    statWriteMiss = registerStatistic<uint64_t>("CacheMisses_Write");

}

void MemCacheController::handleEvent(SST::Event* event) {
    if (!clockOn_) {
        Cycle_t cycle = turnClockOn();
        memBackendConvertor_->turnClockOn(cycle);
    }

    MemEventBase *meb = static_cast<MemEventBase*>(event);

    if (is_debug_event(meb)) {
        Debug(_L3_, "\n%" PRIu64 " (%s) Received: %s\n", getCurrentSimTimeNano(), getName().c_str(), meb->getVerboseString().c_str());
    }

    Command cmd = meb->getCmd();

    if (cmd == Command::CustomReq) {
        handleCustomEvent(meb);
        return;
    }

    MemEvent * ev = static_cast<MemEvent*>(meb);

    // Notify our listeners that we have received an event
    notifyListeners( ev );

    switch (cmd) {
        case Command::GetS:
        case Command::GetSX:
            handleRead(ev, false);
            break;
        case Command::GetX:
            if (ev->queryFlag(MemEvent::F_NONCACHEABLE))
                handleWrite(ev, false);
            else
                handleRead(ev, false);
            break;
        case Command::PutM:
            ev->setFlag(MemEvent::F_NORESPONSE);
            handleWrite(ev, false);
            break;
        case Command::FlushLine:
        case Command::FlushLineInv:
            handleFlush(ev);
            break;
        case Command::PutS:
        case Command::PutE:
            delete ev;
            break;
        case Command::GetXResp:
        case Command::GetSResp:
            handleDataResponse(ev);
            break;
        default:
            out.fatal(CALL_INFO,-1,"Memory cache controller received unrecognized command: %s", CommandString[(int)cmd]);
    }
}


void MemCacheController::handleRead(MemEvent* event, bool replay) {
    Addr cacheIndex = toLocalAddr(event->getBaseAddr());
    if (cacheIndex >= cache_.size())
        out.fatal(CALL_INFO, -1, "%s, Error: cache index exceeds cache size, try again!\n", getName().c_str());
    Addr blockAddr = cache_[cacheIndex].addr;
    State blockState = cache_[cacheIndex].state;

    if (is_debug_event(event)) {
        Debug(_L3_, "%" PRIu64 " (%s) handleRead, Line: %" PRIu64 ", 0x%" PRIx64 ", %s\n",
                getCurrentSimTimeNano(),
                getName().c_str(),
                cacheIndex,
                blockAddr,
                StateString[blockState]);
    }

    if (!replay) {
        MemAccessRecord rec;
        rec.event = event;
        outstandingEvents_.insert(std::make_pair(event->getID(), rec));
        mshr_[cacheIndex].push(event->getID());
    }
    std::map<SST::Event::id_type, MemAccessRecord>::iterator it = outstandingEvents_.find(event->getID());

    if (mshr_.find(cacheIndex)->second.front() != event->getID()) {         // Transition
        it->second.status = AccessStatus::STALL;
        if (is_debug_event(event))
            Debug(_L3_, "%" PRIu64 " (%s) StateTransition %" PRIu64 ", STALL\n", getCurrentSimTimeNano(), getName().c_str(), event->getID().first);
        return;
    } else if (blockState == I || blockAddr != event->getBaseAddr()) {      // MISS
        it->second.status = (blockState == M) ? AccessStatus::MISS_WB : AccessStatus::MISS;
        statReadMiss->addData(1);
        if (is_debug_event(event))
            Debug(_L3_, "%" PRIu64 " (%s) StateTransition %" PRIu64 ", %s\n", getCurrentSimTimeNano(), getName().c_str(), event->getID().first, (blockState == M) ? "MISS_WB" : "MISS");
    } else {                                                                // HIT
        statReadHit->addData(1);
        it->second.status = AccessStatus::HIT;
        if (is_debug_event(event))
            Debug(_L3_, "%" PRIu64 " (%s) StateTransition %" PRIu64 ", HIT\n", getCurrentSimTimeNano(), getName().c_str(), event->getID().first);
    }

    it->second.reqev = new MemEvent(*event);
    it->second.reqev->setBaseAddr(cacheIndex);
    it->second.reqev->setAddr(event->getAddr() - event->getBaseAddr() + cacheIndex);
    it->second.reqev->setCmd(Command::GetS);

    memBackendConvertor_->handleMemEvent(it->second.reqev);
}

void MemCacheController::handleWrite(MemEvent* event, bool replay) {
    Addr cacheIndex = toLocalAddr(event->getBaseAddr());
    Addr blockAddr = cache_[cacheIndex].addr;
    State blockState = cache_[cacheIndex].state;

    if (is_debug_event(event))
        Debug(_L3_, "\n%" PRIu64 " (%s) handleWrite, Line: %" PRIu64 ", 0x%" PRIx64 ", %s\n", getCurrentSimTimeNano(), getName().c_str(), cacheIndex, blockAddr, StateString[blockState]);

    if (!replay) {
        MemAccessRecord rec;
        rec.event = event;
        outstandingEvents_.insert(std::make_pair(event->getID(), rec));
        mshr_[cacheIndex].push(event->getID());
    }
    std::map<SST::Event::id_type, MemAccessRecord>::iterator it = outstandingEvents_.find(event->getID());

    if (mshr_.find(cacheIndex)->second.front() != event->getID()) {         // Transition
        it->second.status = AccessStatus::STALL;
        if (is_debug_event(event))
            Debug(_L3_, "\n%" PRIu64 " (%s) StateTransition %" PRIu64 ", STALL\n", getCurrentSimTimeNano(), getName().c_str(), event->getID().first);
        return;
    } else if (blockState == I || blockAddr != event->getBaseAddr()) {      // MISS
        // Do a read to time the state lookup
        statWriteMiss->addData(1);
        it->second.status = (blockState == M ) ? AccessStatus::MISS_WB : AccessStatus::MISS;
        if (is_debug_event(event))
            Debug(_L3_, "\n%" PRIu64 " (%s) StateTransition %" PRIu64 ", %s\n", getCurrentSimTimeNano(), getName().c_str(), event->getID().first, (blockState == M) ? "MISS_WB" : "MISS");
    } else {                                                                // HIT
        statWriteHit->addData(1);
        it->second.status = AccessStatus::HIT_TAG;
        if (is_debug_event(event))
            Debug(_L3_, "\n%" PRIu64 " (%s) StateTransition %" PRIu64 ", HIT\n", getCurrentSimTimeNano(), getName().c_str(), event->getID().first);
    }

    /* Lookup tag data -> required whether or not this is a hit */
    it->second.reqev = new MemEvent(*event);
    it->second.reqev->setBaseAddr(cacheIndex);
    it->second.reqev->setAddr(event->getAddr() - event->getBaseAddr() + cacheIndex);
    it->second.reqev->setCmd(Command::GetS);

    memBackendConvertor_->handleMemEvent(it->second.reqev);
}



void MemCacheController::handleFlush(MemEvent* event) {
    out.fatal(CALL_INFO, -1, "%s, MemoryCache encountered unhandled event: %s\n",
            getName().c_str(), event->getVerboseString().c_str());
}

/* Response from remote memory */
void MemCacheController::handleDataResponse(MemEvent* event) {
    std::map<SST::Event::id_type,MemAccessRecord>::iterator it = outstandingEvents_.find(event->getID());
    Addr cacheIndex = toLocalAddr(event->getBaseAddr());
    Addr blockAddr = cache_[cacheIndex].addr;
    State blockState = cache_[cacheIndex].state;

    if (is_debug_event(event))
        Debug(_L3_, "\n%" PRIu64 " (%s) handleDataResponse, Line: %" PRIu64 ", 0x%" PRIx64 ", %s\n",
                getCurrentSimTimeNano(), getName().c_str(), cacheIndex, blockAddr, StateString[blockState]);

    // update the backing store from the remote memory response
    if (backing_)
        writeData(event);

    // Update local memory
    it->second.reqev = new MemEvent(*it->second.event);
    it->second.reqev->setAddr(cacheIndex);
    it->second.reqev->setBaseAddr(cacheIndex);
    it->second.reqev->setCmd(Command::PutM);
    it->second.reqev->setPayload(event->getPayload());
    it->second.reqev->clearFlag();
    it->second.reqev->setFlag(MemEvent::F_NORESPONSE);
    it->second.status = AccessStatus::FIN;
    if (is_debug_event(event))
        Debug(_L3_, "\n%" PRIu64 " (%s) StateTransition %" PRIu64 ", FIN\n", getCurrentSimTimeNano(), getName().c_str(), it->second.event->getID().first);
    memBackendConvertor_->handleMemEvent(it->second.reqev);

    // Update backing store from the request that missed if it was a write
    if (it->second.event->getCmd() == Command::PutM || (it->second.event->getCmd() == Command::GetX && it->second.event->queryFlag(MemEvent::F_NONCACHEABLE))) {
        cache_[cacheIndex].state = M;
        if (backing_)
            writeData(it->second.event);
    } else {
        cache_[cacheIndex].state = E;
    }

    // Respond to requestor
    if (!(it->second.event->queryFlag(MemEvent::F_NORESPONSE))) {
        sendResponse(it->second.event, 0);
    }
    delete event;
}

/* Response from memory cache */
void MemCacheController::handleLocalMemResponse( Event::id_type id, uint32_t flags) {
    std::map<SST::Event::id_type,MemAccessRecord>::iterator it = outstandingEvents_.find(id);
    if (it == outstandingEvents_.end())
        out.fatal(CALL_INFO, -1, "%s, MemoryCache received unrecognized response ID: %" PRIu64 ", %" PRIu32 "", getName().c_str(), id.first, id.second);

    delete it->second.reqev;
    MemEventBase * evb = it->second.event;

    if (is_debug_event(evb)) {
        Debug(_L3_, "MemoryCache: %s - Response received to (%s)\n", getName().c_str(), evb->getVerboseString().c_str());
    }

    /* Handle custom events */
    if (evb->getCmd() == Command::CustomReq) {
        MemEventBase * resp = customCommandHandler_->finish(evb, flags);
        if (resp != nullptr)
            link_->send(resp);
        delete evb;
        return;
    }

    MemEvent * ev = static_cast<MemEvent*>(evb);

    bool noncacheable  = ev->queryFlag(MemEvent::F_NONCACHEABLE);

    Addr cacheIndex = toLocalAddr(ev->getBaseAddr());
    Addr blockAddr = cache_[cacheIndex].addr;
    State blockState = cache_[cacheIndex].state;

    if (is_debug_event(ev))
        Debug(_L3_, "\n%" PRIu64 " (%s) handleLocalResponse, Line: %" PRIu64 ", 0x%" PRIx64 ", %s\n",
                getCurrentSimTimeNano(), getName().c_str(), cacheIndex, blockAddr, StateString[blockState]);

    MemEvent * remoteRd, *remoteWr;
    Addr localIndex;
    switch (it->second.status) {
        case AccessStatus::MISS_WB:
            /* Write back data to memory */
            remoteWr = new MemEvent(getName(), blockAddr, blockAddr, Command::PutM, lineSize_);
            readData(remoteWr);
            remoteWr->setFlag(MemEvent::F_NORESPONSE); // Don't send a response to this
            remoteWr->setDst(link_->findTargetDestination(remoteWr->getBaseAddr()));
            link_->send(remoteWr);
        case AccessStatus::MISS:
            /* Read new data from memory */
            remoteRd = new MemEvent(*ev);
            remoteRd->setCmd(Command::GetS);
            remoteRd->setSrc(getName());
            remoteRd->setDst(link_->findTargetDestination(remoteRd->getBaseAddr()));
            if (remoteRd->queryFlag(MemEvent::F_NORESPONSE))
                remoteRd->clearFlag(MemEvent::F_NORESPONSE);
            it->second.reqev = remoteRd;
            link_->send(remoteRd);
            it->second.status = AccessStatus::DATA; // We've request data, waiting for response
            if (is_debug_event(it->second.event))
                Debug(_L3_, "\n%" PRIu64 " (%s) StateTransition %" PRIu64 ", DATA\n", getCurrentSimTimeNano(), getName().c_str(), it->second.event->getID().first);
            cache_[cacheIndex].addr = ev->getBaseAddr();
            cache_[cacheIndex].state = IM;
            break;
        case AccessStatus::HIT_TAG: // tag hit, issue write
            it->second.reqev = new MemEvent(*ev);
            it->second.status = AccessStatus::HIT;
            if (is_debug_event(it->second.event))
                Debug(_L3_, "\n%" PRIu64 " (%s) StateTransition %" PRIu64 ", HIT\n", getCurrentSimTimeNano(), getName().c_str(), it->second.event->getID().first);
            memBackendConvertor_->handleMemEvent(ev);
            cache_[cacheIndex].state = M;
            break;
        case AccessStatus::HIT:
            /* Write data. Here instead of receive to try to match backing access order to backend execute order */
            if (backing_ && (ev->getCmd() == Command::PutM || (ev->getCmd() == Command::GetX && noncacheable)))
                writeData(ev);

            if (!ev->queryFlag(MemEvent::F_NORESPONSE)) {
                sendResponse(ev, flags);
            }
        case AccessStatus::FIN: // Just finished updating the cache, ready for new requests now
            if (is_debug_event(it->second.event))
                Debug(_L3_, "\n%" PRIu64 " (%s) StateTransition %" PRIu64 ", ERASE\n", getCurrentSimTimeNano(), getName().c_str(), it->second.event->getID().first);
            delete ev;
            mshr_[cacheIndex].pop();
            outstandingEvents_.erase(it);
            if (mshr_[cacheIndex].empty())
                mshr_.erase(cacheIndex);
            else
                retry(cacheIndex);
            break;
        default:
            out.fatal(CALL_INFO, -1, "%s, MemoryCache encountered unhandled record status. Event is %s\n",
                getName().c_str(), ev->getVerboseString().c_str());
    }
}

void MemCacheController::retry(uint64_t cacheIndex) {
    MemEvent* ev = outstandingEvents_.find(mshr_[cacheIndex].front())->second.event;

    if (is_debug_event(ev)) {
        Debug(_L3_, "\n%" PRIu64 " (%s) Retrying: %s\n", getCurrentSimTimeNano(), getName().c_str(), ev->getVerboseString().c_str());
    }
    switch (ev->getCmd()) {
        case Command::GetS:
        case Command::GetSX:
            handleRead(ev, true);
            break;
        case Command::GetX:
            if (ev->queryFlag(MemEvent::F_NONCACHEABLE)) {
                handleWrite(ev, true);
            } else {
                handleRead(ev, true);
            }
            break;
        case Command::PutM:

            handleWrite(ev, true);
            break;
        default:
            break;
    }
}

void MemCacheController::sendResponse(MemEvent* ev, uint32_t flags) {
    MemEvent * resp = ev->makeResponse();

    bool noncacheable = ev->queryFlag(MemEvent::F_NONCACHEABLE);
    /* Read order matches execute order so that mis-ordering at backend can result in bad data */
    if (resp->getCmd() == Command::GetSResp || (resp->getCmd() == Command::GetXResp && !noncacheable)) {
        readData(resp);
        if (!noncacheable) resp->setCmd(Command::GetXResp);
    }

    resp->setFlags(flags);
    link_->send( resp );
}


bool MemCacheController::clock(Cycle_t cycle) {
    bool unclockLink = true;
    if (clockLink_) {
        unclockLink = link_->clock();
    }

    bool unclockBack = memBackendConvertor_->clock( cycle );

    if (unclockLink && unclockBack) {
        memBackendConvertor_->turnClockOff();
        clockOn_ = false;
        return true;
    }

    return false;
}

Cycle_t MemCacheController::turnClockOn() {
    Cycle_t cycle = reregisterClock(clockTimeBase_, clockHandler_);
    cycle--;
    clockOn_ = true;
    return cycle;
}

void MemCacheController::handleCustomEvent(MemEventBase * ev) {
    out.fatal(CALL_INFO, -1, "%s, MemoryCache encountered unhandled event: %s\n",
            getName().c_str(), ev->getVerboseString().c_str());
}


void MemCacheController::init(unsigned int phase) {
    link_->init(phase);

    /* Inherit region from our source(s) */
    if (!phase) {
        /* Announce our presence on link */
        link_->sendInitData(new MemEventInitCoherence(getName(), Endpoint::Memory, true, false, memBackendConvertor_->getRequestWidth(), false));
    }

    while (MemEventInit *ev = link_->recvInitData()) {
        processInitEvent(ev);
    }
}

void MemCacheController::setup(void) {
    memBackendConvertor_->setup();
    link_->setup();

}


void MemCacheController::finish(void) {
    if (!clockOn_) {
        Cycle_t cycle = turnClockOn();
        memBackendConvertor_->turnClockOn(cycle);
    }
    memBackendConvertor_->finish();
    link_->finish();
}

void MemCacheController::writeData(MemEvent* event) {
    /* Noncacheable events occur on byte addresses, others on line addresses */
    bool noncacheable = event->queryFlag(MemEvent::F_NONCACHEABLE);
    Addr addr = noncacheable ? event->getAddr() : event->getBaseAddr();

    addr = toLocalAddr(addr);

    if (event->getCmd() == Command::PutM) { /* Write request to memory */
        if (is_debug_event(event)) { Debug(_L4_, "\tUpdate backing. Addr = %" PRIx64 ", Size = %i\n", addr, event->getSize()); }

        backing_->set(addr, event->getSize(), event->getPayload());

        return;
    }

    if (noncacheable && event->getCmd() == Command::GetX) {
        if (is_debug_event(event)) { Debug(_L4_, "\tUpdate backing. Addr = %" PRIx64 ", Size = %i\n", addr, event->getSize()); }

        backing_->set(addr, event->getSize(), event->getPayload());

        return;
    }

}


void MemCacheController::readData(MemEvent* event) {
    bool noncacheable = event->queryFlag(MemEvent::F_NONCACHEABLE);
    Addr localAddr = noncacheable ? event->getAddr() : event->getBaseAddr();

    localAddr = toLocalAddr(localAddr);

    vector<uint8_t> payload;
    payload.resize(event->getSize(), 0);

    if (backing_)
        backing_->get(localAddr, event->getSize(), payload);

    event->setPayload(payload);
}


/* Backing store interactions for custom command subcomponents */
void MemCacheController::writeData(Addr addr, std::vector<uint8_t> * data) {
    if (!backing_) return;

    for (size_t i = 0; i < data->size(); i++)
        backing_->set(addr + i, data->at(i));
}


void MemCacheController::readData(Addr addr, size_t bytes, std::vector<uint8_t> &data) {
    data.resize(bytes, 0);

    if (!backing_) return;

    for (size_t i = 0; i < bytes; i++)
        data[i] = backing_->get(addr + i);
}


/* Translations assume interleaveStep is divisible by interleaveSize */
Addr MemCacheController::toLocalAddr(Addr addr) {
    Addr shift = addr >> lineOffset_;
    Addr rAddr = shift / (region_.interleaveSize >> lineOffset_); // Divide by num caches
    //Addr step = shift / region_.interleaveStep;
    //Addr offset = shift % region_.interleaveStep;
    //Addr rAddr = (step * region_.interleaveSize) + offset;
    rAddr = rAddr % cache_.size();

    if (is_debug_addr(addr)) { Debug(_L10_,"\tConverting global address 0x%" PRIx64 " to local index %" PRIu64 "\n", addr, rAddr); }
    return rAddr;
}



void MemCacheController::processInitEvent( MemEventInit* me ) {
    /* Forward data to remote memory */
    if (Command::NULLCMD == me->getCmd()) {
        if (is_debug_event(me)) { Debug(_L9_, "Memory (%s) received init event: %s\n", getName().c_str(), me->getVerboseString().c_str()); }
    } else {
        if (is_debug_event(me)) { Debug(_L9_,"Memory init %s - Received GetX for %" PRIx64 " size %zu\n", getName().c_str(), me->getAddr(),me->getPayload().size()); }
        MemEventInit * mEv = me->clone();
        mEv->setSrc(getName());
        mEv->setDst(link_->findTargetDestination(mEv->getRoutingAddress()));
        link_->sendInitData(mEv);
    }
    delete me;
}

void MemCacheController::printStatus(Output &statusOut) {
    statusOut.output("MemHierarchy::MemoryController %s\n", getName().c_str());

    statusOut.output("  Outstanding events: %zu\n", outstandingEvents_.size());
/*    for (std::map<SST::Event::id_type, MemEventBase*>::iterator it = outstandingEvents_.begin(); it != outstandingEvents_.end(); it++) {
        statusOut.output("    %s\n", it->second->getVerboseString().c_str());
    }
    */

    statusOut.output("  Link Status: ");
    if (link_)
        link_->printStatus(statusOut);

    statusOut.output("End MemHierarchy::MemoryController\n\n");
}

void MemCacheController::emergencyShutdown() {
    if (out.getVerboseLevel() > 1) {
        if (out.getOutputLocation() == Output::STDOUT)
            out.setOutputLocation(Output::STDERR);

        printStatus(out);

        if (link_) {
            out.output("  Checking for unreceived events on link: \n");
            link_->emergencyShutdownDebug(out);
        }
    }
}
