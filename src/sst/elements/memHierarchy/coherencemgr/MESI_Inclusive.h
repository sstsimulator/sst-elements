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

#ifndef MESICOHERENCECONTROLLER_H
#define MESICOHERENCECONTROLLER_H

#include <array>
#include <cstdint>
#include <iostream>
#include <map>
#include <set>

#include "sst/elements/memHierarchy/coherencemgr/coherenceController.h"
#include "sst/elements/memHierarchy/cacheArray.h"
#include "sst/elements/memHierarchy/lineTypes.h"


namespace SST { namespace MemHierarchy {

class MESIInclusive : public CoherenceController {
public:
    SST_ELI_REGISTER_SUBCOMPONENT(MESIInclusive, "memHierarchy", "coherence.mesi_inclusive", SST_ELI_ELEMENT_VERSION(1,0,0),
            "Implements MESI or MSI coherence for a second level or greater cache", SST::MemHierarchy::CoherenceController)

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
        {"eventSent_Inv",           "Number of Inv requests sent", "events", 2},
        {"eventSent_Fetch",         "Number of Fetch requests sent", "events", 2},
        {"eventSent_FetchInv",      "Number of FetchInv requests sent", "events", 2},
        {"eventSent_FetchInvX",     "Number of FetchInvX requests sent", "events", 2},
        {"eventSent_ForceInv",      "Number of ForceInv requests sent", "events", 2},
        {"eventSent_FetchResp",     "Number of FetchResp requests sent", "events", 2},
        {"eventSent_FetchXResp",    "Number of FetchXResp requests sent", "events", 2},
        {"eventSent_AckInv",        "Number of AckInvs sent", "events", 2},
        {"eventSent_AckPut",        "Number of AckPuts sent", "events", 2},
        {"eventSent_NACK",          "Number of NACKs sent", "events", 2},
        {"eventSent_FlushLine",     "Number of FlushLine requests sent", "events", 2},
        {"eventSent_FlushLineInv",  "Number of FlushLineInv requests sent", "events", 2},
        {"eventSent_FlushAll",      "Number of FlushAll requests sent", "events", 2},
        {"eventSent_ForwardFlush",  "Number of ForwardFlush requests sent", "events", 2},
        {"eventSent_AckFlush",      "Number of AckFlush requests sent", "events", 2},
        {"eventSent_UnblockFlush",  "Number of UnblockFlush requests sent", "events", 2},
        {"eventSent_FlushLineResp", "Number of FlushLineResp responses sent", "events", 2},
        {"eventSent_FlushAllResp",  "Number of FlushAllResp responses sent", "events", 2},
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
        {"stateEvent_GetXResp_SMInv",   "Event/State: Number of times a GetXResp was seen in state SM_Inv", "count", 3},
        {"stateEvent_PutS_S",           "Event/State: Number of times a PutS was seen in state S", "count", 3},
        {"stateEvent_PutS_E",           "Event/State: Number of times a PutS was seen in state E", "count", 3},
        {"stateEvent_PutS_M",           "Event/State: Number of times a PutS was seen in state M", "count", 3},
        {"stateEvent_PutS_SInv",        "Event/State: Number of times a PutS was seen in state S_Inv", "count", 3},
        {"stateEvent_PutS_EInv",        "Event/State: Number of times a PutS was seen in state E_Inv", "count", 3},
        {"stateEvent_PutS_MInv",        "Event/State: Number of times a PutS was seen in state M_Inv", "count", 3},
        {"stateEvent_PutS_SMInv",       "Event/State: Number of times a PutS was seen in state SM_Inv", "count", 3},
        {"stateEvent_PutS_SB",          "Event/State: Number of times a PutS was seen in state S_B", "count", 3},
        {"stateEvent_PutS_SBInv",       "Event/State: Number of times a PutS was seen in state SB_Inv", "count", 3},
        {"stateEvent_PutE_E",           "Event/State: Number of times a PutE was seen in state E", "count", 3},
        {"stateEvent_PutE_M",           "Event/State: Number of times a PutE was seen in state M", "count", 3},
        {"stateEvent_PutE_EInv",        "Event/State: Number of times a PutE was seen in state E_Inv", "count", 3},
        {"stateEvent_PutE_MInv",        "Event/State: Number of times a PutE was seen in state M_Inv", "count", 3},
        {"stateEvent_PutE_EInvX",       "Event/State: Number of times a PutE was seen in state E_InvX", "count", 3},
        {"stateEvent_PutE_MInvX",       "Event/State: Number of times a PutE was seen in state M_InvX", "count", 3},
        {"stateEvent_PutM_E",           "Event/State: Number of times a PutM was seen in state E", "count", 3},
        {"stateEvent_PutM_M",           "Event/State: Number of times a PutM was seen in state M", "count", 3},
        {"stateEvent_PutM_EInv",        "Event/State: Number of times a PutM was seen in state E_Inv", "count", 3},
        {"stateEvent_PutM_MInv",        "Event/State: Number of times a PutM was seen in state M_Inv", "count", 3},
        {"stateEvent_PutM_EInvX",       "Event/State: Number of times a PutM was seen in state E_InvX", "count", 3},
        {"stateEvent_PutM_MInvX",       "Event/State: Number of times a PutM was seen in state M_InvX", "count", 3},
        {"stateEvent_PutX_E",           "Event/State: Number of times a PutX was seen in state E", "count", 3},
        {"stateEvent_PutX_M",           "Event/State: Number of times a PutX was seen in state M", "count", 3},
        {"stateEvent_PutX_EInv",        "Event/State: Number of times a PutX was seen in state E_Inv", "count", 3},
        {"stateEvent_PutX_MInv",        "Event/State: Number of times a PutX was seen in state M_Inv", "count", 3},
        {"stateEvent_PutX_EInvX",       "Event/State: Number of times a PutX was seen in state E_InvX", "count", 3},
        {"stateEvent_PutX_MInvX",       "Event/State: Number of times a PutX was seen in state M_InvX", "count", 3},
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
        {"stateEvent_FetchInv_SMInv",   "Event/State: Number of times a FetchInv was seen in state SM_Inv", "count", 3},
        {"stateEvent_FetchInv_EInv",    "Event/State: Number of times a FetchInv was seen in state E_Inv", "count", 3},
        {"stateEvent_FetchInv_EInvX",   "Event/State: Number of times a FetchInv was seen in state E_InvX", "count", 3},
        {"stateEvent_FetchInv_MInv",    "Event/State: Number of times a FetchInv was seen in state M_Inv", "count", 3},
        {"stateEvent_FetchInv_MInvX",   "Event/State: Number of times a FetchInv was seen in state M_InvX", "count", 3},
        {"stateEvent_FetchInv_IB",      "Event/State: Number of times a FetchInv was seen in state I_B", "count", 3},
        {"stateEvent_FetchInv_SB",      "Event/State: Number of times a FetchInv was seen in state S_B", "count", 3},
        {"stateEvent_FetchInvX_I",      "Event/State: Number of times a FetchInvX was seen in state I", "count", 3},
        {"stateEvent_FetchInvX_IS",     "Event/State: Number of times a FetchInvX was seen in state IS", "count", 3},
        {"stateEvent_FetchInvX_IM",     "Event/State: Number of times a FetchInvX was seen in state IM", "count", 3},
        {"stateEvent_FetchInvX_E",      "Event/State: Number of times a FetchInvX was seen in state E", "count", 3},
        {"stateEvent_FetchInvX_M",      "Event/State: Number of times a FetchInvX was seen in state M", "count", 3},
        {"stateEvent_FetchInvX_EInv",   "Event/State: Number of times a FetchInvX was seen in state E_Inv", "count", 3},
        {"stateEvent_FetchInvX_EInvX",  "Event/State: Number of times a FetchInvX was seen in state E_InvX", "count", 3},
        {"stateEvent_FetchInvX_MInv",   "Event/State: Number of times a FetchInvX was seen in state M_Inv", "count", 3},
        {"stateEvent_FetchInvX_MInvX",  "Event/State: Number of times a FetchInvX was seen in state M_InvX", "count", 3},
        {"stateEvent_FetchInvX_IB",     "Event/State: Number of times a FetchInvX was seen in state I_B", "count", 3},
        {"stateEvent_FetchInvX_SB",     "Event/State: Number of times a FetchInvX was seen in state S_B", "count", 3},
        {"stateEvent_Fetch_I",          "Event/State: Number of times a Fetch was seen in state I", "count", 3},
        {"stateEvent_Fetch_IS",         "Event/State: Number of times a Fetch was seen in state IS", "count", 3},
        {"stateEvent_Fetch_IM",         "Event/State: Number of times a Fetch was seen in state IM", "count", 3},
        {"stateEvent_Fetch_S",          "Event/State: Number of times a Fetch was seen in state S", "count", 3},
        {"stateEvent_Fetch_SM",         "Event/State: Number of times a Fetch was seen in state SM", "count", 3},
        {"stateEvent_Fetch_SMInv",      "Event/State: Number of times a Fetch was seen in state SM_Inv", "count", 3},
        {"stateEvent_Fetch_SInv",       "Event/State: Number of times a Fetch was seen in state S_Inv", "count", 3},
        {"stateEvent_Fetch_IB",         "Event/State: Number of times a Fetch was seen in state I_B", "count", 3},
        {"stateEvent_Fetch_SB",         "Event/State: Number of times a Fetch was seen in state S_B", "count", 3},
        {"stateEvent_ForceInv_I",       "Event/State: Number of times a ForceInv was seen in state I", "count", 3},
        {"stateEvent_ForceInv_IS",      "Event/State: Number of times a ForceInv was seen in state IS", "count", 3},
        {"stateEvent_ForceInv_IM",      "Event/State: Number of times a ForceInv was seen in state IM", "count", 3},
        {"stateEvent_ForceInv_SM",      "Event/State: Number of times a ForceInv was seen in state SM", "count", 3},
        {"stateEvent_ForceInv_S",       "Event/State: Number of times a ForceInv was seen in state S", "count", 3},
        {"stateEvent_ForceInv_E",       "Event/State: Number of times a ForceInv was seen in state E", "count", 3},
        {"stateEvent_ForceInv_M",       "Event/State: Number of times a ForceInv was seen in state M", "count", 3},
        {"stateEvent_ForceInv_SMInv",   "Event/State: Number of times a ForceInv was seen in state SM_Inv", "count", 3},
        {"stateEvent_ForceInv_SInv",    "Event/State: Number of times a ForceInv was seen in state S_Inv", "count", 3},
        {"stateEvent_ForceInv_EInv",    "Event/State: Number of times a ForceInv was seen in state E_Inv", "count", 3},
        {"stateEvent_ForceInv_EInvX",   "Event/State: Number of times a ForceInv was seen in state E_InvX", "count", 3},
        {"stateEvent_ForceInv_MInv",    "Event/State: Number of times a ForceInv was seen in state M_Inv", "count", 3},
        {"stateEvent_ForceInv_MInvX",   "Event/State: Number of times a ForceInv was seen in state M_InvX", "count", 3},
        {"stateEvent_ForceInv_IB",      "Event/State: Number of times a ForceInv was seen in state I_B", "count", 3},
        {"stateEvent_ForceInv_SB",      "Event/State: Number of times a ForceInv was seen in state S_B", "count", 3},
        {"stateEvent_FetchResp_EInv",   "Event/State: Number of times a FetchResp was seen in state E_Inv", "count", 3},
        {"stateEvent_FetchResp_MInv",   "Event/State: Number of times a FetchResp was seen in state M_Inv", "count", 3},
        {"stateEvent_FetchXResp_EInvX", "Event/State: Number of times a FetchXResp was seen in state E_InvX", "count", 3},
        {"stateEvent_FetchXResp_MInvX", "Event/State: Number of times a FetchXResp was seen in state M_InvX", "count", 3},
        {"stateEvent_AckInv_SInv",      "Event/State: Number of times an AckInv was seen in state S_Inv", "count", 3},
        {"stateEvent_AckInv_SMInv",     "Event/State: Number of times an AckInv was seen in state SM_Inv", "count", 3},
        {"stateEvent_AckInv_EInv",      "Event/State: Number of times an AckInv was seen in state E_Inv", "count", 3},
        {"stateEvent_AckInv_MInv",      "Event/State: Number of times an AckInv was seen in state M_Inv", "count", 3},
        {"stateEvent_AckInv_SBInv",     "Event/State: Number of times an AckInv was seen in state SB_Inv", "count", 3},
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
        {"evict_SInv",              "Eviction: Attempted to evict a block in state S_Inv", "count", 3},
        {"evict_EInv",              "Eviction: Attempted to evict a block in state E_Inv", "count", 3},
        {"evict_MInv",              "Eviction: Attempted to evict a block in state M_Inv", "count", 3},
        {"evict_SMInv",             "Eviction: Attempted to evict a block in state SM_Inv", "count", 3},
        {"evict_EInvX",             "Eviction: Attempted to evict a block in state E_InvX", "count", 3},
        {"evict_MInvX",             "Eviction: Attempted to evict a block in state M_InvX", "count", 3},
        {"evict_IB",                "Eviction: Attempted to evict a block in state S_B", "count", 3},
        {"evict_SB",                "Eviction: Attempted to evict a block in state I_B", "count", 3},
        {"evict_SBInv",             "Eviction: Attempted to evict a block in state SB_Inv", "count", 3},
        /* Latency for different kinds of misses*/
        {"latency_GetS_hit",        "Latency for read hits", "cycles", 1},
        {"latency_GetS_miss",       "Latency for read misses, block not present", "cycles", 1},
        {"latency_GetS_inv",        "Latency for read misses, required fetch/inv from owner", "cycles", 1},
        {"latency_GetX_hit",        "Latency for write hits", "cycles", 1},
        {"latency_GetX_miss",       "Latency for write misses, block not present", "cycles", 1},
        {"latency_GetX_inv",        "Latency for write misses, block present but required invalidation/fetch", "cycles", 1},
        {"latency_GetX_upgrade",    "Latency for write misses, block present but in Shared state (includes invs in S)", "cycles", 1},
        {"latency_GetSX_hit",       "Latency for read-exclusive hits", "cycles", 1},
        {"latency_GetSX_miss",      "Latency for read-exclusive misses, block not present", "cycles", 1},
        {"latency_GetSX_inv",       "Latency for read-exclusive misses, block present but required invalidation/fetch", "cycles", 1},
        {"latency_GetSX_upgrade",   "Latency for read-exclusive misses, block present but in Shared state (includes invs in S)", "cycles", 1},
        {"latency_FlushLine",       "Latency for flush requests", "cycles", 1},
        {"latency_FlushLineInv",    "Latency for flush+invalidate requests", "cycles", 1},
        {"latency_FlushAll",        "Latency for flush+all requests", "cycles", 1},
        /* Track what happens to prefetched blocks */
        {"prefetch_useful",         "Prefetched block had a subsequent hit (useful prefetch)", "count", 2},
        {"prefetch_evict",          "Prefetched block was evicted/flushed before being accessed", "count", 2},
        {"prefetch_inv",            "Prefetched block was invalidated before being accessed", "count", 2},
        {"prefetch_coherence_miss", "Prefetched block incurred a coherence miss (upgrade) on its first access", "count", 2},
        {"prefetch_redundant",      "Prefetch issued for a block that was already in cache", "count", 2},
        {"default_stat",            "Default statistic used for unexpected events/states/etc. Should be 0, if not, check for missing statistic registerations.", "none", 7})

    SST_ELI_DOCUMENT_SUBCOMPONENT_SLOTS(
            {"replacement", "Replacement policies, slot 0 is for cache, slot 1 is for directory (if it exists)", "SST::MemHierarchy::ReplacementPolicy"},
            {"hash", "Hash function for mapping addresses to cache lines", "SST::MemHierarchy::HashFunction"} )

/* Class definition */
    /** Note that MESIInclusive handles both MESI & MSI protocols */
    MESIInclusive(SST::ComponentId_t id, Params& params, Params& owner_params, bool prefetch);
    MESIInclusive() = default; // Serialization
    ~MESIInclusive() { delete cache_array_; }

    /** Event handlers - called by controller */
    virtual bool handleGetS(MemEvent * event, bool in_mshr) override;
    virtual bool handleGetX(MemEvent * event, bool in_mshr) override;
    virtual bool handleGetSX(MemEvent * event, bool in_mshr) override;
    virtual bool handleFlushLine(MemEvent * event, bool in_mshr) override;
    virtual bool handleFlushLineInv(MemEvent * event, bool in_mshr) override;
    virtual bool handleFlushAll(MemEvent * event, bool in_mshr) override;
    virtual bool handleForwardFlush(MemEvent * event, bool in_mshr) override;
    virtual bool handlePutS(MemEvent * event, bool in_mshr) override;
    virtual bool handlePutX(MemEvent * event, bool in_mshr) override;
    virtual bool handlePutE(MemEvent * event, bool in_mshr) override;
    virtual bool handlePutM(MemEvent * event, bool in_mshr) override;
    virtual bool handleInv(MemEvent * event, bool in_mshr) override;
    virtual bool handleForceInv(MemEvent * event, bool in_mshr) override;
    virtual bool handleFetch(MemEvent * event, bool in_mshr) override;
    virtual bool handleFetchInv(MemEvent * event, bool in_mshr) override;
    virtual bool handleFetchInvX(MemEvent * event, bool in_mshr) override;
    virtual bool handleGetSResp(MemEvent * event, bool in_mshr) override;
    virtual bool handleGetXResp(MemEvent * event, bool in_mshr) override;
    virtual bool handleFlushLineResp(MemEvent * event, bool in_mshr) override;
    virtual bool handleFlushAllResp(MemEvent * event, bool in_mshr) override;
    virtual bool handleFetchResp(MemEvent * event, bool in_mshr) override;
    virtual bool handleFetchXResp(MemEvent * event, bool in_mshr) override;
    virtual bool handleAckInv(MemEvent * event, bool in_mshr) override;
    virtual bool handleAckPut(MemEvent * event, bool in_mshr) override;
    virtual bool handleAckFlush(MemEvent * event, bool in_mshr) override;
    virtual bool handleUnblockFlush(MemEvent * event, bool in_mshr) override;
    virtual bool handleNACK(MemEvent * event, bool in_mshr) override;
    virtual bool handleNULLCMD(MemEvent * event, bool in_mshr) override;

    /** Bank conflict detection - used by controller */
    virtual Addr getBank(Addr addr) override { return cache_array_->getBank(addr); }

    /** Configuration **/
    MemEventInitCoherence * getInitCoherenceEvent() override;
    std::set<Command> getValidReceiveEvents() override;
    virtual void setSliceAware(uint64_t size, uint64_t step) override { cache_array_->setSliceAware(size, step); }

    /** Status output */
    void printStatus(Output &out) override;

    /** Serialization */
    void serialize_order(SST::Core::Serialization::serializer& ser) override;
    ImplementSerializable(SST::MemHierarchy::MESIInclusive);

private:

    /** Cache and MSHR management */
    MemEventStatus processCacheMiss(MemEvent * event, SharedCacheLine * line, bool in_mshr);
    SharedCacheLine * allocateLine(MemEvent * event, SharedCacheLine * line);
    bool handleEviction(Addr addr, SharedCacheLine *& line);
    void cleanUpAfterRequest(MemEvent * event, bool in_mshr);
    void cleanUpAfterResponse(MemEvent * event, bool in_mshr);
    void cleanUpEvent(MemEvent * event, bool in_mshr);
    void retry(Addr addr);

    /** Invalidation **/
    bool invalidateExceptRequestor(MemEvent * event, SharedCacheLine * line, bool in_mshr);
    bool invalidateAll(MemEvent * event, SharedCacheLine * line, bool in_mshr, Command cmd = Command::NULLCMD);
    uint64_t invalidateSharer(std::string shr, MemEvent * event, SharedCacheLine * line, bool in_mshr, Command cmd = Command::Inv);
    bool invalidateOwner(MemEvent * event, SharedCacheLine * line, bool in_mshr, Command cmd = Command::FetchInv);

    /** Forward flush line request, with or without data */
    void forwardFlush(MemEvent * event, SharedCacheLine * line, bool data);

    /** Send response up (towards processor) */
    SimTime_t sendResponseUp(MemEvent * event, vector<uint8_t>* data, bool in_mshr, uint64_t time, Command cmd = Command::NULLCMD, bool success = true);

    /** Send response down (towards memory) */
    void sendResponseDown(MemEvent * event, SharedCacheLine * line, bool data, bool evict);

    /** Send writeback */
    void sendWriteback(Command cmd, SharedCacheLine * line, bool dirty);

    /** Send AckPut */
    void sendAckPut(MemEvent * event);

    /** Downgrade request */
    void downgradeOwner(MemEvent * event, SharedCacheLine * line, bool in_mshr);

    /** Evict a block */
    State doEviction(MemEvent * event, SharedCacheLine * line, State state);

    /** Call through to coherenceController with statistic recording */
    void forwardByAddress(MemEventBase* ev, Cycle_t timestamp) override;
    void forwardByDestination(MemEventBase* ev, Cycle_t timestamp) override;

/* Miscellaneous functions */
    /* Record prefetch statistics. Line cannot be null. */
    void recordPrefetchResult(SharedCacheLine * line, Statistic<uint64_t> * stat);

    /* Record latency */
    void recordLatency(Command cmd, int type, uint64_t latency) override;

    void printLine(Addr addr);

    void beginCompleteStage() override;
    void processCompleteEvent(MemEventInit* event, MemLinkBase* highlink, MemLinkBase* lowlink) override;

/* Variables */
    CacheArray<SharedCacheLine> * cache_array_ = nullptr;
    State protocol_state_;       // State to transition to on exclusive response to read/shared request
    bool protocol_;             // True for MESI, false for MSI

    std::map<Addr, std::map<std::string, MemEvent::id_type> > responses_;

    FlushState flush_state_;

    int shutdown_flush_counter_;

    /* Statistics */
    Statistic<uint64_t>* stat_latency_GetS_[3] = {nullptr, nullptr, nullptr}; // HIT, MISS, INV
    Statistic<uint64_t>* stat_latency_GetX_[4] = {nullptr, nullptr, nullptr, nullptr};
    Statistic<uint64_t>* stat_latency_GetSX_[4] = {nullptr, nullptr, nullptr, nullptr};
    Statistic<uint64_t>* stat_latency_FlushLine_ = nullptr;
    Statistic<uint64_t>* stat_latency_FlushLineInv_ = nullptr;
    Statistic<uint64_t>* stat_latency_FlushAll_ = nullptr;
    Statistic<uint64_t>* stat_hit_[3][2] = { {nullptr, nullptr}, {nullptr, nullptr}, {nullptr, nullptr} };
    Statistic<uint64_t>* stat_miss_[3][2] = { {nullptr, nullptr}, {nullptr, nullptr}, {nullptr, nullptr} };
    Statistic<uint64_t>* stat_hits_ = nullptr;
    Statistic<uint64_t>* stat_misses_ = nullptr;
};


}}


#endif

