// Copyright 2009-2017 Sandia Corporation. Under the terms
// of Contract DE-NA0003525 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2017, Sandia Corporation
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
#include <sst/core/event.h>

#include "sst/elements/memHierarchy/memEvent.h"
#include "sst/elements/memHierarchy/cacheListener.h"
#include "sst/elements/memHierarchy/memLinkBase.h"
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

    virtual void handleMemResponse( SST::Event::id_type id, uint32_t flags );
    
    SST::Cycle_t turnClockOn();

private:

    MemController();  // for serialization only
    ~MemController() {}

    void notifyListeners( MemEvent* ev ) {
        if (  ! listeners_.empty()) {
            CacheListenerNotification notify(ev->getAddr(), ev->getVirtualAddress(), 
                        ev->getInstructionPointer(), ev->getSize(), READ, HIT);

            for (unsigned long int i = 0; i < listeners_.size(); ++i) {
                listeners_[i]->notifyAccess(notify);
            }
        }
    }

    void handleEvent( SST::Event* );
    bool clock( SST::Cycle_t );
    void writeData( MemEvent* );
    void readData( MemEvent* );
    void processInitEvent( MemEventInit* );

    Output dbg;

    MemBackendConvertor*    memBackendConvertor_;
    Backend::Backing*       backing_; 

    MemLinkBase* link_;         // Link to the rest of memHierarchy 

    std::vector<CacheListener*> listeners_;
    
    std::map<SST::Event::id_type, MemEvent*> outstandingEvents_; // For sending responses. Expect backend to respond to ALL requests so that we know the execution order

    bool isRequestAddressValid(Addr addr){
        return region_.contains(addr);
    }

    size_t      memSize_;

    bool clockOn_;
    Clock::Handler<MemController>* clockHandler_;
    TimeConverter* clockTimeBase_;

    MemRegion region_; // Which address region we are, for translating to local addresses
    Addr privateMemOffset_; // If we reserve any memory locations for ourselves/directories/etc. and they are NOT part of the physical address space, shift regular addresses by this much
    Addr translateToLocal(Addr addr);
    Addr translateToGlobal(Addr addr);
};

}}

#endif /* _MEMORYCONTROLLER_H */
