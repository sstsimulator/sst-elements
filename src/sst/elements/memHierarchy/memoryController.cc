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

/* 
 * File:   memController.cc
 * Author: Caesar De la Paz III
 * Email:  caesar.sst@gmail.com
 */

#include <sst_config.h>
#include <sst/core/element.h>
#include <sst/core/params.h>
#include <sst/core/simulation.h>

#include "memoryController.h"
#include "util.h"

#include "membackend/memBackendConvertor.h"
#include "memEvent.h"
#include "bus.h"
#include "cacheListener.h"
#include "memNIC.h"

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
MemController::MemController(ComponentId_t id, Params &params) : Component(id), networkLink_(NULL), cacheLink_(NULL), backing_(NULL) {
            
    int debugLevel = params.find<int>("debug_level", 0);

    fixupParam( params, "backend", "backendConvertor.backend" );
    fixupParams( params, "backend.", "backendConvertor.backend." );
    fixupParams( params, "clock", "backendConvertor.backend.clock" );
    fixupParams( params, "request_width", "backendConvertor.request_width" );
    fixupParams( params, "max_requests_per_cycle", "backendConvertor.backend.max_requests_per_cycle" );

    // Output for debug
    dbg.init("@t:--->  ", debugLevel, 0, (Output::output_location_t)params.find<int>("debug", 0));
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

    string protocolStr      = params.find<std::string>("coherence_protocol", "MESI");
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

    // Check protocol string - note this is only used if directory controller is not present in system to ensure LLC gets the right permissions
    if (protocolStr != "MESI" && protocolStr != "mesi" && protocolStr != "msi" && protocolStr != "MSI" && protocolStr != "none" && protocolStr != "NONE") {
        dbg.fatal(CALL_INFO, -1, "Invalid param(%s): protocol - must be one of 'MESI', 'MSI', or 'NONE'. You specified '%s'\n", getName().c_str(), protocolStr.c_str());
    }

    protocol_ = (protocolStr == "mesi" || protocolStr == "MESI") ? 1 : 0;

    if (isPortConnected("direct_link")) {
        cacheLink_   = configureLink( "direct_link", link_lat, new Event::Handler<MemController>(this, &MemController::handleEvent));
    } else {

        if (!isPortConnected("network")) {
            dbg.fatal(CALL_INFO,-1,"%s, Error: No connected port detected. Connect 'direct_link' or 'network' port.\n", getName().c_str());
        }

        Params tmpParams = params.find_prefix_params( "memNIC.");
        tmpParams.insert("mem_size", params.find<std::string>("backendConvertor.backend.mem_size"));

        networkLink_ = new MemNIC( this, tmpParams );
        networkLink_->setRecvHandler( new Event::Handler<MemController>(this, &MemController::handleEvent) );
        networkLink_->setOutput( &dbg );
    }

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

    if ( !isRequestAddressValid(ev->getAddr()) ) {
        dbg.fatal(CALL_INFO, 1, "MemoryController \"%s\" received request from \"%s\" with invalid address.\n"
                "\t\tRequested Address:   0x%" PRIx64 "\n"
                "\t\tMC Memory End:       0x%" PRIx64 "\n",
                getName().c_str(), ev->getSrc().c_str(), ev->getAddr(), (Addr) memSize_);
    }

    Debug(_L10_,"\n\n----------------------------------------------------------------------------------------\n");
    Debug(_L10_,"Memory Controller: %s - Event Received. Cmd = %s\n", getName().c_str(), CommandString[ev->getCmd()]);
    Debug(_L10_,"Event info: Addr: 0x%" PRIx64 ", dst = %s, src = %s, rqstr = %s, size = %d, prefetch = %d, vAddr = 0x%" PRIx64 ", instPtr = %" PRIx64 "\n",
        ev->getBaseAddr(), ev->getDst().c_str(), ev->getSrc().c_str(), ev->getRqstr().c_str(), ev->getSize(), ev->isPrefetch(), ev->getVirtualAddress(), ev->getInstructionPointer());

    Command cmd = ev->getCmd();

    // Notify our listeners that we have received an event
    switch (cmd) {
        case GetS:
        case GetX:
        case GetSEx:
        case PutM:

            // Handle all backing stuff first since memBackend may not provide a response for writes
            // TODO fix so that membackend always provides a response and this happens at the return instead
            // That way, the order of accesses to the backing should match the backend's execution order
            performRequest( ev );
            recordResponsePayload( ev );
            notifyListeners( ev );
            memBackendConvertor_->handleMemEvent( ev );
            break;

        case FlushLine:
        case FlushLineInv:
            {
                MemEvent* put = NULL;
                if ( ev->getPayloadSize() != 0 ) {
                    put = new MemEvent( *ev );
                    put->setCmd(PutM);
                }

                ev->setCmd(FlushLine);
                memBackendConvertor_->handleMemEvent( ev );

                if ( put ) {
                    notifyListeners(ev);
                    memBackendConvertor_->handleMemEvent( put );
                }
            }
            break;

        case PutS:
        case PutE:
            break;
        default:
            dbg.fatal(CALL_INFO,-1,"Memory controller received unrecognized command: %s", CommandString[cmd]);
    }
}

bool MemController::clock(Cycle_t cycle) {

    if (networkLink_) networkLink_->clock();

    memBackendConvertor_->clock( cycle );

    return false;
}

void MemController::handleMemResponse( MemEvent* ev ) {

    Debug(_L10_,"\n\n----------------------------------------------------------------------------------------\n");
    Debug(_L10_,"Memory Controller: %s - Response Received. Cmd = %s\n", getName().c_str(), CommandString[ev->getCmd()]);
    Debug(_L10_,"Event info: Addr: 0x%" PRIx64 ", dst = %s, src = %s, rqstr = %s, size = %d, prefetch = %d, vAddr = 0x%" PRIx64 ", instPtr = %" PRIx64 "\n",
        ev->getBaseAddr(), ev->getDst().c_str(), ev->getSrc().c_str(), ev->getRqstr().c_str(), ev->getSize(), ev->isPrefetch(), ev->getVirtualAddress(), ev->getInstructionPointer());

    if (ev->queryFlag(MemEvent::F_NORESPONSE)) {
        delete ev;
        return;
    }
    
    performResponse( ev );

    if ( networkLink_ ) {
        networkLink_->send( ev );
    } else {
        cacheLink_->send( ev );
    }
}

void MemController::init(unsigned int phase) {
    if (! networkLink_ ) {
        SST::Event *ev = NULL;
        while (NULL != (ev = cacheLink_->recvInitData())) {
            MemEvent *me = dynamic_cast<MemEvent*>(ev);
            if (!me) {
                delete ev;
                return;
            }
            /* Push data to memory */
            processInitEvent( me );
        }
    } else {
        networkLink_->init(phase);

        while (MemEvent *ev = networkLink_->recvInitData()) {
            if (ev->getDst() == getName()) {
                /* Push data to memory */
                processInitEvent( ev );
            }
        }
    }
}

void MemController::setup(void) {
    memBackendConvertor_->setup();
    if (networkLink_) networkLink_->setup();
}


void MemController::finish(void) {
    memBackendConvertor_->finish();
    if (networkLink_) networkLink_->finish();
}

void MemController::performRequest(MemEvent* event) {

    if ( ! backing_ ) {
        return;
    }

    Addr addr = event->queryFlag(MemEvent::F_NONCACHEABLE) ?  event->getAddr() : event->getBaseAddr();

    if (event->getCmd() == PutM) {  /* Write request to memory */
        Debug(_L10_,"WRITE.  Addr = %" PRIx64 ", Request size = %i\n", addr, event->getSize());

        for ( size_t i = 0 ; i < event->getSize() ; i++)
             backing_->set( addr + i, event->getPayload()[i] );

    } else {
        bool noncacheable  = event->queryFlag(MemEvent::F_NONCACHEABLE);

        if (noncacheable && event->getCmd()== GetX) {
            Debug(_L10_,"WRITE. Noncacheable request, Addr = %" PRIx64 ", Request size = %i\n", addr, event->getSize());

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

    if (event->getCmd() == GetXResp) {
        event->setGrantedState(M);
    } else {
        if (protocol_) {
            event->setGrantedState(E); // Directory controller supersedes this; only used if DirCtrl does not exist
        } else {
            event->setGrantedState(S);
        }
    }
}

void MemController::recordResponsePayload( MemEvent * ev) {
    if (ev->queryFlag(MemEvent::F_NORESPONSE)) return;

    bool noncacheable = ev->queryFlag(MemEvent::F_NONCACHEABLE);
    Addr localAddr = noncacheable ? ev->getAddr() : ev->getBaseAddr();

    vector<uint8_t> payload;
    if (ev->getCmd() == GetS || ev->getCmd() == GetSEx || (ev->getCmd() == GetX && !noncacheable)) {
        payload.resize(ev->getSize(), 0);
        if (backing_) {
            for ( size_t i = 0 ; i < ev->getSize() ; i++)
                payload[i] = backing_->get( localAddr + i );
        }
        payloads_.insert(std::make_pair(ev->getID(), payload));
    } 
}

void MemController::processInitEvent( MemEvent* me ) {

    /* Push data to memory */
    if (GetX == me->getCmd()) {
        Addr addr = me->getAddr();
        Debug(_L10_,"Memory init %s - Received GetX for %" PRIx64 " size %" PRIu32 "\n", getName().c_str(), me->getAddr(),me->getSize());
        if ( isRequestAddressValid(addr) && backing_ ) {
            for ( size_t i = 0 ; i < me->getSize() ; i++) {
                backing_->set( addr + i,  me->getPayload()[i] );
            }
        }
    } else {
        Output out("", 0, 0, Output::STDERR);
        out.debug(_L10_,"Memory received unexpected Init Command: %d\n", me->getCmd());
    }

    delete me;
}

