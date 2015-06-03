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
 * File:   cache.cc
 * Author: Caesar De la Paz III
 * Email:  caesar.sst@gmail.com
 */


#include <sst_config.h>
#include <sst/core/serialization.h>
#include <sst/core/params.h>
#include <sst/core/simulation.h>
#include <sst/core/interfaces/stringEvent.h>

#include <csignal>
#include <boost/variant.hpp>

#include "cacheController.h"
#include "../memEvent.h"
#include <vector>

using namespace SST;
using namespace SST::MemHierarchy;


Sieve::Sieve(Component *comp, Params &params) {
    cpu_link = configureLink("cpu_link",
               new Event::Handler<Sieve>(this, &Sieve::processEvent));
}


void Sieve::processEvent(SST::Event* _ev) {
    Command cmd     = event->getCmd();
    MemEvent* event = static_cast<MemEvent*>(_ev);
    
    event->setBaseAddr(toBaseAddr(event->getAddr()));
    Addr baseAddr   = event->getBaseAddr();
            
    int lineIndex = cf_.cacheArray_->find(baseAddr, true);
        
    if (lineIndex == -1) {                                     /* Miss.  If needed, evict candidate */
        // d_->debug(_L3_,"-- Cache Miss --\n");
        int wbLineIndex = cf_.cacheArray_->preReplace(_baseAddr);       // Find a replacement candidate
        cf_.cacheArray_->replace(_baseAddr, wbLineIndex);
    }
        
    MemEvent * responseEvent;
    if (cmd == GetS) {
        responseEvent = event->makeResponse(S);
    } else {
        responseEvent = event->makeResponse(M);
    }

    // TODO does ariel care about payload?
    
    responseEvent->setDst(_event->getSrc());
    cpu_link->send(responseEvent);
    
    //d_->debug(_L3_,"%s, Sending Response, Addr = %" PRIx64 "\n", getName().c_str(), _event->getAddr());
    
    delete _ev;
}

void Sieve::init(unsigned int phase) {
    if (!phase) {
        cpu_link->sendInitData(new Interfaces::StringEvent("SST::MemHierarchy::MemEvent"));
    }

    SST::Event *ev;
    while (ev = cpu_link->recvInitData()) {
        delete ev;
    }
}

void Sieve::finish(){
    listener_->printStats(*d_);
    delete cf_.cacheArray_;
    delete cf_.rm_;
    delete d_;
}

