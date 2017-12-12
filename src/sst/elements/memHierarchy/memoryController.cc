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

#include "memoryController.h"
#include "util.h"

#include "membackend/memBackendConvertor.h"
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

/*************************** Memory Controller ********************/
MemController::MemController(ComponentId_t id, Params &params) : Component(id), backing_(NULL) {
            
    int debugLevel = params.find<int>("debug_level", 0);

    fixupParam( params, "backend", "backendConvertor.backend" );
    fixupParams( params, "backend.", "backendConvertor.backend." );
    fixupParams( params, "clock", "backendConvertor.backend.clock" );
    fixupParams( params, "request_width", "backendConvertor.request_width" );
    fixupParams( params, "max_requests_per_cycle", "backendConvertor.backend.max_requests_per_cycle" );

    // Output for debug
    dbg.init("", debugLevel, 0, (Output::output_location_t)params.find<int>("debug", 0));
    if (debugLevel < 0 || debugLevel > 10)
        dbg.fatal(CALL_INFO, -1, "Debugging level must be between 0 and 10. \n");

    // Debug address
    std::vector<Addr> addrArr;
    params.find_array<Addr>("debug_addr", addrArr);
    for (std::vector<Addr>::iterator it = addrArr.begin(); it != addrArr.end(); it++) {
        DEBUG_ADDR.insert(*it);
    }

    // Output for warnings
    Output out("", 1, 0, Output::STDOUT);
    
    // Check for deprecated parameters and warn/fatal
    // Currently deprecated - mem_size (replaced by backend.mem_size), network_num_vc, statistic, direct_link 
    bool found;
    params.find<int>("statistics", 0, found);
    if (found) {
        out.output("%s, **WARNING** ** Found deprecated parameter: statistics **  memHierarchy statistics have been moved to the Statistics API. Please see sst-info to view available statistics and update your input deck accordingly.\nNO statistics will be printed otherwise! Remove this parameter from your deck to eliminate this message.\n", getName().c_str());
    }
    params.find<int>("mem_size", 0, found);
    if (found) {
        out.fatal(CALL_INFO, -1, "%s, Error - you specified memory size by the \"mem_size\" parameter, this must now be backend.mem_size, change the parameter name in your input deck.\n", getName().c_str());
    }

    params.find<int>("network_num_vc", 0, found);
    if (found) {
        out.output("%s, ** Found deprecated parameter: network_num_vc ** MemHierarchy does not use multiple virtual channels. Remove this parameter from your input deck to eliminate this message.\n", getName().c_str());
    }

    params.find<int>("direct_link", 0, found);
    if (found) {
        out.output("%s, ** Found deprecated parameter: direct_link ** The value of this parameter is now auto-detected by the link configuration in your input deck. Remove this parameter from your input deck to eliminate this message.\n", getName().c_str());
    }

    std::string name        = params.find<std::string>("backendConvertor", "memHierarchy.simpleMemBackendConvertor");

    string link_lat         = params.find<std::string>("direct_link_latency", "10 ns");

    if (nullptr == (memBackendConvertor_ = dynamic_cast<MemBackendConvertor*>(loadNamedSubComponent("backendConvertor")))) {
        /* Load the old-fashioned way */
        Params tmpParams = params.find_prefix_params("backendConvertor.");
        memBackendConvertor_  = dynamic_cast<MemBackendConvertor*>(loadSubComponent(name, this, tmpParams));
    }

    if (memBackendConvertor_ == nullptr) {
        out.fatal(CALL_INFO, -1, "%s, Error - unable to load MemBackendConvertor.", getName().c_str());
    }

    memSize_ = memBackendConvertor_->getMemSize();

    const uint32_t listenerCount  = params.find<uint32_t>("listenercount", 0);
    char* nextListenerName   = (char*) malloc(sizeof(char) * 64);
    char* nextListenerParams = (char*) malloc(sizeof(char) * 64);

    for (uint32_t i = 0; i < listenerCount; ++i) {
        sprintf(nextListenerName, "listener%" PRIu32, i);
        string listenerMod     = params.find<std::string>(nextListenerName, "");

        if (listenerMod != "") {
            sprintf(nextListenerParams, "listener%" PRIu32 ".", i);
            Params listenerParams = params.find_prefix_params(nextListenerParams);

            CacheListener* loadedListener = dynamic_cast<CacheListener*>(loadSubComponent(listenerMod, this, listenerParams));
            listeners_.push_back(loadedListener);
        }
    }

    free(nextListenerName);
    free(nextListenerParams);


    if (isPortConnected("direct_link")) {
        Params linkParams = params.find_prefix_params("cpulink.");
        linkParams.insert("port", "direct_link");
        linkParams.insert("latency", link_lat, false);
        linkParams.insert("accept_region", "1", false);
        link_ = dynamic_cast<MemLink*>(loadSubComponent("memHierarchy.MemLink", this, linkParams));
        link_->setRecvHandler( new Event::Handler<MemController>(this, &MemController::handleEvent));
        clockLink_ = false;
    } else {

        if (!isPortConnected("network")) {
            dbg.fatal(CALL_INFO,-1,"%s, Error: No connected port detected. Connect 'direct_link' or 'network' port.\n", getName().c_str());
        }

        Params nicParams = params.find_prefix_params("memNIC.");
        nicParams.insert("port", "network");
        nicParams.insert("group", "4", false);
        nicParams.insert("accept_region", "1", false);

        link_ = dynamic_cast<MemNIC*>(loadSubComponent("memHierarchy.MemNIC", this, nicParams)); 
        link_->setRecvHandler( new Event::Handler<MemController>(this, &MemController::handleEvent) );
        clockLink_ = true;
    }
    
    region_ = link_->getRegion();
    privateMemOffset_ = 0;

    // Set up backing store if needed
    std::string backingType = params.find<std::string>("backing", "malloc", found); /* Default to using a malloc backing store */
    backing_ = nullptr;
    if (!found) {
        bool oldBackVal = params.find<bool>("do_not_back", false, found);
        if (found) {
            out.output("%s, ** Found deprecated parameter: do_not_back ** Use 'backing' parameter instead and specify 'none', 'malloc', or 'mmap'. Remove this parameter from your input deck to eliminate this message.\n", 
                    getName().c_str());
        }
        if (oldBackVal) backingType = "malloc";
    }

    if (backingType != "none" && backingType != "mmap" && backingType != "malloc") {
        out.fatal(CALL_INFO, -1, "%s, Error - Invalid param: backing. Must be one of 'none', 'malloc', or 'mmap'. You specified: %s\n",
                getName().c_str(), backingType.c_str());
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
                dbg.fatal(CALL_INFO, -1, "%s, Error - unable to open memory_file. You specified '%s'.\n", getName().c_str(), memoryFile.c_str());
            else if (e == 2)
                dbg.fatal(CALL_INFO, -1, "%s, Error - mmap of backing store failed.\n", getName().c_str());
            else 
                dbg.fatal(CALL_INFO, -1, "%s, Error - unable to create backing store. Exception thrown is %d.\n", getName().c_str(), e);
        }
    } else if (backingType == "malloc") {
        std::string size = params.find<std::string>("backing_size_hint", "1MiB");
        UnitAlgebra size_ua(size);
        if (!size_ua.hasUnits("B")) {
            out.fatal(CALL_INFO, -1, "%s, Error - Invalid param: backing_size_hint. Must have units of bytes (B). SI ok. You specified: %s\n",
                    getName().c_str(), size.c_str());
        }
        size_t sizeBytes = size_ua.getRoundedValue();
        if (sizeBytes > memBackendConvertor_->getMemSize() ) 
            sizeBytes = memBackendConvertor_->getMemSize();

        backing_ = new Backend::BackingMalloc(sizeBytes);
    }

    /* Clock Handler */
    clockHandler_ = new Clock::Handler<MemController>(this, &MemController::clock);
    clockTimeBase_ = registerClock(memBackendConvertor_->getClockFreq(), clockHandler_);
    clockOn_ = true;

    registerTimeBase("1 ns", true);

    /* Custom command handler */
    if (nullptr == (customCommandHandler_ = dynamic_cast<CustomCmdMemHandler*>(loadNamedSubComponent("customCmdHandler")))) {
        std::string customHandlerName = params.find<std::string>("customCmdHandler", "");
        if (customHandlerName != "") {
            customCommandHandler_ = dynamic_cast<CustomCmdMemHandler*>(loadSubComponent(customHandlerName, this, params));
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

    if (ev->isAddrGlobal()) {
        ev->setBaseAddr(translateToLocal(ev->getBaseAddr()));
        ev->setAddr(translateToLocal(ev->getAddr()));
    }


    // Notify our listeners that we have received an event
    switch (cmd) {
        case Command::PutM:
            ev->setFlag(MemEvent::F_NORESPONSE);
        case Command::GetS:
        case Command::GetX:
        case Command::GetSX:
            outstandingEvents_.insert(std::make_pair(ev->getID(), ev));
            notifyListeners( ev );
            memBackendConvertor_->handleMemEvent( ev );
            break;

        case Command::FlushLine:
        case Command::FlushLineInv:
            {
                MemEvent* put = NULL;
                if ( ev->getPayloadSize() != 0 ) {
                    put = new MemEvent(this, ev->getBaseAddr(), ev->getBaseAddr(), Command::PutM, ev->getPayload() );
                    put->setFlag(MemEvent::F_NORESPONSE);
                    outstandingEvents_.insert(std::make_pair(put->getID(), put));
                    notifyListeners(ev);
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
            dbg.fatal(CALL_INFO,-1,"Memory controller received unrecognized command: %s", CommandString[(int)cmd]);
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
        dbg.fatal(CALL_INFO, -1, "%s, Error: Received custom event but no handler loaded. Ev = %s. Time = %" PRIu64 "ns\n",
                getName().c_str(), ev->getVerboseString().c_str(), getCurrentSimTimeNano());

    CustomCmdMemHandler::MemEventInfo evInfo = customCommandHandler_->receive(ev);
    if (evInfo.shootdown) {
        Debug(_WARNING_, "%s, WARNING: Custom event expects a shootdown but this memory controller does not support shootdowns. Ev = %s\n", getName().c_str(), ev->getVerboseString().c_str());
    }
    
    CustomCmdInfo * info = customCommandHandler_->ready(ev);
    outstandingEvents_.insert(std::make_pair(ev->getID(), ev));
    memBackendConvertor_->handleCustomEvent(info);
}


void MemController::handleMemResponse( Event::id_type id, uint32_t flags ) {

    std::map<SST::Event::id_type,MemEventBase*>::iterator it = outstandingEvents_.find(id);
    if (it == outstandingEvents_.end())
        dbg.fatal(CALL_INFO, -1, "Memory controller (%s) received unrecognized response ID: %" PRIu64 ", %" PRIu32 "", getName().c_str(), id.first, id.second);

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
    
    /* Inherit region from our source(s) */
    if (!phase) {
        /* Announce our presence on link */
        link_->sendInitData(new MemEventInitCoherence(getName(), Endpoint::Memory, true, false, memBackendConvertor_->getRequestWidth()));
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
    memBackendConvertor_->finish();
    link_->finish();
}

void MemController::writeData(MemEvent* event) {
    /* Noncacheable events occur on byte addresses, others on line addresses */
    bool noncacheable = event->queryFlag(MemEvent::F_NONCACHEABLE);
    Addr addr = noncacheable ? event->getAddr() : event->getBaseAddr();

    if (event->getCmd() == Command::PutM) { /* Write request to memory */
        if (is_debug_event(event)) { Debug(_L4_, "\tUpdate backing. Addr = %" PRIx64 ", Size = %i\n", addr, event->getSize()); }
        
        for (size_t i = 0; i < event->getSize(); i++)
            backing_->set( addr + i, event->getPayload()[i] );
        return;
    }
    
    if (noncacheable && event->getCmd() == Command::GetX) {
        if (is_debug_event(event)) { Debug(_L4_, "\tUpdate backing. Addr = %" PRIx64 ", Size = %i\n", addr, event->getSize()); }
        
        for (size_t i = 0; i < event->getSize(); i++)
            backing_->set( addr + i, event->getPayload()[i] );
        return;
    }

}


void MemController::readData(MemEvent* event) { 
    bool noncacheable = event->queryFlag(MemEvent::F_NONCACHEABLE);
    Addr localAddr = noncacheable ? event->getAddr() : event->getBaseAddr();

    vector<uint8_t> payload;
    payload.resize(event->getSize(), 0);

    if (!backing_) return;

    for ( size_t i = 0 ; i < event->getSize() ; i++)
        payload[i] = backing_->get( localAddr + i );
    
    event->setPayload(payload);
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
        if (is_debug_event(me)) { Debug(_L10_,"Memory init %s - Received GetX for %" PRIx64 " size %zu\n", getName().c_str(), me->getAddr(),me->getPayload().size()); }
        if ( isRequestAddressValid(addr) && backing_ ) {
            for ( size_t i = 0 ; i < me->getPayload().size() ; i++) {
                backing_->set( addr + i,  me->getPayload()[i] );
            }
        }
    } else if (Command::NULLCMD == me->getCmd()) {
        if (is_debug_event(me)) { Debug(_L10_, "Memory (%s) received init event: %s\n", getName().c_str(), me->getVerboseString().c_str()); }
    } else {
        Output out("", 0, 0, Output::STDERR);
        out.debug(_L10_,"Memory received unexpected Init Command: %d\n", (int)me->getCmd());
    }

    delete me;
}

