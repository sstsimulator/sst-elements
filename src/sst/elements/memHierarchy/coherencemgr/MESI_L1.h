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

#ifndef MEMHIERARCHY_L1COHERENCECONTROLLER_H
#define MEMHIERARCHY_L1COHERENCECONTROLLER_H

#include <iostream>
#include <array>

#include "coherencemgr/coherenceController.h"
#include "sst/elements/memHierarchy/memTypes.h"
#include "sst/elements/memHierarchy/lineTypes.h"
#include "sst/elements/memHierarchy/cacheArray.h"


namespace SST { namespace MemHierarchy {

class MESIL1 : public CoherenceController {
public:
/* Element Library Info */
    SST_ELI_REGISTER_SUBCOMPONENT(MESIL1, "memHierarchy", "coherence.mesi_l1", SST_ELI_ELEMENT_VERSION(1,0,0),
            "Implements MESI or MSI coherence for an L1 cache", SST::MemHierarchy::CoherenceController)

    SST_ELI_DOCUMENT_STATISTICS(
        /* Event hits & misses */
        {"CacheHits",               "Accesses that hit in the cache", "accesses", 1},
        {"CacheMisses",             "Accesses that missed in the cache", "accesses", 1},
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
        {"eventSent_PutS",          "Number of PutS requests sent", "events", 2},
        {"eventSent_PutE",          "Number of PutE requests sent", "events", 2},
        {"eventSent_PutM",          "Number of PutM requests sent", "events", 2},
        {"eventSent_FetchResp",     "Number of FetchResp requests sent", "events", 2},
        {"eventSent_FetchXResp",    "Number of FetchXResp requests sent", "events", 2},
        {"eventSent_AckInv",        "Number of AckInvs sent", "events", 2},
        {"eventSent_NACK",          "Number of NACKs sent ", "events", 2},
        {"eventSent_FlushLine",     "Number of FlushLine requests sent", "events", 2},
        {"eventSent_FlushLineInv",  "Number of FlushLineInv requests sent", "events", 2},
        {"eventSent_FlushAll",      "Number of FlushAll requests sent", "events", 2},
        {"eventSent_FlushLineResp", "Number of FlushLineResp responses sent", "events", 2},
        {"eventSent_FlushAllResp",  "Number of FlushAllResp responses sent", "events", 2},
        {"eventSent_AckFlush",      "Number of AckFlush responses sent", "events", 2},
        {"eventSent_Put",           "Number of Put requests sent", "events", 6},
        {"eventSent_Get",           "Number of Get requests sent", "events", 6},
        {"eventSent_AckMove",       "Number of AckMove responses sent", "events", 6},
        {"eventSent_CustomReq",     "Number of CustomReq requests sent", "events", 4},
        {"eventSent_CustomResp",    "Number of CustomResp responses sent", "events", 4},
        {"eventSent_CustomAck",     "Number of CustomAck responses sent", "events", 4},
        /* Event/State combinations - Count how many times an event was seen in particular state */
        {"stateEvent_GetS_I",           "Event/State: Number of times a GetS was seen in state I (Miss)", "count", 3},
        {"stateEvent_GetS_S",           "Event/State: Number of times a GetS was seen in state S (Hit)", "count", 3},
        {"stateEvent_GetS_E",           "Event/State: Number of times a GetS was seen in state E (Hit)", "count", 3},
        {"stateEvent_GetS_M",           "Event/State: Number of times a GetS was seen in state M (Hit)", "count", 3},
        {"stateEvent_GetX_I",           "Event/State: Number of times a GetX was seen in state I (Miss)", "count", 3},
        {"stateEvent_GetX_S",           "Event/State: Number of times a GetX was seen in state S (Miss)", "count", 3},
        {"stateEvent_GetX_E",           "Event/State: Number of times a GetX was seen in state E (Hit)", "count", 3},
        {"stateEvent_GetX_M",           "Event/State: Number of times a GetX was seen in state M (Hit)", "count", 3},
        {"stateEvent_GetSX_I",          "Event/State: Number of times a GetSX was seen in state I (Miss)", "count", 3},
        {"stateEvent_GetSX_S",          "Event/State: Number of times a GetSX was seen in state S (Miss)", "count", 3},
        {"stateEvent_GetSX_E",          "Event/State: Number of times a GetSX was seen in state E (Hit)", "count", 3},
        {"stateEvent_GetSX_M",          "Event/State: Number of times a GetSX was seen in state M (Hit)", "count", 3},
        {"stateEvent_GetSResp_IS",      "Event/State: Number of times a GetSResp was seen in state IS", "count", 3},
        {"stateEvent_GetXResp_IS",      "Event/State: Number of times a GetXResp was seen in state IS", "count", 3},
        {"stateEvent_GetXResp_IM",      "Event/State: Number of times a GetXResp was seen in state IM", "count", 3},
        {"stateEvent_GetXResp_SM",      "Event/State: Number of times a GetXResp was seen in state SM", "count", 3},
        {"stateEvent_Inv_I",            "Event/State: Number of times an Inv was seen in state I", "count", 3},
        {"stateEvent_Inv_IS",           "Event/State: Number of times an Inv was seen in state IS", "count", 3},
        {"stateEvent_Inv_IM",           "Event/State: Number of times an Inv was seen in state IM", "count", 3},
        {"stateEvent_Inv_IB",           "Event/State: Number of times an Inv was seen in state I_B", "count", 3},
        {"stateEvent_Inv_S",            "Event/State: Number of times an Inv was seen in state S", "count", 3},
        {"stateEvent_Inv_SM",           "Event/State: Number of times an Inv was seen in state SM", "count", 3},
        {"stateEvent_Inv_SB",           "Event/State: Number of times an Inv was seen in state S_B", "count", 3},
        {"stateEvent_FetchInv_I",       "Event/State: Number of times a FetchInv was seen in state I", "count", 3},
        {"stateEvent_FetchInv_IS",      "Event/State: Number of times a FetchInv was seen in state IS", "count", 3},
        {"stateEvent_FetchInv_IM",      "Event/State: Number of times a FetchInv was seen in state IM", "count", 3},
        {"stateEvent_FetchInv_SM",      "Event/State: Number of times a FetchInv was seen in state SM", "count", 3},
        {"stateEvent_FetchInv_S",       "Event/State: Number of times a FetchInv was seen in state S", "count", 3},
        {"stateEvent_FetchInv_E",       "Event/State: Number of times a FetchInv was seen in state E", "count", 3},
        {"stateEvent_FetchInv_M",       "Event/State: Number of times a FetchInv was seen in state M", "count", 3},
        {"stateEvent_FetchInv_IB",      "Event/State: Number of times a FetchInv was seen in state I_B", "count", 3},
        {"stateEvent_FetchInv_SB",      "Event/State: Number of times a FetchInv was seen in state S_B", "count", 3},
        {"stateEvent_FetchInvX_I",      "Event/State: Number of times a FetchInvX was seen in state I", "count", 3},
        {"stateEvent_FetchInvX_IS",     "Event/State: Number of times a FetchInvX was seen in state IS", "count", 3},
        {"stateEvent_FetchInvX_IM",     "Event/State: Number of times a FetchInvX was seen in state IM", "count", 3},
        {"stateEvent_FetchInvX_E",      "Event/State: Number of times a FetchInvX was seen in state E", "count", 3},
        {"stateEvent_FetchInvX_M",      "Event/State: Number of times a FetchInvX was seen in state M", "count", 3},
        {"stateEvent_FetchInvX_IB",     "Event/State: Number of times a FetchInvX was seen in state I_B", "count", 3},
        {"stateEvent_FetchInvX_SB",     "Event/State: Number of times a FetchInvX was seen in state S_B", "count", 3},
        {"stateEvent_ForceInv_I",       "Event/State: Number of times a ForceInv was seen in state I", "count", 3},
        {"stateEvent_ForceInv_IS",      "Event/State: Number of times a ForceInv was seen in state IS", "count", 3},
        {"stateEvent_ForceInv_IM",      "Event/State: Number of times a ForceInv was seen in state IM", "count", 3},
        {"stateEvent_ForceInv_SM",      "Event/State: Number of times a ForceInv was seen in state SM", "count", 3},
        {"stateEvent_ForceInv_S",       "Event/State: Number of times a ForceInv was seen in state S", "count", 3},
        {"stateEvent_ForceInv_E",       "Event/State: Number of times a ForceInv was seen in state E", "count", 3},
        {"stateEvent_ForceInv_M",       "Event/State: Number of times a ForceInv was seen in state M", "count", 3},
        {"stateEvent_ForceInv_IB",      "Event/State: Number of times a ForceInv was seen in state I_B", "count", 3},
        {"stateEvent_ForceInv_SB",      "Event/State: Number of times a ForceInv was seen in state S_B", "count", 3},
        {"stateEvent_Fetch_I",          "Event/State: Number of times a Fetch was seen in state I", "count", 3},
        {"stateEvent_Fetch_IS",         "Event/State: Number of times a Fetch was seen in state IS", "count", 3},
        {"stateEvent_Fetch_IM",         "Event/State: Number of times a Fetch was seen in state IM", "count", 3},
        {"stateEvent_Fetch_S",          "Event/State: Number of times a Fetch was seen in state S", "count", 3},
        {"stateEvent_Fetch_SM",         "Event/State: Number of times a Fetch was seen in state SM", "count", 3},
        {"stateEvent_Fetch_IB",         "Event/State: Number of times a Fetch was seen in state I_B", "count", 3},
        {"stateEvent_Fetch_SB",         "Event/State: Number of times a Fetch was seen in state S_B", "count", 3},
        {"stateEvent_AckPut_I",         "Event/State: Number of times an AckPut was seen in state I", "count", 3},
        {"stateEvent_FlushLine_I",      "Event/State: Number of times a FlushLine was seen in state I", "count", 3},
        {"stateEvent_FlushLine_S",      "Event/State: Number of times a FlushLine was seen in state S", "count", 3},
        {"stateEvent_FlushLine_E",      "Event/State: Number of times a FlushLine was seen in state E", "count", 3},
        {"stateEvent_FlushLine_M",      "Event/State: Number of times a FlushLine was seen in state M", "count", 3},
        {"stateEvent_FlushLineInv_I",       "Event/State: Number of times a FlushLineInv was seen in state I", "count", 3},
        {"stateEvent_FlushLineInv_S",       "Event/State: Number of times a FlushLineInv was seen in state S", "count", 3},
        {"stateEvent_FlushLineInv_E",       "Event/State: Number of times a FlushLineInv was seen in state E", "count", 3},
        {"stateEvent_FlushLineInv_M",       "Event/State: Number of times a FlushLineInv was seen in state M", "count", 3},
        {"stateEvent_FlushLineResp_I",      "Event/State: Number of times a FlushLineResp was seen in state I", "count", 3},
        {"stateEvent_FlushLineResp_IB",     "Event/State: Number of times a FlushLineResp was seen in state I_B", "count", 3},
        {"stateEvent_FlushLineResp_SB",     "Event/State: Number of times a FlushLineResp was seen in state S_B", "count", 3},
        /* Eviction - count attempts to evict in a particular state */
        {"evict_I",                 "Eviction: Attempted to evict a block in state I", "count", 3},
        {"evict_S",                 "Eviction: Attempted to evict a block in state S", "count", 3},
        {"evict_E",                 "Eviction: Attempted to evict a block in state E", "count", 3},
        {"evict_M",                 "Eviction: Attempted to evict a block in state M", "count", 3},
        {"evict_IS",                "Eviction: Attempted to evict a block in state IS", "count", 3},
        {"evict_IM",                "Eviction: Attempted to evict a block in state IM", "count", 3},
        {"evict_SM",                "Eviction: Attempted to evict a block in state SM", "count", 3},
        {"evict_IB",                "Eviction: Attempted to evict a block in state S_B", "count", 3},
        {"evict_SB",                "Eviction: Attempted to evict a block in state I_B", "count", 3},
        /* Latency for different kinds of misses*/
        {"latency_GetS_miss",           "Latency for read misses", "cycles", 1},
        {"latency_GetS_hit",            "Latency for read hits", "cycles", 1},
        {"latency_GetX_miss",           "Latency for write misses (block not present)", "cycles", 1},
        {"latency_GetX_upgrade",        "Latency for write misses, block present but in Shared state", "cycles", 1},
        {"latency_GetX_hit",            "Latency for write hits", "cycles", 1},
        {"latency_GetSX_miss",          "Latency for read-exclusive misses (block not present)", "cycles", 2},
        {"latency_GetSX_upgrade",       "Latency for read-exclusive misses, block present but in Shared state", "cycles", 2},
        {"latency_GetSX_hit",           "Latency for read-exclusive hits", "cycles", 2},
        {"latency_FlushLine",           "Latency for Flush requests", "cycles", 2},
        {"latency_FlushLine_fail",      "Latency for Flush requests that failed (e.g., line was locked)", "cycles", 2},
        {"latency_FlushLineInv",        "Latency for Flush and Invalidate requests", "cycles", 2},
        {"latency_FlushLineInv_fail",   "Latency for Flush and Invalidate requests that failed (e.g., line was locked)", "cycles", 2},
        {"latency_FlushAll",            "Latency for FlushAll (full cache) requests", "cycles", 2},
        /* Track what happens to prefetched blocks */
        {"prefetch_useful",         "Prefetched block had a subsequent hit (useful prefetch)", "count", 2},
        {"prefetch_evict",          "Prefetched block was evicted/flushed before being accessed", "count", 2},
        {"prefetch_inv",            "Prefetched block was invalidated before being accessed", "count", 2},
        {"prefetch_coherence_miss", "Prefetched block incurred a coherence miss (upgrade) on its first access", "count", 2},
        {"prefetch_redundant",      "Prefetch issued for a block that was already in cache", "count", 2},
        /* Miscellaneous */
        {"EventStalledForLockedCacheline",  "Number of times an event (FetchInv, FetchInvX, eviction, Fetch, etc.) was stalled because a cache line was locked", "instances", 1},
        {"default_stat",            "Default statistic used for unexpected events/states/etc. Should be 0, if not, check for missing statistic registrations.", "none", 7})

    SST_ELI_DOCUMENT_SUBCOMPONENT_SLOTS(
            {"replacement", "Replacement policies, slot 0 is for cache, slot 1 is for directory (if it exists)", "SST::MemHierarchy::ReplacementPolicy"},
            {"hash", "Hash function for mapping addresses to cache lines", "SST::MemHierarchy::HashFunction"} )

/* Class definition */
    MESIL1(ComponentId_t id, Params& params, Params& owner_params, bool prefetch);
    MESIL1() = default; // Serialization
    ~MESIL1() { delete cache_array_; }

    /** Event handlers - called by controller */
    bool handleGetS(MemEvent * event, bool in_mshr) override;
    bool handleWrite(MemEvent * event, bool in_mshr) override;
    bool handleGetX(MemEvent * event, bool in_mshr) override;
    bool handleGetSX(MemEvent * event, bool in_mshr) override;
    bool handleFlushLine(MemEvent * event, bool in_mshr) override;
    bool handleFlushLineInv(MemEvent * event, bool in_mshr) override;
    bool handleFlushAll(MemEvent * event, bool in_mshr) override;
    bool handleForwardFlush(MemEvent * event, bool in_mshr) override;
    bool handleFetch(MemEvent * event, bool in_mshr) override;
    bool handleInv(MemEvent * event, bool in_mshr) override;
    bool handleForceInv(MemEvent * event, bool in_mshr) override;
    bool handleFetchInv(MemEvent * event, bool in_mshr) override;
    bool handleFetchInvX(MemEvent * event, bool in_mshr) override;
    bool handleGetSResp(MemEvent * event, bool in_mshr) override;
    bool handleGetXResp(MemEvent * event, bool in_mshr) override;
    bool handleFlushLineResp(MemEvent * event, bool in_mshr) override;
    bool handleFlushAllResp(MemEvent * event, bool in_mshr) override;
    bool handleUnblockFlush(MemEvent * event, bool in_mshr) override;
    bool handleAckPut(MemEvent * event, bool in_mshr) override;
    bool handleNULLCMD(MemEvent * event, bool in_mshr) override;
    bool handleNACK(MemEvent * event, bool in_mshr) override;

    /** Bank conflict detection - used by controller */
    Addr getBank(Addr addr) override;

    /** Configuration */
    MemEventInitCoherence* getInitCoherenceEvent() override;
    virtual std::set<Command> getValidReceiveEvents() override;
    void setSliceAware(uint64_t interleave_size, uint64_t interleave_step) override;

    /** Status output */
    void printStatus(Output& out) override;

    /* LoadLink wakeup event */
    class LoadLinkWakeup : public SST::Event {
        public:
            LoadLinkWakeup(Addr addr, id_type id) : SST::Event(), addr_(addr), id_(id) { }
            LoadLinkWakeup() = default;
            ~LoadLinkWakeup() = default;

            Addr addr_;  // Address that was locked
            id_type id_; // id of event that stalled for the locked address

            void serialize_order(SST::Core::Serialization::serializer& ser) override {
                SST::Event::serialize_order(ser);
                SST_SER(addr_);
                SST_SER(id_);
            }
            ImplementSerializable(LoadLinkWakeup);
    };

    /** Serialization */
    void serialize_order(SST::Core::Serialization::serializer& ser) override;
    ImplementSerializable(SST::MemHierarchy::MESIL1)

private:

    /** Cache and MSHR management */
    MemEventStatus processCacheMiss(MemEvent * event, L1CacheLine * line, bool in_mshr);
    MemEventStatus checkMSHRCollision(MemEvent* event, bool in_mshr);
    L1CacheLine* allocateLine(MemEvent * event, L1CacheLine * line);
    bool handleEviction(Addr addr, L1CacheLine *& line, bool flush);
    void cleanUpAfterRequest(MemEvent * event, bool in_mshr);
    void cleanUpAfterResponse(MemEvent * event, bool in_mshr);
    void cleanUpAfterFlush(MemEvent * req, MemEvent * resp = nullptr, bool in_mshr = true);
    void retry(Addr addr);
    void handleLoadLinkExpiration(SST::Event* ev);

    /** Event send */
    uint64_t sendResponseUp(MemEvent * event, vector<uint8_t>* data, bool in_mshr, uint64_t time, bool success = true) override;
    void sendResponseDown(MemEvent * event, L1CacheLine * line, bool data);
    void forwardFlush(MemEvent * event, L1CacheLine * line, bool evict);
    void sendWriteback(Command cmd, L1CacheLine * line, bool dirty, bool flush);
    void snoopInvalidation(MemEvent * event, L1CacheLine * line);
    void forwardByAddress(MemEventBase* ev, Cycle_t timestamp) override;
    void forwardByDestination(MemEventBase* ev, Cycle_t timestamp) override;

    /** Statistics/Listeners */
    inline void recordPrefetchResult(L1CacheLine * line, Statistic<uint64_t>* stat);
    void recordLatency(Command cmd, int type, uint64_t latency) override;
    void eventProfileAndNotify(MemEvent * event, State state, NotifyAccessType type, NotifyResultType result, bool in_mshr);

    /** Miscellaneous */
    void printLine(Addr addr);
    void beginCompleteStage() override;
    void processCompleteEvent(MemEventInit* event, MemLinkBase* highlink, MemLinkBase* lowlink) override;

    /** Member variables */
    bool is_flushing_;
    bool flush_drain_;
    Cycle_t flush_complete_timestamp_;
    bool snoop_l1_invs_;
    State protocol_read_state_;   // E for MESI, S for MSI
    State protocol_exclusive_state_;   // E for MESI, M for MSI
    Cycle_t llsc_block_cycles_;
    Link* llsc_timeout_ = nullptr; // A self-link to trigger waiting requests when a LoadLink times out

    CacheArray<L1CacheLine>* cache_array_ = nullptr;

    /** Statistics */
    Statistic<uint64_t>* stat_event_stalled_for_lock = nullptr;
    Statistic<uint64_t>* stat_latency_GetS_[2] = {nullptr, nullptr};
    Statistic<uint64_t>* stat_latency_GetX_[4] = {nullptr, nullptr, nullptr, nullptr};
    Statistic<uint64_t>* stat_latency_GetSX_[4] = {nullptr, nullptr, nullptr, nullptr};
    Statistic<uint64_t>* stat_latency_FlushLine_[2] = {nullptr, nullptr};
    Statistic<uint64_t>* stat_latency_FlushLineInv_[2] = {nullptr, nullptr};
    Statistic<uint64_t>* stat_latency_FlushAll_ = nullptr;
    Statistic<uint64_t>* stat_hit_[3][2] = { {nullptr, nullptr}, {nullptr, nullptr}, {nullptr, nullptr} };
    Statistic<uint64_t>* stat_miss_[3][2] = { {nullptr, nullptr}, {nullptr, nullptr}, {nullptr, nullptr} };
    Statistic<uint64_t>* stat_hits_ = nullptr;
    Statistic<uint64_t>* stat_misses_ = nullptr;
};


}}


#endif

