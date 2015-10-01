// Copyright 2009-2015 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2015, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

/*
 * File:   sieveController.cc
 */


#include <sst_config.h>
//#include <sst/core/serialization.h>
//#include <sst/core/simulation.h>
#include <sst/core/interfaces/stringEvent.h>

#include "sieveController.h"
#include "../memEvent.h"

using namespace SST;
using namespace SST::MemHierarchy;


void Sieve::processEvent(SST::Event* _ev) {
    MemEvent* event = static_cast<MemEvent*>(_ev);
    Command cmd     = event->getCmd();
    
    event->setBaseAddr(toBaseAddr(event->getAddr()));
    Addr baseAddr   = event->getBaseAddr();
            
    int lineIndex = cf_.cacheArray_->find(baseAddr, true);
        
    if (lineIndex == -1) {                                     /* Miss.  If needed, evict candidate */
        // d_->debug(_L3_,"-- Cache Miss --\n");
        CacheLine * line = cf_.cacheArray_->findReplacementCandidate(baseAddr, false);
        cf_.cacheArray_->replace(baseAddr, line->getIndex());
        
        auto cmdT = (GetS == cmd) ? READ : WRITE;
        //std::cout << "VA: = " << event->getVirtualAddress() << "\n";
        //exit(0);
        // Notify listener (AddrHistogrammer) on a MISS
        CacheListenerNotification notify(event->getBaseAddr(),
			event->getVirtualAddress(), event->getInstructionPointer(),
			event->getSize(), cmdT, MISS);
        listener_->notifyAccess(notify);
    }
        
    MemEvent * responseEvent;
    if (cmd == GetS) {
        responseEvent = event->makeResponse(S);
    } else {
        responseEvent = event->makeResponse(M);
    }

    // Sieve cannot work with Gem5 because we do not have a backing store.
    // It is meant for other processor models, particularly Ariel.
    // Currently, Ariel does not care about the payload.  Therefore,
    // there is no need to construct the payload.
    
    responseEvent->setDst(event->getSrc());
    cpu_link->send(responseEvent);
    
    //d_->debug(_L3_,"%s, Sending Response, Addr = %" PRIx64 "\n", getName().c_str(), _event->getAddr());
    
    delete _ev;
}

void Sieve::init(unsigned int phase) {
    if (!phase) {
        cpu_link->sendInitData(new Interfaces::StringEvent("SST::MemHierarchy::MemEvent"));
    }

    while (SST::Event* ev = cpu_link->recvInitData()) {
        if (ev) delete ev;
    }
}

void Sieve::finish(){
    listener_->printStats(*d_);
    delete cf_.cacheArray_;
    delete cf_.rm_;
    delete d_;
}

