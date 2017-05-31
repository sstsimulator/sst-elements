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

#ifndef _SCRATCHPAD_H
#define _SCRATCHPAD_H


#include <sst/core/event.h>
#include <sst/core/sst_types.h>
#include <sst/core/component.h>
#include <sst/core/link.h>
#include <sst/core/output.h>
#include <map>
#include <unordered_map>
#include <queue>

#include "sst/elements/memHierarchy/membackend/backing.h"
#include "moveEvent.h"
#include "memEvent.h"
#include "memNIC.h"

namespace SST {
namespace MemHierarchy {

class ScratchBackendConvertor;

class Scratchpad : public SST::Component {
public:
    Scratchpad(ComponentId_t id, Params &params);
    void init(unsigned int);
    void setup();
    void finish();

    // Output for debug info
    Output dbg;

    // Handler for backend responses
    void handleScratchResponse(SST::Event::id_type id);

private:

    ~Scratchpad() {}

    // Parameters - scratchpad
    uint64_t scratchSize_;      // Size of the total scratchpad in bytes - any address above this is assumed to address remote memory
    uint64_t scratchLineSize_;  // Size of each line in the scratchpad in bytes
    bool doNotBack_;            // For very large scratchpads where data doesn't matter
    
    // Parameters - memory
    uint64_t remoteAddrOffset_;   // Offset for remote addresses, defaults to scratchSize (i.e., CPU addr scratchSize = mem addr 0)
    uint64_t remoteLineSize_;

    // Backend
    ScratchBackendConvertor * scratch_;

    // Backing store for scratchpad
    Backend::Backing * backing_;

    // Local variables
    uint64_t timestamp_;

    // Event handling
    bool clock(SST::Cycle_t cycle);

    void processIncomingCPUEvent(SST::Event* event);
    void processIncomingRemoteEvent(SST::Event* event);

    void handleRead(MemEventBase * event);
    void handleWrite(MemEventBase * event);

    void handleScratchRead(MemEvent * event);
    void handleScratchWrite(MemEvent * event);
    void handleRemoteRead(MemEvent * event);
    void handleRemoteWrite(MemEvent * event);
    void handleScratchGet(MemEventBase * event);
    void handleScratchPut(MemEventBase * event);

    // Helper methods
    void removeFromMSHR(Addr addr);

    // Links
    SST::Link* linkUp_;     // To cache/cpu
    SST::Link* linkDown_;   // To memory
    MemNIC* linkNet_;       // To memory over network

    // ScratchPair
    struct ScratchPair {
        MemEventBase * request;
        MemEvent *  memEvent;

        ScratchPair(MemEventBase * req, MemEvent * memev) : request(req), memEvent(memev) {}
    };

    // Request tracking
    std::map<SST::Event::id_type, ScratchPair> pendingPool_; // Global map of all outstanding requests by ID mapped to request and response
    std::map<SST::Event::id_type,SST::Event::id_type> remoteIDMap_; // Map outstanding remote requests to initiating scratch event by IDs (1-1 but conversion from scratchEvent to memEvent)
    std::map<SST::Event::id_type, std::pair<SST::Event::id_type, Addr> > scratchIDMap_; // Map oustanding scratch requests to initiating scratch event by IDs (1-1 for reads, multi-1 for get/put)
    std::map<SST::Event::id_type, uint64_t> scratchCounters_; // Map of a ScratchPutID to the number of scratch reads we are waiting for

    std::unordered_map<Addr, std::queue<MemEvent*> >  scratchMSHR_; // MSHR for managing conflicts to scratch

    // Outgoing message queues - map send timestamp to event
    std::multimap<uint64_t, MemEventBase*> procMsgQueue_;
    std::multimap<uint64_t, MemEvent*> memMsgQueue_;
    
    // Throughput limits
    uint32_t responsesPerCycle_;
    
    // Caching information
    bool caching_;  // Whether or not caching is possible
    std::vector<bool> cacheStatus_; // One entry per scratchpad line, whether line may be cached

    // Statistics
    Statistic<uint64_t>* stat_ScratchReadReceived;
    Statistic<uint64_t>* stat_ScratchWriteReceived;
    Statistic<uint64_t>* stat_RemoteReadReceived;
    Statistic<uint64_t>* stat_RemoteWriteReceived;
    Statistic<uint64_t>* stat_ScratchGetReceived;
    Statistic<uint64_t>* stat_ScratchPutReceived;
    Statistic<uint64_t>* stat_ScratchReadIssued;
    Statistic<uint64_t>* stat_ScratchWriteIssued;
};

}}

#endif /* _SCRATCHPAD_H */
