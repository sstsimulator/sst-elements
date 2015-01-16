// Copyright 2009-2014 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2014, Sandia Corporation
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

namespace SST {
namespace MemHierarchy {

class MemBackend;
using namespace std;

class MemController : public SST::Component {
public:
    struct DRAMReq {
        enum Status_t {NEW, PROCESSING, RETURNED, DONE};

        MemEvent*   reqEvent_;
        MemEvent*   respEvent_;
        int         respSize_;
        Command     cmd_;
        size_t      size_;
        size_t      amtInProcess_;
        size_t      amtProcessed_;
        Status_t    status_;
        Addr        baseAddr_;
        Addr        addr_;
        bool        isWrite_;

        DRAMReq(MemEvent *ev, const size_t busWidth, const size_t cacheLineSize) :
            reqEvent_(new MemEvent(*ev)), respEvent_(NULL), amtInProcess_(0),
            amtProcessed_(0), status_(NEW){
            
            cmd_      = ev->getCmd();
            isWrite_  = (cmd_ == PutM) ? true : false;
            baseAddr_ = ev->getBaseAddr();
            addr_     = ev->getBaseAddr();
            setSize(cacheLineSize);
        }

        ~DRAMReq() { delete reqEvent_; }

        void setSize(size_t _size){ size_ = _size; }
        void setAddr(Addr _baseAddr){ baseAddr_ = _baseAddr; }

    };

    MemController(ComponentId_t id, Params &params);
    void init(unsigned int);
    void setup();
    void finish();

    void handleMemResponse(DRAMReq *req);

    Output dbg;

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
    SST::Link*  lowNetworkLink_;
    MemBackend* backend_;
    int         protocol_;
    dramReq_t   requestQueue_;
    dramReq_t   requests_;
    int         backingFd_;
    uint8_t*    memBuffer_;
    size_t      memSize_;
    size_t      requestSize_;
    Addr        rangeStart_;
    size_t      numPages_;
    Addr        interleaveSize_;
    Addr        interleaveStep_;
    size_t      cacheLineSize_;
    size_t      requestWidth_;
    uint64_t    GetSReqReceived_;
    uint64_t    GetXReqReceived_;
    uint64_t    PutMReqReceived_;
    uint64_t    GetSExReqReceived_;
    uint64_t    numReqOutstanding_;
    uint64_t    numCycles_;
    std::vector<CacheListener*> listeners_;

    Output::output_location_t statsOutputTarget_;
//#ifdef HAVE_LIBZ
//    gzFile traceFP;
//#else
    FILE *traceFP;
//#endif


};



}}

#endif /* _MEMORYCONTROLLER_H */
