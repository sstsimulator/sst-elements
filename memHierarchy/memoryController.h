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

#ifndef _MEMORYCONTROLLER_H
#define _MEMORYCONTROLLER_H


#include <sst/core/event.h>
#include <sst/core/module.h>
#include <sst/core/sst_types.h>
#include <sst/core/component.h>
#include <sst/core/link.h>
#include <sst/core/output.h>
#include <map>

#ifdef HAVE_LIBZ
#include <zlib.h>
#endif

#include "sst/elements/memHierarchy/memEvent.h"
#include "sst/elements/memHierarchy/bus.h"
#include "sst/elements/memHierarchy/cacheListener.h"
#include "sst/elements/memHierarchy/memNIC.h"
#include "sst/elements/memHierarchy/memResponseHandler.h"
#include "sst/elements/memHierarchy/DRAMReq.h"

namespace SST {
namespace MemHierarchy {

class MemBackend;
using namespace std;

class MemController : public SST::Component, public MemResponseHandler {
public:
    MemController(ComponentId_t id, Params &params);
    void init(unsigned int);
    void setup();
    void finish();

    Output dbg;
    virtual void handleMemResponse(DRAMReq* _req);

private:

    MemController();  // for serialization only
    ~MemController();

    void handleEvent(SST::Event* _event);
    void addRequest(MemEvent* _ev);
    bool clock(SST::Cycle_t _cycle);
    void performRequest(DRAMReq* _req);
    void sendResponse(DRAMReq* _req);
    void printMemory(DRAMReq* _req, Addr _localAddr);
    int setBackingFile(string memoryFile);

    bool isRequestAddressValid(MemEvent *ev){
        Addr addr = ev->getAddr();
        Addr step;

        if(0 == numPages_)     return (addr >= rangeStart_ && addr < (rangeStart_ + memSize_));
        if(addr < rangeStart_) return false;
        
        addr = addr - rangeStart_;
        step = addr / interleaveStep_;
        if(step >= numPages_)  return false;

        Addr offset = addr % interleaveStep_;
        if (offset >= interleaveSize_) return false;

        return true;
    }


    Addr convertAddressToLocalAddress(Addr addr){
        if (0 == numPages_) return addr - rangeStart_;
        
        addr = addr - rangeStart_;
        Addr step = addr / interleaveStep_;
        Addr offset = addr % interleaveStep_;
        return (step * interleaveSize_) + offset;
    }
    
    void cancelEvent(MemEvent *ev) {}
    void sendBusPacket(Bus::key_t key){}
    void sendBusCancel(Bus::key_t key){}
    void handleBusEvent(SST::Event *event){}
    
    typedef deque<DRAMReq*> dramReq_t;
    
    bool        divertDCLookups_;
    SST::Link*  cacheLink_;         // Link to the rest of memHierarchy 
    MemNIC*     networkLink_;       // Link to the rest of memHierarchy if we're communicating over a network
    MemBackend* backend_;
    int         protocol_;
    dramReq_t   requestQueue_;      // Requests waiting to be issued
    set<DRAMReq*>   requestPool_;   // All requests that are in flight at the memory controller (including those waiting to be issued) 
    int         backingFd_;
    uint8_t*    memBuffer_;
    uint64_t    memSize_;
    uint64_t    requestSize_;
    Addr        rangeStart_;
    uint64_t    numPages_;
    Addr        interleaveSize_;
    Addr        interleaveStep_;
    bool        doNotBack_;
    uint64_t    cacheLineSize_;
    uint64_t    requestWidth_;
    std::vector<CacheListener*> listeners_;

    Statistic<uint64_t>* cyclesWithIssue;
    Statistic<uint64_t>* cyclesAttemptIssueButRejected;
    Statistic<uint64_t>* totalCycles;
    Statistic<uint64_t>* stat_GetSReqReceived;
    Statistic<uint64_t>* stat_GetXReqReceived;
    Statistic<uint64_t>* stat_PutMReqReceived;
    Statistic<uint64_t>* stat_GetSExReqReceived;
    Statistic<uint64_t>* stat_outstandingReqs;


    Output::output_location_t statsOutputTarget_;
//#ifdef HAVE_LIBZ
//    gzFile traceFP;
//#else
    FILE *traceFP;
//#endif


};



}}

#endif /* _MEMORYCONTROLLER_H */
