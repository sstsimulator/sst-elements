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


#ifdef __SST_DEBUG_OUTPUT__
#define Debug(level, fmt, ... ) dbg.debug( level, fmt, ##__VA_ARGS__  )
#else
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
    dbg.debug(_L10_,"---");

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
        Params linkParams = params.find_prefix_params("ulink.");
        linkParams.insert("name", "direct_link");
        linkParams.insert("latency", link_lat, false);
        linkParams.insert("accept_region", "1", false);
        link_ = dynamic_cast<MemLink*>(loadSubComponent("memHierarchy.MemLink", this, linkParams));
        link_->setRecvHandler( new Event::Handler<MemController>(this, &MemController::handleEvent));
    } else {

        if (!isPortConnected("network")) {
            dbg.fatal(CALL_INFO,-1,"%s, Error: No connected port detected. Connect 'direct_link' or 'network' port.\n", getName().c_str());
        }

        Params nicParams = params.find_prefix_params("memNIC.");
        nicParams.insert("port", "network");
        nicParams.insert("class", "4", false);
        nicParams.insert("accept_region", "1", false);

        link_ = dynamic_cast<MemNIC*>(loadSubComponent("memHierarchy.MemNIC", this, nicParams)); 
        link_->setRecvHandler( new Event::Handler<MemController>(this, &MemController::handleEvent) );
    }
    
    region_ = link_->getRegion();
    privateMemOffset_ = 0;

    // Set up backing store if needed
    if ( ! params.find<bool>("do_not_back",false)  ) {
        string memoryFile = params.find<std::string>("memory_file", NO_STRING_DEFINED );
        if ( 0 == memoryFile.compare( NO_STRING_DEFINED ) ) {
            memoryFile.clear();
        }
        try { 
            backing_ = new Backend::Backing( memoryFile, memBackendConvertor_->getMemSize() );
        }
        catch ( int ) {
            dbg.fatal(CALL_INFO, -1, "%s, Error - conflicting parameters. 'do_not_back' cannot be true if 'memory_file' is specified.  memory_file = %s\n",
                "MemBackendConvertor", memoryFile.c_str());
        }
    }

    /* Clock Handler */
    registerClock(memBackendConvertor_->getClockFreq(), new Clock::Handler<MemController>(this, &MemController::clock));
    registerTimeBase("1 ns", true);
}

void MemController::handleEvent(SST::Event* event) {
    MemEvent *ev = static_cast<MemEvent*>(event);
    
    Debug(_L3_,"\n%" PRIu64 " (%s) Recieved: %s\n", getCurrentSimTimeNano(), getName().c_str(), ev->getVerboseString().c_str());

    if (ev->isAddrGlobal()) {
        ev->setBaseAddr(translateToLocal(ev->getBaseAddr()));
        ev->setAddr(translateToLocal(ev->getAddr()));
    }

    Command cmd = ev->getCmd();

    // Notify our listeners that we have received an event
    switch (cmd) {
        case Command::GetS:
        case Command::GetX:
        case Command::GetSX:
        case Command::PutM:

            // Handle all backing stuff first since memBackend may not provide a response for writes
            // TODO fix so that membackend always provides a response and this happens at the return instead
            // That way, the order of accesses to the backing should match the backend's execution order
            performRequest( ev );
            recordResponsePayload( ev );
            notifyListeners( ev );
            memBackendConvertor_->handleMemEvent( ev );
            break;

        case Command::FlushLine:
        case Command::FlushLineInv:
            {
                MemEvent* put = NULL;
                if ( ev->getPayloadSize() != 0 ) {
                    put = new MemEvent( *ev );
                    put->setCmd(Command::PutM);
                }

                ev->setCmd(Command::FlushLine);
                memBackendConvertor_->handleMemEvent( ev );

                if ( put ) {
                    notifyListeners(ev);
                    memBackendConvertor_->handleMemEvent( put );
                }
            }
            break;

        case Command::PutS:
        case Command::PutE:
            break;
        default:
            dbg.fatal(CALL_INFO,-1,"Memory controller received unrecognized command: %s", CommandString[(int)cmd]);
    }
}

bool MemController::clock(Cycle_t cycle) {

    link_->clock();

    memBackendConvertor_->clock( cycle );

    return false;
}

void MemController::handleMemResponse( MemEvent* ev ) {

    Debug(_L3_,"Memory Controller: %s - Response Received. %s\n", getName().c_str(), ev->getVerboseString().c_str());

    if (ev->queryFlag(MemEvent::F_NORESPONSE)) {
        delete ev;
        return;
    }

    performResponse( ev );
    
    if (ev->isAddrGlobal()) {
        ev->setBaseAddr(translateToGlobal(ev->getBaseAddr()));
        ev->setAddr(translateToGlobal(ev->getAddr()));
    }
    
    link_->send( ev );
}

void MemController::init(unsigned int phase) {
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

void MemController::setup(void) {
    memBackendConvertor_->setup();
    link_->setup();

}


void MemController::finish(void) {
    memBackendConvertor_->finish();
    link_->finish();
}

void MemController::performRequest(MemEvent* event) {

    if ( ! backing_ ) {
        return;
    }

    Addr addr = event->queryFlag(MemEvent::F_NONCACHEABLE) ?  event->getAddr() : event->getBaseAddr();

    if (event->getCmd() == Command::PutM) {  /* Write request to memory */
        Debug(_L4_,"\tWRITE. Addr = %" PRIx64 ", Request size = %i\n", addr, event->getSize());

        for ( size_t i = 0 ; i < event->getSize() ; i++)
             backing_->set( addr + i, event->getPayload()[i] );

    } else {
        bool noncacheable  = event->queryFlag(MemEvent::F_NONCACHEABLE);

        if (noncacheable && event->getCmd()== Command::GetX) {
            Debug(_L4_,"\tWRITE. Noncacheable request, Addr = %" PRIx64 ", Request size = %i\n", addr, event->getSize());

            for ( size_t i = 0 ; i < event->getSize() ; i++)
                    backing_->set( addr + i, event->getPayload()[i] );
        }
    }
}

void MemController::performResponse(MemEvent* event) { 
    bool noncacheable  = event->queryFlag(MemEvent::F_NONCACHEABLE);

    if (payloads_.find(event->getResponseToID()) != payloads_.end()) {
        event->setPayload(payloads_.find(event->getResponseToID())->second);
        payloads_.erase(event->getResponseToID());
    }

    if (noncacheable) event->setFlag(MemEvent::F_NONCACHEABLE);

    if (!noncacheable && event->getCmd() == Command::GetSResp) 
        event->setCmd(Command::GetXResp); // Memory always sends exclusive responses, dir/caches decide what to do with it
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
    Debug(_L10_,"\tConverting global address 0x%" PRIx64 " to local address 0x%" PRIx64 "\n", addr, rAddr);
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
    Debug(_L10_,"\tConverting local address 0x%" PRIx64 " to global address 0x%" PRIx64 "\n", addr, rAddr);
    return rAddr;
}


void MemController::recordResponsePayload( MemEvent * ev) {
    if (ev->queryFlag(MemEvent::F_NORESPONSE)) return;

    bool noncacheable = ev->queryFlag(MemEvent::F_NONCACHEABLE);
    Addr localAddr = noncacheable ? ev->getAddr() : ev->getBaseAddr();

    vector<uint8_t> payload;
    if (ev->getCmd() == Command::GetS || ev->getCmd() == Command::GetSX || (ev->getCmd() == Command::GetX && !noncacheable)) {
        payload.resize(ev->getSize(), 0);
        if (backing_) {
            for ( size_t i = 0 ; i < ev->getSize() ; i++)
                payload[i] = backing_->get( localAddr + i );
        }
        payloads_.insert(std::make_pair(ev->getID(), payload));
    } 
}

void MemController::processInitEvent( MemEventInit* me ) {
    /* Push data to memory */
    if (Command::GetX == me->getCmd()) {
        me->setAddr(translateToLocal(me->getAddr()));
        Addr addr = me->getAddr();
        Debug(_L10_,"Memory init %s - Received GetX for %" PRIx64 " size %" PRIu32 "\n", getName().c_str(), me->getAddr(),me->getPayload().size());
        if ( isRequestAddressValid(addr) && backing_ ) {
            for ( size_t i = 0 ; i < me->getPayload().size() ; i++) {
                backing_->set( addr + i,  me->getPayload()[i] );
            }
        }
    } else if (Command::NULLCMD == me->getCmd()) {
        Debug(_L10_, "Memory (%s) received init event: %s\n", getName().c_str(), me->getVerboseString().c_str());
    } else {
        Output out("", 0, 0, Output::STDERR);
        out.debug(_L10_,"Memory received unexpected Init Command: %d\n", me->getCmd());
    }

    delete me;
}

