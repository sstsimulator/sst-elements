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


struct mshrType {
    boost::variant<Addr, MemEvent*> elem;
    MemEvent* memEvent_;
    mshrType(MemEvent* _memEvent) : elem(_memEvent), memEvent_(_memEvent) {}
    mshrType(Addr _addr) : elem(_addr) {}
};

typedef map<Addr, vector<mshrType> >    mshrTable;

#define HUGE_MSHR 100000

/**
 *  Implements an MSHR with entries of type mshrType
 */
class MSHR {
public:
        
    // used externally
    MSHR(Output* dbg, int maxSize);                                     
    bool exists(Addr baseAddr);                             
    
    bool insertAll(Addr, vector<mshrType>);                 
    bool insert(Addr baseAddr, MemEvent* event);            
    bool insertPointer(Addr keyAddr, Addr pointerAddr);     
    bool insertInv(Addr baseAddr, MemEvent* event, int index);            
    
    MemEvent* removeFront(Addr _baseAddr);                  
    void removeElement(Addr baseAddr, MemEvent* event);     
    vector<mshrType> removeAll(Addr);                       
    
    const vector<mshrType> lookup(Addr baseAddr);           
    bool isHit(Addr baseAddr);                              
    bool isHitAndStallNeeded(Addr baseAddr, Command cmd);   // should this fcn really be in MSHR? MSHR should just be buffer
    bool elementIsHit(Addr baseAddr, MemEvent *event);
    bool isFull();                                          // external
    bool isAlmostFull();                                    // external
    MemEvent* getOldestRequest() const;                     // external
    
    // used internally
    bool insert(Addr baseAddr, Addr pointer);               // internal
    bool insert(Addr baseAddr, mshrType mshrEntry);         // internal
    bool insertInv(Addr baseAddr, mshrType mshrEntry, int index);         // internal
    void removeElement(Addr baseAddr, mshrType mshrEntry);  // internal
    

    // unimplemented or unused functions
    unsigned int getSize(){ return size_; }                 // not implemented
    void printEntry(Addr baseAddr);                         // not implemented
    void printEntry2(vector<MemEvent*> events);             // not implemented
    MemEvent* lookupFront(Addr _baseAddr);      
    void removeElement(Addr baseAddr, Addr pointer);
    void insertFront(Addr baseAddr, MemEvent* event);

private:
    mshrTable map_;
    Output* d_;
    Output* d2_;
    int size_;
    int maxSize_;

};
}}
#endif
