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

#include "coherentMemoryController.h"
#include "util.h"

#include "membackend/memBackendConvertor.h"
#include "memLink.h"
#include "memNIC.h"

#define NO_STRING_DEFINED "N/A"

using namespace std;
using namespace SST;
using namespace SST::MemHierarchy;

/* Debug macros */
#ifdef __SST_DEBUG_OUTPUT__ /* From sst-core, enable with --enable-debug */
#define is_debug_addr(addr) (DEBUG_ADDR.empty() || DEBUG_ADDR.find(addr) != DEBUG_ADDR.end())
#define is_debug_event(ev) (DEBUG_ADDR.empty() || ev->doDebug(DEBUG_ADDR))
#define Debug(level, fmt, ... ) dbg.debug( level, fmt, ##__VA_ARGS__ )
#else
#define is_debug_addr(addr) false
#define is_debug_event(ev) false
#define DEBUG(level, fmt, ... )
#endif

CoherentMemController::CoherentMemController(ComponentId_t id, Params &params) : Component(id) {
/**** Copy from MemoryController ****/
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
        out.output("%s, **WARNING** ** Found deprecated parameter: statistics **  memHierarchy statistics have been moved to the Statistics API. Please see sst-info to view available statistics and update your input deck accordingly.    \nNO statistics will be printed otherwise! Remove this parameter from your deck to eliminate this message.\n", getName().c_str());
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
        out.output("%s, ** Found deprecated parameter: direct_link ** The value of this parameter is now auto-detected by the link configuration in your input deck. Remove this parameter from your input deck to eliminate this message    .\n", getName().c_str());
    }
              
    std::string name        = params.find<std::string>("backendConvertor", "memHierarchy.simpleMemBackendConvertor");
               
    string link_lat         = params.find<std::string>("direct_link_latency", "10 ns");
             
    Params tmpParams = params.find_prefix_params("backendConvertor.");
              
    memBackendConvertor_  = dynamic_cast<MemBackendConvertor*>(loadSubComponent(name, this, tmpParams));
              
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
        link_->setRecvHandler( new Event::Handler<CoherentMemController>(this, &CoherentMemController::handleEvent));
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
        link_->setRecvHandler( new Event::Handler<CoherentMemController>(this, &CoherentMemController::handleEvent) );
        clockLink_ = true;
    }
    region_ = link_->getRegion();
    privateMemOffset_ = 0;
    
    // Set up backing store if needed
    std::string memoryFile = params.find<std::string>("memory_file", NO_STRING_DEFINED );
    if ( ! params.find<bool>("do_not_back",false)  ) {
        if ( 0 == memoryFile.compare( NO_STRING_DEFINED ) ) {
            memoryFile.clear();
        }
        try {
             backing_ = new Backend::Backing( memoryFile, memBackendConvertor_->getMemSize() );
        }
        catch ( int e) {
            if (e == 1)
                dbg.fatal(CALL_INFO, -1, "%s, Error - unable to open memory_file. You specified '%s'.\n", getName().c_str(), memoryFile.c_str());
            else if (e == 2)
                dbg.fatal(CALL_INFO, -1, "%s, Error - mmap of backing store failed.\n", getName().c_str());
            else
                dbg.fatal(CALL_INFO, -1, "%s, Error - unable to create backing store. Exception thrown is %d.\n", getName().c_str(), e);
        }
    } else if (memoryFile != NO_STRING_DEFINED) {
        dbg.fatal(CALL_INFO, -1, "%s, Error - conflicting parameters. 'do_not_back' cannot be true if 'memory_file' is specified.  memory_file = %s\n",
                getName().c_str(), memoryFile.c_str());
    }

    /* Clock Handler */
    clockHandler_ = new Clock::Handler<CoherentMemController>(this, &CoherentMemController::clock);
    clockTimeBase_ = registerClock(memBackendConvertor_->getClockFreq(), clockHandler_);
    clockOn_ = true;
    
    registerTimeBase("1 ns", true);
/***** END COPY FROM MEMORYCONTROLLER *****/

    std::string customHandlerName = params.find<std::string>("customCmdHandler", "");
    if (customHandlerName != "") {
        customCommandHandler_ = dynamic_cast<CustomCmdMemHandler*>(loadSubComponent(name, this, params));
    } else 
        customCommandHandler_ = nullptr;

    directory_ = false; /* Updated during init */
    timestamp_ = 0;
}

void CoherentMemController::init(unsigned int phase) {
    link_->init(phase);

    /* Inherit region from our source(s) */
    region_ = link_->getRegion(); // This can change during init, but should stabilize before we start receiving init data
    if (!phase) {
        /* Announce our presence on link */
        link_->sendInitData(new MemEventInitCoherence(getName(), Endpoint::Memory, true, false, memBackendConvertor_->getRequestWidth()));
    }
    
    while (MemEventInit *ev = link_->recvInitData()) {
        if (ev->getDst() == getName()) {
            processInitEvent(ev);
        } else delete ev;
    }
}


void CoherentMemController::setup(void) { 
    memBackendConvertor_->setup();
    link_->setup();
    
    cacheStatus_.resize(memSize_/lineSize_, false); /* Initialize all cache status to false */
}


void CoherentMemController::finish(void) {
    if (!clockOn_) {
        Cycle_t cycle = turnClockOn();
        memBackendConvertor_->turnClockOn(cycle);
    }
    memBackendConvertor_->finish();
    link_->finish();

}

/* 
 */
void CoherentMemController::handleEvent(SST::Event* event) {
    if (!clockOn_) {
        Cycle_t cycle = turnClockOn();
        memBackendConvertor_->turnClockOn(cycle);
    }

    MemEventBase * ev = static_cast<MemEventBase*>(event);
    
    if (is_debug_event(ev))
        dbg.debug(_L3_, "\n(%s) Received: %s\n", getName().c_str(), ev->getVerboseString().c_str());
    
    Command cmd = ev->getCmd();
    
    switch(cmd) {    
        case Command::GetX:
        case Command::GetS:
        case Command::GetSX:
            handleRequest(static_cast<MemEvent*>(ev));
            break;
        case Command::PutM:
            ev->setFlag(MemEventBase::F_NORESPONSE);
        case Command::PutE:
        case Command::PutS:
            handleReplacement(static_cast<MemEvent*>(ev));
            break;
        case Command::CustomReq:
            handleCustomReq(ev);
            break;
        case Command::AckInv:
            handleAckInv(static_cast<MemEvent*>(ev));
            break;
        case Command::FetchResp:
            handleFetchResp(static_cast<MemEvent*>(ev));
            break;
        case Command::NACK:
            handleNack(static_cast<MemEvent*>(ev));
            break;
        case Command::FlushLine:
        case Command::FlushLineInv:
            handleFlush(static_cast<MemEvent*>(ev));
            break;
        default:
            dbg.fatal(CALL_INFO, -1, "CoherentMemController (%s) received unhandled event: cmd = %s, cycles = %" PRIu64 ".\n", getName().c_str(), CommandString[(int)cmd], timestamp_);
    }
}


bool CoherentMemController::clock(Cycle_t cycle) {
    timestamp_++;
    
    bool debug = false;


    // issue ready events
    while (!msgQueue_.empty() && msgQueue_.begin()->first < timestamp_) {
        MemEventBase * sendEv = msgQueue_.begin()->second;
        
        if (is_debug_event(sendEv)) {
            if (!debug) dbg.debug(_L4_, "\n");
            debug = true;
            dbg.debug(_L4_, "%" PRIu64 " (%s) Sending event to processor: %s\n", timestamp_, getName().c_str(), sendEv->getBriefString().c_str());
        }
        
        link_->send(sendEv);
        msgQueue_.erase(msgQueue_.begin());
    }

    bool unclockLink = true;
    if (clockLink_)
        unclockLink = link_->clock();
    bool unclockBack = memBackendConvertor_->clock(cycle);

    if (unclockLink && unclockBack) {
        memBackendConvertor_->turnClockOff();
    }

    return false;
}

Cycle_t CoherentMemController::turnClockOn() {
    Cycle_t cycle = reregisterClock(clockTimeBase_, clockHandler_);
    cycle--;
    clockOn_ = true;
    return cycle;
}


/***************** request and response handlers ***********************/
void CoherentMemController::handleRequest(MemEvent * ev) {
    if (ev->isAddrGlobal()) {
        ev->setBaseAddr(translateToLocal(ev->getBaseAddr()));
        ev->setAddr(translateToLocal(ev->getAddr()));
    }

    outstandingEventList_.insert(std::make_pair(ev->getID(),OutstandingEvent(ev, ev->getBaseAddr())));
    
    notifyListeners( ev );

    if (mshr_.find(ev->getBaseAddr()) == mshr_.end()) {
        mshr_.insert(std::make_pair(ev->getBaseAddr(), std::list<MSHREntry>(1, MSHREntry(ev->getID(), ev->getCmd()))));
        if (!ev->queryFlag(MemEvent::F_NONCACHEABLE)) {
            cacheStatus_.at(ev->getBaseAddr()/lineSize_) = true;
        }
        memBackendConvertor_->handleMemEvent(ev);
    } else {
        mshr_.find(ev->getBaseAddr())->second.push_back(MSHREntry(ev->getID(), ev->getCmd()));
    }
}


void CoherentMemController::handleReplacement(MemEvent * ev) {
   
    if (ev->isAddrGlobal()) {
        ev->setBaseAddr(translateToLocal(ev->getBaseAddr()));
        ev->setAddr(translateToLocal(ev->getAddr()));
    }

    /* Check for writeback races with shootdown invalidations */
    if (!directory_ && mshr_.find(ev->getBaseAddr()) != mshr_.end()) {
        MSHREntry * entry = &(mshr_.find(ev->getBaseAddr())->second.front());
        if (entry->cmd == Command::CustomReq && entry->shootdown) {
            ev->getPayload().empty() ? handleAckInv(ev) : handleFetchResp(ev);
            return;
        }
    }

    /* Drop clean writebacks after updating cache status */
    if (!ev->getDirty()) {
        cacheStatus_.at(ev->getBaseAddr()/lineSize_) = directory_;
        delete ev;
        return;
    }
    
    /* Now we have a PutM, handle it */
    outstandingEventList_.insert(std::make_pair(ev->getID(), OutstandingEvent(ev, ev->getBaseAddr())));

    if (mshr_.find(ev->getBaseAddr()) == mshr_.end()) {
        mshr_.insert(std::make_pair(ev->getBaseAddr(), std::list<MSHREntry>(1, MSHREntry(ev->getID(), ev->getCmd()))));
        cacheStatus_.at(ev->getBaseAddr()/lineSize_) = directory_;
        memBackendConvertor_->handleMemEvent(ev);
    } else { /* Jump writeback ahead of a custom request w/ shootdown to that the shootdown doesn't miss updated data */
        std::list<MSHREntry>* entry = &(mshr_.find(ev->getBaseAddr())->second);
        for (std::list<MSHREntry>::iterator it = entry->begin(); it != entry->end(); it++) {
            if (it->cmd == Command::CustomReq && it->shootdown) {
                if (it == entry->begin()) { /* shootdown in progress */
                    it->writebacks.insert(ev->getID());
                    memBackendConvertor_->handleMemEvent(ev);
                } else {
                    it = entry->insert(it, MSHREntry(ev->getID(), ev->getCmd()));
                }
                return;
            }
        }
    }
}


void CoherentMemController::handleCustomReq(MemEventBase * ev) {
    if (!customCommandHandler_) {
        dbg.fatal(CALL_INFO, -1, "%s, Error: Received custom event but no handler loaded. Ev = %s. Time = %" PRIu64 "ns\n",
                getName().c_str(), ev->getVerboseString().c_str(), getCurrentSimTimeNano());
    }

    CustomCmdMemHandler::MemEventInfo evInfo = customCommandHandler_->receive(ev);
    outstandingEventList_.insert(std::make_pair(ev->getID(), OutstandingEvent(ev, evInfo.addrs, evInfo.shootdown)));
    OutstandingEvent * outEv = &(outstandingEventList_.find(ev->getID())->second);

    for (std::set<Addr>::iterator it = outEv->addrs.begin(); it != outEv->addrs.end(); it++) {
        if (mshr_.find(*it) == mshr_.end()) {
            if (!evInfo.shootdown) {
                mshr_.insert(std::make_pair(*it, std::list<MSHREntry>(1, MSHREntry(ev->getID(), ev->getCmd()))));
                outEv->decrementCount();
            } else if (doShootdown(*it, ev)) {
                mshr_.insert(std::make_pair(*it, std::list<MSHREntry>(1, MSHREntry(ev->getID(), ev->getCmd(), evInfo.shootdown))));
            } else { /* shootdown but line is uncached so good to go */
                mshr_.insert(std::make_pair(*it, std::list<MSHREntry>(1, MSHREntry(ev->getID(), ev->getCmd()))));
                outEv->decrementCount();
            }
        } else {
            mshr_.find(*it)->second.push_back(MSHREntry(ev->getID(), ev->getCmd(), evInfo.shootdown));
        }
    }

    if (outEv->getCount() == 0) {
        CustomCmdInfo * info = customCommandHandler_->ready(ev); 
        memBackendConvertor_->handleCustomEvent(info);
    }
}


void CoherentMemController::handleMemResponse(SST::Event::id_type id, uint32_t flags) {
    if (outstandingEventList_.find(id)->second.request->getCmd() == Command::CustomReq) {
        finishCustomReq(id, flags);
    } else { // Any MemEvent response
        finishMemReq(id, flags);
    }
}


bool CoherentMemController::doShootdown(Addr addr, MemEventBase * ev) {
    if (cacheStatus_.at(addr/lineSize_) == true) {
        Addr globalAddr = translateToGlobal(addr);
        MemEvent * inv = new MemEvent(this, globalAddr, globalAddr, Command::FetchInv, lineSize_);
        inv->setRqstr(ev->getRqstr());
        inv->setDst(ev->getSrc());
        
        msgQueue_.insert(std::make_pair(timestamp_, inv)); /* send this on the next clock */
        return true;
    }
    return false;
}

/* The command can be an AckInv or a PutE/PutS */
void CoherentMemController::handleAckInv(MemEvent * response) {
    if (response->isAddrGlobal()) {
        response->setBaseAddr(translateToLocal(response->getBaseAddr()));
        response->setAddr(translateToLocal(response->getAddr()));
    }

    Addr baseAddr = response->getBaseAddr();
    
    /* Update cache status */
    cacheStatus_.at(baseAddr/lineSize_) = false;
    
    /* Look up request in mshr */
    MSHREntry * entry = &(mshr_.find(baseAddr)->second.front());
    SST::Event::id_type requestID = entry->id;
    
    OutstandingEvent * outEv = &(outstandingEventList_.find(requestID)->second);

    entry->shootdown = false;
    delete response;

    if (entry->writebacks.empty()) {
        outEv->decrementCount();
        if (outEv->getCount() == 0) {
            CustomCmdInfo * info = customCommandHandler_->ready(outEv->request); 
            memBackendConvertor_->handleCustomEvent(info);
        }
    }
}


void CoherentMemController::handleFetchResp(MemEvent * response) {
    if (response->isAddrGlobal()) {
        response->setBaseAddr(translateToLocal(response->getBaseAddr()));
        response->setAddr(translateToLocal(response->getAddr()));
    }

    Addr baseAddr = response->getBaseAddr();

    /* Look up request in mshr */
    MSHREntry * entry = &(mshr_.find(baseAddr)->second.front());
    SST::Event::id_type requestID = entry->id;

    OutstandingEvent * outEv = &(outstandingEventList_.find(requestID)->second);

    /* Update cache status */
    cacheStatus_.at(baseAddr/lineSize_) = false;

    // Write dirty data if needed
    if (response->getDirty()) {
        MemEvent * write = new MemEvent(this, response->getAddr(), baseAddr, Command::PutM, response->getPayload());
        write->setRqstr(response->getRqstr());
        write->setFlag(MemEvent::F_NORESPONSE);
        
        entry->writebacks.insert(write->getID());
        outstandingEventList_.insert(std::make_pair(write->getID(), OutstandingEvent(write, baseAddr)));

        memBackendConvertor_->handleMemEvent(write);
    }

    delete response;

    entry->shootdown = false; // Complete
    if (entry->writebacks.empty()) { 
        outEv->decrementCount();
        if (outEv->getCount() == 0) {
            CustomCmdInfo * info = customCommandHandler_->ready(outEv->request);
            memBackendConvertor_->handleCustomEvent(info);
        }
    }
}


/**
 * Handle NACKs
 * Only invalidations will be NACKed since they are the
 * only type of requests sent to caches
 * (responses are never NACKed).
 */
void CoherentMemController::handleNack(MemEvent * nack) {
    
    MemEvent * nackedEvent = nack->getNACKedEvent();
    Addr baseAddr = nackedEvent->isAddrGlobal() ? translateToLocal(nackedEvent->getBaseAddr()) : nackedEvent->getBaseAddr();
    
    if (mshr_.find(baseAddr) == mshr_.end()) { /* NACKed event no longer needed due to race between replacement and Inv */
        delete nackedEvent;
        delete nack;
        return;
    }

    MSHREntry * entry = &(mshr_.find(baseAddr)->second.front());
    if (entry->shootdown) { // Still need the shootdown

        /* Compute backoff to avoid excessive NACKing */
        int retries = nackedEvent->getRetries();
        if (retries > 10) retries = 10;
        uint64_t backoff = (0x1 << retries);
        nackedEvent->incrementRetries();

        msgQueue_.insert(std::make_pair(timestamp_ + backoff, nackedEvent));
        
    } else {
        delete nackedEvent;
    }

    delete nack;
}

void CoherentMemController::handleFlush(MemEvent * flush) {
    if (flush->isAddrGlobal()) {
        flush->setBaseAddr(translateToLocal(flush->getBaseAddr()));
        flush->setAddr(translateToLocal(flush->getAddr()));
    }

    if (flush->getPayloadSize() != 0) {
        MemEvent * put = new MemEvent(this, flush->getBaseAddr(), flush->getBaseAddr(), Command::PutM, flush->getPayload());
        put->setFlag(MemEvent::F_NORESPONSE);
        outstandingEventList_.insert(std::make_pair(put->getID(), OutstandingEvent(put, put->getBaseAddr())));
        notifyListeners(flush);
        if (mshr_.find(flush->getBaseAddr()) == mshr_.end()) {
            mshr_.insert(std::make_pair(put->getBaseAddr(), std::list<MSHREntry>(1, MSHREntry(put->getID(), put->getCmd()))));
            memBackendConvertor_->handleMemEvent(put);
        } else {
            mshr_.find(put->getBaseAddr())->second.push_back(MSHREntry(put->getID(), put->getCmd()));
        }
    }

    if (mshr_.find(flush->getBaseAddr()) == mshr_.end()) {
        mshr_.insert(std::make_pair(flush->getBaseAddr(), std::list<MSHREntry>(1, MSHREntry(flush->getID(), flush->getCmd()))));
        if (flush->getCmd() == Command::FlushLineInv) {
            cacheStatus_.at(flush->getBaseAddr()/lineSize_) = false;
            flush->setCmd(Command::FlushLine);
        }
        memBackendConvertor_->handleMemEvent(flush);
    } else {
        mshr_.find(flush->getBaseAddr())->second.push_back(MSHREntry(flush->getID(), flush->getCmd()));
    }
}

// Update MSHR
void CoherentMemController::updateMSHR(Addr baseAddr) {
    // Remove top event
    mshr_.find(baseAddr)->second.pop_front();

    // Start next event
    if (!mshr_.find(baseAddr)->second.empty()) {
        MSHREntry * entry = &(mshr_.find(baseAddr)->second.front());
        OutstandingEvent * outEv = &(outstandingEventList_.find(entry->id)->second);

        if (entry->cmd == Command::CustomReq) {
            /* Check what needs to happen */
            if (entry->shootdown && doShootdown(baseAddr, outEv->request)) { /* Start shootdown */
                return;
            } else if (entry->writebacks.empty()) { /* Command ready to issue */
                if (outEv->decrementCount() == 0) {
                    CustomCmdInfo * info = customCommandHandler_->ready(outEv->request);
                    memBackendConvertor_->handleCustomEvent(info);
                }
            }
            entry->shootdown = false;
        } else { /* MemEvent */
            replayMemEvent(static_cast<MemEvent*>((outstandingEventList_.find(entry->id)->second).request));
        }
    } else
        mshr_.erase(baseAddr);
}

void CoherentMemController::replayMemEvent(MemEvent* request) {
    switch (request->getCmd()) {
        case Command::GetS:
        case Command::GetSX:
        case Command::GetX:
            if (!request->queryFlag(MemEvent::F_NONCACHEABLE)) {
                cacheStatus_.at(request->getBaseAddr()/lineSize_) = true;
            }
            memBackendConvertor_->handleMemEvent(request);
            break;
        case Command::PutM:
            cacheStatus_.at(request->getBaseAddr()/lineSize_) = directory_;
            memBackendConvertor_->handleMemEvent(request);
            break;
        case Command::FlushLineInv:
            cacheStatus_.at(request->getBaseAddr()/lineSize_) = false;
            request->setCmd(Command::FlushLine);
        case Command::FlushLine:
            memBackendConvertor_->handleMemEvent(request);
            break;
        default:
            break;
    }

}

void CoherentMemController::finishMemReq(SST::Event::id_type id, uint32_t flags) {
    /* Find the event that returned */
    OutstandingEvent * ev = &(outstandingEventList_.find(id)->second);
    MemEvent * request = static_cast<MemEvent*>(ev->request);
    Addr baseAddr = *(ev->addrs.begin()); /* only one address */
    MSHREntry * entry = &(mshr_.find(baseAddr)->second.front());

    /* Update backing store */
    bool noncacheable = request->queryFlag(MemEvent::F_NONCACHEABLE);
    if (backing_ && (request->getCmd() == Command::PutM || (request->getCmd() == Command::GetX && noncacheable))) {
        writeData(request);
    }

    /* Special case - response to a writeback needed for a shootdown */
    if (entry->writebacks.find(id) != entry->writebacks.end()) {
        entry->writebacks.erase(id);
        delete request;
        outstandingEventList_.erase(id);
        if (entry->writebacks.empty() && !entry->shootdown) { /* Request now ready */
            if (ev->decrementCount() == 0) {
                CustomCmdInfo * info = customCommandHandler_->ready(request);
                memBackendConvertor_->handleCustomEvent(info);
            }
        }
        return;
    }

    /* Handle regular MemEvent */
    if (request->queryFlag(MemEvent::F_NORESPONSE)) {
        delete request;
        outstandingEventList_.erase(id);
        updateMSHR(baseAddr);
        return;
    }
    
    /* Build response */
    MemEvent * resp = request->makeResponse();
    if (resp->getCmd() == Command::GetSResp || (resp->getCmd() == Command::GetXResp && !noncacheable)) {
        readData(resp);
        if (!noncacheable) resp->setCmd(Command::GetXResp);
    }
    
    resp->setFlags(flags);

    if (request->isAddrGlobal()) {
        resp->setBaseAddr(translateToGlobal(request->getBaseAddr()));
        resp->setAddr(translateToGlobal(request->getAddr()));
    }

    link_->send(resp);
    /* Clean up */
    delete request;
    outstandingEventList_.erase(id);
    updateMSHR(baseAddr);
}


void CoherentMemController::finishCustomReq(SST::Event::id_type id, uint32_t flags) {
    OutstandingEvent * outEv = &(outstandingEventList_.find(id)->second);
    MemEventBase * ev = outEv->request;

    MemEventBase * resp = customCommandHandler_->finish(ev, flags);
    if (resp != nullptr)
        link_->send(resp);

    /* Clean up state */
    for (std::set<Addr>::iterator it = outEv->addrs.begin(); it != outEv->addrs.end(); it++) {
        updateMSHR(*it);
    }   
    delete ev;
    outstandingEventList_.erase(id);
}

/***************** other functions ***********************/
void CoherentMemController::writeData(MemEvent* event) {
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
        
void CoherentMemController::readData(MemEvent* event) {
    bool noncacheable = event->queryFlag(MemEvent::F_NONCACHEABLE);
    Addr localAddr = noncacheable ? event->getAddr() : event->getBaseAddr();
                
    vector<uint8_t> payload;
    payload.resize(event->getSize(), 0);
                
    if (!backing_) return;
                
    for ( size_t i = 0 ; i < event->getSize() ; i++)
        payload[i] = backing_->get( localAddr + i );
                
    event->setPayload(payload);
}

void CoherentMemController::writeData(Addr addr, std::vector<uint8_t> * data) {
    if (!backing_) return;
    
    for ( size_t i = 0; i < data->size(); i++)
        backing_->set( addr + i, data->at(i));
}

void CoherentMemController::readData(Addr addr, size_t bytes, std::vector<uint8_t> &data) {
    data.resize(bytes, 0);

    if (!backing_) return;
    
    for (size_t i = 0; i < bytes; i++)
        data[i] = backing_->get(addr + i);
}

Addr CoherentMemController::translateToLocal(Addr addr) {
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
    

Addr CoherentMemController::translateToGlobal(Addr addr) {
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
        

void CoherentMemController::processInitEvent( MemEventInit* me ) {
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
        if (me->getInitCmd() == MemEventInit::InitCommand::Coherence) {
            MemEventInitCoherence * eventC = static_cast<MemEventInitCoherence*>(me);
            if (eventC->getType() == Endpoint::Directory)
                directory_ = true;
            lineSize_ = eventC->getLineSize();
        }
    } else {
        Output out("", 0, 0, Output::STDERR);
        out.debug(_L10_,"Memory received unexpected Init Command: %d\n", me->getCmd());
    }
    
    delete me;
}

