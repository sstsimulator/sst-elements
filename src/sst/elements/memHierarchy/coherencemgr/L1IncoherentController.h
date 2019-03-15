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

#ifndef L1INCOHERENTCONTROLLER_H
#define L1INCOHERENTCONTROLLER_H

#include <iostream>

#include <sst/core/elementinfo.h>

#include "sst/elements/memHierarchy/coherencemgr/coherenceController.h"


namespace SST { namespace MemHierarchy {

class L1IncoherentController : public CoherenceController {
public:
/* Element Library Info */
    SST_ELI_REGISTER_SUBCOMPONENT(L1IncoherentController, "memHierarchy", "L1IncoherentController", SST_ELI_ELEMENT_VERSION(1,0,0), 
            "Implements an L1 cache without coherence", "SST::MemHierarchy::CoherenceController")

    SST_ELI_DOCUMENT_STATISTICS(
        /* Event sends */
        {"eventSent_GetS",              "Number of GetS requests sent", "events", 2},
        {"eventSent_GetX",              "Number of GetX requests sent", "events", 2},
        {"eventSent_GetSX",             "Number of GetSX requests sent", "events", 2},
        {"eventSent_GetSResp",          "Number of GetSResp responses sent", "events", 2},
        {"eventSent_GetXResp",          "Number of GetXResp responses sent", "events", 2},
        {"eventSent_PutE",              "Number of PutE requests sent", "events", 2},
        {"eventSent_PutM",              "Number of PutM requests sent", "events", 2},
        {"eventSent_NACK_up",           "Number of NACKs sent up (towards CPU)", "events", 2},
        {"eventSent_NACK_down",         "Number of NACKs sent down (towards main memory)", "events", 2},
        {"eventSent_FlushLine",         "Number of FlushLine requests sent", "events", 2},
        {"eventSent_FlushLineInv",      "Number of FlushLineInv requests sent", "events", 2},
        {"eventSent_FlushLineResp",     "Number of FlushLineResp responses sent", "events", 2},
        /* Event/State combinations - Count how many times an event was seen in particular state */
        {"stateEvent_GetS_I",           "Event/State: Number of times a GetS was seen in state I (Miss)", "count", 3},
        {"stateEvent_GetS_E",           "Event/State: Number of times a GetS was seen in state E (Hit)", "count", 3},
        {"stateEvent_GetS_M",           "Event/State: Number of times a GetS was seen in state M (Hit)", "count", 3},
        {"stateEvent_GetX_I",           "Event/State: Number of times a GetX was seen in state I (Miss)", "count", 3},
        {"stateEvent_GetX_E",           "Event/State: Number of times a GetX was seen in state E (Hit)", "count", 3},
        {"stateEvent_GetX_M",           "Event/State: Number of times a GetX was seen in state M (Hit)", "count", 3},
        {"stateEvent_GetSX_I",          "Event/State: Number of times a GetSX was seen in state I (Miss)", "count", 3},
        {"stateEvent_GetSX_E",          "Event/State: Number of times a GetSX was seen in state E (Hit)", "count", 3},
        {"stateEvent_GetSX_M",          "Event/State: Number of times a GetSX was seen in state M (Hit)", "count", 3},
        {"stateEvent_GetSResp_IS",      "Event/State: Number of times a GetSResp was seen in state IS", "count", 3},
        {"stateEvent_GetXResp_IM",      "Event/State: Number of times a GetXResp was seen in state IM", "count", 3},
        {"stateEvent_FlushLine_I",      "Event/State: Number of times a FlushLine was seen in state I", "count", 3},
        {"stateEvent_FlushLine_E",      "Event/State: Number of times a FlushLine was seen in state E", "count", 3},
        {"stateEvent_FlushLine_M",      "Event/State: Number of times a FlushLine was seen in state M", "count", 3},
        {"stateEvent_FlushLine_IS",     "Event/State: Number of times a FlushLine was seen in state IS", "count", 3},
        {"stateEvent_FlushLine_IM",     "Event/State: Number of times a FlushLine was seen in state IM", "count", 3},
        {"stateEvent_FlushLine_IB",     "Event/State: Number of times a FlushLine was seen in state I_B", "count", 3},
        {"stateEvent_FlushLine_SB",     "Event/State: Number of times a FlushLine was seen in state S_B", "count", 3},
        {"stateEvent_FlushLineInv_I",       "Event/State: Number of times a FlushLineInv was seen in state I", "count", 3},
        {"stateEvent_FlushLineInv_E",       "Event/State: Number of times a FlushLineInv was seen in state E", "count", 3},
        {"stateEvent_FlushLineInv_M",       "Event/State: Number of times a FlushLineInv was seen in state M", "count", 3},
        {"stateEvent_FlushLineInv_IS",      "Event/State: Number of times a FlushLineInv was seen in state IS", "count", 3},
        {"stateEvent_FlushLineInv_IM",      "Event/State: Number of times a FlushLineInv was seen in state IM", "count", 3},
        {"stateEvent_FlushLineInv_IB",      "Event/State: Number of times a FlushLineInv was seen in state I_B", "count", 3},
        {"stateEvent_FlushLineInv_SB",      "Event/State: Number of times a FlushLineInv was seen in state S_B", "count", 3},
        {"stateEvent_FlushLineResp_I",      "Event/State: Number of times a FlushLineResp was seen in state I", "count", 3},
        {"stateEvent_FlushLineResp_IB",     "Event/State: Number of times a FlushLineResp was seen in state I_B", "count", 3},
        {"stateEvent_FlushLineResp_SB",     "Event/State: Number of times a FlushLineResp was seen in state S_B", "count", 3},
        /* Eviction - count attempts to evict in a particular state */
        {"evict_I",                 "Eviction: Attempted to evict a block in state I", "count", 3},
        {"evict_E",                 "Eviction: Attempted to evict a block in state E", "count", 3},
        {"evict_M",                 "Eviction: Attempted to evict a block in state M", "count", 3},
        {"evict_IS",                "Eviction: Attempted to evict a block in state IS", "count", 3},
        {"evict_IM",                "Eviction: Attempted to evict a block in state IM", "count", 3},
        {"evict_IB",                "Eviction: Attempted to evict a block in state S_B", "count", 3},
        {"evict_SB",                "Eviction: Attempted to evict a block in state I_B", "count", 3},
        /* Latency for different kinds of misses*/
        {"latency_GetS_IS",         "Latency for read misses in I state", "cycles", 1},
        {"latency_GetS_M",          "Latency for read misses that find the block owned by another cache in M state", "cycles", 1},
        {"latency_GetX_IM",         "Latency for write misses in I state", "cycles", 1},
        {"latency_GetX_SM",         "Latency for write misses in S state", "cycles", 1},
        {"latency_GetX_M",          "Latency for write misses that find the block owned by another cache in M state", "cycles", 1},
        {"latency_GetSX_IM",        "Latency for read-exclusive misses in I state", "cycles", 1},
        {"latency_GetSX_SM",        "Latency for read-exclusive misses in S state", "cycles", 1},
        {"latency_GetSX_M",         "Latency for read-exclusive misses that find the block owned by another cache in M state", "cycles", 1},
        /* Track what happens to prefetched blocks */
        {"prefetch_useful",         "Prefetched block had a subsequent hit (useful prefetch)", "count", 2},
        {"prefetch_evict",          "Prefetched block was evicted/flushed before being accessed", "count", 2},
        {"prefetch_redundant",      "Prefetch issued for a block that was already in cache", "count", 2})

/* Class definition */
    
        
/* Begin class definition */    
    /** Constructor for L1IncoherentController */
    L1IncoherentController(Component* comp, Params& params) : CoherenceController(comp, params) {
        debug->debug(_INFO_,"--------------------------- Initializing [L1Controller] ... \n\n");
       
        /* Register statistics */
        stat_stateEvent_GetS_I = registerStatistic<uint64_t>("stateEvent_GetS_I");
        stat_stateEvent_GetS_E = registerStatistic<uint64_t>("stateEvent_GetS_E");
        stat_stateEvent_GetS_M = registerStatistic<uint64_t>("stateEvent_GetS_M");
        stat_stateEvent_GetX_I = registerStatistic<uint64_t>("stateEvent_GetX_I");
        stat_stateEvent_GetX_E = registerStatistic<uint64_t>("stateEvent_GetX_E");
        stat_stateEvent_GetX_M = registerStatistic<uint64_t>("stateEvent_GetX_M");
        stat_stateEvent_GetSX_I = registerStatistic<uint64_t>("stateEvent_GetSX_I");
        stat_stateEvent_GetSX_E = registerStatistic<uint64_t>("stateEvent_GetSX_E");
        stat_stateEvent_GetSX_M = registerStatistic<uint64_t>("stateEvent_GetSX_M");
        stat_stateEvent_GetSResp_IS = registerStatistic<uint64_t>("stateEvent_GetSResp_IS");
        stat_stateEvent_GetXResp_IM = registerStatistic<uint64_t>("stateEvent_GetXResp_IM");
        stat_stateEvent_FlushLine_I = registerStatistic<uint64_t>("stateEvent_FlushLine_I");
        stat_stateEvent_FlushLine_E = registerStatistic<uint64_t>("stateEvent_FlushLine_E");
        stat_stateEvent_FlushLine_M = registerStatistic<uint64_t>("stateEvent_FlushLine_M");
        stat_stateEvent_FlushLine_IS = registerStatistic<uint64_t>("stateEvent_FlushLine_IS");
        stat_stateEvent_FlushLine_IM = registerStatistic<uint64_t>("stateEvent_FlushLine_IM");
        stat_stateEvent_FlushLine_IB = registerStatistic<uint64_t>("stateEvent_FlushLine_IB");
        stat_stateEvent_FlushLine_SB = registerStatistic<uint64_t>("stateEvent_FlushLine_SB");
        stat_stateEvent_FlushLineInv_I = registerStatistic<uint64_t>("stateEvent_FlushLineInv_I");
        stat_stateEvent_FlushLineInv_E = registerStatistic<uint64_t>("stateEvent_FlushLineInv_E");
        stat_stateEvent_FlushLineInv_M = registerStatistic<uint64_t>("stateEvent_FlushLineInv_M");
        stat_stateEvent_FlushLineInv_IS = registerStatistic<uint64_t>("stateEvent_FlushLineInv_IS");
        stat_stateEvent_FlushLineInv_IM = registerStatistic<uint64_t>("stateEvent_FlushLineInv_IM");
        stat_stateEvent_FlushLineInv_IB = registerStatistic<uint64_t>("stateEvent_FlushLineInv_IB");
        stat_stateEvent_FlushLineInv_SB = registerStatistic<uint64_t>("stateEvent_FlushLineInv_SB");
        stat_stateEvent_FlushLineResp_I = registerStatistic<uint64_t>("stateEvent_FlushLineResp_I");
        stat_stateEvent_FlushLineResp_IB = registerStatistic<uint64_t>("stateEvent_FlushLineResp_IB");
        stat_stateEvent_FlushLineResp_SB = registerStatistic<uint64_t>("stateEvent_FlushLineResp_SB");
        stat_eventSent_GetS =           registerStatistic<uint64_t>("eventSent_GetS");
        stat_eventSent_GetX =           registerStatistic<uint64_t>("eventSent_GetX");
        stat_eventSent_GetSX =         registerStatistic<uint64_t>("eventSent_GetSX");
        stat_eventSent_PutM =           registerStatistic<uint64_t>("eventSent_PutM");
        stat_eventSent_NACK_down =      registerStatistic<uint64_t>("eventSent_NACK_down");
        stat_eventSent_FlushLine =      registerStatistic<uint64_t>("eventSent_FlushLine");
        stat_eventSent_FlushLineInv =   registerStatistic<uint64_t>("eventSent_FlushLineInv");
        stat_eventSent_GetSResp =       registerStatistic<uint64_t>("eventSent_GetSResp");
        stat_eventSent_GetXResp =       registerStatistic<uint64_t>("eventSent_GetXResp");
        stat_eventSent_FlushLineResp =  registerStatistic<uint64_t>("eventSent_FlushLineResp");
        stat_eventSent_NACK_up =        registerStatistic<uint64_t>("eventSent_NACK_up");
    
        /* Only for caches that write back clean blocks (i.e., lower cache is non-inclusive and may need the data) but don't know yet and can't register statistics later. Always enabled for now. */
        stat_eventSent_PutE =           registerStatistic<uint64_t>("eventSent_PutE");
        
        /* Prefetch statistics */
        if (!params.find<std::string>("prefetcher", "").empty()) {
            statPrefetchEvict = registerStatistic<uint64_t>("prefetch_evict");
            statPrefetchHit = registerStatistic<uint64_t>("prefetch_useful");
            statPrefetchRedundant = registerStatistic<uint64_t>("prefetch_redundant");
        }
    }

    ~L1IncoherentController() {}
    
    
    /** Used to determine in advance if an event will be a miss (and which kind of miss)
     * Used for statistics only
     */
    int isCoherenceMiss(MemEvent* event, CacheLine* cacheLine);
    
    /* Event handlers called by cache controller */
    /** Send cache line data to the lower level caches */
    CacheAction handleEviction(CacheLine* wbCacheLine, string origRqstr, bool ignoredParam=false);

    /** Process new cache request:  GetX, GetS, GetSX */
    CacheAction handleRequest(MemEvent* event, CacheLine* cacheLine, bool replay);
   
    /** Process replacement - implemented for compatibility with CoherenceController but L1s do not receive replacements */
    CacheAction handleReplacement(MemEvent* event, CacheLine* cacheLine, MemEvent * reqEvent, bool replay);
    
    /** Process Inv */
    CacheAction handleInvalidationRequest(MemEvent *event, CacheLine* cacheLine, MemEvent * collisionEvent, bool replay);

    /** Process responses */
    CacheAction handleResponse(MemEvent* responseEvent, CacheLine* cacheLine, MemEvent* origRequest);

    /* Methods for sending events, called by cache controller */
    /** Send response up (to processor) */
    uint64_t sendResponseUp(MemEvent * event, vector<uint8_t>* data, bool replay, uint64_t baseTime, bool atomic = false);
    
    /** Call through to coherenceController with statistic recording */
    void addToOutgoingQueue(Response& resp);
    void addToOutgoingQueueUp(Response& resp);

/* Miscellaneous */
    /** Determine whether a NACKed event should be retried */
    bool isRetryNeeded(MemEvent * event, CacheLine * cacheLine);


    void printData(vector<uint8_t> * data, bool set);

private:
    /* Statistics */
    Statistic<uint64_t>* stat_stateEvent_GetS_I;
    Statistic<uint64_t>* stat_stateEvent_GetS_E;
    Statistic<uint64_t>* stat_stateEvent_GetS_M;
    Statistic<uint64_t>* stat_stateEvent_GetX_I;
    Statistic<uint64_t>* stat_stateEvent_GetX_E;
    Statistic<uint64_t>* stat_stateEvent_GetX_M;
    Statistic<uint64_t>* stat_stateEvent_GetSX_I;
    Statistic<uint64_t>* stat_stateEvent_GetSX_E;
    Statistic<uint64_t>* stat_stateEvent_GetSX_M;
    Statistic<uint64_t>* stat_stateEvent_GetSResp_IS;
    Statistic<uint64_t>* stat_stateEvent_GetXResp_IM;
    Statistic<uint64_t>* stat_stateEvent_FlushLine_I;
    Statistic<uint64_t>* stat_stateEvent_FlushLine_E;
    Statistic<uint64_t>* stat_stateEvent_FlushLine_M;
    Statistic<uint64_t>* stat_stateEvent_FlushLine_IS;
    Statistic<uint64_t>* stat_stateEvent_FlushLine_IM;
    Statistic<uint64_t>* stat_stateEvent_FlushLine_IB;
    Statistic<uint64_t>* stat_stateEvent_FlushLine_SB;
    Statistic<uint64_t>* stat_stateEvent_FlushLineInv_I;
    Statistic<uint64_t>* stat_stateEvent_FlushLineInv_E;
    Statistic<uint64_t>* stat_stateEvent_FlushLineInv_M;
    Statistic<uint64_t>* stat_stateEvent_FlushLineInv_IS;
    Statistic<uint64_t>* stat_stateEvent_FlushLineInv_IM;
    Statistic<uint64_t>* stat_stateEvent_FlushLineInv_IB;
    Statistic<uint64_t>* stat_stateEvent_FlushLineInv_SB;
    Statistic<uint64_t>* stat_stateEvent_FlushLineResp_I;
    Statistic<uint64_t>* stat_stateEvent_FlushLineResp_IB;
    Statistic<uint64_t>* stat_stateEvent_FlushLineResp_SB;
    Statistic<uint64_t>* stat_eventSent_GetS;
    Statistic<uint64_t>* stat_eventSent_GetX;
    Statistic<uint64_t>* stat_eventSent_GetSX;
    Statistic<uint64_t>* stat_eventSent_PutE;
    Statistic<uint64_t>* stat_eventSent_PutM;
    Statistic<uint64_t>* stat_eventSent_NACK_down;
    Statistic<uint64_t>* stat_eventSent_FlushLine;
    Statistic<uint64_t>* stat_eventSent_FlushLineInv;
    Statistic<uint64_t>* stat_eventSent_GetSResp;
    Statistic<uint64_t>* stat_eventSent_GetXResp;
    Statistic<uint64_t>* stat_eventSent_FlushLineResp;
    Statistic<uint64_t>* stat_eventSent_NACK_up;

    /* Private event handlers */
    /** Handle GetX request. Request upgrade if needed */
    CacheAction handleGetXRequest(MemEvent* event, CacheLine* cacheLine, bool replay);
    
    /** Handle GetS request. Request block if needed */
    CacheAction handleGetSRequest(MemEvent* event, CacheLine* cacheLine, bool replay);
    
    /** Handle FlushLine request. */
    CacheAction handleFlushLineRequest(MemEvent *event, CacheLine* cacheLine, MemEvent* reqEvent, bool replay);
    
    /** Handle FlushLineInv request */
    CacheAction handleFlushLineInvRequest(MemEvent *event, CacheLine* cacheLine, MemEvent* reqEvent, bool replay);

    /** Handle data response - GetSResp or GetXResp */
    void handleDataResponse(MemEvent* responseEvent, CacheLine * cacheLine, MemEvent * reqEvent);

    
    /* Methods for sending events */
    /** Send writeback request to lower level caches */
    void sendWriteback(Command cmd, CacheLine* cacheLine, string origRqstr);

    /** Forward a flush line request, with or without data */
    void forwardFlushLine(Addr baseAddr, Command cmd, string origRqstr, CacheLine * cacheLine);
    
    /** Send response to a flush request */
    void sendFlushResponse(MemEvent * requestEvent, bool success, uint64_t baseTime, bool replay);

    /* Statistics recording */
    void recordStateEventCount(Command cmd, State state);
    void recordEventSentDown(Command cmd);
    void recordEventSentUp(Command cmd);
};


}}


#endif

