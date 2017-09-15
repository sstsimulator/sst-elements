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

#ifndef _COHERENTMEMORYCONTROLLER_H
#define _COHERENTMEMORYCONTROLLER_H


#include <sst/core/sst_types.h>

#include <sst/core/event.h>
#include <sst/core/component.h>

#include <map>
#include <list>

#include "sst/elements/memHierarchy/memEvent.h"
#include "sst/elements/memHierarchy/memEventBase.h"
#include "sst/elements/memHierarchy/cacheListener.h"
#include "sst/elements/memHierarchy/memLinkBase.h"
#include "sst/elements/memHierarchy/membackend/backing.h"
#include "sst/elements/memHierarchy/customCmdMemory.h"

namespace SST {
namespace MemHierarchy {

class MemBackendConvertor;

class CoherentMemController : public SST::Component {
public:
    typedef uint64_t ReqId;

    CoherentMemController(ComponentId_t id, Params &params);
    void init(unsigned int);
    void setup();
    void finish();

    // Handler for backend responses
    void handleMemResponse(SST::Event::id_type id, uint32_t flags);

    SST::Cycle_t turnClockOn();

    /* Interface to custom request handler */
    Addr translateToLocal(Addr addr);   /* Global addresses used outside mem controller, local used internally */
    Addr translateToGlobal(Addr addr);
    void writeData(Addr addr, std::vector<uint8_t> * data); /* Update backing store */
    void readData(Addr addr, size_t bytes, std::vector<uint8_t> &data); /* Read 'bytes' bytes from address 'addr' into 'data' vector */

private:

    CoherentMemController();
    ~CoherentMemController() {}

    // Event handlers
    bool clock(SST::Cycle_t cycle);         /* Clock */
    void handleEvent(SST::Event* event);    /* Link */
    void processInitEvent(MemEventInit* ev);
    void notifyListeners( MemEvent * ev ) {
        if (!listeners_.empty()) {
            CacheListenerNotification notify(ev->getAddr(), ev->getVirtualAddress(), ev->getInstructionPointer(), ev->getSize(), READ, HIT);

            for (unsigned long int i = 0; i < listeners_.size(); ++i) {
                listeners_[i]->notifyAccess(notify);
            }
        }
    }

    // Local event handlers
    void handleRequest(MemEvent * ev);
    void handleReplacement(MemEvent * ev);
    void handleFlush(MemEvent * ev);
    void handleCustomReq(MemEventBase * ev);
    void handleAckInv(MemEvent * ev);
    void handleFetchResp(MemEvent * ev);
    void handleNack(MemEvent * ev);
    void finishMemReq(SST::Event::id_type id, uint32_t flags);
    void finishCustomReq(SST::Event::id_type id, uint32_t flags);

    // Helper functions
    bool doShootdown(Addr addr, MemEventBase * ev); /* Returns whether shootdown was necessary */
    void updateMSHR(Addr addr); /* Remove top MSHR event for addr and process the next one */
    void writeData(MemEvent*);
    void readData(MemEvent*);
    void replayMemEvent(MemEvent*);
    bool isRequestAddressValid(Addr addr) { return region_.contains(addr); }

/** Local variables **/
    Output dbg;
    std::set<Addr> DEBUG_ADDR;

    /* Subcomponents */
    MemBackendConvertor *   memBackendConvertor_;
    Backend::Backing*       backing_;
    CustomCmdMemHandler *   customCommandHandler_;
    std::vector<CacheListener*> listeners_;
    MemLinkBase*            link_;

    /* Handlers */
    Clock::Handler<CoherentMemController>* clockHandler_;
    TimeConverter* clockTimeBase_;

    /*  */
    bool clockLink_;
    bool clockOn_;
    size_t memSize_;
    MemRegion region_;
    Addr privateMemOffset_;
    Addr lineSize_;
    Cycle_t timestamp_;

    std::multimap<uint64_t, MemEventBase*> msgQueue_;

    // Have map of ID->OutstandingEvent
    class OutstandingEvent {
        public:
            MemEventBase * request;         // Request (outstanding event)
            std::set<Addr> addrs;           // Which (base) addresses we are blocking in the MSHR
            
            uint32_t count;                 // Number of lines we are waiting on - when 0, the request is ready to issue
            
            // MemEvents use this
            OutstandingEvent(MemEventBase * request, Addr addr) : request(request), count(0) { addrs.insert(addr); }

            // Custom events use this
            OutstandingEvent(MemEventBase * request, std::set<Addr> addrs, bool shootdown) : request(request), addrs(addrs) { 
                count = addrs.size();
            }

            uint32_t decrementCount() { count--; return count; }
            void incrementCount() { count++; }
            uint32_t getCount() { return count; }
    };

    // MSHR entry
    // MSHR is a map of base address -> MSHREntry pairs
    class MSHREntry {
        public:
            SST::Event::id_type id;         // Event id
            Command cmd;                    // Event command, mostly for quick filtering without needing to lookup outstandingEvent map
            bool shootdown;                 // Whether this requires a shootdown before being processed
            std::set<SST::Event::id_type> writebacks; // writebacks that this command is waiting for, due to shootdown

            MSHREntry(SST::Event::id_type id, Command cmd, bool sdown = false) : id(id), cmd(cmd), shootdown(sdown) { }
    };

    std::map<SST::Event::id_type,OutstandingEvent> outstandingEventList_; // List of all outstanding events
    std::map<Addr,std::list<MSHREntry> > mshr_; // MSHR for keeping outstanding events coherent

    // Caching information
    bool directory_; // Whether or not a directory is managing the caches - if so we cannot assume on a writeback that the data is not cached
    std::vector<bool> cacheStatus_; // One entry per line, whether line may be cached - NOTE if this becomes too big, change to a set of cache entries

};

}}

#endif /* _COHERENTMEMORYCONTROLLER_H */
