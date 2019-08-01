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

#ifndef INCOHERENTCONTROLLER_H
#define INCOHERENTCONTROLLER_H

#include <iostream>
#include <array>

#include "sst/elements/memHierarchy/coherencemgr/coherenceController.h"


namespace SST { namespace MemHierarchy {

class IncoherentController : public CoherenceController{
public:
    SST_ELI_REGISTER_SUBCOMPONENT_DERIVED(IncoherentController, "memHierarchy", "IncoherentController", SST_ELI_ELEMENT_VERSION(1,0,0), 
            "Implements an second level or greater cache without coherence", SST::MemHierarchy::CoherenceController)

    SST_ELI_DOCUMENT_STATISTICS(
        /* Event sends */
        {"eventSent_GetS",          "Number of GetS requests sent", "events", 2},
        {"eventSent_GetX",          "Number of GetX requests sent", "events", 2},
        {"eventSent_GetSX",         "Number of GetSX requests sent", "events", 2},
        {"eventSent_GetSResp",      "Number of GetSResp responses sent", "events", 2},
        {"eventSent_GetXResp",      "Number of GetXResp responses sent", "events", 2},
        {"eventSent_PutE",          "Number of PutE requests sent", "events", 2},
        {"eventSent_PutM",          "Number of PutM requests sent", "events", 2},
        {"eventSent_NACK",          "Number of NACKs sent", "events", 2},
        {"eventSent_FlushLine",     "Number of FlushLine requests sent", "events", 2},
        {"eventSent_FlushLineInv",  "Number of FlushLineInv requests sent", "events", 2},
        {"eventSent_FlushLineResp", "Number of FlushLineResp responses sent", "events", 2},
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
        {"stateEvent_GetXResp_IS",      "Event/State: Number of times a GetXResp was seen in state IS", "count", 3},
        {"stateEvent_GetXResp_IM",      "Event/State: Number of times a GetXResp was seen in state IM", "count", 3},
        {"stateEvent_PutE_I",           "Event/State: Number of times a PutE was seen in state I", "count", 3},
        {"stateEvent_PutE_E",           "Event/State: Number of times a PutE was seen in state E", "count", 3},
        {"stateEvent_PutE_M",           "Event/State: Number of times a PutE was seen in state M", "count", 3},
        {"stateEvent_PutE_IM",          "Event/State: Number of times a PutE was seen in state IM", "count", 3},
        {"stateEvent_PutE_IS",          "Event/State: Number of times a PutE was seen in state IS", "count", 3},
        {"stateEvent_PutE_IB",          "Event/State: Number of times a PutE was seen in state I_B", "count", 3},
        {"stateEvent_PutE_SB",          "Event/State: Number of times a PutE was seen in state S_B", "count", 3},
        {"stateEvent_PutM_I",           "Event/State: Number of times a PutM was seen in state I", "count", 3},
        {"stateEvent_PutM_E",           "Event/State: Number of times a PutM was seen in state E", "count", 3},
        {"stateEvent_PutM_M",           "Event/State: Number of times a PutM was seen in state M", "count", 3},
        {"stateEvent_PutM_IS",          "Event/State: Number of times a PutM was seen in state IS", "count", 3},
        {"stateEvent_PutM_IM",          "Event/State: Number of times a PutM was seen in state IM", "count", 3},
        {"stateEvent_PutM_IB",          "Event/State: Number of times a PutM was seen in state I_B", "count", 3},
        {"stateEvent_PutM_SB",          "Event/State: Number of times a PutM was seen in state S_B", "count", 3},
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
        {"latency_FlushLineInv",        "Latency for Flush and Invalidate requests", "cycles", 1},
        /* Track what happens to prefetched blocks */
        {"prefetch_useful",         "Prefetched block had a subsequent hit (useful prefetch)", "count", 2},
        {"prefetch_evict",          "Prefetched block was evicted/flushed before being accessed", "count", 2},
        {"prefetch_redundant",      "Prefetch issued for a block that was already in cache", "count", 2},
        {"default_stat",            "Default statistic used for unexpected events/states/etc. Should be 0, if not, check for missing statistic registerations.", "none", 7})

/* Class definition */
    /** Constructor for IncoherentController. */
    IncoherentController(SST::Component* comp, Params& params) : CoherenceController (comp, params) { }
    IncoherentController(SST::ComponentId_t id, Params& params, Params& ownerParams, bool prefetch) : CoherenceController(id, params, ownerParams, prefetch) {
        params.insert(ownerParams);
        debug->debug(_INFO_,"--------------------------- Initializing [Incoherent Controller] ... \n\n");
        inclusive_ = params.find<bool>("inclusive", true);
    
        /* Register statistics */
        Statistic<uint64_t>* defStat = registerStatistic<uint64_t>("default_stat");
        for (int i = 0; i < (int)Command::LAST_CMD; i++) {
            stat_eventSent[i] = defStat;
            for (int j = 0; j < LAST_STATE; j++) {
                stat_eventState[i][j] = defStat;
            }
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
        stat_eventState[(int)Command::GetXResp][IS] = registerStatistic<uint64_t>("stateEvent_GetXResp_IS");
        stat_eventState[(int)Command::GetXResp][IM] = registerStatistic<uint64_t>("stateEvent_GetXResp_IM");
        stat_eventState[(int)Command::PutE][I] = registerStatistic<uint64_t>("stateEvent_PutE_I");
        stat_eventState[(int)Command::PutE][E] = registerStatistic<uint64_t>("stateEvent_PutE_E");
        stat_eventState[(int)Command::PutE][M] = registerStatistic<uint64_t>("stateEvent_PutE_M");
        stat_eventState[(int)Command::PutE][IS] = registerStatistic<uint64_t>("stateEvent_PutE_IS");
        stat_eventState[(int)Command::PutE][IM] = registerStatistic<uint64_t>("stateEvent_PutE_IM");
        stat_eventState[(int)Command::PutE][I_B] = registerStatistic<uint64_t>("stateEvent_PutE_IB");
        stat_eventState[(int)Command::PutE][S_B] = registerStatistic<uint64_t>("stateEvent_PutE_SB");
        stat_eventState[(int)Command::PutM][I] = registerStatistic<uint64_t>("stateEvent_PutM_I");
        stat_eventState[(int)Command::PutM][E] = registerStatistic<uint64_t>("stateEvent_PutM_E");
        stat_eventState[(int)Command::PutM][M] = registerStatistic<uint64_t>("stateEvent_PutM_M");
        stat_eventState[(int)Command::PutM][IS] = registerStatistic<uint64_t>("stateEvent_PutM_IS");
        stat_eventState[(int)Command::PutM][IM] = registerStatistic<uint64_t>("stateEvent_PutM_IM");
        stat_eventState[(int)Command::PutM][I_B] = registerStatistic<uint64_t>("stateEvent_PutM_IB");
        stat_eventState[(int)Command::PutM][S_B] = registerStatistic<uint64_t>("stateEvent_PutM_SB");
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
        stat_eventSent[(int)Command::GetS]             = registerStatistic<uint64_t>("eventSent_GetS");
        stat_eventSent[(int)Command::GetX]             = registerStatistic<uint64_t>("eventSent_GetX");
        stat_eventSent[(int)Command::GetSX]           = registerStatistic<uint64_t>("eventSent_GetSX");
        stat_eventSent[(int)Command::PutE]             = registerStatistic<uint64_t>("eventSent_PutE");
        stat_eventSent[(int)Command::PutM]             = registerStatistic<uint64_t>("eventSent_PutM");
        stat_eventSent[(int)Command::FlushLine]        = registerStatistic<uint64_t>("eventSent_FlushLine");
        stat_eventSent[(int)Command::FlushLineInv]     = registerStatistic<uint64_t>("eventSent_FlushLineInv");
        stat_eventSent[(int)Command::NACK]           = registerStatistic<uint64_t>("eventSent_NACK");
        stat_eventSent[(int)Command::GetSResp]         = registerStatistic<uint64_t>("eventSent_GetSResp");
        stat_eventSent[(int)Command::GetXResp]         = registerStatistic<uint64_t>("eventSent_GetXResp");
        stat_eventSent[(int)Command::FlushLineResp]    = registerStatistic<uint64_t>("eventSent_FlushLineResp");
        stat_eventSent[(int)Command::Put]           = registerStatistic<uint64_t>("eventSent_Put");
        stat_eventSent[(int)Command::Get]           = registerStatistic<uint64_t>("eventSent_Get");
        stat_eventSent[(int)Command::AckMove]       = registerStatistic<uint64_t>("eventSent_AckMove");
        stat_eventSent[(int)Command::CustomReq]     = registerStatistic<uint64_t>("eventSent_CustomReq");
        stat_eventSent[(int)Command::CustomResp]    = registerStatistic<uint64_t>("eventSent_CustomResp");
        stat_eventSent[(int)Command::CustomAck]     = registerStatistic<uint64_t>("eventSent_CustomAck");
        stat_latencyGetS[LatType::HIT] = registerStatistic<uint64_t>("latency_GetS_hit");
        stat_latencyGetS[LatType::MISS] = registerStatistic<uint64_t>("latency_GetS_miss");
        stat_latencyGetX[LatType::HIT] = registerStatistic<uint64_t>("latency_GetX_hit");
        stat_latencyGetX[LatType::MISS] = registerStatistic<uint64_t>("latency_GetX_miss");
        stat_latencyGetSX[LatType::HIT] = registerStatistic<uint64_t>("latency_GetSX_hit");
        stat_latencyGetSX[LatType::MISS] = registerStatistic<uint64_t>("latency_GetSX_miss");
        stat_latencyFlushLine = registerStatistic<uint64_t>("latency_FlushLine");
        stat_latencyFlushLineInv = registerStatistic<uint64_t>("latency_FlushLineInv");
        
        if (prefetch) {
            statPrefetchEvict = registerStatistic<uint64_t>("prefetch_evict");
            statPrefetchHit = registerStatistic<uint64_t>("prefetch_useful");
            statPrefetchRedundant = registerStatistic<uint64_t>("prefetch_redundant");
        }
    }

    ~IncoherentController() {}
    
 
/*----------------------------------------------------------------------------------------------------------------------
 *  Public functions form external interface to the coherence controller
 *---------------------------------------------------------------------------------------------------------------------*/    

/* Event handlers */
    /* Public event handlers called by cache controller */
    /** Send cache line data to the lower level caches */
    CacheAction handleEviction(CacheLine* _wbCacheLine, string _origRqstr, bool ignoredParameter=false);

    /** Process cache request:  GetX, GetS, GetSX */
    CacheAction handleRequest(MemEvent* event, CacheLine* cacheLine, bool replay);
    
    /** Process replacement request - PutS, PutE, PutM */
    CacheAction handleReplacement(MemEvent* event, CacheLine* cacheLine, MemEvent * reqEvent, bool replay);
    
    /** Process invalidation requests - Inv, FetchInv, FetchInvX */
    CacheAction handleInvalidationRequest(MemEvent *event, bool inMSHR);

    /** Process responses - GetSResp, GetXResp, FetchResp */
    CacheAction handleCacheResponse(MemEvent* event, bool inMSHR);
    CacheAction handleFetchResponse(MemEvent* event, bool inMSHR);
   
    bool handleNACK(MemEvent* event, bool inMSHR);

/* Miscellaneous */

    /** Determine in advance if a request will miss (and what kind of miss). Used for stats */
    bool isCacheHit(MemEvent* event);
    
    void addToOutgoingQueue(Response& resp);
    void addToOutgoingQueueUp(Response& resp);

    void setCacheArray(CacheArray* ptr) { cacheArray_ = ptr; }

private:
/* Private data members */
    bool        inclusive_;
    CacheArray* cacheArray_;

/* Statistics */
    std::array<std::array<Statistic<uint64_t>*, LAST_STATE>, (int)Command::LAST_CMD> stat_eventState;
    Statistic<uint64_t>* stat_eventSent[(int)Command::LAST_CMD];
    Statistic<uint64_t>* stat_latencyGetS[2];
    Statistic<uint64_t>* stat_latencyGetX[2];
    Statistic<uint64_t>* stat_latencyGetSX[2];
    Statistic<uint64_t>* stat_latencyFlushLine;
    Statistic<uint64_t>* stat_latencyFlushLineInv;
    
/* Private event handlers */
    /** Handle GetX request. Request upgrade if needed */
    CacheAction handleGetXRequest(MemEvent* event, CacheLine* _cacheLine, bool replay);

    /** Handle GetS request. Request block if needed */
    CacheAction handleGetSRequest(MemEvent* event, CacheLine* cacheLine, bool replay);
    
    /** Handle PutM request. Write data to cache line.  Update E->M state if necessary */
    CacheAction handlePutMRequest(MemEvent* event, CacheLine* cacheLine);
    
    /** Handle FlushLine request. Forward if needed */
    CacheAction handleFlushLineRequest(MemEvent * event, CacheLine * cacheLine, MemEvent * reqEvent, bool replay);

    /** Handle FlushLineInv request. Forward if needed */
    CacheAction handleFlushLineInvRequest(MemEvent * event, CacheLine * cacheLine, MemEvent * reqEvent, bool replay);

    /** Process GetSResp/GetXResp.  Update the cache line */
    CacheAction handleDataResponse(MemEvent* responseEvent, CacheLine * cacheLine, MemEvent * reqEvent);
    
/* Private methods for sending events */
    /** Send writeback request to lower level caches */
    void sendWriteback(Command cmd, CacheLine* cacheLine, string origRqstr);
    
    /** Send a flush response */
    void sendFlushResponse(MemEvent * reqEent, bool success);
    
    /** Forward a FlushLine request with or without data */
    void forwardFlushLine(Addr baseAddr, string origRqstr, CacheLine * cacheLine, Command cmd);

/* Helper methods */
   
    void printData(vector<uint8_t> * data, bool set);

/* Statistics */
    void recordStateEventCount(Command cmd, State state);
    void recordEventSentDown(Command cmd);
    void recordEventSentUp(Command cmd);
    void recordLatency(Command cmd, int type, uint64_t latency);
};


}}


#endif

