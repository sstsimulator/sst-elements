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
#include "sst/elements/memHierarchy/customcmd/customCmdMemory.h"

namespace SST {
namespace MemHierarchy {

class MemBackendConvertor;

class MemController : public SST::Component {
public:
    typedef uint64_t ReqId;

    MemController(ComponentId_t id, Params &params);
    virtual void init(unsigned int);
    virtual void setup();
    void finish();

    virtual void handleMemResponse( SST::Event::id_type id, uint32_t flags );
    
    SST::Cycle_t turnClockOn();

protected:
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
    
    virtual void handleEvent( SST::Event* );
    virtual void processInitEvent( MemEventInit* );

    virtual bool clock( SST::Cycle_t );
    void writeData( MemEvent* );
    void readData( MemEvent* );

    Output dbg;
    std::set<Addr> DEBUG_ADDR;

    MemBackendConvertor*    memBackendConvertor_;
    Backend::Backing*       backing_; 

    MemLinkBase* link_;         // Link to the rest of memHierarchy 
    bool clockLink_;            // Flag - should we call clock() on this link or not

    std::vector<CacheListener*> listeners_;
    
    bool isRequestAddressValid(Addr addr){
        return region_.contains(addr);
    }

    size_t memSize_;

    bool clockOn_;

    MemRegion region_; // Which address region we are, for translating to local addresses
    Addr privateMemOffset_; // If we reserve any memory locations for ourselves/directories/etc. and they are NOT part of the physical address space, shift regular addresses by this much
    Addr translateToLocal(Addr addr);
    Addr translateToGlobal(Addr addr);
    
    Clock::Handler<MemController>* clockHandler_;
    TimeConverter* clockTimeBase_;
    
    CustomCmdMemHandler * customCommandHandler_;

private:
    
    std::map<SST::Event::id_type, MemEventBase*> outstandingEvents_; // For sending responses. Expect backend to respond to ALL requests so that we know the execution order

    void handleCustomEvent(MemEventBase* ev);
};

}}

#endif /* _MEMORYCONTROLLER_H */
