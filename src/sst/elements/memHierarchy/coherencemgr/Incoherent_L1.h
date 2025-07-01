// Copyright 2009-2025 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2025, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// of the distribution for more information.
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
    SST_ELI_REGISTER_SUBCOMPONENT(IncoherentL1, "memHierarchy", "coherence.incoherent_l1", SST_ELI_ELEMENT_VERSION(1,0,0),
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
        {"eventSent_Write",         "Number of Write requests sent", "events", 2},
        {"eventSent_GetSResp",      "Number of GetSResp responses sent", "events", 2},
        {"eventSent_GetXResp",      "Number of GetXResp responses sent", "events", 2},
        {"eventSent_WriteResp",     "Number of WriteResp responses sent", "events", 2},
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
        {"stateEvent_GetSResp_IM",      "Event/State: Number of times a GetSResp was seen in state IM", "count", 3},
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
    IncoherentL1(ComponentId_t id, Params& params, Params& owner_params, bool prefetch);
    IncoherentL1() = default;
    ~IncoherentL1() = default;

    bool handleGetS(MemEvent * event, bool in_mshr) override;
    bool handleGetX(MemEvent * event, bool in_mshr) override;
    bool handleGetSX(MemEvent * event, bool in_mshr) override;
    bool handleWrite(MemEvent * event, bool in_mshr) override;
    bool handleFlushLine(MemEvent * event, bool in_mshr) override;
    bool handleFlushLineInv(MemEvent * event, bool in_mshr) override;
    bool handleNULLCMD(MemEvent * event, bool in_mshr) override;
    bool handleGetSResp(MemEvent * event, bool in_mshr) override;
    bool handleGetXResp(MemEvent * event, bool in_mshr) override;
    bool handleFlushLineResp(MemEvent * event, bool in_mshr) override;
    bool handleNACK(MemEvent * event, bool in_mshr) override;

    Addr getBank(Addr addr) override { return cache_array_->getBank(addr); }
    void setSliceAware(uint64_t size, uint64_t step) override { cache_array_->setSliceAware(size, step); }

    MemEventInitCoherence * getInitCoherenceEvent() override;
    std::set<Command> getValidReceiveEvents() override;

    /** Serialization */
    void serialize_order(SST::Core::Serialization::serializer& ser) override;
    ImplementSerializable(SST::MemHierarchy::IncoherentL1)

private:

    MemEventStatus processCacheMiss(MemEvent * event, L1CacheLine * line, bool in_mshr);
    MemEventStatus allocateMSHR(MemEvent * event, bool fwdReq, int pos = -1);
    L1CacheLine * allocateLine(MemEvent * event, L1CacheLine * line);
    bool handleEviction (Addr addr, L1CacheLine*& line);
    void cleanUpAfterRequest(MemEvent * event, bool in_mshr);
    void cleanUpAfterResponse(MemEvent * event, bool in_mshr);
    void retry(Addr addr);

    /** Forward a flush line request, with or without data */
    void forwardFlush(MemEvent * event, L1CacheLine * line, bool data);

    /** Send response up (to processor) */
    uint64_t sendResponseUp(MemEvent * event, vector<uint8_t>* data, bool in_mshr, uint64_t base_time, bool success = true) override;

    /** Send response down (towards memory) */
    void sendResponseDown(MemEvent * event, L1CacheLine * line, bool data);

    /** Send writeback request to lower level caches */
    void sendWriteback(Command cmd, L1CacheLine * line, bool dirty);

    /** Call through to coherenceController with statistic recording */
    void forwardByAddress(MemEventBase* ev, Cycle_t timestamp) override;
    void forwardByDestination(MemEventBase* ev, Cycle_t timestamp) override;

/* Miscellaneous */

    /* Statistics recording */
    void recordPrefetchResult(L1CacheLine * line, Statistic<uint64_t> * stat);
    void recordLatency(Command cmd, int type, uint64_t timestamp) override;

    /* Debug output */
    void printLine(Addr addr);

    CacheArray<L1CacheLine>* cache_array_ = nullptr;

    Cycle_t llsc_block_cycles_;

    /* Statistics */
    Statistic<uint64_t>* stat_latency_GetS_[2]  = {nullptr, nullptr};
    Statistic<uint64_t>* stat_latency_GetX_[2]  = {nullptr, nullptr};
    Statistic<uint64_t>* stat_latency_GetSX_[2] = {nullptr, nullptr};
    Statistic<uint64_t>* stat_latency_FlushLine_[2] = {nullptr, nullptr};
    Statistic<uint64_t>* stat_latency_FlushLineInv_[2] = {nullptr, nullptr};
    Statistic<uint64_t>* stat_hit_[3][2]  = { {nullptr, nullptr}, {nullptr, nullptr}, {nullptr, nullptr} };
    Statistic<uint64_t>* stat_miss_[3][2] = { {nullptr, nullptr}, {nullptr, nullptr}, {nullptr, nullptr} };
    Statistic<uint64_t>* stat_hits_   = nullptr;
    Statistic<uint64_t>* stat_misses_ = nullptr;
};


}}


#endif

