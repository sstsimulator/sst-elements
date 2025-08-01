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

#include <sst/core/sst_config.h>
#include <sst/core/params.h>


#include "memoryController.h"
#include "util.h"

#include "membackend/memBackendConvertor.h"
#include "membackend/memBackend.h"
#include "memEventBase.h"
#include "memEvent.h"
#include "memEventCustom.h"
#include "cacheListener.h"
#include "memNIC.h"
#include "memLink.h"

#define SST_MEMH_NO_STRING_DEFINED "N/A"

using namespace std;
using namespace SST;
using namespace SST::MemHierarchy;

/* Debug macros included from util.h */
/*
 *  Debug levels:
 *  3  - event receive/response
 *  4  - backing store
 *  9  - init()
 *  10 - address translation
 */

/*************************** Memory Controller ********************/
MemController::MemController(ComponentId_t id, Params &params) : Component(id), backing_(NULL) {

    dlevel = params.find<int>("debug_level", 0);

    fixupParams( params, "backend", "backendConvertor.backend" );
    fixupParams( params, "backend.", "backendConvertor.backend." );
    fixupParams( params, "request_width", "backendConvertor.request_width" );
    fixupParams( params, "max_requests_per_cycle", "backendConvertor.backend.max_requests_per_cycle" );

    uint32_t requestWidth = params.find<uint32_t>("backendConvertor.request_width", 64);

    // Output for debug
    dbg.init("", dlevel, 0, (Output::output_location_t)params.find<int>("debug", 0));

    checkpointDir_ = params.find<std::string>("checkpointDir", "");
    auto tmp = params.find<std::string>("checkpoint", "");
    if ( ! tmp.empty() ) {
        assert( ! checkpointDir_.empty() );
        if ( 0 == tmp.compare("load") ) {
            checkpoint_ = CHECKPOINT_LOAD;
        } else if ( 0 == tmp.compare("save") ) {
            checkpoint_ = CHECKPOINT_SAVE;
        } else {
            assert(0);
        }
    } else {
        checkpoint_ = NO_CHECKPOINT;
    }

    bool initBacking = params.find<bool>("backing_init_zero", false);
    // Debug address
    std::vector<Addr> addrArr;
    params.find_array<Addr>("debug_addr", addrArr);
    for (std::vector<Addr>::iterator it = addrArr.begin(); it != addrArr.end(); it++) {
        debug_addr_filter_.insert(*it);
    }

    // Output for warnings
    out.init("", params.find<int>("verbose", 1), 0, Output::STDOUT);

    // Check for deprecated parameters and warn/fatal
    // Currently deprecated - network_num_vc, statistic, direct_link
    bool found;

    /* Clock Handler */
    std::string clockfreq = params.find<std::string>("clock");
    UnitAlgebra clock_ua(clockfreq);
    if (!(clock_ua.hasUnits("Hz") || clock_ua.hasUnits("s")) || clock_ua.getRoundedValue() <= 0) {
        out.fatal(CALL_INFO, -1, "%s, ERROR - Invalid parameter: 'clock'. Must have units of Hz or s and be > 0. (SI prefixes ok). You specified '%s'\n", getName().c_str(), clockfreq.c_str());
    }

    clockHandler_ = new Clock::Handler2<MemController, &MemController::clock>(this);
    clockTimeBase_ = registerClock(clockfreq, clockHandler_);
    clockOn_ = true;

    backing_outscreen_ = params.find<bool>("backing_out_screen", false);

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
            out.output("%s, WARNING: Loaded backend in legacy mode (from parameter set). Instead, load backend into this controller's 'backend' slot via ctrl.setSubComponent() in configuration.\n", getName().c_str());
        }
        Params tmpParams = params.get_scoped_params("backendConvertor.backend");
        std::string name = params.find<std::string>("backendConvertor.backend", "memHierarchy.simpleMem");
        memory = loadAnonymousSubComponent<MemBackend>(name, "backend", 0, ComponentInfo::INSERT_STATS | ComponentInfo::SHARE_PORTS, tmpParams);
        if (!memory) {
            out.fatal(CALL_INFO, -1, "%s, ERROR: Unable to load backend '%s'. Use setSubComponent() on this controller to specify backend in your input configuration; check for valid backend name.\n",
                    getName().c_str(), name.c_str());
        }
    }

    std::string convertortype = memory->getBackendConvertorType();
    Params tmpParams = params.get_scoped_params("backendConvertor");
    memBackendConvertor_ = loadAnonymousSubComponent<MemBackendConvertor>(convertortype, "backendConvertor", 0, ComponentInfo::INSERT_STATS, tmpParams, memory, requestWidth);

    if (memBackendConvertor_ == nullptr) {
        out.fatal(CALL_INFO, -1, "%s, ERROR: Unable to load MemBackendConvertor.", getName().c_str());
    }

    using std::placeholders::_1;
    using std::placeholders::_2;
    memBackendConvertor_->setCallbackHandlers(std::bind(&MemController::handleMemResponse, this, _1, _2), std::bind(&MemController::turnClockOn, this));
    memSize_ = memBackendConvertor_->getMemSize();
    if (memSize_ == 0)
        out.fatal(CALL_INFO, -1, "%s, ERROR: Tried to get memory size from backend but size is 0B. Either backend is missing a 'mem_size' parameter or value is invalid.\n", getName().c_str());

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
        size_t bufferSize = sizeof(char) * 64;
        char* nextListenerName   = (char*) malloc(bufferSize);
        char* nextListenerParams = (char*) malloc(bufferSize);

        for (uint32_t i = 0; i < listenerCount; ++i) {
            snprintf(nextListenerName, bufferSize, "listener%" PRIu32, i);
            string listenerMod     = params.find<std::string>(nextListenerName, "");

            if (listenerMod != "") {
                snprintf(nextListenerParams, bufferSize, "listener%" PRIu32 "", i);
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

    link_ = loadUserSubComponent<MemLinkBase>("highlink", ComponentInfo::SHARE_NONE, &clockTimeBase_);
    if (!link_) {
        link_ = loadUserSubComponent<MemLinkBase>("cpulink", ComponentInfo::SHARE_NONE, &clockTimeBase_);
        if (link_) {
            out.output("%s, WARNING - Deprecation: The 'cpulink' subcomponent slot has been renamed to 'highlink' to improve name standardization. Please change this in your input file.\n", getName().c_str());
        }
    }

    if (!link_ && isPortConnected("direct_link")) {
        Params linkParams = params.get_scoped_params("cpulink");
        linkParams.insert("port", "direct_link");
        linkParams.insert("latency", link_lat, false);
        link_ = loadAnonymousSubComponent<MemLinkBase>("memHierarchy.MemLink", "highlink", 0, ComponentInfo::SHARE_PORTS | ComponentInfo::INSERT_STATS, linkParams, &clockTimeBase_);
        out.output("%s, WARNING - Deprecation: To standardize port names across memHierarchy elements, the MemController's port 'direct_link' has been renamed to 'highlink'. The 'direct_link' port will be removed in SST 16.\n", getName().c_str());
    } else if (!link_ && isPortConnected("highlink")) {
        Params linkParams = params.get_scoped_params("highlink");
        linkParams.insert("port", "highlink");
        linkParams.insert("latency", link_lat, false);
        link_ = loadAnonymousSubComponent<MemLinkBase>("memHierarchy.MemLink", "highlink", 0, ComponentInfo::SHARE_PORTS | ComponentInfo::INSERT_STATS, linkParams, &clockTimeBase_);
    } else if (!link_) {

        if (!isPortConnected("network")) {
            out.fatal(CALL_INFO,-1,"%s, ERROR: No connected port detected. Connect 'highlink' port, or if this memory is on a network, fill the 'highlink' subcomponent slot with memHierarchy.MemNIC or memHierarchy.MemNICFour.\n", getName().c_str());
        }

        out.output("%s, WARNING - Deprecation: Use of the network* ports in MemController is deprecated. Instead, fill this component's 'highlink' subcomponent slot with memHierarchy.MemNIC or memHierarchy.MemNICFour\n", getName().c_str());

        Params nicParams = params.get_scoped_params("memNIC");
        nicParams.insert("group", "4", false);

        if (isPortConnected("network_ack") && isPortConnected("network_fwd") && isPortConnected("network_data")) {
            nicParams.insert("req.port", "network");
            nicParams.insert("ack.port", "network_ack");
            nicParams.insert("fwd.port", "network_fwd");
            nicParams.insert("data.port", "network_data");
            link_ = loadAnonymousSubComponent<MemLinkBase>("memHierarchy.MemNICFour", "highlink", 0, ComponentInfo::SHARE_PORTS | ComponentInfo::INSERT_STATS, nicParams, &clockTimeBase_);
        } else {
            nicParams.insert("port", "network");
            link_ = loadAnonymousSubComponent<MemLinkBase>("memHierarchy.MemNIC", "highlink", 0, ComponentInfo::SHARE_PORTS | ComponentInfo::INSERT_STATS, nicParams, &clockTimeBase_);
        }
    }

    clockLink_ = link_->isClocked();
    link_->setRecvHandler( new Event::Handler2<MemController, &MemController::handleEvent>(this));

    link_->setRegion(region_);

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
        out.fatal(CALL_INFO, -1, "%s, ERROR - Invalid parameter: 'backing'. Must be one of 'none', 'malloc', or 'mmap'. You specified: %s\n",
                getName().c_str(), backingType.c_str());
    }

    std::string size = params.find<std::string>("backing_size_unit", "1MiB");
    UnitAlgebra size_ua(size);
    if (!size_ua.hasUnits("B")) {
        out.fatal(CALL_INFO, -1, "%s, ERROR - Invalid parameter: 'backing_size_unit'. Must have units of bytes (B). SI ok. You specified: %s\n",
                getName().c_str(), size.c_str());
    }
    size_t sizeBytes = size_ua.getRoundedValue();

    if (sizeBytes > memBackendConvertor_->getMemSize()) {
        sizeBytes = memBackendConvertor_->getMemSize();
        // Since getMemSize() might not be a power of 2, but malloc store needs it....get a reasonably close power of 2
        sizeBytes = 1 << log2Of(memBackendConvertor_->getMemSize());
    }

    /* Create the backing store */
    std::string infile = params.find<std::string>("backing_in_file", "");
    backing_outfile_ = params.find<std::string>("backing_out_file", "", found);

    if (backingType == "mmap") {
        if (!found) { // memory_file deprecated in SST 15, remove SST 16
            backing_outfile_ = params.find<std::string>("memory_file", "", found );
            if (found) {
                out.output("%s, WARNING - Deprecation: The parameter 'memory_file' is deprecated. Use 'backing_in_file' and/or 'backing_out_file' instead.", getName().c_str());
                if ( 0 == backing_outfile_.compare( SST_MEMH_NO_STRING_DEFINED ) ) {
                    backing_outfile_.clear();
                }
                infile = backing_outfile_;
            }
        }
            // Place backing file in sst output directory
            if ( backing_outfile_ != infile && infile != "")
                backing_outfile_ = SST::Util::Filesystem::getAbsolutePath(backing_outfile_, getOutputDirectory());
        try {
            backing_ = new Backend::BackingMMAP( backing_outfile_, infile, memBackendConvertor_->getMemSize() );
        }
        catch ( int e ) {
            if ( e == 1 )
                out.fatal(CALL_INFO, -1, "%s, ERROR: Unable to open backing_out_file. Requested file is '%s'.\n", getName().c_str(), backing_outfile_.c_str());
            else if ( e == 3 )
                out.fatal(CALL_INFO, -1, "%s, ERROR: Unable to open backing_in_file. Requested file is '%s'.\n", getName().c_str(), infile.c_str());
            else if ( e == 2 ) {
                if ( backing_outfile_ == "" && infile == "" ) {
                    out.verbose(CALL_INFO, 1, 0, "%s, WARNING: Could not MMAP backing store (likely, simulated memory exceeds available memory space). Creating malloc based store instead.\n", getName().c_str());
                    backing_ = new Backend::BackingMalloc(sizeBytes,initBacking);
                } else if ( infile != "" ) {
                    out.fatal(CALL_INFO, -1, "%s, ERROR: Could not MMAP backing store (likely, simulated memory exceeds available memory). Cannot initialize malloc based store from provided mmap input file %s.\n", getName().c_str(), infile.c_str());
                } else {
                    out.fatal(CALL_INFO, -1, "%s, ERROR: Could not MMAP backing store from file %s\n", getName().c_str(), backing_outfile_.c_str());
                }
            } else if ( e == 4 ) {
                out.fatal(CALL_INFO, -1, "%s, ERROR: Could not MMAP input file %s\n", getName().c_str(), infile.c_str());
            } else
                out.fatal(CALL_INFO, -1, "%s, ERROR: Unable to create backing store. Exception thrown is %d.\n", getName().c_str(), e);
        }
    } else if ( backingType == "malloc" ) {
        if ( infile != "" ) {
            try {
                backing_ = new Backend::BackingMalloc(infile);
            } catch (int e) {
                if ( e == 1 ) {
                    out.fatal(CALL_INFO, -1, "%s, ERROR: Unable to open 'backing_in_file'. Does file exist? Filename='%s'\n", getName().c_str(), infile.c_str());
                } else
                    out.fatal(CALL_INFO, -1, "%s, ERROR: Unable to create backing store. Exception thrown is %d.\n", getName().c_str(), e);
            }
        } else {
            backing_ = new Backend::BackingMalloc(sizeBytes,initBacking);
        }
        // Test outfile to find issues before simulation begins
        if ( backing_outfile_ != "" ) {
            auto fp = fopen(backing_outfile_.c_str(),"wb+");
            sst_assert(fp, CALL_INFO, -1, "%s, ERROR: Unable to open 'backing_out_file'. Is filepath accessible? Filename='%s'\n", getName().c_str(), backing_outfile_.c_str());
            fclose(fp);
        }
    } else {
        backing_outfile_ = "";
    }


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
    if (mem_h_is_debug_event(meb)) {
        mem_h_debug_output(_L3_, "E: %-20" PRIu64 " %-20" PRIu64 " %-20s Event:New     (%s)\n",
                    getCurrentSimCycle(), getNextClockCycle(clockTimeBase_) - 1, getName().c_str(), meb->getVerboseString(dlevel).c_str());
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
        out.fatal(CALL_INFO, -1, "%s, ERROR: Received an event with a base address that does not map to this controller. Event: %s\n", getName().c_str(), ev->getVerboseString(dlevel).c_str());
    }
    if (!region_.contains(ev->getAddr())) {
        out.fatal(CALL_INFO, -1, "%s, ERROR: Received an event with an address that does not map to this controller. Event: %s\n", getName().c_str(), ev->getVerboseString(dlevel).c_str());
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
                out.fatal(CALL_INFO, -1, "%s: ERROR: Received an event for an address range that does not map to this controller. Event: %s\n", getName().c_str(), ev->getVerboseString(dlevel).c_str());
            }
        }
    } else if (ev->getSize() > 0) { // Contiguous address region, make sure last address of request falls in region
        if (!region_.contains(chkAddr + ev->getSize() - 1))
        out.fatal(CALL_INFO, -1, "%s, ERROR: Received an event for an address range that does not map to this controller. Event: %s\n", getName().c_str(), ev->getVerboseString(dlevel).c_str());
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
        case Command::Write:
            outstandingEvents_.insert(std::make_pair(ev->getID(), ev));
            if (mem_h_is_debug_event(ev)) {
                mem_h_debug_output(_L4_, "B: %-20" PRIu64 " %-20" PRIu64 " %-20s Bkend:Send    (%s)\n",
                        getCurrentSimCycle(), getNextClockCycle(clockTimeBase_) - 1, getName().c_str(),
                        ev->getVerboseString().c_str());
            }
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
                    if (mem_h_is_debug_event(put)) {
                        mem_h_debug_output(_L4_, "B: %-20" PRIu64 " %-20" PRIu64 " %-20s Bkend:Send    (%s)\n",
                                getCurrentSimCycle(), getNextClockCycle(clockTimeBase_) - 1, getName().c_str(),
                                put->getVerboseString().c_str());
                    }
                    memBackendConvertor_->handleMemEvent( put );
                }

                outstandingEvents_.insert(std::make_pair(ev->getID(), ev));
                ev->setCmd(Command::FlushLine);
                if (mem_h_is_debug_event(ev)) {
                    mem_h_debug_output(_L4_, "B: %-20" PRIu64 " %-20" PRIu64 " %-20s Bkend:Send    (%s)\n",
                            getCurrentSimCycle(), getNextClockCycle(clockTimeBase_) - 1, getName().c_str(),
                            ev->getVerboseString().c_str());
                }
                memBackendConvertor_->handleMemEvent( ev );

            }
            break;
        case Command::FlushAll:
            {
                MemEvent * resp = ev->makeResponse();

                if (mem_h_is_debug_event(resp)) {
                    mem_h_debug_output(_L4_, "E: %-20" PRIu64 " %-20" PRIu64 " %-20s Event:Send    (%s)\n",
                        getCurrentSimCycle(), getNextClockCycle(clockTimeBase_) - 1, getName().c_str(), resp->getVerboseString(dlevel).c_str());
                }
                link_->send( resp );
                delete ev;
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
        out.fatal(CALL_INFO, -1, "%s, ERROR: Received custom event but no handler loaded. Ev = %s. Time = %" PRIu64 "ns\n",
                getName().c_str(), ev->getVerboseString(dlevel).c_str(), getCurrentSimTimeNano());

    CustomCmdMemHandler::MemEventInfo evInfo = customCommandHandler_->receive(ev);
    if (evInfo.shootdown) {
        out.verbose(CALL_INFO, 1, 0, "%s, WARNING: Custom event expects a shootdown but this memory controller does not support shootdowns. Ev = %s\n", getName().c_str(), ev->getVerboseString(dlevel).c_str());
    }

    Interfaces::StandardMem::CustomData* info = customCommandHandler_->ready(ev);
    outstandingEvents_.insert(std::make_pair(ev->getID(), ev));
    if (mem_h_is_debug_event(ev)) {
        mem_h_debug_output(_L4_, "B: %-20" PRIu64 " %-20" PRIu64 " %-20s Bkend:Send    (%s)\n",
                getCurrentSimCycle(), getNextClockCycle(clockTimeBase_) - 1, getName().c_str(),
                ev->getVerboseString().c_str());
    }
    memBackendConvertor_->handleCustomEvent(info, ev->getID(), ev->getRqstr());
}


void MemController::handleMemResponse( Event::id_type id, uint32_t flags ) {

    std::map<SST::Event::id_type,MemEventBase*>::iterator it = outstandingEvents_.find(id);
    if (it == outstandingEvents_.end())
        out.fatal(CALL_INFO, -1, "%s, ERROR: Memory controller received unrecognized response ID: %" PRIu64 ", %" PRIu32 "", getName().c_str(), id.first, id.second);

    MemEventBase * evb = it->second;
    outstandingEvents_.erase(it);

    if (mem_h_is_debug_event(evb)) {
        mem_h_debug_output(_L4_, "B: %-20" PRIu64 " %-20" PRIu64 " %-20s Bkend:Recv    (<%" PRIu64 ",%" PRIu32 ">)\n",
                    getCurrentSimCycle(), getNextClockCycle(clockTimeBase_) - 1, getName().c_str(), id.first, id.second);
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
    if (backing_ && (ev->getCmd() == Command::PutM || (ev->getCmd() == Command::Write)))
        writeData(ev);

    if (ev->queryFlag(MemEvent::F_NORESPONSE)) {
        delete ev;
        return;
    }

    MemEvent * resp = ev->makeResponse();

    /* Read order matches execute order so that mis-ordering at backend can result in bad data */
    if (resp->getCmd() == Command::GetSResp || resp->getCmd() == Command::GetXResp) {
        readData(resp);
        if (!resp->queryFlag(MemEvent::F_NONCACHEABLE)) resp->setCmd(Command::GetXResp);
    }

    resp->setFlags(flags);

    if (ev->isAddrGlobal()) {
        resp->setBaseAddr(translateToGlobal(ev->getBaseAddr()));
        resp->setAddr(translateToGlobal(ev->getAddr()));
    }

    if (mem_h_is_debug_event(resp)) {
        mem_h_debug_output(_L4_, "E: %-20" PRIu64 " %-20" PRIu64 " %-20s Event:Send    (%s)\n",
                getCurrentSimCycle(), getNextClockCycle(clockTimeBase_) - 1, getName().c_str(), resp->getVerboseString(dlevel).c_str());
    }

    link_->send( resp );
    delete ev;
}

void MemController::init(unsigned int phase) {
    link_->init(phase);

    region_ = link_->getRegion(); // This can change during init, but should stabilize before we start receiving init data

    adjustRegionToMemSize();

    if (!phase) {
        /* Announce our presence on link */
        link_->sendUntimedData(new MemEventInitCoherence(getName(), Endpoint::Memory, true, false, memBackendConvertor_->getRequestWidth(), false));
        link_->sendUntimedData(new MemEventInitEndpoint(getName().c_str(), Endpoint::Memory, region_, true));
    }

    while (MemEventInit *ev = link_->recvUntimedData()) {
        processInitEvent(ev);
    }
}

void MemController::setup(void) {
    memBackendConvertor_->setup();
    link_->setup();
}

void MemController::complete(unsigned int phase) {
    memBackendConvertor_->complete(phase);
    link_->complete(phase);

    // Initiate flush here if configured to do so
    if (!phase && (backing_outfile_ != "" || backing_outscreen_)) {
        MemEventUntimedFlush* flush = new MemEventUntimedFlush(getName());
        mem_h_debug_output(_L10_, "U: %-20s   Event:Untimed   (%s)\n", getName().c_str(), flush->getVerboseString().c_str());
        link_->sendUntimedData(flush, true); /* Broadcast to all sources */
    }

    while (MemEventInit *ev = link_->recvUntimedData()) {
        processInitEvent(ev);
    }
}

void MemController::finish(void) {
    Cycle_t cycle = getNextClockCycle(clockTimeBase_); // Get finish time
    cycle--;
    memBackendConvertor_->finish(cycle);
    link_->finish();
    if ( backing_outfile_ != "" ) {
        try {
            backing_->printToFile(backing_outfile_);
        } catch (int e) { // Don't fatal so late in simulation
            out.output("%s, WARNING: Unable to open file '%s' provided by parameter 'backing_out_file' to write memory contents. Memory contents will not be written.\n", getName().c_str(), backing_outfile_.c_str());
        }
    }
    if (backing_outscreen_) {
        backing_->printToScreen(privateMemOffset_, region_.start, region_.interleaveSize, region_.interleaveStep);
    }
}

void MemController::writeData(MemEvent* event) {
    if (event->getCmd() == Command::PutM) { /* Write request to memory */
        Addr addr = event->queryFlag(MemEvent::F_NONCACHEABLE) ? event->getAddr() : event->getBaseAddr();
        if (mem_h_is_debug_event(event)) {
            mem_h_debug_output(_L8_, "S: Update backing. Addr = %" PRIx64 ", Size = %i\n", addr, event->getSize());
            printDataValue(addr, &(event->getPayload()), true);
        }

        backing_->set(addr, event->getSize(), event->getPayload());

        return;
    }

    if (event->getCmd() == Command::Write) {
        Addr addr = event->getAddr();
        if (mem_h_is_debug_event(event)) {
            mem_h_debug_output(_L8_, "S: Update backing. Addr = %" PRIx64 ", Size = %i\n", addr, event->getSize());
            printDataValue(addr, &(event->getPayload()), true);
        }

        backing_->set(addr, event->getSize(), event->getPayload());

        return;
    }

}


void MemController::readData(MemEvent* event) {
    bool noncacheable = event->queryFlag(MemEvent::F_NONCACHEABLE);
    Addr localAddr = noncacheable ? event->getAddr() : event->getBaseAddr();

    vector<uint8_t> payload;
    payload.resize(event->getSize(), 0);

    if (backing_) {
        backing_->get(localAddr, event->getSize(), payload);
        if (mem_h_is_debug_addr(localAddr))
            printDataValue(localAddr, &(payload), false);
    }

    event->setPayload(payload);
}


/* Backing store interactions for custom command subcomponents */
void MemController::writeData(Addr addr, std::vector<uint8_t> * data) {
    if (!backing_) return;

    for (size_t i = 0; i < data->size(); i++) {
        backing_->set(addr + i, data->at(i));
    }

    if (mem_h_is_debug_addr(addr))
        printDataValue(addr, data, true);
}


void MemController::readData(Addr addr, size_t bytes, std::vector<uint8_t> &data) {
    data.resize(bytes, 0);

    if (!backing_) return;

    for (size_t i = 0; i < bytes; i++)
        data[i] = backing_->get(addr + i);

    if (mem_h_is_debug_addr(addr))
        printDataValue(addr, &data, false);
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
    if (mem_h_is_debug_addr(addr) && addr != rAddr) {
        mem_h_debug_output(_L10_, "C: %-40" PRIu64 "  %-20s ConvertAddr   Local, 0x%" PRIx64 ", 0x%" PRIx64"\n",
                    getCurrentSimCycle(), getName().c_str(), addr, rAddr);
    }
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
    if (mem_h_is_debug_addr(rAddr) && addr != rAddr) {
        mem_h_debug_output(_L10_, "C: %-40" PRIu64 "  %-20s ConvertAddr   Global, 0x%" PRIx64 ", 0x%" PRIx64"\n",
                    getCurrentSimCycle(), getName().c_str(), addr, rAddr);
    }
    return rAddr;
}


void MemController::processInitEvent( MemEventInit* me ) {
    /* Push data to memory */
    if ( Command::Write == me->getCmd() ) {
        if ( isRequestAddressValid(me->getAddr()) && backing_ ) {
            me->setAddr(translateToLocal(me->getAddr()));
            Addr addr = me->getAddr();
            mem_h_debug_output(_L10_, "U: %-20s   Event:Write     (%s)\n", getName().c_str(), me->getVerboseString().c_str());
            backing_->set(addr, me->getPayload().size(), me->getPayload());
        } else {
            mem_h_debug_output(_L10_, "U: %-20s   Event:Write     (%s) IGNORE\n", getName().c_str(), me->getVerboseString().c_str());
        }
    } else if ( Command::GetS == me->getCmd() ) {
        if ( isRequestAddressValid(me->getAddr()) ) {
            Addr local_addr = translateToLocal(me->getAddr());
            mem_h_debug_output(_L10_, "U: %-20s   Event:GetS      (%s)\n", getName().c_str(), me->getVerboseString().c_str());

            MemEventInit * resp = me->makeResponse();
            vector<uint8_t> payload;

            if (backing_) {
                backing_->get(local_addr, me->getSize(), payload);
            } else {
                payload.resize(me->getSize(), 0);
            }

            resp->setPayload(payload);
            mem_h_debug_output(_L10_, "U: %-20s   Event:Send      (%s)\n", getName().c_str(), resp->getVerboseString().c_str());
            link_->sendUntimedData(resp);
        } else {
            mem_h_debug_output(_L10_, "U: %-20s   Event:GetS      (%s) IGNORE\n", getName().c_str(), me->getVerboseString().c_str());
        }
    } else if (Command::NULLCMD == me->getCmd()) {
        mem_h_debug_output(_L10_, "U: %-20s   Event:Untimed   (%s)\n", getName().c_str(), me->getVerboseString().c_str());
    } else {
        mem_h_debug_output(_L10_, "U: %-20s   Event:Untimed   UNKNOWN COMMAND (%s)\n", getName().c_str(), me->getVerboseString().c_str());
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
    if (regSize != region_.REGION_MAX) {  // The default is for region_.end = uint64_t -1, but then if we add one we wrap...
        regSize++; // Since region_.end and region_.start are inclusive
    }
    if (region_.interleaveStep != 0) {
        uint64_t steps = regSize / region_.interleaveStep;
        regSize = steps * region_.interleaveSize;

        if (regSize > memSize_) { /* Reduce the end point so that regSize is no larger than memSize */
#ifdef __SST_DEBUG_OUTPUT__
            out.output("%s, Notice: memory controller's region is larger than the backend's mem_size, controller is limiting accessible memory to mem_size\n"
                    "Region: start=%" PRIu64 ", end=%" PRIu64 ", interleaveStep=%" PRIu64 ", interleaveSize=%" PRIu64 ". MemSize: %zuB\n",
                    getName().c_str(), region_.start, region_.end, region_.interleaveStep, region_.interleaveSize, memSize_);
#endif
            steps = memSize_ / region_.interleaveSize;
            regSize = steps * region_.interleaveStep;
            region_.end = region_.start + regSize - 1;
        }
    } else if (regSize > memSize_) {
#ifdef __SST_DEBUG_OUTPUT__
        out.output("%s, Notice: memory controller's region is larger than the backend's mem_size, controller is limiting accessible memory to mem_size\n"
                "Region: start=%" PRIu64 ", end=%" PRIu64 ", interleaveStep=%" PRIu64 ", interleaveSize=%" PRIu64 ". MemSize: %zuB\n",
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
        statusOut.output("    %s\n", it->second->getVerboseString(dlevel).c_str());
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

void MemController::printDataValue(Addr addr, std::vector<uint8_t>* data, bool set) {
    if (dlevel < 11) return;

    std::string action = set ? "WRITE" : "READ";
    std::stringstream value;
    value << std::hex << std::setfill('0');
    for (unsigned int i = 0; i < data->size(); i++) {
        value << std::hex << std::setw(2) << (int)data->at(i);
    }

    dbg.debug(_L11_, "V: %-20" PRIu64 " %-20" PRIu64 " %-20s %-13s 0x%-16" PRIx64 " B: %-3zu %s\n",
            getCurrentSimCycle(), getNextClockCycle(clockTimeBase_) - 1, getName().c_str(), action.c_str(),
            addr, data->size(), value.str().c_str());
}

/*
 * Default constructor
*/
MemController::MemController() : Component() {}

/*
 * Serialize function
*/
void MemController::serialize_order(SST::Core::Serialization::serializer& ser) {
    Component::serialize_order(ser);

    SST_SER(out);
    SST_SER(dbg);
    SST_SER(debug_addr_filter_);
    SST_SER(dlevel);

    SST_SER(memBackendConvertor_);

    if ( ser.mode() == SST::Core::Serialization::serializer::UNPACK ) {
        using std::placeholders::_1;
        using std::placeholders::_2;
        memBackendConvertor_->setCallbackHandlers(
            std::bind(&MemController::handleMemResponse, this, _1, _2),
            std::bind(&MemController::turnClockOn, this));
    }

    SST_SER(backing_);
    SST_SER(backing_outfile_);

    SST_SER(link_);
    SST_SER(clockLink_);

    //SST_SER(listeners_);
    SST_SER(checkpointDir_);

    SST_SER(memSize_);

    SST_SER(clockOn_);

    SST_SER(region_);
    SST_SER(privateMemOffset_);

    SST_SER(clockHandler_);
    SST_SER(clockTimeBase_);

    SST_SER(customCommandHandler_);

    SST_SER(outstandingEvents_);
}
