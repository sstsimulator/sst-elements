// Copyright 2009-2019 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2019, NTESS
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
#include <array>

#include "sst/elements/memHierarchy/coherencemgr/coherenceController.h"


namespace SST { namespace MemHierarchy {

class IncoherentL1 : public CoherenceController {
public:
/* Element Library Info */
    SST_ELI_REGISTER_SUBCOMPONENT_DERIVED(IncoherentL1, "memHierarchy", "coherence.incoherent_l1", SST_ELI_ELEMENT_VERSION(1,0,0), 
            "Implements an L1 cache without coherence", SST::MemHierarchy::CoherenceController)

    SST_ELI_DOCUMENT_STATISTICS(
        /* Event sends */
        {"eventSent_GetS",          "Number of GetS requests sent", "events", 2},
        {"eventSent_GetX",          "Number of GetX requests sent", "events", 2},
        {"eventSent_GetSX",         "Number of GetSX requests sent", "events", 2},
        {"eventSent_GetSResp",      "Number of GetSResp responses sent", "events", 2},
        {"eventSent_GetXResp",      "Number of GetXResp responses sent", "events", 2},
        {"eventSent_PutE",          "Number of PutE requests sent", "events", 2},
        {"eventSent_PutM",          "Number of PutM requests sent", "events", 2},
        {"eventSent_FlushLine",     "Number of FlushLine requests sent", "events", 2},
        {"eventSent_FlushLineInv",  "Number of FlushLineInv requests sent", "events", 2},
        {"eventSent_FlushLineResp", "Number of FlushLineResp responses sent", "events", 2},
        {"eventSent_NACK",          "Number of NACKs sent", "events", 2},
        {"eventSent_Put",           "Number of Put requests sent", "events", 6},
        {"eventSent_Get",           "Number of Get requests sent", "events", 6},
        {"eventSent_AckMove",       "Number of AckMove responses sent", "events", 6},
        {"eventSent_CustomReq",     "Number of CustomReq requests sent", "events", 4},
        {"eventSent_CustomResp",    "Number of CustomResp responses sent", "events", 4},
        {"eventSent_CustomAck",     "Number of CustomAck responses sent", "events", 4},
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
        {"latency_GetS_miss",           "Latency for read misses", "cycles", 1},
        {"latency_GetS_hit",            "Latency for read hits", "cycles", 1},
        {"latency_GetX_miss",           "Latency for write misses", "cycles", 1},
        {"latency_GetX_hit",            "Latency for write hits", "cycles", 1},
        {"latency_GetSX_miss",          "Latency for read-exclusive misses", "cycles", 1},
        {"latency_GetSX_hit",           "Latency for read-exclusive hits", "cycles", 1},
        {"latency_FlushLine",           "Latency for Flush requests", "cycles", 1},
        {"latency_FlushLine_fail",      "Latency for Flush requests that failed", "cycles", 1},
        {"latency_FlushLineInv",        "Latency for Flush and Invalidate requests", "cycles", 1},
        {"latency_FlushLineInv_fail",   "Latency for Flush and Invalidate requests that failed", "cycles", 1},
        /* Track what happens to prefetched blocks */
        {"prefetch_useful",         "Prefetched block had a subsequent hit (useful prefetch)", "count", 2},
        {"prefetch_evict",          "Prefetched block was evicted/flushed before being accessed", "count", 2},
        {"prefetch_redundant",      "Prefetch issued for a block that was already in cache", "count", 2},
        {"default_stat",            "Default statistic used for unexpected events/states/etc. Should be 0, if not, check for missing statistic registerations.", "none", 7})

/* Class definition */
    
        
/* Begin class definition */    
    /** Constructor for IncoherentL1 */
    IncoherentL1(Component* comp, Params& params) : CoherenceController(comp, params) { }
    IncoherentL1(ComponentId_t id, Params& params, Params& ownerParams, bool prefetch) : CoherenceController(id, params, ownerParams, prefetch) {
        params.insert(ownerParams);
        debug->debug(_INFO_,"--------------------------- Initializing [L1Controller] ... \n\n");

        /* Register statistics */
        Statistic<uint64_t>* defStat = registerStatistic<uint64_t>("default_stat");
        for (int i = 0; i < (int)Command::LAST_CMD; i++) {
            stat_eventSent[i] = defStat;
            for (int j = 0; j < LAST_STATE; j++)
                stat_eventState[i][j] = defStat;
        }

        stat_eventState[(int)Command::GetS][I] = registerStatistic<uint64_t>("stateEvent_GetS_I");
        stat_eventState[(int)Command::GetS][E] = registerStatistic<uint64_t>("stateEvent_GetS_E");
        stat_eventState[(int)Command::GetS][M] = registerStatistic<uint64_t>("stateEvent_GetS_M");
        stat_eventState[(int)Command::GetX][I] = registerStatistic<uint64_t>("stateEvent_GetX_I");
        stat_eventState[(int)Command::GetX][E] = registerStatistic<uint64_t>("stateEvent_GetX_E");
        stat_eventState[(int)Command::GetX][M] = registerStatistic<uint64_t>("stateEvent_GetX_M");
        stat_eventState[(int)Command::GetSX][I] = registerStatistic<uint64_t>("stateEvent_GetSX_I");
        stat_eventState[(int)Command::GetSX][E] = registerStatistic<uint64_t>("stateEvent_GetSX_E");
        stat_eventState[(int)Command::GetSX][M] = registerStatistic<uint64_t>("stateEvent_GetSX_M");
        stat_eventState[(int)Command::GetSResp][IS] = registerStatistic<uint64_t>("stateEvent_GetSResp_IS");
        stat_eventState[(int)Command::GetXResp][IM] = registerStatistic<uint64_t>("stateEvent_GetXResp_IM");
        stat_eventState[(int)Command::FlushLine][I] = registerStatistic<uint64_t>("stateEvent_FlushLine_I");
        stat_eventState[(int)Command::FlushLine][E] = registerStatistic<uint64_t>("stateEvent_FlushLine_E");
        stat_eventState[(int)Command::FlushLine][M] = registerStatistic<uint64_t>("stateEvent_FlushLine_M");
        stat_eventState[(int)Command::FlushLine][IS] = registerStatistic<uint64_t>("stateEvent_FlushLine_IS");
        stat_eventState[(int)Command::FlushLine][IM] = registerStatistic<uint64_t>("stateEvent_FlushLine_IM");
        stat_eventState[(int)Command::FlushLine][I_B] = registerStatistic<uint64_t>("stateEvent_FlushLine_IB");
        stat_eventState[(int)Command::FlushLine][S_B] = registerStatistic<uint64_t>("stateEvent_FlushLine_SB");
        stat_eventState[(int)Command::FlushLineInv][I] = registerStatistic<uint64_t>("stateEvent_FlushLineInv_I");
        stat_eventState[(int)Command::FlushLineInv][E] = registerStatistic<uint64_t>("stateEvent_FlushLineInv_E");
        stat_eventState[(int)Command::FlushLineInv][M] = registerStatistic<uint64_t>("stateEvent_FlushLineInv_M");
        stat_eventState[(int)Command::FlushLineInv][IS] = registerStatistic<uint64_t>("stateEvent_FlushLineInv_IS");
        stat_eventState[(int)Command::FlushLineInv][IM] = registerStatistic<uint64_t>("stateEvent_FlushLineInv_IM");
        stat_eventState[(int)Command::FlushLineInv][I_B] = registerStatistic<uint64_t>("stateEvent_FlushLineInv_IB");
        stat_eventState[(int)Command::FlushLineInv][S_B] = registerStatistic<uint64_t>("stateEvent_FlushLineInv_SB");
        stat_eventState[(int)Command::FlushLineResp][I] = registerStatistic<uint64_t>("stateEvent_FlushLineResp_I");
        stat_eventState[(int)Command::FlushLineResp][I_B] = registerStatistic<uint64_t>("stateEvent_FlushLineResp_IB");
        stat_eventState[(int)Command::FlushLineResp][S_B] = registerStatistic<uint64_t>("stateEvent_FlushLineResp_SB");
        stat_eventSent[(int)Command::GetS] =           registerStatistic<uint64_t>("eventSent_GetS");
        stat_eventSent[(int)Command::GetX] =           registerStatistic<uint64_t>("eventSent_GetX");
        stat_eventSent[(int)Command::GetSX] =         registerStatistic<uint64_t>("eventSent_GetSX");
        stat_eventSent[(int)Command::PutM] =           registerStatistic<uint64_t>("eventSent_PutM");
        stat_eventSent[(int)Command::NACK] =      registerStatistic<uint64_t>("eventSent_NACK");
        stat_eventSent[(int)Command::FlushLine] =      registerStatistic<uint64_t>("eventSent_FlushLine");
        stat_eventSent[(int)Command::FlushLineInv] =   registerStatistic<uint64_t>("eventSent_FlushLineInv");
        stat_eventSent[(int)Command::GetSResp] =       registerStatistic<uint64_t>("eventSent_GetSResp");
        stat_eventSent[(int)Command::GetXResp] =       registerStatistic<uint64_t>("eventSent_GetXResp");
        stat_eventSent[(int)Command::FlushLineResp] =  registerStatistic<uint64_t>("eventSent_FlushLineResp");
        stat_eventSent[(int)Command::Put]           = registerStatistic<uint64_t>("eventSent_Put");
        stat_eventSent[(int)Command::Get]           = registerStatistic<uint64_t>("eventSent_Get");
        stat_eventSent[(int)Command::AckMove]       = registerStatistic<uint64_t>("eventSent_AckMove");
        stat_eventSent[(int)Command::CustomReq]     = registerStatistic<uint64_t>("eventSent_CustomReq");
        stat_eventSent[(int)Command::CustomResp]    = registerStatistic<uint64_t>("eventSent_CustomResp");
        stat_eventSent[(int)Command::CustomAck]     = registerStatistic<uint64_t>("eventSent_CustomAck");
        stat_latencyGetS[LatType::HIT]  = registerStatistic<uint64_t>("latency_GetS_hit");
        stat_latencyGetS[LatType::MISS] = registerStatistic<uint64_t>("latency_GetS_miss");
        stat_latencyGetX[LatType::HIT]  = registerStatistic<uint64_t>("latency_GetX_hit");
        stat_latencyGetX[LatType::MISS] = registerStatistic<uint64_t>("latency_GetX_miss");
        stat_latencyGetSX[LatType::HIT]  = registerStatistic<uint64_t>("latency_GetSX_hit");
        stat_latencyGetSX[LatType::MISS] = registerStatistic<uint64_t>("latency_GetSX_miss");
        stat_latencyFlushLine[LatType::HIT] = registerStatistic<uint64_t>("latency_FlushLine");
        stat_latencyFlushLine[LatType::MISS] = registerStatistic<uint64_t>("latency_FlushLine_fail");
        stat_latencyFlushLineInv[LatType::HIT] = registerStatistic<uint64_t>("latency_FlushLineInv");
        stat_latencyFlushLineInv[LatType::MISS] = registerStatistic<uint64_t>("latency_FlushLineInv_fail");
    
        /* Only for caches that write back clean blocks (i.e., lower cache is non-inclusive and may need the data) but don't know yet and can't register statistics later. Always enabled for now. */
        stat_eventSent[(int)Command::PutE] =           registerStatistic<uint64_t>("eventSent_PutE");
        
        /* Prefetch statistics */
        if (prefetch) {
            statPrefetchEvict = registerStatistic<uint64_t>("prefetch_evict");
            statPrefetchHit = registerStatistic<uint64_t>("prefetch_useful");
            statPrefetchRedundant = registerStatistic<uint64_t>("prefetch_redundant");
        }
    }

    ~IncoherentL1() {}
    
    
    /** Used to determine in advance if an event will be a miss (and which kind of miss)
     * Used for statistics only
     */
    bool isCacheHit(MemEvent* event);
    
    /* Event handlers called by cache controller */
    /** Send cache line data to the lower level caches */
    CacheAction handleEviction(CacheLine* wbCacheLine, string origRqstr, bool ignoredParam=false);

    /** Process new cache request:  GetX, GetS, GetSX */
    CacheAction handleRequest(MemEvent* event, bool replay);
   
    /** Process replacement - implemented for compatibility with CoherenceController but L1s do not receive replacements */
    CacheAction handleReplacement(MemEvent* event, bool replay);
    CacheAction handleFlush(MemEvent* event, CacheLine* cacheLine, MemEvent* reqEvent, bool replay);
    
    /** Process Inv */
    CacheAction handleInvalidationRequest(MemEvent *event, bool inMSHR);

    /** Process responses */
    CacheAction handleCacheResponse(MemEvent* event, bool inMSHR);
    CacheAction handleFetchResponse(MemEvent* event, bool inMSHR);

    bool handleNACK(MemEvent* event, bool inMSHR);

    /* Methods for sending events, called by cache controller */
    /** Send response up (to processor) */
    uint64_t sendResponseUp(MemEvent * event, vector<uint8_t>* data, bool replay, uint64_t baseTime, bool atomic = false);
    
    /** Call through to coherenceController with statistic recording */
    void addToOutgoingQueue(Response& resp);
    void addToOutgoingQueueUp(Response& resp);

/* Miscellaneous */
    void printData(vector<uint8_t> * data, bool set);

/* Temporary */
    void setCacheArray(CacheArray* arrayptr) { cacheArray_ = arrayptr; }

    void printLine(Addr addr);

private:
    CacheArray* cacheArray_;

    /* Statistics */
    std::array<std::array<Statistic<uint64_t>*, LAST_STATE>, (int)Command::LAST_CMD> stat_eventState;
    Statistic<uint64_t>* stat_eventSent[(int)Command::LAST_CMD];
    Statistic<uint64_t>* stat_latencyGetS[2];
    Statistic<uint64_t>* stat_latencyGetX[2];
    Statistic<uint64_t>* stat_latencyGetSX[2];
    Statistic<uint64_t>* stat_latencyFlushLine[2];
    Statistic<uint64_t>* stat_latencyFlushLineInv[2];

    void printLine(Addr addr, CacheLine* line);

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

/* Private methods for managing data structures */
    bool allocateLine(Addr addr, MemEvent* event);

    
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
    void recordLatency(Command cmd, int type, uint64_t timestamp);
};


}}


#endif

