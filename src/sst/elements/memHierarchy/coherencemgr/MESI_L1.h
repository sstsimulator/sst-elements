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

#ifndef MEMHIERARCHY_L1COHERENCECONTROLLER_H
#define MEMHIERARCHY_L1COHERENCECONTROLLER_H

#include <iostream>
#include <array>

#include "sst/elements/memHierarchy/coherencemgr/coherenceController.h"
#include "sst/elements/memHierarchy/memTypes.h"
#include "sst/elements/memHierarchy/lineTypes.h"
#include "sst/elements/memHierarchy/cacheArray.h"


namespace SST { namespace MemHierarchy {

class MESIL1 : public CoherenceController {
public:
/* Element Library Info */
    SST_ELI_REGISTER_SUBCOMPONENT_DERIVED(MESIL1, "memHierarchy", "coherence.mesi_l1", SST_ELI_ELEMENT_VERSION(1,0,0),
            "Implements MESI or MSI coherence for an L1 cache", SST::MemHierarchy::CoherenceController)

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
        {"eventSent_PutS",          "Number of PutS requests sent", "events", 2},
        {"eventSent_PutE",          "Number of PutE requests sent", "events", 2},
        {"eventSent_PutM",          "Number of PutM requests sent", "events", 2},
        {"eventSent_FetchResp",     "Number of FetchResp requests sent", "events", 2},
        {"eventSent_FetchXResp",    "Number of FetchXResp requests sent", "events", 2},
        {"eventSent_AckInv",        "Number of AckInvs sent", "events", 2},
        {"eventSent_NACK",          "Number of NACKs sent ", "events", 2},
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
    /** Constructor for MESIL1 */

    MESIL1(ComponentId_t id, Params& params, Params& ownerParams, bool prefetch) : CoherenceController(id, params, ownerParams, prefetch) {
        params.insert(ownerParams);
        debug->debug(_INFO_,"--------------------------- Initializing [L1Controller] ... \n\n");

        snoopL1Invs_ = params.find<bool>("snoop_l1_invalidations", false);
        bool MESI = params.find<bool>("protocol", true);

        // State to transition to on a GetXResp/clean to a read (GetS)
        if (MESI)
            protocolState_ = E;
        else
            protocolState_ = S;

        // Cache Array
        uint64_t lines = params.find<uint64_t>("lines", 0);
        uint64_t assoc = params.find<uint64_t>("associativity", 0);
        ReplacementPolicy * rmgr = createReplacementPolicy(lines, assoc, params, true);
        HashFunction * ht = createHashFunction(params);

        cacheArray_ = new CacheArray<L1CacheLine>(debug, lines, assoc, lineSize_, rmgr, ht);
        cacheArray_->setBanked(params.find<uint64_t>("banks", 0));

        // Register statistics
        stat_eventState[(int)Command::GetS][I] =      registerStatistic<uint64_t>("stateEvent_GetS_I");
        stat_eventState[(int)Command::GetS][S] =      registerStatistic<uint64_t>("stateEvent_GetS_S");
        stat_eventState[(int)Command::GetS][M] =      registerStatistic<uint64_t>("stateEvent_GetS_M");
        stat_eventState[(int)Command::GetX][I] =      registerStatistic<uint64_t>("stateEvent_GetX_I");
        stat_eventState[(int)Command::GetX][S] =      registerStatistic<uint64_t>("stateEvent_GetX_S");
        stat_eventState[(int)Command::GetX][M] =      registerStatistic<uint64_t>("stateEvent_GetX_M");
        stat_eventState[(int)Command::GetSX][I] =     registerStatistic<uint64_t>("stateEvent_GetSX_I");
        stat_eventState[(int)Command::GetSX][S] =     registerStatistic<uint64_t>("stateEvent_GetSX_S");
        stat_eventState[(int)Command::GetSX][M] =     registerStatistic<uint64_t>("stateEvent_GetSX_M");
        stat_eventState[(int)Command::GetSResp][IS] = registerStatistic<uint64_t>("stateEvent_GetSResp_IS");
        stat_eventState[(int)Command::GetXResp][IS] = registerStatistic<uint64_t>("stateEvent_GetXResp_IS");
        stat_eventState[(int)Command::GetXResp][IM] = registerStatistic<uint64_t>("stateEvent_GetXResp_IM");
        stat_eventState[(int)Command::GetXResp][SM] = registerStatistic<uint64_t>("stateEvent_GetXResp_SM");
        stat_eventState[(int)Command::Inv][I] =       registerStatistic<uint64_t>("stateEvent_Inv_I");
        stat_eventState[(int)Command::Inv][S] =       registerStatistic<uint64_t>("stateEvent_Inv_S");
        stat_eventState[(int)Command::Inv][IS] =      registerStatistic<uint64_t>("stateEvent_Inv_IS");
        stat_eventState[(int)Command::Inv][IM] =      registerStatistic<uint64_t>("stateEvent_Inv_IM");
        stat_eventState[(int)Command::Inv][SM] =      registerStatistic<uint64_t>("stateEvent_Inv_SM");
        stat_eventState[(int)Command::Inv][S_B] =       registerStatistic<uint64_t>("stateEvent_Inv_SB");
        stat_eventState[(int)Command::Inv][I_B] =       registerStatistic<uint64_t>("stateEvent_Inv_IB");
        stat_eventState[(int)Command::FetchInvX][I] =   registerStatistic<uint64_t>("stateEvent_FetchInvX_I");
        stat_eventState[(int)Command::FetchInvX][M] =   registerStatistic<uint64_t>("stateEvent_FetchInvX_M");
        stat_eventState[(int)Command::FetchInvX][IS] =  registerStatistic<uint64_t>("stateEvent_FetchInvX_IS");
        stat_eventState[(int)Command::FetchInvX][IM] =  registerStatistic<uint64_t>("stateEvent_FetchInvX_IM");
        stat_eventState[(int)Command::FetchInvX][S_B] = registerStatistic<uint64_t>("stateEvent_FetchInvX_SB");
        stat_eventState[(int)Command::FetchInvX][I_B] = registerStatistic<uint64_t>("stateEvent_FetchInvX_IB");
        stat_eventState[(int)Command::Fetch][I] =       registerStatistic<uint64_t>("stateEvent_Fetch_I");
        stat_eventState[(int)Command::Fetch][S] =       registerStatistic<uint64_t>("stateEvent_Fetch_S");
        stat_eventState[(int)Command::Fetch][IS] =      registerStatistic<uint64_t>("stateEvent_Fetch_IS");
        stat_eventState[(int)Command::Fetch][IM] =      registerStatistic<uint64_t>("stateEvent_Fetch_IM");
        stat_eventState[(int)Command::Fetch][SM] =      registerStatistic<uint64_t>("stateEvent_Fetch_SM");
        stat_eventState[(int)Command::Fetch][I_B] =     registerStatistic<uint64_t>("stateEvent_Fetch_IB");
        stat_eventState[(int)Command::Fetch][S_B] =     registerStatistic<uint64_t>("stateEvent_Fetch_SB");
        stat_eventState[(int)Command::FetchInv][I] =    registerStatistic<uint64_t>("stateEvent_FetchInv_I");
        stat_eventState[(int)Command::FetchInv][S] =    registerStatistic<uint64_t>("stateEvent_FetchInv_S");
        stat_eventState[(int)Command::FetchInv][M] =    registerStatistic<uint64_t>("stateEvent_FetchInv_M");
        stat_eventState[(int)Command::FetchInv][IS] =   registerStatistic<uint64_t>("stateEvent_FetchInv_IS");
        stat_eventState[(int)Command::FetchInv][IM] =   registerStatistic<uint64_t>("stateEvent_FetchInv_IM");
        stat_eventState[(int)Command::FetchInv][SM] =   registerStatistic<uint64_t>("stateEvent_FetchInv_SM");
        stat_eventState[(int)Command::FetchInv][S_B] =  registerStatistic<uint64_t>("stateEvent_FetchInv_SB");
        stat_eventState[(int)Command::FetchInv][I_B] =  registerStatistic<uint64_t>("stateEvent_FetchInv_IB");
        stat_eventState[(int)Command::ForceInv][I] =    registerStatistic<uint64_t>("stateEvent_ForceInv_I");
        stat_eventState[(int)Command::ForceInv][S] =    registerStatistic<uint64_t>("stateEvent_ForceInv_S");
        stat_eventState[(int)Command::ForceInv][M] =    registerStatistic<uint64_t>("stateEvent_ForceInv_M");
        stat_eventState[(int)Command::ForceInv][IS] =   registerStatistic<uint64_t>("stateEvent_ForceInv_IS");
        stat_eventState[(int)Command::ForceInv][IM] =   registerStatistic<uint64_t>("stateEvent_ForceInv_IM");
        stat_eventState[(int)Command::ForceInv][SM] =   registerStatistic<uint64_t>("stateEvent_ForceInv_SM");
        stat_eventState[(int)Command::ForceInv][S_B] =  registerStatistic<uint64_t>("stateEvent_ForceInv_SB");
        stat_eventState[(int)Command::ForceInv][I_B] =  registerStatistic<uint64_t>("stateEvent_ForceInv_IB");
        stat_eventState[(int)Command::FlushLine][I] =   registerStatistic<uint64_t>("stateEvent_FlushLine_I");
        stat_eventState[(int)Command::FlushLine][S] =   registerStatistic<uint64_t>("stateEvent_FlushLine_S");
        stat_eventState[(int)Command::FlushLine][M] =   registerStatistic<uint64_t>("stateEvent_FlushLine_M");
        stat_eventState[(int)Command::FlushLineInv][I] =    registerStatistic<uint64_t>("stateEvent_FlushLineInv_I");
        stat_eventState[(int)Command::FlushLineInv][S] =    registerStatistic<uint64_t>("stateEvent_FlushLineInv_S");
        stat_eventState[(int)Command::FlushLineInv][M] =    registerStatistic<uint64_t>("stateEvent_FlushLineInv_M");
        stat_eventState[(int)Command::FlushLineResp][I] =   registerStatistic<uint64_t>("stateEvent_FlushLineResp_I");
        stat_eventState[(int)Command::FlushLineResp][I_B] = registerStatistic<uint64_t>("stateEvent_FlushLineResp_IB");
        stat_eventState[(int)Command::FlushLineResp][S_B] = registerStatistic<uint64_t>("stateEvent_FlushLineResp_SB");
        stat_eventSent[(int)Command::GetS] =            registerStatistic<uint64_t>("eventSent_GetS");
        stat_eventSent[(int)Command::GetX] =            registerStatistic<uint64_t>("eventSent_GetX");
        stat_eventSent[(int)Command::GetSX] =           registerStatistic<uint64_t>("eventSent_GetSX");
        stat_eventSent[(int)Command::PutM] =            registerStatistic<uint64_t>("eventSent_PutM");
        stat_eventSent[(int)Command::NACK] =            registerStatistic<uint64_t>("eventSent_NACK");
        stat_eventSent[(int)Command::FlushLine] =       registerStatistic<uint64_t>("eventSent_FlushLine");
        stat_eventSent[(int)Command::FlushLineInv] =    registerStatistic<uint64_t>("eventSent_FlushLineInv");
        stat_eventSent[(int)Command::FetchResp] =       registerStatistic<uint64_t>("eventSent_FetchResp");
        stat_eventSent[(int)Command::FetchXResp] =      registerStatistic<uint64_t>("eventSent_FetchXResp");
        stat_eventSent[(int)Command::AckInv] =          registerStatistic<uint64_t>("eventSent_AckInv");
        stat_eventSent[(int)Command::GetSResp] =        registerStatistic<uint64_t>("eventSent_GetSResp");
        stat_eventSent[(int)Command::GetXResp] =        registerStatistic<uint64_t>("eventSent_GetXResp");
        stat_eventSent[(int)Command::FlushLineResp] =   registerStatistic<uint64_t>("eventSent_FlushLineResp");
        stat_eventSent[(int)Command::Put]           = registerStatistic<uint64_t>("eventSent_Put");
        stat_eventSent[(int)Command::Get]           = registerStatistic<uint64_t>("eventSent_Get");
        stat_eventSent[(int)Command::AckMove]       = registerStatistic<uint64_t>("eventSent_AckMove");
        stat_eventSent[(int)Command::CustomReq]     = registerStatistic<uint64_t>("eventSent_CustomReq");
        stat_eventSent[(int)Command::CustomResp]    = registerStatistic<uint64_t>("eventSent_CustomResp");
        stat_eventSent[(int)Command::CustomAck]     = registerStatistic<uint64_t>("eventSent_CustomAck");
        stat_eventStalledForLock                = registerStatistic<uint64_t>("EventStalledForLockedCacheline");
        stat_evict[I]                           = registerStatistic<uint64_t>("evict_I");
        stat_evict[S]                           = registerStatistic<uint64_t>("evict_S");
        stat_evict[M]                           = registerStatistic<uint64_t>("evict_M");
        stat_evict[IS]                          = registerStatistic<uint64_t>("evict_IS");
        stat_evict[IM]                          = registerStatistic<uint64_t>("evict_IM");
        stat_evict[SM]                          = registerStatistic<uint64_t>("evict_SM");
        stat_evict[I_B]                         = registerStatistic<uint64_t>("evict_SB");
        stat_evict[S_B]                         = registerStatistic<uint64_t>("evict_SB");
        stat_latencyGetS[LatType::HIT]          = registerStatistic<uint64_t>("latency_GetS_hit");
        stat_latencyGetS[LatType::MISS]         = registerStatistic<uint64_t>("latency_GetS_miss");
        stat_latencyGetX[LatType::HIT]          = registerStatistic<uint64_t>("latency_GetX_hit");
        stat_latencyGetX[LatType::MISS]         = registerStatistic<uint64_t>("latency_GetX_miss");
        stat_latencyGetX[LatType::UPGRADE]      = registerStatistic<uint64_t>("latency_GetX_upgrade");
        stat_latencyGetSX[LatType::HIT]         = registerStatistic<uint64_t>("latency_GetSX_hit");
        stat_latencyGetSX[LatType::MISS]        = registerStatistic<uint64_t>("latency_GetSX_miss");
        stat_latencyGetSX[LatType::UPGRADE]     = registerStatistic<uint64_t>("latency_GetSX_upgrade");
        stat_latencyFlushLine[LatType::HIT]     = registerStatistic<uint64_t>("latency_FlushLine");
        stat_latencyFlushLine[LatType::MISS]    = registerStatistic<uint64_t>("latency_FlushLine_fail");
        stat_latencyFlushLineInv[LatType::HIT]  = registerStatistic<uint64_t>("latency_FlushLineInv");
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

        /* Only for caches that expect writeback acks but don't know yet and can't register statistics later. Always enabled for now. */
        stat_eventState[(int)Command::AckPut][I] = registerStatistic<uint64_t>("stateEvent_AckPut_I");

        /* Only for caches that don't silently drop clean blocks but don't know yet and can't register statistics later. Always enabled for now. */
        stat_eventSent[(int)Command::PutS] = registerStatistic<uint64_t>("eventSent_PutS");
        stat_eventSent[(int)Command::PutE] = registerStatistic<uint64_t>("eventSent_PutE");

        // Only for caches that forward invs to the processor
        if (snoopL1Invs_) {
            stat_eventSent[(int)Command::Inv] = registerStatistic<uint64_t>("eventSent_Inv");
        }

        /* Prefetch statistics */
        if (prefetch) {
            statPrefetchEvict = registerStatistic<uint64_t>("prefetch_evict");
            statPrefetchInv = registerStatistic<uint64_t>("prefetch_inv");
            statPrefetchHit = registerStatistic<uint64_t>("prefetch_useful");
            statPrefetchUpgradeMiss = registerStatistic<uint64_t>("prefetch_coherence_miss");
            statPrefetchRedundant = registerStatistic<uint64_t>("prefetch_redundant");
        }

        /* MESI-specific statistics (as opposed to MSI) */
        if (MESI) {
            stat_eventState[(int)Command::GetS][E]          = registerStatistic<uint64_t>("stateEvent_GetS_E");
            stat_eventState[(int)Command::GetX][E]          = registerStatistic<uint64_t>("stateEvent_GetX_E");
            stat_eventState[(int)Command::GetSX][E]         = registerStatistic<uint64_t>("stateEvent_GetSX_E");
            stat_eventState[(int)Command::FlushLine][E]     = registerStatistic<uint64_t>("stateEvent_FlushLine_E");
            stat_eventState[(int)Command::FlushLineInv][E]  = registerStatistic<uint64_t>("stateEvent_FlushLineInv_E");
            stat_eventState[(int)Command::FetchInv][E]      = registerStatistic<uint64_t>("stateEvent_FetchInv_E");
            stat_eventState[(int)Command::ForceInv][E]      = registerStatistic<uint64_t>("stateEvent_ForceInv_E");
            stat_eventState[(int)Command::FetchInvX][E]     = registerStatistic<uint64_t>("stateEvent_FetchInvX_E");
            stat_evict[E]                                   = registerStatistic<uint64_t>("evict_E");
        }
    }

    ~MESIL1() {}

    /** Event handlers - called by controller */
    bool handleGetS(MemEvent * event, bool inMSHR);
    bool handleGetX(MemEvent * event, bool inMSHR);
    bool handleGetSX(MemEvent * event, bool inMSHR);
    bool handleFlushLine(MemEvent * event, bool inMSHR);
    bool handleFlushLineInv(MemEvent * event, bool inMSHR);
    bool handleFetch(MemEvent * event, bool inMSHR);
    bool handleInv(MemEvent * event, bool inMSHR);
    bool handleForceInv(MemEvent * event, bool inMSHR);
    bool handleFetchInv(MemEvent * event, bool inMSHR);
    bool handleFetchInvX(MemEvent * event, bool inMSHR);
    bool handleGetSResp(MemEvent * event, bool inMSHR);
    bool handleGetXResp(MemEvent * event, bool inMSHR);
    bool handleFlushLineResp(MemEvent * event, bool inMSHR);
    bool handleAckPut(MemEvent * event, bool inMSHR);
    bool handleNULLCMD(MemEvent * event, bool inMSHR);
    bool handleNACK(MemEvent * event, bool inMSHR);

    /** Configuration */
    MemEventInitCoherence* getInitCoherenceEvent();
    virtual std::set<Command> getValidReceiveEvents();
    void setSliceAware(uint64_t interleaveSize, uint64_t interleaveStep);

    void printStatus(Output& out);

    Addr getBank(Addr addr);

private:

    /** Cache and MSHR management */
    MemEventStatus processCacheMiss(MemEvent * event, L1CacheLine * line, bool inMSHR);
    L1CacheLine* allocateLine(MemEvent * event, L1CacheLine * line);
    bool handleEviction(Addr addr, L1CacheLine *& line);
    void cleanUpAfterRequest(MemEvent * event, bool inMSHR);
    void cleanUpAfterResponse(MemEvent * event, bool inMSHR);
    void retry(Addr addr);

    /** Event send */
    uint64_t sendResponseUp(MemEvent * event, vector<uint8_t>* data, bool inMSHR, uint64_t time, bool success = false);
    void sendResponseDown(MemEvent * event, L1CacheLine * line, bool data);
    void forwardFlush(MemEvent * event, L1CacheLine * line, bool evict);
    void sendWriteback(Command cmd, L1CacheLine * line, bool dirty);
    void snoopInvalidation(MemEvent * event, L1CacheLine * line);
    void addToOutgoingQueue(Response& resp);
    void addToOutgoingQueueUp(Response& resp);

    /** Statistics/Listeners */
    inline void recordPrefetchResult(L1CacheLine * line, Statistic<uint64_t>* stat);
    void recordLatency(Command cmd, int type, uint64_t latency);
    void eventProfileAndNotify(MemEvent * event, State state, NotifyAccessType type, NotifyResultType result, bool inMSHR);

    /** Miscellaneous */
    void printLine(Addr addr);
    void printData(Addr addr);
    void printData(vector<uint8_t> * data, bool set);

    bool snoopL1Invs_;
    State protocolState_; // E for MESI, S for MSI

    CacheArray<L1CacheLine>* cacheArray_;

    /** Statistics */
    Statistic<uint64_t>* stat_eventStalledForLock;
    Statistic<uint64_t>* stat_latencyGetS[2];
    Statistic<uint64_t>* stat_latencyGetX[4];
    Statistic<uint64_t>* stat_latencyGetSX[4];
    Statistic<uint64_t>* stat_latencyFlushLine[2];
    Statistic<uint64_t>* stat_latencyFlushLineInv[2];
    Statistic<uint64_t>* stat_hit[3][2];
    Statistic<uint64_t>* stat_miss[3][2];
    Statistic<uint64_t>* stat_hits;
    Statistic<uint64_t>* stat_misses;
};


}}


#endif

