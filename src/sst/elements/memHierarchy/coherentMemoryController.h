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

#ifndef _COHERENTMEMORYCONTROLLER_H
#define _COHERENTMEMORYCONTROLLER_H


#include <sst/core/sst_types.h>

#include <sst/core/event.h>
#include <sst/core/component.h>

#include <map>
#include <list>

#include "sst/elements/memHierarchy/memoryController.h"
#include "sst/elements/memHierarchy/memEvent.h"
#include "sst/elements/memHierarchy/memEventBase.h"
#include "sst/elements/memHierarchy/cacheListener.h"
#include "sst/elements/memHierarchy/memLinkBase.h"
#include "sst/elements/memHierarchy/membackend/backing.h"

namespace SST {
namespace MemHierarchy {

class CoherentMemController : public MemController {
public:
/* Element Library Info */
    SST_ELI_REGISTER_COMPONENT(CoherentMemController, "memHierarchy", "CoherentMemController", SST_ELI_ELEMENT_VERSION(1,0,0),
            "Coherent memory controller, supports cache shootdowns and interfaces to a main memory model for timing", COMPONENT_CATEGORY_MEMORY)

    SST_ELI_DOCUMENT_PARAMS( MEMCONTROLLER_ELI_PARAMS )

    SST_ELI_DOCUMENT_PORTS( MEMCONTROLLER_ELI_PORTS )

    SST_ELI_DOCUMENT_SUBCOMPONENT_SLOTS( MEMCONTROLLER_ELI_SUBCOMPONENTSLOTS )

/* Begin class definition */
    typedef uint64_t ReqId;

    CoherentMemController(ComponentId_t id, Params &params);

    /* Event handling */
    void handleMemResponse(SST::Event::id_type id, uint32_t flags);
    
    /* Component API */
    virtual void init(unsigned int phase);
    virtual void setup();

protected:
    virtual void processInitEvent(MemEventInit* ev);
    
    virtual void handleEvent(SST::Event * event);

    virtual bool clock(Cycle_t cycle);

private:

    CoherentMemController();
    ~CoherentMemController() {}

    void handleRequest(MemEvent * ev);
    void handleReplacement(MemEvent * ev);
    void handleFlush(MemEvent * ev);
    void handleAckInv(MemEvent * ev);
    void handleFetchResp(MemEvent * ev);
    void handleNack(MemEvent * ev);
    void handleCustomCmd(MemEventBase * ev);

    bool doShootdown(Addr addr, MemEventBase * ev);

    void finishMemReq(SST::Event::id_type id, uint32_t flags);
    void finishCustomReq(SST::Event::id_type id, uint32_t flags);

    void updateMSHR(Addr baseAddr);
    void replayMemEvent(MemEvent * ev);

    // Custom command handler 
    // TODO ability to specify multiple handlers
    //CustomCmdMemHandler * customCommandHandler_;

    // Outgoing event handling
    Cycle_t timestamp_;
    std::multimap<uint64_t, MemEventBase*> msgQueue_;

    // Caching information
    bool directory_; /* Whether directory is above us, i.e., whether a PutM indicates block is no longer cached or not */
    std::vector<bool> cacheStatus_;
    Addr lineSize_;

    // MSHR
    class MSHREntry {
        public:
            SST::Event::id_type id;                     // Event id, use to find event in OutstandEventList_
            Command cmd;                                // Event command, mostly for quick lookup 
            bool shootdown;                             // Whether event requires a shootdown before being processed
            std::set<SST::Event::id_type> writebacks;   // Writebacks this event is waiting for, due to shootdown
            
            MSHREntry(SST::Event::id_type id, Command cmd, bool sdown = false) : id(id), cmd(cmd), shootdown(sdown) { }
    };
    
    std::map<Addr, std::list<MSHREntry> > mshr_;    // Keeps outstanding events coherent

    // Event tracking
    class OutstandingEvent {
        public:
            MemEventBase * request;     // Event
            std::set<Addr> addrs;       // Set of (base) addresses this event touches, should be one MSHR entry for each
            uint32_t count;             // If customcmd, number of MSHR entries that are not yet ready to issue

            OutstandingEvent(MemEventBase * request, Addr addr) : request(request), count(0) { addrs.insert(addr); }
            OutstandingEvent(MemEventBase * request, std::set<Addr> addrs) : request(request), addrs(addrs) { count = addrs.size(); }

            uint32_t decrementCount() { return --count; }
            uint32_t getCount() { return count; }
    };

    std::map<SST::Event::id_type,OutstandingEvent> outstandingEventList_; // List of all outstanding events
};


}}

#endif /* _COHERENTMEMORYCONTROLLER_H */
