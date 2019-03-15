// Copyright 2009-2018 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2018, NTESS
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
 * File:   sieveController.cc
 */


#include <sst_config.h>
//#include <sst/core/simulation.h>
#include <sst/core/interfaces/stringEvent.h>

#include "sieveController.h"
#include "../memEvent.h"

using namespace SST;
using namespace SST::MemHierarchy;

void Sieve::recordMiss(Addr addr, bool isRead) {
    allocMap_t::iterator allocI = activeAllocMap.lower_bound(addr);
    
    // lower_bound returns iterator to address just above or equal to addr
    if (allocI->first != addr) {
        if (allocI == activeAllocMap.begin()) {
            allocI = activeAllocMap.end(); // Not found
        } else {
            allocI--; // Actually want the address just below the one we got
        }
    }
    if (allocI != activeAllocMap.end()) {
        // is it in range
        if (addr < (allocI->first +  allocI->second.size)) {
            uint64_t allocID = allocI->second.id;
            allocCountMap_t::iterator evI = allocMap.find(allocID);
            if (evI == allocMap.end()) {
                allocMap[allocID] = rwCount_t();
                evI = allocMap.find(allocID);
            }
            if (isRead) {
                evI->second.first++;
                statReadMisses->addData(1);
            } else {
                evI->second.second++;
                statWriteMisses->addData(1);
            }
            return;
        }
    }
    
    if (isRead) {
        statUnassocReadMisses->addData(1);
        statReadMisses->addData(1);
    } else {
        statUnassocWriteMisses->addData(1);
        statWriteMisses->addData(1);
    }
}

void Sieve::processAllocEvent(SST::Event* event) {
    // should only recieve arielAllocTrackEvent events
    ArielComponent::arielAllocTrackEvent* ev = (ArielComponent::arielAllocTrackEvent*)event;
    
    if (ev->getType() == ArielComponent::arielAllocTrackEvent::ALLOC) {
        // add to the list of active allocations (i.e. not FREEd)
        mallocEntry entry = {ev->getInstructionPointer(), ev->getAllocateLength()};
        activeAllocMap[ev->getVirtualAddress()] = entry;

#ifdef __SST_DEBUG_OUTPUT__
        if (activeAllocMap.find(ev->getVirtualAddress()) != activeAllocMap.end()) {
            // sometimes ariel replaces both malloc() and _malloc(), so we get two reports. Just ignore the first. 
            output_->debug(_INFO_, "Trying to add allocation event at an address (%p %" PRIx64") with an active allocation. %" PRIu64 "\n", ev, ev->getVirtualAddress(), (uint64_t)activeAllocMap.size());
        }
#endif
        delete ev;
    } else if (ev->getType() == ArielComponent::arielAllocTrackEvent::FREE) {
        allocMap_t::iterator targ = activeAllocMap.find(ev->getVirtualAddress());
        if (targ != activeAllocMap.end()) {
            uint64_t allocID = targ->second.id;
            allocCountMap_t::iterator mapIt = allocMap.find(allocID);
            
            // In this case ALWAYS delete the entry from active alloc & delete the event
            activeAllocMap.erase(targ);
            
            // If the entry in the count map is 0, remove it as well
            if (mapIt != allocMap.end() && (mapIt->second.first == 0 && mapIt->second.second == 0)) {
                allocMap.erase(mapIt);
            }

#ifdef __SST_DEBUG_OUTPUT__
        } else {
            output_->debug(_INFO_,"FREEing an address that was never ALLOCd\n");
#endif
        }
        delete ev;
    } else if (ev->getType() == ArielComponent::arielAllocTrackEvent::BUOY) {
        // output stats
        outputStats(ev->getInstructionPointer());
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
            
    CacheLine * cline = cacheArray_->lookup(baseAddr, true);
    bool miss = (cline == nullptr);
    Addr replacementAddr = 0;

    if (miss) {                                     /* Miss.  If needed, evict candidate */
        // output_->debug(_L3_,"-- Cache Miss --\n");
        CacheLine * line = cacheArray_->findReplacementCandidate(baseAddr, false);
        replacementAddr = line->getBaseAddr();
        cacheArray_->replace(baseAddr, line);
        line->setState(M);

        bool isRead = (cmd == Command::GetS);
        recordMiss(event->getVirtualAddress(), isRead);

        // notify profiler, if any
        if (listener_) {
            CacheListenerNotification notify(event->getAddr(),
                                             event->getBaseAddr(), 
                                             event->getVirtualAddress(),
                                             event->getInstructionPointer(), 
                                             event->getSize(), 
                                             isRead ? READ : WRITE,
                                             MISS);
            listener_->notifyAccess(notify);
        }

    } else {
        if (cmd == Command::GetS) statReadHits->addData(1);
        else statWriteHits->addData(1);
    }

    // Debug output. Ifdef this for even better performance
#ifdef __SST_DEBUG_OUTPUT__
    output_->debug(_L4_, "%s, Src = %s, Cmd = %s, BaseAddr = %" PRIx64 ", Addr = %" PRIx64 ", VA = %" PRIx64 ", PC = %" PRIx64 ", Size = %d: %s\n",
            getName().c_str(), event->getSrc().c_str(), CommandString[(int)cmd], baseAddr, event->getAddr(), event->getVirtualAddress(), event->getInstructionPointer(), event->getSize(), miss ? "MISS" : "HIT");
    if (miss) output_->debug(_L5_, "%s, Replaced address %" PRIx64 "\n", getName().c_str(), replacementAddr);
#endif

    MemEvent * responseEvent;
    if (cmd == Command::GetS) {
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

void Sieve::outputStats(int marker) {
    // create name <outFileName> + <sequence> + marker (optional)
    stringstream fileName;
    fileName << outFileName << "-" << outCount;
    outCount++;
    if (-1 != marker)  {
        fileName << "-" << marker;
    }
    fileName << ".txt";

    // create new file
    Output* output_file = new Output("",0,0,SST::Output::FILE, fileName.str());

    // have the listener (if any) output stats
    if (listener_) {
        listener_->printStats(*output_file);
    }

    // print out all the allocations and how often they were touched
    output_file->output(CALL_INFO, "#Printing allocation memory accesses (mallocID, reads, writes):\n");
    vector<uint64_t> entriesToErase;
    for (allocCountMap_t::iterator i = allocMap.begin(); i != allocMap.end(); i++) {
	rwCount_t &counts = i->second;

        if (counts.first == 0 && counts.second == 0) {
            entriesToErase.push_back(i->first);
            continue;
        }
        output_file->output(CALL_INFO, "%" PRIu64 " %" PRId64 " %" PRId64 "\n",
                            i->first, counts.first, counts.second);
        
        // clear the counts
        if (resetStatsOnOutput) {
            counts.first = 0;
            counts.second = 0;
            entriesToErase.push_back(i->first);
        }
    }
    for (vector<uint64_t>::iterator it = entriesToErase.begin(); it != entriesToErase.end(); it++) {
        allocMap.erase(*it); // remove entry from allocMap
    }
    // clean up
    delete output_file;
}

void Sieve::finish(){
    outputStats(-1);
}


Sieve::~Sieve(){
    delete cacheArray_;
    delete output_;
}
