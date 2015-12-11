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

void Sieve::recordMiss(Addr addr, bool isRead) {
    const allocMap_t::iterator allocI = actAllocMap.lower_bound(addr);
    if (allocI != actAllocMap.end()) {
        // is it in range
        if (addr <= (addr +  allocI->second->getAllocateLength())) {
            allocCountMap_t::iterator evI = allocMap.find(allocI->second);
            if (isRead) {
                evI->second.first++;
            } else {
                evI->second.second++;
            }
        } else {
            // no alloc associated
            unassociatedMisses++;
        }
    } else {
        // no alloc associated
        unassociatedMisses++;
    }
}

void Sieve::processAllocEvent(SST::Event* event) {
    // should only recieve arielAllocTrackEvent events
    ArielComponent::arielAllocTrackEvent* ev = (ArielComponent::arielAllocTrackEvent*)event;

    if (ev->getType() == ArielComponent::arielAllocTrackEvent::ALLOC) {
        // add to the big list of all allocations
        if (allocMap.find(ev) == allocMap.end()) {
            allocMap[ev] = rwCount_t();
        } else {
            cf_.dbg_->fatal(CALL_INFO, -1, "Trying to add allocation event which has already been added. \n");
        }
        
        // add to the list of active allocations (i.e. not FREEd)
        if (actAllocMap.find(ev->getVirtualAddress()) == actAllocMap.end()) {
            actAllocMap[ev->getVirtualAddress()] = ev;
        } else {
            // not sure if should be fatal, or should just replace the 'old' alloc
            cf_.dbg_->fatal(CALL_INFO, -1, "Trying to add allocation event at an address with an active allocation. \n");
        }
    } else if (ev->getType() == ArielComponent::arielAllocTrackEvent::FREE) {
        allocMap_t::iterator targ = actAllocMap.find(ev->getVirtualAddress());
        if (targ == actAllocMap.end()) {
            cf_.dbg_->debug(_INFO_,"FREEing an address taht was never ALLOCd\n");
        } else {
            actAllocMap.erase(targ);
        }
    } else {
        cf_.dbg_->fatal(CALL_INFO, -1, "Unrecognized Ariel Allocation Tracking Event Type \n");
    }
}

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

        recordMiss(event->getVirtualAddress(), (cmdT == READ));
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

    // print out all the allocations and how often they were touched
#warning should switch to file output
    printf("Printing out allocation hits (addr, len, reads, writes):\n");
    uint64_t tMiss = 0;
    for(allocCountMap_t::iterator i = allocMap.begin(); 
        i != allocMap.end(); ++i) {
        ArielComponent::arielAllocTrackEvent *ev = i->first;
        printf("%#" PRIx64 " %#" PRIx64 " %#" PRIx64 " %#" PRIx64 "\n", ev->getVirtualAddress(), ev->getAllocateLength(),
               i->second.first, i->second.second);
        tMiss = tMiss + i->second.first + i->second.second;
    }
    printf("Unassociated Misses: %#" PRIx64 " (%.2f%%)\n", unassociatedMisses, 
           double(unassociatedMisses)/(double(tMiss)+double(unassociatedMisses)));
}


Sieve::~Sieve(){
    delete cf_.cacheArray_;
    delete cf_.rm_;
    delete d_;

    for(allocCountMap_t::iterator i = allocMap.begin();
        i != allocMap.end(); ++i) {
        delete i->first;
    }
}
