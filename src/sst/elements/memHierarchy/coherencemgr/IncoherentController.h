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

#ifndef INCOHERENTCONTROLLER_H
#define INCOHERENTCONTROLLER_H

#include <iostream>

#include <sst/core/elementinfo.h>

#include "sst/elements/memHierarchy/coherencemgr/coherenceController.h"


namespace SST { namespace MemHierarchy {

class IncoherentController : public CoherenceController{
public:
    SST_ELI_REGISTER_SUBCOMPONENT(IncoherentController, "memHierarchy", "IncoherentController", SST_ELI_ELEMENT_VERSION(1,0,0), 
            "Implements an second level or greater cache without coherence", "SST::MemHierarchy::CoherenceController")

    SST_ELI_DOCUMENT_STATISTICS(
        /* Event sends */
        {"eventSent_GetS",          "Number of GetS requests sent", "events", 2},
        {"eventSent_GetX",          "Number of GetX requests sent", "events", 2},
        {"eventSent_GetSX",         "Number of GetSX requests sent", "events", 2},
        {"eventSent_GetSResp",      "Number of GetSResp responses sent", "events", 2},
        {"eventSent_GetXResp",      "Number of GetXResp responses sent", "events", 2},
        {"eventSent_PutE",          "Number of PutE requests sent", "events", 2},
        {"eventSent_PutM",          "Number of PutM requests sent", "events", 2},
        {"eventSent_NACK_up",       "Number of NACKs sent up (towards CPU)", "events", 2},
        {"eventSent_NACK_down",     "Number of NACKs sent down (towards main memory)", "events", 2},
        {"eventSent_FlushLine",     "Number of FlushLine requests sent", "events", 2},
        {"eventSent_FlushLineInv",  "Number of FlushLineInv requests sent", "events", 2},
        {"eventSent_FlushLineResp", "Number of FlushLineResp responses sent", "events", 2},
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
    /** Constructor for IncoherentController. */
    IncoherentController(SST::Component* comp, Params& params) : CoherenceController (comp, params) {
        debug->debug(_INFO_,"--------------------------- Initializing [Incoherent Controller] ... \n\n");
        inclusive_ = params.find<bool>("inclusive", true);
    
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
        stat_stateEvent_GetXResp_IS = registerStatistic<uint64_t>("stateEvent_GetXResp_IS");
        stat_stateEvent_GetXResp_IM = registerStatistic<uint64_t>("stateEvent_GetXResp_IM");
        stat_stateEvent_PutE_I = registerStatistic<uint64_t>("stateEvent_PutE_I");
        stat_stateEvent_PutE_E = registerStatistic<uint64_t>("stateEvent_PutE_E");
        stat_stateEvent_PutE_M = registerStatistic<uint64_t>("stateEvent_PutE_M");
        stat_stateEvent_PutE_IS = registerStatistic<uint64_t>("stateEvent_PutE_IS");
        stat_stateEvent_PutE_IM = registerStatistic<uint64_t>("stateEvent_PutE_IM");
        stat_stateEvent_PutE_IB = registerStatistic<uint64_t>("stateEvent_PutE_IB");
        stat_stateEvent_PutE_SB = registerStatistic<uint64_t>("stateEvent_PutE_SB");
        stat_stateEvent_PutM_I = registerStatistic<uint64_t>("stateEvent_PutM_I");
        stat_stateEvent_PutM_E = registerStatistic<uint64_t>("stateEvent_PutM_E");
        stat_stateEvent_PutM_M = registerStatistic<uint64_t>("stateEvent_PutM_M");
        stat_stateEvent_PutM_IS = registerStatistic<uint64_t>("stateEvent_PutM_IS");
        stat_stateEvent_PutM_IM = registerStatistic<uint64_t>("stateEvent_PutM_IM");
        stat_stateEvent_PutM_IB = registerStatistic<uint64_t>("stateEvent_PutM_IB");
        stat_stateEvent_PutM_SB = registerStatistic<uint64_t>("stateEvent_PutM_SB");
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
        stat_eventSent_GetS             = registerStatistic<uint64_t>("eventSent_GetS");
        stat_eventSent_GetX             = registerStatistic<uint64_t>("eventSent_GetX");
        stat_eventSent_GetSX           = registerStatistic<uint64_t>("eventSent_GetSX");
        stat_eventSent_PutE             = registerStatistic<uint64_t>("eventSent_PutE");
        stat_eventSent_PutM             = registerStatistic<uint64_t>("eventSent_PutM");
        stat_eventSent_FlushLine        = registerStatistic<uint64_t>("eventSent_FlushLine");
        stat_eventSent_FlushLineInv     = registerStatistic<uint64_t>("eventSent_FlushLineInv");
        stat_eventSent_NACK_down        = registerStatistic<uint64_t>("eventSent_NACK_down");
        stat_eventSent_GetSResp         = registerStatistic<uint64_t>("eventSent_GetSResp");
        stat_eventSent_GetXResp         = registerStatistic<uint64_t>("eventSent_GetXResp");
        stat_eventSent_FlushLineResp    = registerStatistic<uint64_t>("eventSent_FlushLineResp");
        stat_eventSent_NACK_up          = registerStatistic<uint64_t>("eventSent_NACK_up");
        
        if (!params.find<std::string>("prefetcher", "").empty()) {
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
    CacheAction handleInvalidationRequest(MemEvent *event, CacheLine* cacheLine, MemEvent * collisonEvent, bool replay);

    /** Process responses - GetSResp, GetXResp, FetchResp */
    CacheAction handleResponse(MemEvent* _responseEvent, CacheLine* cacheLine, MemEvent* _origRequest);

/* Miscellaneous */

    /** Determine in advance if a request will miss (and what kind of miss). Used for stats */
    int isCoherenceMiss(MemEvent* event, CacheLine* cacheLine);
    
    /** Determine whether a retry of a NACKed event is needed */
    bool isRetryNeeded(MemEvent* event, CacheLine* cacheLine);
   
    /** Message send: Call through to coherenceController with statistic recording */
    void addToOutgoingQueue(Response& resp);
    void addToOutgoingQueueUp(Response& resp);

private:
/* Private data members */
    bool                inclusive_;
    
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
    Statistic<uint64_t>* stat_stateEvent_GetXResp_IS;
    Statistic<uint64_t>* stat_stateEvent_GetXResp_IM;
    Statistic<uint64_t>* stat_stateEvent_PutE_I;
    Statistic<uint64_t>* stat_stateEvent_PutE_E;
    Statistic<uint64_t>* stat_stateEvent_PutE_M;
    Statistic<uint64_t>* stat_stateEvent_PutE_IS;
    Statistic<uint64_t>* stat_stateEvent_PutE_IM;
    Statistic<uint64_t>* stat_stateEvent_PutE_IB;
    Statistic<uint64_t>* stat_stateEvent_PutE_SB;
    Statistic<uint64_t>* stat_stateEvent_PutM_I;
    Statistic<uint64_t>* stat_stateEvent_PutM_E;
    Statistic<uint64_t>* stat_stateEvent_PutM_M;
    Statistic<uint64_t>* stat_stateEvent_PutM_IS;
    Statistic<uint64_t>* stat_stateEvent_PutM_IM;
    Statistic<uint64_t>* stat_stateEvent_PutM_IB;
    Statistic<uint64_t>* stat_stateEvent_PutM_SB;
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
    Statistic<uint64_t>* stat_eventSent_FlushLine;
    Statistic<uint64_t>* stat_eventSent_FlushLineInv;
    Statistic<uint64_t>* stat_eventSent_NACK_down;
    Statistic<uint64_t>* stat_eventSent_GetSResp;
    Statistic<uint64_t>* stat_eventSent_GetXResp;
    Statistic<uint64_t>* stat_eventSent_FlushLineResp;
    Statistic<uint64_t>* stat_eventSent_NACK_up;
    
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
};


}}


#endif

