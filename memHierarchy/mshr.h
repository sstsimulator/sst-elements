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

#ifndef _MSHR_H_
#define _MSHR_H_

#include <boost/assert.hpp>
#include <map>

#include <sst/core/event.h>
#include <sst/core/sst_types.h>
#include <sst/core/component.h>
#include <sst/core/output.h>

#include <memEvent.h>
#include <util.h>
#include <boost/assert.hpp>
#include <string>
#include <sstream>

namespace SST { namespace MemHierarchy {

using namespace std;

/* MSHRs hold both events and pointers to events (e.g., the address of an event to replay when the current event resolves) */
struct mshrType {
    boost::variant<Addr, MemEvent*> elem;
    MemEvent * event;
    mshrType(MemEvent* ev) : elem(ev), event(ev) {}
    mshrType(Addr addr) : elem(addr) {}
};

/* An MSHR entry has a vector of mshrTypes (events & pointers) along with some bookkeeping for outstanding requests 
 * If we were just doing inclusive caches, the bookkeeping could also be kept with the cache state
 */
struct mshrEntry {
    vector<mshrType> mshrQueue; // Events and pointers to events for this address
    uint32_t        acksNeeded; // Acks needed for request at top of queue. Here instead of at cacheline for non-inclusive caches
    vector<uint8_t> tempData;   // Temporary holding place for response data during replay of request events (for non-inclusive caches)
};

typedef map<Addr, mshrEntry >   mshrTable;

#define HUGE_MSHR 100000

/**
 *  Implements an MSHR with entries of type mshrEntry
 */
class MSHR {
public:
        
    // used externally
    MSHR(Output* dbg, int maxSize, string cacheName, bool debugAll, Addr debugAddr);                                     
    bool exists(Addr baseAddr);                             
    
    bool insertAll(Addr, vector<mshrType>&);                 
    bool insert(Addr baseAddr, MemEvent* event);            
    bool insertPointer(Addr keyAddr, Addr pointerAddr);
    bool insertInv(Addr baseAddr, MemEvent* event, bool inProgress);            
    bool insertWriteback(Addr keyAddr);
    
    MemEvent* removeFront(Addr baseAddr);                  
    void removeElement(Addr baseAddr, MemEvent* event);     
    vector<mshrType> removeAll(Addr);                       
    void removeWriteback(Addr baseAddr);

    const vector<mshrType> lookup(Addr baseAddr);           
    bool isHit(Addr baseAddr);                              
    bool elementIsHit(Addr baseAddr, MemEvent *event);
    bool isFull();                                          // external
    bool isAlmostFull();                                    // external
    MemEvent* getOldestRequest() const;                     // external
    bool pendingWriteback(Addr baseAddr);

    // Bookkeeping getters/setters
    int getAcksNeeded(Addr baseAddr);
    void setAcksNeeded(Addr baseAddr, int acksNeeded);
    void incrementAcksNeeded(Addr baseAddr);
    void decrementAcksNeeded(Addr baseAddr);
    vector<uint8_t> * getTempData(Addr baseAddr);
    void setTempData(Addr baseAddr, vector<uint8_t>& data);
    void clearTempData(Addr baseAddr);

    // used internally
    bool insert(Addr baseAddr, mshrType entry);         // internal
    bool insert(Addr keyAddr, Addr ptrAddr);         // internal
    bool insertInv(Addr baseAddr, mshrType entry, bool inProgress);         // internal
    void removeElement(Addr baseAddr, mshrType entry);  // internal
    

    // unimplemented or unused functions
    unsigned int getSize(){ return size_; }                 
    void printEntry(Addr baseAddr);                         // not implemented
    MemEvent* lookupFront(Addr baseAddr);      
    void removeElement(Addr baseAddr, Addr pointer);
    void insertFront(Addr baseAddr, MemEvent* event);
    void printTable();

private:
    mshrTable map_;
    Output* d_;
    Output* d2_;
    int size_;
    int maxSize_;
    string ownerName_;
    bool DEBUG_ALL;
    Addr DEBUG_ADDR;
};
}}
#endif
