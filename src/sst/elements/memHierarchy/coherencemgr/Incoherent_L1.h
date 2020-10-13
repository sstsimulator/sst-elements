// Copyright 2009-2020 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2020, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef MEMHIERARCHY_L1INCOHERENTCONTROLLER_H
#define MEMHIERARCHY_L1INCOHERENTCONTROLLER_H

#include <iostream>
#include <array>

#include "sst/elements/memHierarchy/coherencemgr/coherenceController.h"
#include "sst/elements/memHierarchy/lineTypes.h"
#include "sst/elements/memHierarchy/cacheArray.h"

namespace SST { namespace MemHierarchy {

class IncoherentL1 : public CoherenceController {
public:
/* Element Library Info */
    SST_ELI_REGISTER_SUBCOMPONENT_DERIVED(IncoherentL1, "memHierarchy", "coherence.incoherent_l1", SST_ELI_ELEMENT_VERSION(1,0,0),
            "Implements an L1 cache without coherence", SST::MemHierarchy::CoherenceController)

    SST_ELI_DOCUMENT_STATISTICS(
        /* Event hits & misses */
        {"CacheHits",               "Acceses that hit in the cache", "accesses", 1},
        {"CacheMisses",             "Acceses that missed in the cache", "accesses", 1},
        {"GetSHit_Arrival",         "GetS was handled at arrival and was a cache hit", "count", 1},
        {"GetXHit_Arrival",         "GetX was handled at arrival and was a cache hit", "count", 1},
        {"GetSXHit_Arrival",        "GetSX was handled at arrival and was a cache hit", "count", 1},
        {"GetSMiss_Arrival",        "GetS was handled at arrival and was a cache miss", "count", 1},
        {"GetXMiss_Arrival",        "GetX was handled at arrival and was a cache miss", "count", 1},
        {"GetSXMiss_Arrival",       "GetSX was handled at arrival and was a cache miss", "count", 1},
        {"GetSHit_Blocked",         "GetS was blocked in MSHR at arrival and later was a cache hit", "count", 1},
        {"GetXHit_Blocked",         "GetX was blocked in MSHR at arrival and later was a cache hit", "count", 1},
        {"GetSXHit_Blocked",        "GetSX was blocked in MSHR at arrival and later was a cache hit", "count", 1},
        {"GetSMiss_Blocked",        "GetS was blocked in MSHR at arrival and later was a cache miss", "count", 1},
        {"GetXMiss_Blocked",        "GetX was blocked in MSHR at arrival and later was a cache miss", "count", 1},
        {"GetSXMiss_Blocked",       "GetSX was blocked in MSHR at arrival and later was a cache miss", "count", 1},
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

    SST_ELI_DOCUMENT_SUBCOMPONENT_SLOTS(
            {"replacement", "Replacement policies, slot 0 is for cache, slot 1 is for directory (if it exists)", "SST::MemHierarchy::ReplacementPolicy"},
            {"hash", "Hash function for mapping addresses to cache lines", "SST::MemHierarchy::HashFunction"} )


/* Begin class definition */
    /** Constructor for IncoherentL1 */
    IncoherentL1(ComponentId_t id, Params& params, Params& ownerParams, bool prefetch) : CoherenceController(id, params, ownerParams, prefetch) {
        params.insert(ownerParams);
        debug->debug(_INFO_,"--------------------------- Initializing [L1Controller] ... \n\n");

        // Cache Array
        uint64_t lines = params.find<uint64_t>("lines");
        uint64_t assoc = params.find<uint64_t>("associativity");
        ReplacementPolicy * rmgr = createReplacementPolicy(lines, assoc, params, true);
        HashFunction * ht = createHashFunction(params);

        cacheArray_ = new CacheArray<L1CacheLine>(debug, lines, assoc, lineSize_, rmgr, ht);
        cacheArray_->setBanked(params.find<uint64_t>("banks", 0));

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
        stat_hit[0][0] = registerStatistic<uint64_t>("GetSHit_Arrival");
        stat_hit[1][0] = registerStatistic<uint64_t>("GetXHit_Arrival");
        stat_hit[2][0] = registerStatistic<uint64_t>("GetSXHit_Arrival");
        stat_hit[0][1] = registerStatistic<uint64_t>("GetSHit_Blocked");
        stat_hit[1][1] = registerStatistic<uint64_t>("GetXHit_Blocked");
        stat_hit[2][1] = registerStatistic<uint64_t>("GetSXHit_Blocked");
        stat_miss[0][0] = registerStatistic<uint64_t>("GetSMiss_Arrival");
        stat_miss[1][0] = registerStatistic<uint64_t>("GetXMiss_Arrival");
        stat_miss[2][0] = registerStatistic<uint64_t>("GetSXMiss_Arrival");
        stat_miss[0][1] = registerStatistic<uint64_t>("GetSMiss_Blocked");
        stat_miss[1][1] = registerStatistic<uint64_t>("GetXMiss_Blocked");
        stat_miss[2][1] = registerStatistic<uint64_t>("GetSXMiss_Blocked");
        stat_hits = registerStatistic<uint64_t>("CacheHits");
        stat_misses = registerStatistic<uint64_t>("CacheMisses");

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

    bool handleGetS(MemEvent * event, bool inMSHR);
    bool handleGetX(MemEvent * event, bool inMSHR);
    bool handleGetSX(MemEvent * event, bool inMSHR);
    bool handleFlushLine(MemEvent * event, bool inMSHR);
    bool handleFlushLineInv(MemEvent * event, bool inMSHR);
    bool handleNULLCMD(MemEvent * event, bool inMSHR);
    bool handleGetSResp(MemEvent * event, bool inMSHR);
    bool handleGetXResp(MemEvent * event, bool inMSHR);
    bool handleFlushLineResp(MemEvent * event, bool inMSHR);
    bool handleNACK(MemEvent * event, bool inMSHR);

    virtual Addr getBank(Addr addr) { return cacheArray_->getBank(addr); }
    virtual void setSliceAware(uint64_t size, uint64_t step) { cacheArray_->setSliceAware(size, step); }

    MemEventInitCoherence * getInitCoherenceEvent();

    std::set<Command> getValidReceiveEvents() {
        std::set<Command> cmds = { Command::GetS,
            Command::GetX,
            Command::GetSX,
            Command::FlushLine,
            Command::FlushLineInv,
            Command::NULLCMD,
            Command::GetSResp,
            Command::GetXResp,
            Command::FlushLineResp,
            Command::NACK };
        return cmds;
    }

private:

    MemEventStatus processCacheMiss(MemEvent * event, L1CacheLine * line, bool inMSHR);
    MemEventStatus allocateMSHR(MemEvent * event, bool fwdReq, int pos = -1);
    L1CacheLine * allocateLine(MemEvent * event, L1CacheLine * line);
    bool handleEviction (Addr addr, L1CacheLine*& line);
    void cleanUpAfterRequest(MemEvent * event, bool inMSHR);
    void cleanUpAfterResponse(MemEvent * event, bool inMSHR);
    void retry(Addr addr);

    /** Forward a flush line request, with or without data */
    void forwardFlush(MemEvent * event, L1CacheLine * line, bool data);

    /** Send response up (to processor) */
    uint64_t sendResponseUp(MemEvent * event, vector<uint8_t>* data, bool inMSHR, uint64_t baseTime, bool success = false);

    /** Send response down (towards memory) */
    void sendResponseDown(MemEvent * event, L1CacheLine * line, bool data);

    /** Send writeback request to lower level caches */
    void sendWriteback(Command cmd, L1CacheLine * line, bool dirty);

    /** Call through to coherenceController with statistic recording */
    void addToOutgoingQueue(Response& resp);
    void addToOutgoingQueueUp(Response& resp);

/* Miscellaneous */

    /* Statistics recording */
    void recordPrefetchResult(L1CacheLine * line, Statistic<uint64_t> * stat);
    void recordLatency(Command cmd, int type, uint64_t timestamp);
    void eventProfileAndNotify(MemEvent * event, State state, NotifyAccessType type, NotifyResultType result, bool inMSHR, bool stalled);

    /* Debug output */
    void printData(vector<uint8_t> * data, bool set);
    void printLine(Addr addr);

    CacheArray<L1CacheLine>* cacheArray_;

    /* Statistics */
    Statistic<uint64_t>* stat_latencyGetS[2];
    Statistic<uint64_t>* stat_latencyGetX[2];
    Statistic<uint64_t>* stat_latencyGetSX[2];
    Statistic<uint64_t>* stat_latencyFlushLine[2];
    Statistic<uint64_t>* stat_latencyFlushLineInv[2];
    Statistic<uint64_t>* stat_hit[3][2];
    Statistic<uint64_t>* stat_miss[3][2];
    Statistic<uint64_t>* stat_hits;
    Statistic<uint64_t>* stat_misses;

};


}}


#endif

