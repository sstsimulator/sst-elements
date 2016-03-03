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
        // add to the big map and list of all allocations
        if (allocMap.find(ev) == allocMap.end()) {
            allocMap[ev] = rwCount_t();
	    allocList.push_back(ev);
        } else {
            output_->fatal(CALL_INFO, -1, "Trying to add allocation event which has already been added. \n");
        }
        
        // add to the list of active allocations (i.e. not FREEd)
        if (actAllocMap.find(ev->getVirtualAddress()) == actAllocMap.end()) {
            actAllocMap[ev->getVirtualAddress()] = ev;
        } else {
            // sometimes ariel replaces both malloc() and _malloc(), so we get two reports. Just ignore the first. 
            output_->debug(_INFO_, "Trying to add allocation event at an address (%p %" PRIx64") with an active allocation. %" PRIu64 "\n", ev, ev->getVirtualAddress(), (uint64_t)actAllocMap.size());
	    // replace the 'old' alloc
	  actAllocMap[ev->getVirtualAddress()] = ev;
        }
    } else if (ev->getType() == ArielComponent::arielAllocTrackEvent::FREE) {
        allocMap_t::iterator targ = actAllocMap.find(ev->getVirtualAddress());
        if (targ == actAllocMap.end()) {
            output_->debug(_INFO_,"FREEing an address that was never ALLOCd\n");
        } else {
            actAllocMap.erase(targ);
        }
        delete ev;
    } else {
        output_->fatal(CALL_INFO, -1, "Unrecognized Ariel Allocation Tracking Event Type \n");
    }
}

void Sieve::processEvent(SST::Event* ev) {
    MemEvent* event = static_cast<MemEvent*>(ev);
    Command cmd     = event->getCmd();
    
    event->setBaseAddr(toBaseAddr(event->getAddr()));
    Addr baseAddr   = event->getBaseAddr();
            
    int lineIndex = cacheArray_->find(baseAddr, true);
    bool miss = (lineIndex == -1) ? true : false; 
    Addr replacementAddr = 0;

    if (miss) {                                     /* Miss.  If needed, evict candidate */
        // output_->debug(_L3_,"-- Cache Miss --\n");
        CacheLine * line = cacheArray_->findReplacementCandidate(baseAddr, false);
        replacementAddr = line->getBaseAddr();
        cacheArray_->replace(baseAddr, line->getIndex());
        line->setState(M);

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

    // Debug output. Ifdef this for even better performance
    output_->debug(_L4_, "%s, Src = %s, Cmd = %s, BaseAddr = %" PRIx64 ", Addr = %" PRIx64 ", VA = %" PRIx64 ", PC = %" PRIx64 ", Size = %d: %s\n",
            getName().c_str(), event->getSrc().c_str(), CommandString[cmd], baseAddr, event->getAddr(), event->getVirtualAddress(), event->getInstructionPointer(), event->getSize(), miss ? "MISS" : "HIT");
    if (miss) output_->debug(_L5_, "%s, Replaced address %" PRIx64 "\n", getName().c_str(), replacementAddr);
 
    /* Record statistics */
    if (miss) {
        if (cmd == GetS) statReadMisses->addData(1);
        else statWriteMisses->addData(1);
    } else {
        if (cmd == GetS) statReadHits->addData(1);
        else statWriteHits->addData(1);
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
    SST::Link * link = event->getDeliveryLink();
    link->send(responseEvent);
    
    //output_->debug(_L3_,"%s, Sending Response, Addr = %" PRIx64 "\n", getName().c_str(), event->getAddr());
    
    delete ev;
}

void Sieve::init(unsigned int phase) {
    if (!phase) {
        for (int i = 0; i < cpuLinkCount_; i++) {
            cpuLinks_[i]->sendInitData(new Interfaces::StringEvent("SST::MemHierarchy::MemEvent"));
        }
    }

    for (int i = 0; i < cpuLinkCount_; i++) {
        while (SST::Event* ev = cpuLinks_[i]->recvInitData()) {
            if (ev) delete ev;
        }
    }
}

void Sieve::finish(){

    listener_->printStats(*output_);

    // print out all the allocations and how often they were touched
    output_file->output(CALL_INFO, "#Printing out allocation hits (addr, IP, len, reads, writes, 'density'):\n");
    uint64_t tMiss = 0;
    for(allocList_t::iterator i = allocList.begin(); 
        i != allocList.end(); ++i) {
        ArielComponent::arielAllocTrackEvent *ev = *i;
	rwCount_t counts = allocMap[ev];
	double density = double(counts.first + counts.second) / double(ev->getAllocateLength());
        output_file->output(CALL_INFO, "%#" PRIx64 " %#" PRIx64 " %" PRId64 " %" PRId64 " %" PRId64 " %.3f\n", 
			    ev->getVirtualAddress(), 
			    ev->getInstructionPointer(), 
			    ev->getAllocateLength(),
			    counts.first, counts.second, density);
        tMiss = tMiss + counts.first + counts.second;
    }
    output_file->output(CALL_INFO, "#Unassociated Misses: %" PRId64 " (%.2f%%)\n", unassociatedMisses, 
			double(100*unassociatedMisses)/(double(tMiss)+double(unassociatedMisses)));
}


Sieve::~Sieve(){
    delete cacheArray_;
    delete output_;

    for(allocCountMap_t::iterator i = allocMap.begin();
        i != allocMap.end(); ++i) {
        delete i->first;
    }
}
