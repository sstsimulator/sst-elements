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

#ifndef INCOHERENTCONTROLLER_H
#define INCOHERENTCONTROLLER_H

#include <iostream>
#include <array>

#include "sst/elements/memHierarchy/coherencemgr/coherenceController.h"
#include "sst/elements/memHierarchy/cacheArray.h"

namespace SST { namespace MemHierarchy {

class Incoherent : public CoherenceController{
public:
    SST_ELI_REGISTER_SUBCOMPONENT_DERIVED(Incoherent, "memHierarchy", "coherence.incoherent", SST_ELI_ELEMENT_VERSION(1,0,0),
            "Implements an second level or greater cache without coherence", SST::MemHierarchy::CoherenceController)

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

    SST_ELI_DOCUMENT_SUBCOMPONENT_SLOTS(
            {"replacement", "Replacement policies, slot 0 is for cache, slot 1 is for directory (if it exists)", "SST::MemHierarchy::ReplacementPolicy"},
            {"hash", "Hash function for mapping addresses to cache lines", "SST::MemHierarchy::HashFunction"} )
/* Class definition */
    /** Constructor for Incoherent. */
    Incoherent(SST::ComponentId_t id, Params& params, Params& ownerParams, bool prefetch) : CoherenceController(id, params, ownerParams, prefetch) {
        params.insert(ownerParams);
        debug->debug(_INFO_,"--------------------------- Initializing [Incoherent Controller] ... \n\n");

        // Cache Array
        uint64_t lines = params.find<uint64_t>("lines");
        uint64_t assoc = params.find<uint64_t>("associativity");
        ReplacementPolicy * rmgr = createReplacementPolicy(lines, assoc, params, true);
        HashFunction * ht = createHashFunction(params);

        cacheArray_ = new CacheArray<PrivateCacheLine>(debug, lines, assoc, lineSize_, rmgr, ht);
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

        if (prefetch) {
            statPrefetchEvict = registerStatistic<uint64_t>("prefetch_evict");
            statPrefetchHit = registerStatistic<uint64_t>("prefetch_useful");
            statPrefetchRedundant = registerStatistic<uint64_t>("prefetch_redundant");
        }
    }

    ~Incoherent() {}

    /** Event handlers */
    virtual bool handleGetS(MemEvent * event, bool inMSHR);
    virtual bool handleGetX(MemEvent * event, bool inMSHR);
    virtual bool handleGetSX(MemEvent * event, bool inMSHR);
    virtual bool handleFlushLine(MemEvent * event, bool inMSHR);
    virtual bool handleFlushLineInv(MemEvent * event, bool inMSHR);
    virtual bool handlePutE(MemEvent * event, bool inMSHR);
    virtual bool handlePutM(MemEvent * event, bool inMSHR);
    virtual bool handleGetSResp(MemEvent * event, bool inMSHR);
    virtual bool handleGetXResp(MemEvent * event, bool inMSHR);
    virtual bool handleFlushLineResp(MemEvent * event, bool inMSHR);
    virtual bool handleNULLCMD(MemEvent * event, bool inMSHR);
    virtual bool handleNACK(MemEvent * event, bool inMSHR);

    Addr getBank(Addr addr) { return cacheArray_->getBank(addr); }
    void setSliceAware(uint64_t interleaveSize, uint64_t interleaveStep) { cacheArray_->setSliceAware(interleaveSize, interleaveStep); }

    MemEventInitCoherence * getInitCoherenceEvent();

    void recordLatency(Command cmd, int type, uint64_t latency);

    virtual std::set<Command> getValidReceiveEvents() {
        std::set<Command> cmds = { Command::GetS,
            Command::GetX,
            Command::GetSX,
            Command::FlushLine,
            Command::FlushLineInv,
            Command::GetSResp,
            Command::GetXResp,
            Command::FlushLineResp,
            Command::PutE,
            Command::PutM,
            Command::NULLCMD,
            Command::NACK };
        return cmds;
    }

private:

    MemEventStatus processCacheMiss(MemEvent * event, PrivateCacheLine* &line, bool inMSHR);

    MemEventStatus allocateLine(MemEvent * event, PrivateCacheLine* &line, bool inMSHR);

    bool handleEviction(Addr addr, PrivateCacheLine* &line, dbgin &diStruct);

    void cleanUpAfterRequest(MemEvent * event, bool inMSHR);

    void cleanUpAfterResponse(MemEvent * event);

    void retry(Addr addr);

    void doEvict(MemEvent * event, PrivateCacheLine * line);

    SimTime_t sendResponseUp(MemEvent * event, vector<uint8_t> * data, bool inMSHR, SimTime_t time, Command cmd = Command::NULLCMD, bool success = false);

    void sendWriteback(Command cmd, PrivateCacheLine * line, bool dirty);

    void forwardFlush(MemEvent * event, bool evict, std::vector<uint8_t> * data, bool dirty, uint64_t time);

    void sendWritebackAck(MemEvent * event);

    void addToOutgoingQueue(Response& resp);
    void addToOutgoingQueueUp(Response& resp);

    void recordPrefetchResult(PrivateCacheLine * line, Statistic<uint64_t> * stat);

    void printLine(Addr addr);

/* Data members */

    CacheArray<PrivateCacheLine>* cacheArray_;

/* Statistics */
    Statistic<uint64_t>* stat_latencyGetS[2];
    Statistic<uint64_t>* stat_latencyGetX[2];
    Statistic<uint64_t>* stat_latencyGetSX[2];
    Statistic<uint64_t>* stat_latencyFlushLine;
    Statistic<uint64_t>* stat_latencyFlushLineInv;
    Statistic<uint64_t>* stat_hit[3][2];
    Statistic<uint64_t>* stat_miss[3][2];
    Statistic<uint64_t>* stat_hits;
    Statistic<uint64_t>* stat_misses;
};


}}


#endif

