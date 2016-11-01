// Copyright 2009-2016 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2016, Sandia Corporation
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef _MEMORYCONTROLLER_H
#define _MEMORYCONTROLLER_H

#include <sst/core/sst_types.h>

#include <sst/core/component.h>
#include <sst/core/link.h>

#include "sst/elements/memHierarchy/memEvent.h"
#include "sst/elements/memHierarchy/cacheListener.h"
#include "sst/elements/memHierarchy/memNIC.h"
#include "sst/elements/memHierarchy/membackend/backing.h"

namespace SST {
namespace MemHierarchy {

class MemBackendConvertor;

class MemController : public SST::Component {
public:
	typedef uint64_t ReqId;

    MemController(ComponentId_t id, Params &params);
    void init(unsigned int);
    void setup();
    void finish();

    virtual void handleMemResponse( MemEvent* );

private:

    MemController();  // for serialization only
    ~MemController() {}

    void handleEvent( SST::Event* );
    bool clock( SST::Cycle_t );
    void performRequest( MemEvent* );
    void performResponse( MemEvent* );
    void processInitEvent( MemEvent* );

    Output dbg;

    MemBackendConvertor*  memBackendConvertor_;

    SST::Link*  cacheLink_;         // Link to the rest of memHierarchy 
#if 0
    MemNIC*     networkLink_;       // Link to the rest of memHierarchy if we're communicating over a network
#endif

    std::vector<CacheListener*> listeners_;

    Backend::Backing* m_backing; 

    bool isRequestAddressValid(Addr addr){
        return (addr < memSize_);
    }
#if 0
    bool isRequestAddressValid(MemEvent *ev){
        Addr addr = ev->getAddr();
        return (addr >= rangeStart_ && addr < rangeStart_ + memSize_);
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
#endif
#if 0
    Addr convertAddressToLocalAddress(Addr addr){
        return addr - rangeStart_;
        if (0 == numPages_) return addr - rangeStart_;

        addr = addr - rangeStart_;
        Addr step = addr / interleaveStep_;
        Addr offset = addr % interleaveStep_;
        return (step * interleaveSize_) + offset;
        return 0;
    }
#endif
    int         protocol_;
    size_t      memSize_;
#if 0
    size_t      rangeStart_;
    uint64_t    requestSize_;
    uint64_t    numPages_;
    Addr        interleaveSize_;
    Addr        interleaveStep_;
    uint32_t    cacheLineSize_;
    uint32_t    requestWidth_;
#endif

};

}}

#endif /* _MEMORYCONTROLLER_H */
