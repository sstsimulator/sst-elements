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

#include "memoryController.h"
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
MemController::MemController(ComponentId_t id, Params &params) : Component(id), backing_(NULL) {

    int debugLevel = params.find<int>("debug_level", 0);

    fixupParam( params, "backend", "backendConvertor.backend" );
    fixupParams( params, "backend.", "backendConvertor.backend." );
    fixupParams( params, "request_width", "backendConvertor.request_width" );
    fixupParams( params, "max_requests_per_cycle", "backendConvertor.backend.max_requests_per_cycle" );

    uint32_t requestWidth = params.find<uint32_t>("backendConvertor.request_width", 64);

    // Output for debug
    dbg.init("", debugLevel, 0, (Output::output_location_t)params.find<int>("debug", 0));

    // Debug address
    std::vector<Addr> addrArr;
    params.find_array<Addr>("debug_addr", addrArr);
    for (std::vector<Addr>::iterator it = addrArr.begin(); it != addrArr.end(); it++) {
        DEBUG_ADDR.insert(*it);
    }

    // Output for warnings
    out.init("", params.find<int>("verbose", 1), 0, Output::STDOUT);

    // Check for deprecated parameters and warn/fatal
    // Currently deprecated - network_num_vc, statistic, direct_link
    bool found;
    params.find<int>("statistics", 0, found);
    if (found) {
        out.output("%s, **WARNING** ** Found deprecated parameter: statistics **  memHierarchy statistics have been moved to the Statistics API. Please see sst-info to view available statistics and update your input deck accordingly.\nNO statistics will be printed otherwise! Remove this parameter from your deck to eliminate this message.\n", getName().c_str());
    }

    params.find<int>("network_num_vc", 0, found);
    if (found) {
        out.output("%s, ** Found deprecated parameter: network_num_vc ** MemHierarchy does not use multiple virtual channels. Remove this parameter from your input deck to eliminate this message.\n", getName().c_str());
    }

    params.find<int>("direct_link", 0, found);
    if (found) {
        out.output("%s, ** Found deprecated parameter: direct_link ** The value of this parameter is now auto-detected by the link configuration in your input deck. Remove this parameter from your input deck to eliminate this message.\n", getName().c_str());
    }


    string link_lat         = params.find<std::string>("direct_link_latency", "10 ns");

    /* MemController supports multiple ways of loading in backends:
     *  Legacy:
     *      Define backend and/or backendConvertor in the memcontroller's parameter set
     *  Better way:
     *      Fill backend slot with backend and memcontroller loads the compatible convertor
     *
     */

    MemBackend * memory = loadUserSubComponent<MemBackend>("backend");
    if (!memory) {  /* Try to load from our parameters (legacy mode 1) */
        /* Check if there's an error with the subcomponent the user specified */
        SubComponentSlotInfo * info = getSubComponentSlotInfo("backend");
        if (info && info->isPopulated(0)) {
            out.fatal(CALL_INFO, -1, "%s, ERROR: Unable to load the subcomponent in the 'backend' slot. Check that the requested subcomponent is registered with the SST core.\n", 
                    getName().c_str());
        } else {
            out.output("%s, WARNING: loading backend in legacy mode (from parameter set). Instead, load backend into this controller's 'backend' slot via ctrl.setSubComponent() in configuration.\n", getName().c_str());
        }
        Params tmpParams = params.get_scoped_params("backendConvertor.backend");
        std::string name = params.find<std::string>("backendConvertor.backend", "memHierarchy.simpleMem");
        memory = loadAnonymousSubComponent<MemBackend>(name, "backend", 0, ComponentInfo::INSERT_STATS | ComponentInfo::SHARE_PORTS, tmpParams);
        if (!memory) {
            out.fatal(CALL_INFO, -1, "%s, Error: unable to load backend '%s'. Use setSubComponent() on this controller to specify backend in your input configuration; check for valid backend name.\n",
                    getName().c_str(), name.c_str());
        }
    }

    std::string convertortype = memory->getBackendConvertorType();
    Params tmpParams = params.get_scoped_params("backendConvertor");
    memBackendConvertor_ = loadAnonymousSubComponent<MemBackendConvertor>(convertortype, "backendConvertor", 0, ComponentInfo::INSERT_STATS, tmpParams, memory, requestWidth);

    if (memBackendConvertor_ == nullptr) {
        out.fatal(CALL_INFO, -1, "%s, Error - unable to load MemBackendConvertor.", getName().c_str());
    }

    using std::placeholders::_1;
    using std::placeholders::_2;
    memBackendConvertor_->setCallbackHandlers(std::bind(&MemController::handleMemResponse, this, _1, _2), std::bind(&MemController::turnClockOn, this));
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
        //lists->createAll<CacheListener>(listeners_, false, ComponentInfo::SHARE_NONE);
    } else { // Manually load via the old way of doing it
        const uint32_t listenerCount  = params.find<uint32_t>("listenercount", 0);
        char* nextListenerName   = (char*) malloc(sizeof(char) * 64);
        char* nextListenerParams = (char*) malloc(sizeof(char) * 64);

        for (uint32_t i = 0; i < listenerCount; ++i) {
            sprintf(nextListenerName, "listener%" PRIu32, i);
            string listenerMod     = params.find<std::string>(nextListenerName, "");

            if (listenerMod != "") {
                sprintf(nextListenerParams, "listener%" PRIu32 "", i);
                Params listenerParams = params.get_scoped_params(nextListenerParams);

                CacheListener* loadedListener = loadAnonymousSubComponent<CacheListener>(listenerMod, "listener", i, ComponentInfo::INSERT_STATS, listenerParams);
                listeners_.push_back(loadedListener);
            }
        }
        free(nextListenerName);
        free(nextListenerParams);
    }

    // Memory region - overwrite with what we got if we got some
    bool gotRegion = false;
    uint64_t addrStart = params.find<uint64_t>("addr_range_start", 0, gotRegion);
    uint64_t addrEnd = params.find<uint64_t>("addr_range_end", (uint64_t) - 1, found);
    gotRegion |= found;
    string ilSize = params.find<std::string>("interleave_size", "0B", found);
    gotRegion |= found;
    string ilStep = params.find<std::string>("interleave_step", "0B", found);
    gotRegion |= found;

    // Ensure SI units are power-2 not power-10 - for backward compability
    fixByteUnits(ilSize);
    fixByteUnits(ilStep);

    if (!UnitAlgebra(ilSize).hasUnits("B")) {
        dbg.fatal(CALL_INFO, -1, "Invalid param(%s): interleave_size - must be specified in bytes with units (SI units OK). For example, '1KiB'. You specified '%s'\n",
                getName().c_str(), ilSize.c_str());
    }

    if (!UnitAlgebra(ilStep).hasUnits("B")) {
        dbg.fatal(CALL_INFO, -1, "Invalid param(%s): interleave_step - must be specified in bytes with units (SI units OK). For example, '1KiB'. You specified '%s'\n",
                getName().c_str(), ilSize.c_str());
    }

    region_.start = addrStart;
    region_.end = addrEnd;
    region_.interleaveSize = UnitAlgebra(ilSize).getRoundedValue();
    region_.interleaveStep = UnitAlgebra(ilStep).getRoundedValue();

    link_ = loadUserSubComponent<MemLinkBase>("cpulink");

    if (!link_ && isPortConnected("direct_link")) {
        Params linkParams = params.get_scoped_params("cpulink");
        linkParams.insert("port", "direct_link");
        linkParams.insert("latency", link_lat, false);
        linkParams.insert("accept_region", "1", false);
        link_ = loadAnonymousSubComponent<MemLinkBase>("memHierarchy.MemLink", "cpulink", 0, ComponentInfo::SHARE_PORTS | ComponentInfo::INSERT_STATS, linkParams);
    } else if (!link_) {

        if (!isPortConnected("network")) {
            out.fatal(CALL_INFO,-1,"%s, Error: No connected port detected. Connect 'direct_link' or 'network' port.\n", getName().c_str());
        }

        Params nicParams = params.get_scoped_params("memNIC");
        nicParams.insert("group", "4", false);
        nicParams.insert("accept_region", "1", false);

        if (isPortConnected("network_ack") && isPortConnected("network_fwd") && isPortConnected("network_data")) {
            nicParams.insert("req.port", "network");
            nicParams.insert("ack.port", "network_ack");
            nicParams.insert("fwd.port", "network_fwd");
            nicParams.insert("data.port", "network_data");
            link_ = loadAnonymousSubComponent<MemLinkBase>("memHierarchy.MemNICFour", "cpulink", 0, ComponentInfo::SHARE_PORTS | ComponentInfo::INSERT_STATS, nicParams);
        } else {
            nicParams.insert("port", "network");
            link_ = loadAnonymousSubComponent<MemLinkBase>("memHierarchy.MemNIC", "cpulink", 0, ComponentInfo::SHARE_PORTS | ComponentInfo::INSERT_STATS, nicParams);
        }
    }

    clockLink_ = link_->isClocked();
    link_->setRecvHandler( new Event::Handler<MemController>(this, &MemController::handleEvent));

    if (gotRegion) {
        link_->setRegion(region_);
    } else
        region_ = link_->getRegion();

    adjustRegionToMemSize();

    privateMemOffset_ = 0;

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
    clockHandler_ = new Clock::Handler<MemController>(this, &MemController::clock);
    clockTimeBase_ = registerClock(clockfreq, clockHandler_);
    clockOn_ = true;

    /* Custom command handler */
    using std::placeholders::_3;
    customCommandHandler_ = loadUserSubComponent<CustomCmdMemHandler>("customCmdHandler", ComponentInfo::SHARE_NONE,
            std::bind(static_cast<void(MemController::*)(Addr,size_t,std::vector<uint8_t>&)>(&MemController::readData), this, _1, _2, _3),
            std::bind(static_cast<void(MemController::*)(Addr,std::vector<uint8_t>*)>(&MemController::writeData), this, _1, _2));
    if (nullptr == customCommandHandler_) {
        std::string customHandlerName = params.find<std::string>("customCmdHandler", "");
        if (customHandlerName != "") {
            customCommandHandler_ = loadAnonymousSubComponent<CustomCmdMemHandler>(customHandlerName, "customCmdHandler", 0, ComponentInfo::INSERT_STATS, params,
                    std::bind(static_cast<void(MemController::*)(Addr,size_t,std::vector<uint8_t>&)>(&MemController::readData), this, _1, _2, _3),
                    std::bind(static_cast<void(MemController::*)(Addr,std::vector<uint8_t>*)>(&MemController::writeData), this, _1, _2));
        }
    }
}

void MemController::handleEvent(SST::Event* event) {
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

#ifdef __SST_DEBUG_OUTPUT__
    // Check that the request address(es) belong to this memory
    // Disabled except in debug mode
    if (!region_.contains(ev->getBaseAddr())) {
        out.fatal(CALL_INFO, -1, "%s, Error: Received an event with a base address that does not map to this controller. Event: %s\n", getName().c_str(), ev->getVerboseString().c_str());
    }
    if (!region_.contains(ev->getAddr())) {
        out.fatal(CALL_INFO, -1, "%s, Error: Received an event with an address that does not map to this controller. Event: %s\n", getName().c_str(), ev->getVerboseString().c_str());
    }
    
    bool noncacheable = ev->queryFlag(MemEvent::F_NONCACHEABLE);
    Addr chkAddr = noncacheable ? ev->getAddr() : ev->getBaseAddr();

    if (region_.interleaveStep != 0 && ev->getSize() > 0) {  // Non-contiguous address region so make sure this request doesn't fall outside the region
        Addr a0 = (chkAddr - region_.start) / region_.interleaveStep; 
        a0 = a0 * region_.interleaveStep + region_.start; // Address of the interleave chunk that contains the target address
        Addr b0 = a0 + region_.interleaveSize - 1; // Last address in the interleave chunk that contains the target starting address
        Addr a1 = a0 + region_.interleaveStep; // Address of the next interleave chunk

        // If the request size is larger than one of our interleaved chunks
        // then the only way for it to completely fall in our region is if our interleaving is contiguous (e.g., not reall interleaving)
        if (b0 < (chkAddr + ev->getSize() - 1)) {
            if ((b0 + 1) != a1) {
                out.fatal(CALL_INFO, -1, "%s: Error: Received an event for an address range that does not map to this controller. Event: %s\n", getName().c_str(), ev->getVerboseString().c_str());
            }
        }
    } else if (ev->getSize() > 0) { // Contiguous address region, make sure last address of request falls in region
        if (!region_.contains(chkAddr + ev->getSize() - 1))
        out.fatal(CALL_INFO, -1, "%s, Error: Received an event for an address range that does not map to this controller. Event: %s\n", getName().c_str(), ev->getVerboseString().c_str());
    }
#endif

    if (ev->isAddrGlobal()) {
        ev->setBaseAddr(translateToLocal(ev->getBaseAddr()));
        ev->setAddr(translateToLocal(ev->getAddr()));
    }


    // Notify our listeners that we have received an event
    notifyListeners( ev );

    switch (cmd) {
        case Command::PutM:
            ev->setFlag(MemEvent::F_NORESPONSE);
        case Command::GetS:
        case Command::GetX:
        case Command::GetSX:
            outstandingEvents_.insert(std::make_pair(ev->getID(), ev));
            memBackendConvertor_->handleMemEvent( ev );
            break;

        case Command::FlushLine:
        case Command::FlushLineInv:
            {
                MemEvent* put = NULL;
                if ( ev->getPayloadSize() != 0 ) {
                    put = new MemEvent(getName(), ev->getBaseAddr(), ev->getBaseAddr(), Command::PutM, ev->getPayload());
                    put->setFlag(MemEvent::F_NORESPONSE);
                    outstandingEvents_.insert(std::make_pair(put->getID(), put));
                    memBackendConvertor_->handleMemEvent( put );
                }

                outstandingEvents_.insert(std::make_pair(ev->getID(), ev));
                ev->setCmd(Command::FlushLine);
                memBackendConvertor_->handleMemEvent( ev );

            }
            break;

        case Command::PutS:
        case Command::PutE:
            delete ev;
            break;
        default:
            out.fatal(CALL_INFO,-1,"Memory controller received unrecognized command: %s", CommandString[(int)cmd]);
    }
}

bool MemController::clock(Cycle_t cycle) {
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

Cycle_t MemController::turnClockOn() {
    Cycle_t cycle = reregisterClock(clockTimeBase_, clockHandler_);
    cycle--;
    clockOn_ = true;
    return cycle;
}

void MemController::handleCustomEvent(MemEventBase * ev) {
    if (!customCommandHandler_)
        out.fatal(CALL_INFO, -1, "%s, Error: Received custom event but no handler loaded. Ev = %s. Time = %" PRIu64 "ns\n",
                getName().c_str(), ev->getVerboseString().c_str(), getCurrentSimTimeNano());

    CustomCmdMemHandler::MemEventInfo evInfo = customCommandHandler_->receive(ev);
    if (evInfo.shootdown) {
        out.verbose(CALL_INFO, 1, 0, "%s, WARNING: Custom event expects a shootdown but this memory controller does not support shootdowns. Ev = %s\n", getName().c_str(), ev->getVerboseString().c_str());
    }

    CustomCmdInfo * info = customCommandHandler_->ready(ev);
    outstandingEvents_.insert(std::make_pair(ev->getID(), ev));
    memBackendConvertor_->handleCustomEvent(info);
}


void MemController::handleMemResponse( Event::id_type id, uint32_t flags ) {

    std::map<SST::Event::id_type,MemEventBase*>::iterator it = outstandingEvents_.find(id);
    if (it == outstandingEvents_.end())
        out.fatal(CALL_INFO, -1, "Memory controller (%s) received unrecognized response ID: %" PRIu64 ", %" PRIu32 "", getName().c_str(), id.first, id.second);

    MemEventBase * evb = it->second;
    outstandingEvents_.erase(it);

    if (is_debug_event(evb)) {
        Debug(_L3_, "Memory Controller: %s - Response received to (%s)\n", getName().c_str(), evb->getVerboseString().c_str());
    }

    /* Handle custom events */
    if (evb->getCmd() == Command::CustomReq) {
        MemEventBase * resp = customCommandHandler_->finish(evb, flags);
        if (resp != nullptr)
            link_->send(resp);
        delete evb;
        return;
    }

    /* Handle MemEvents */
    MemEvent * ev = static_cast<MemEvent*>(evb);

    bool noncacheable  = ev->queryFlag(MemEvent::F_NONCACHEABLE);

    /* Write data. Here instead of receive to try to match backing access order to backend execute order */
    if (backing_ && (ev->getCmd() == Command::PutM || (ev->getCmd() == Command::GetX && noncacheable)))
        writeData(ev);

    if (ev->queryFlag(MemEvent::F_NORESPONSE)) {
        delete ev;
        return;
    }

    MemEvent * resp = ev->makeResponse();

    /* Read order matches execute order so that mis-ordering at backend can result in bad data */
    if (resp->getCmd() == Command::GetSResp || (resp->getCmd() == Command::GetXResp && !noncacheable)) {
        readData(resp);
        if (!noncacheable) resp->setCmd(Command::GetXResp);
    }

    resp->setFlags(flags);

    if (ev->isAddrGlobal()) {
        resp->setBaseAddr(translateToGlobal(ev->getBaseAddr()));
        resp->setAddr(translateToGlobal(ev->getAddr()));
    }

    link_->send( resp );
    delete ev;
}

void MemController::init(unsigned int phase) {
    link_->init(phase);

    region_ = link_->getRegion(); // This can change during init, but should stabilize before we start receiving init data

    adjustRegionToMemSize();

    /* Inherit region from our source(s) */
    if (!phase) {
        /* Announce our presence on link */
        link_->sendInitData(new MemEventInitCoherence(getName(), Endpoint::Memory, true, false, memBackendConvertor_->getRequestWidth(), false));
    }

    while (MemEventInit *ev = link_->recvInitData()) {
        processInitEvent(ev);
    }
}

void MemController::setup(void) {
    memBackendConvertor_->setup();
    link_->setup();

}


void MemController::finish(void) {
    if (!clockOn_) {
        Cycle_t cycle = turnClockOn();
        memBackendConvertor_->turnClockOn(cycle);
    }

    backing_->printContents(dbg);

    memBackendConvertor_->finish();
    link_->finish();
}

void MemController::writeData(MemEvent* event) {
    /* Noncacheable events occur on byte addresses, others on line addresses */
    bool noncacheable = event->queryFlag(MemEvent::F_NONCACHEABLE);
    Addr addr = noncacheable ? event->getAddr() : event->getBaseAddr();

    if (event->getCmd() == Command::PutM) { /* Write request to memory */
        if (is_debug_event(event)) { Debug(_L4_, "\tUpdate backing. Addr = %" PRIx64 ", Size = %i\n", addr, event->getSize()); }

        backing_->set(addr, event->getSize(), event->getPayload());
        if (is_debug_event(event)) {
            Debug(_L4_, "\tContents ");
            for( auto i : event->getPayload() )
                std::cerr << uint32_t(i) << " ";
            std::cerr << "\n";
        }

        return;
    }

    if (noncacheable && event->getCmd() == Command::GetX) {
        if (is_debug_event(event)) { Debug(_L4_, "\tUpdate backing. Addr = %" PRIx64 ", Size = %i\n", addr, event->getSize()); }

        backing_->set(addr, event->getSize(), event->getPayload());
        if (is_debug_event(event)) {
            Debug(_L4_, "\tContents ");
            for( auto i : event->getPayload() )
                std::cerr << uint32_t(i) << " ";
            std::cerr << "\n";
        }

        return;
    }

}


void MemController::readData(MemEvent* event) {
    bool noncacheable = event->queryFlag(MemEvent::F_NONCACHEABLE);
    Addr localAddr = noncacheable ? event->getAddr() : event->getBaseAddr();

    vector<uint8_t> payload;
    payload.resize(event->getSize(), 0);

    if (backing_)
        backing_->get(localAddr, event->getSize(), payload);

    event->setPayload(payload);
}


/* Backing store interactions for custom command subcomponents */
void MemController::writeData(Addr addr, std::vector<uint8_t> * data) {
    if (!backing_) return;

    for (size_t i = 0; i < data->size(); i++)
        backing_->set(addr + i, data->at(i));
}


void MemController::readData(Addr addr, size_t bytes, std::vector<uint8_t> &data) {
    data.resize(bytes, 0);

    if (!backing_) return;

    for (size_t i = 0; i < bytes; i++)
        data[i] = backing_->get(addr + i);
}


/* Translations assume interleaveStep is divisible by interleaveSize */
Addr MemController::translateToLocal(Addr addr) {
    Addr rAddr = addr;
    if (region_.interleaveSize == 0) {
        rAddr = rAddr - region_.start + privateMemOffset_;
    } else {
        Addr shift = rAddr - region_.start;
        Addr step = shift / region_.interleaveStep;
        Addr offset = shift % region_.interleaveStep;
        rAddr = (step * region_.interleaveSize) + offset + privateMemOffset_;
    }
    if (is_debug_addr(addr)) { Debug(_L10_,"\tConverting global address 0x%" PRIx64 " to local address 0x%" PRIx64 "\n", addr, rAddr); }
    return rAddr;
}


Addr MemController::translateToGlobal(Addr addr) {
    Addr rAddr = addr - privateMemOffset_;
    if (region_.interleaveSize == 0) {
        rAddr += region_.start;
    } else {
        Addr offset = rAddr % region_.interleaveSize;
        rAddr -= offset;
        rAddr = rAddr / region_.interleaveSize;
        rAddr = rAddr * region_.interleaveStep + offset + region_.start;
    }
    if (is_debug_addr(rAddr)) { Debug(_L10_,"\tConverting local address 0x%" PRIx64 " to global address 0x%" PRIx64 "\n", addr, rAddr); }
    return rAddr;
}


void MemController::processInitEvent( MemEventInit* me ) {
    /* Push data to memory */
    if (Command::GetX == me->getCmd()) {
        me->setAddr(translateToLocal(me->getAddr()));
        Addr addr = me->getAddr();
        if (is_debug_event(me)) { Debug(_L9_,"Memory init %s - Received GetX for %" PRIx64 " size %zu\n", getName().c_str(), me->getAddr(),me->getPayload().size()); }
        if ( isRequestAddressValid(addr) && backing_ ) {
            backing_->set(addr, me->getPayload().size(), me->getPayload());
            if (is_debug_event(me)) {
                Debug(_L4_, "\tContents ");
                for( auto i : me->getPayload() )
                    std::cerr << uint32_t(i) << " ";
                std::cerr << "\n";
            }
        }
    } else if (Command::NULLCMD == me->getCmd()) {
        if (is_debug_event(me)) { Debug(_L9_, "Memory (%s) received init event: %s\n", getName().c_str(), me->getVerboseString().c_str()); }
    } else {
        out.debug(_L10_,"Memory received unexpected Init Command: %d\n", (int)me->getCmd());
    }

    delete me;
}
    

void MemController::adjustRegionToMemSize() {
    // Check memSize_ against region
    // Set region_ to the smaller of the two
    // It's sometimes useful to be able to adjust one of the params and not the other
    // So, a mismatch is likely not an error, but alert the user in debug mode just in case
    // TODO deprecate mem_size & just use region? 
    uint64_t regSize = region_.end - region_.start;
    if (regSize != ((uint64_t) - 1)) {  // The default is for region_.end = uint64_t -1, but then if we add one we wrap to 0...
        regSize--;
    }
    if (region_.interleaveStep != 0) {
        uint64_t steps = regSize / region_.interleaveStep;
        regSize = steps * region_.interleaveSize;

        if (regSize > memSize_) { /* Reduce the end point so that regSize is no larger than memSize */
#ifdef __SST_DEBUG_OUTPUT__
            out.output("%s, Notice: memory controller's region is larger than the backend's mem_size, controller is limiting accessible memory to mem_size\n"
                    "Region: start=%" PRIu64 ", end=%" PRIu64 ", interleaveStep=%" PRIu64 ", interleaveSize=%" PRIu64 ". MemSize: %" PRIu64 "B\n",
                    getName().c_str(), region_.start, region_.end, region_.interleaveStep, region_.interleaveSize, memSize_);
#endif
            steps = memSize_ / region_.interleaveSize;
            regSize = steps * region_.interleaveStep;
            region_.end = region_.start + regSize - 1;
        }
    } else if (regSize > memSize_) {
#ifdef __SST_DEBUG_OUTPUT__
        out.output("%s, Notice: memory controller's region is larger than the backend's mem_size, controller is limiting accessible memory to mem_size\n"
                "Region: start=%" PRIu64 ", end=%" PRIu64 ", interleaveStep=%" PRIu64 ", interleaveSize=%" PRIu64 ". MemSize: %" PRIu64 "B\n",
                getName().c_str(), region_.start, region_.end, region_.interleaveStep, region_.interleaveSize, memSize_);
#endif
        region_.end = region_.start + memSize_ - 1;
    }

    // Synchronize our region with link in case we adjusted it above
    link_->setRegion(region_);
}

void MemController::printStatus(Output &statusOut) {
    statusOut.output("MemHierarchy::MemoryController %s\n", getName().c_str());

    statusOut.output("  Outstanding events: %zu\n", outstandingEvents_.size());
    for (std::map<SST::Event::id_type, MemEventBase*>::iterator it = outstandingEvents_.begin(); it != outstandingEvents_.end(); it++) {
        statusOut.output("    %s\n", it->second->getVerboseString().c_str());
    }

    statusOut.output("  Link Status: ");
    if (link_)
        link_->printStatus(statusOut);

    statusOut.output("End MemHierarchy::MemoryController\n\n");
}

void MemController::emergencyShutdown() {
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
