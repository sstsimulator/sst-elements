// Copyright 2009-2021 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2021, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef MESICOHERENCECONTROLLER_H
#define MESICOHERENCECONTROLLER_H

#include <iostream>
#include <array>

#include "sst/elements/memHierarchy/coherencemgr/coherenceController.h"
#include "sst/elements/memHierarchy/cacheArray.h"
#include "sst/elements/memHierarchy/lineTypes.h"


namespace SST { namespace MemHierarchy {

class MESIInclusive : public CoherenceController {
public:
    SST_ELI_REGISTER_SUBCOMPONENT_DERIVED(MESIInclusive, "memHierarchy", "coherence.mesi_inclusive", SST_ELI_ELEMENT_VERSION(1,0,0),
            "Implements MESI or MSI coherence for a second level or greater cache", SST::MemHierarchy::CoherenceController)

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
    /** Constructor for MESIInclusive. Note that MESIInclusive handles both MESI & MSI protocols */
    MESIInclusive(SST::ComponentId_t id, Params& params, Params& ownerParams, bool prefetch) : CoherenceController(id, params, ownerParams, prefetch) {
        params.insert(ownerParams);
        debug->debug(_INFO_,"--------------------------- Initializing [MESI Controller] ... \n\n");

        protocol_ = params.find<bool>("protocol", 1);
        if (protocol_)
            protocolState_ = E;
        else
            protocolState_ = S;

        // Cache Array
        uint64_t lines = params.find<uint64_t>("lines");
        uint64_t assoc = params.find<uint64_t>("associativity");

        ReplacementPolicy * rmgr = createReplacementPolicy(lines, assoc, params, false);
        HashFunction * ht = createHashFunction(params);
        cacheArray_ = new CacheArray<SharedCacheLine>(debug, lines, assoc, lineSize_, rmgr, ht);
        cacheArray_->setBanked(params.find<uint64_t>("banks", 0));

        /* Statistics */
        stat_evict[I] =         registerStatistic<uint64_t>("evict_I");
        stat_evict[IS] =        registerStatistic<uint64_t>("evict_IS");
        stat_evict[IM] =        registerStatistic<uint64_t>("evict_IM");
        stat_evict[S] =         registerStatistic<uint64_t>("evict_S");
        stat_evict[SM] =        registerStatistic<uint64_t>("evict_SM");
        stat_evict[S_Inv] =     registerStatistic<uint64_t>("evict_SInv");
        stat_evict[M] =         registerStatistic<uint64_t>("evict_M");
        stat_evict[M_Inv] =     registerStatistic<uint64_t>("evict_MInv");
        stat_evict[SM_Inv] =    registerStatistic<uint64_t>("evict_SMInv");
        stat_evict[M_InvX] =    registerStatistic<uint64_t>("evict_MInvX");
        stat_evict[I_B] =       registerStatistic<uint64_t>("evict_IB");
        stat_evict[S_B] =       registerStatistic<uint64_t>("evict_SB");
        stat_evict[SB_Inv] =    registerStatistic<uint64_t>("evict_SBInv");
        stat_eventState[(int)Command::GetS][I] =    registerStatistic<uint64_t>("stateEvent_GetS_I");
        stat_eventState[(int)Command::GetS][S] =    registerStatistic<uint64_t>("stateEvent_GetS_S");
        stat_eventState[(int)Command::GetS][M] =    registerStatistic<uint64_t>("stateEvent_GetS_M");
        stat_eventState[(int)Command::GetX][I] =    registerStatistic<uint64_t>("stateEvent_GetX_I");
        stat_eventState[(int)Command::GetX][S] =    registerStatistic<uint64_t>("stateEvent_GetX_S");
        stat_eventState[(int)Command::GetX][M] =    registerStatistic<uint64_t>("stateEvent_GetX_M");
        stat_eventState[(int)Command::GetSX][I] =   registerStatistic<uint64_t>("stateEvent_GetSX_I");
        stat_eventState[(int)Command::GetSX][S] =   registerStatistic<uint64_t>("stateEvent_GetSX_S");
        stat_eventState[(int)Command::GetSX][M] =   registerStatistic<uint64_t>("stateEvent_GetSX_M");
        stat_eventState[(int)Command::GetSResp][IS] =       registerStatistic<uint64_t>("stateEvent_GetSResp_IS");
        stat_eventState[(int)Command::GetXResp][IS] =       registerStatistic<uint64_t>("stateEvent_GetXResp_IS");
        stat_eventState[(int)Command::GetXResp][IM] =       registerStatistic<uint64_t>("stateEvent_GetXResp_IM");
        stat_eventState[(int)Command::GetXResp][SM] =       registerStatistic<uint64_t>("stateEvent_GetXResp_SM");
        stat_eventState[(int)Command::GetXResp][SM_Inv] =   registerStatistic<uint64_t>("stateEvent_GetXResp_SMInv");
        stat_eventState[(int)Command::PutS][S] =        registerStatistic<uint64_t>("stateEvent_PutS_S");
        stat_eventState[(int)Command::PutS][M] =        registerStatistic<uint64_t>("stateEvent_PutS_M");
        stat_eventState[(int)Command::PutS][M_Inv] =    registerStatistic<uint64_t>("stateEvent_PutS_MInv");
        stat_eventState[(int)Command::PutS][S_Inv] =    registerStatistic<uint64_t>("stateEvent_PutS_SInv");
        stat_eventState[(int)Command::PutS][SM_Inv] =   registerStatistic<uint64_t>("stateEvent_PutS_SMInv");
        stat_eventState[(int)Command::PutS][S_B] =      registerStatistic<uint64_t>("stateEvent_PutS_SB");
        stat_eventState[(int)Command::PutS][SB_Inv] =   registerStatistic<uint64_t>("stateEvent_PutS_SBInv");
        stat_eventState[(int)Command::PutM][M] =        registerStatistic<uint64_t>("stateEvent_PutM_M");
        stat_eventState[(int)Command::PutM][M_Inv] =    registerStatistic<uint64_t>("stateEvent_PutM_MInv");
        stat_eventState[(int)Command::PutM][M_InvX] =   registerStatistic<uint64_t>("stateEvent_PutM_MInvX");
        stat_eventState[(int)Command::PutX][M] =        registerStatistic<uint64_t>("stateEvent_PutX_M");
        stat_eventState[(int)Command::PutX][M_Inv] =    registerStatistic<uint64_t>("stateEvent_PutX_MInv");
        stat_eventState[(int)Command::PutX][M_InvX] =   registerStatistic<uint64_t>("stateEvent_PutX_MInvX");
        stat_eventState[(int)Command::Inv][I] =         registerStatistic<uint64_t>("stateEvent_Inv_I");
        stat_eventState[(int)Command::Inv][S] =         registerStatistic<uint64_t>("stateEvent_Inv_S");
        stat_eventState[(int)Command::Inv][IS] =        registerStatistic<uint64_t>("stateEvent_Inv_IS");
        stat_eventState[(int)Command::Inv][IM] =        registerStatistic<uint64_t>("stateEvent_Inv_IM");
        stat_eventState[(int)Command::Inv][SM] =        registerStatistic<uint64_t>("stateEvent_Inv_SM");
        stat_eventState[(int)Command::Inv][S_B] =       registerStatistic<uint64_t>("stateEvent_Inv_SB");
        stat_eventState[(int)Command::Inv][I_B] =       registerStatistic<uint64_t>("stateEvent_Inv_IB");
        stat_eventState[(int)Command::FetchInvX][I] =   registerStatistic<uint64_t>("stateEvent_FetchInvX_I");
        stat_eventState[(int)Command::FetchInvX][M] =   registerStatistic<uint64_t>("stateEvent_FetchInvX_M");
        stat_eventState[(int)Command::FetchInvX][IS] =  registerStatistic<uint64_t>("stateEvent_FetchInvX_IS");
        stat_eventState[(int)Command::FetchInvX][IM] =  registerStatistic<uint64_t>("stateEvent_FetchInvX_IM");
        stat_eventState[(int)Command::FetchInvX][M_Inv] =   registerStatistic<uint64_t>("stateEvent_FetchInvX_MInv");
        stat_eventState[(int)Command::FetchInvX][M_InvX] =  registerStatistic<uint64_t>("stateEvent_FetchInvX_MInvX");
        stat_eventState[(int)Command::FetchInvX][I_B] = registerStatistic<uint64_t>("stateEvent_FetchInvX_IB");
        stat_eventState[(int)Command::FetchInvX][S_B] = registerStatistic<uint64_t>("stateEvent_FetchInvX_SB");
        stat_eventState[(int)Command::Fetch][I] =       registerStatistic<uint64_t>("stateEvent_Fetch_I");
        stat_eventState[(int)Command::Fetch][S] =       registerStatistic<uint64_t>("stateEvent_Fetch_S");
        stat_eventState[(int)Command::Fetch][IS] =      registerStatistic<uint64_t>("stateEvent_Fetch_IS");
        stat_eventState[(int)Command::Fetch][IM] =      registerStatistic<uint64_t>("stateEvent_Fetch_IM");
        stat_eventState[(int)Command::Fetch][SM] =      registerStatistic<uint64_t>("stateEvent_Fetch_SM");
        stat_eventState[(int)Command::Fetch][SM_Inv] =  registerStatistic<uint64_t>("stateEvent_Fetch_SMInv");
        stat_eventState[(int)Command::Fetch][S_Inv] =   registerStatistic<uint64_t>("stateEvent_Fetch_SInv");
        stat_eventState[(int)Command::Fetch][I_B] =     registerStatistic<uint64_t>("stateEvent_Fetch_IB");
        stat_eventState[(int)Command::Fetch][S_B] =     registerStatistic<uint64_t>("stateEvent_Fetch_SB");
        stat_eventState[(int)Command::FetchInv][I] =        registerStatistic<uint64_t>("stateEvent_FetchInv_I");
        stat_eventState[(int)Command::FetchInv][S] =        registerStatistic<uint64_t>("stateEvent_FetchInv_S");
        stat_eventState[(int)Command::FetchInv][M] =        registerStatistic<uint64_t>("stateEvent_FetchInv_M");
        stat_eventState[(int)Command::FetchInv][IS] =       registerStatistic<uint64_t>("stateEvent_FetchInv_IS");
        stat_eventState[(int)Command::FetchInv][IM] =       registerStatistic<uint64_t>("stateEvent_FetchInv_IM");
        stat_eventState[(int)Command::FetchInv][SM] =       registerStatistic<uint64_t>("stateEvent_FetchInv_SM");
        stat_eventState[(int)Command::FetchInv][SM_Inv] =   registerStatistic<uint64_t>("stateEvent_FetchInv_SMInv");
        stat_eventState[(int)Command::FetchInv][M_Inv] =    registerStatistic<uint64_t>("stateEvent_FetchInv_MInv");
        stat_eventState[(int)Command::FetchInv][M_InvX] =   registerStatistic<uint64_t>("stateEvent_FetchInv_MInvX");
        stat_eventState[(int)Command::FetchInv][I_B] =      registerStatistic<uint64_t>("stateEvent_FetchInv_IB");
        stat_eventState[(int)Command::FetchInv][S_B] =      registerStatistic<uint64_t>("stateEvent_FetchInv_SB");
        stat_eventState[(int)Command::ForceInv][I] =        registerStatistic<uint64_t>("stateEvent_ForceInv_I");
        stat_eventState[(int)Command::ForceInv][S] =        registerStatistic<uint64_t>("stateEvent_ForceInv_S");
        stat_eventState[(int)Command::ForceInv][M] =        registerStatistic<uint64_t>("stateEvent_ForceInv_M");
        stat_eventState[(int)Command::ForceInv][IS] =       registerStatistic<uint64_t>("stateEvent_ForceInv_IS");
        stat_eventState[(int)Command::ForceInv][IM] =       registerStatistic<uint64_t>("stateEvent_ForceInv_IM");
        stat_eventState[(int)Command::ForceInv][SM] =       registerStatistic<uint64_t>("stateEvent_ForceInv_SM");
        stat_eventState[(int)Command::ForceInv][S_Inv] =    registerStatistic<uint64_t>("stateEvent_ForceInv_SInv");
        stat_eventState[(int)Command::ForceInv][SM_Inv] =   registerStatistic<uint64_t>("stateEvent_ForceInv_SMInv");
        stat_eventState[(int)Command::ForceInv][M_Inv] =    registerStatistic<uint64_t>("stateEvent_ForceInv_MInv");
        stat_eventState[(int)Command::ForceInv][M_InvX] =   registerStatistic<uint64_t>("stateEvent_ForceInv_MInvX");
        stat_eventState[(int)Command::ForceInv][I_B] =      registerStatistic<uint64_t>("stateEvent_ForceInv_IB");
        stat_eventState[(int)Command::ForceInv][S_B] =      registerStatistic<uint64_t>("stateEvent_ForceInv_SB");
        stat_eventState[(int)Command::FetchResp][M_Inv] =   registerStatistic<uint64_t>("stateEvent_FetchResp_MInv");
        stat_eventState[(int)Command::FetchXResp][M_InvX] = registerStatistic<uint64_t>("stateEvent_FetchXResp_MInvX");
        stat_eventState[(int)Command::AckInv][M_Inv] =      registerStatistic<uint64_t>("stateEvent_AckInv_MInv");
        stat_eventState[(int)Command::AckInv][S_Inv] =      registerStatistic<uint64_t>("stateEvent_AckInv_SInv");
        stat_eventState[(int)Command::AckInv][SM_Inv] =     registerStatistic<uint64_t>("stateEvent_AckInv_SMInv");
        stat_eventState[(int)Command::AckInv][SB_Inv] =     registerStatistic<uint64_t>("stateEvent_AckInv_SBInv");
        stat_eventState[(int)Command::FlushLine][I] =       registerStatistic<uint64_t>("stateEvent_FlushLine_I");
        stat_eventState[(int)Command::FlushLine][S] =       registerStatistic<uint64_t>("stateEvent_FlushLine_S");
        stat_eventState[(int)Command::FlushLine][M] =       registerStatistic<uint64_t>("stateEvent_FlushLine_M");
        stat_eventState[(int)Command::FlushLineInv][I] =    registerStatistic<uint64_t>("stateEvent_FlushLineInv_I");
        stat_eventState[(int)Command::FlushLineInv][S] =    registerStatistic<uint64_t>("stateEvent_FlushLineInv_S");
        stat_eventState[(int)Command::FlushLineInv][M] =    registerStatistic<uint64_t>("stateEvent_FlushLineInv_M");
        stat_eventState[(int)Command::FlushLineResp][I] =       registerStatistic<uint64_t>("stateEvent_FlushLineResp_I");
        stat_eventState[(int)Command::FlushLineResp][I_B] =     registerStatistic<uint64_t>("stateEvent_FlushLineResp_IB");
        stat_eventState[(int)Command::FlushLineResp][S_B] =     registerStatistic<uint64_t>("stateEvent_FlushLineResp_SB");
        stat_eventSent[(int)Command::GetS]          = registerStatistic<uint64_t>("eventSent_GetS");
        stat_eventSent[(int)Command::GetX]          = registerStatistic<uint64_t>("eventSent_GetX");
        stat_eventSent[(int)Command::GetSX]         = registerStatistic<uint64_t>("eventSent_GetSX");
        stat_eventSent[(int)Command::Write]         = registerStatistic<uint64_t>("eventSent_Write");
        stat_eventSent[(int)Command::PutS]          = registerStatistic<uint64_t>("eventSent_PutS");
        stat_eventSent[(int)Command::PutM]          = registerStatistic<uint64_t>("eventSent_PutM");
        stat_eventSent[(int)Command::FlushLine]     = registerStatistic<uint64_t>("eventSent_FlushLine");
        stat_eventSent[(int)Command::FlushLineInv]  = registerStatistic<uint64_t>("eventSent_FlushLineInv");
        stat_eventSent[(int)Command::FetchResp]     = registerStatistic<uint64_t>("eventSent_FetchResp");
        stat_eventSent[(int)Command::FetchXResp]    = registerStatistic<uint64_t>("eventSent_FetchXResp");
        stat_eventSent[(int)Command::AckInv]        = registerStatistic<uint64_t>("eventSent_AckInv");
        stat_eventSent[(int)Command::NACK]          = registerStatistic<uint64_t>("eventSent_NACK");
        stat_eventSent[(int)Command::GetSResp]      = registerStatistic<uint64_t>("eventSent_GetSResp");
        stat_eventSent[(int)Command::GetXResp]      = registerStatistic<uint64_t>("eventSent_GetXResp");
        stat_eventSent[(int)Command::WriteResp]     = registerStatistic<uint64_t>("eventSent_WriteResp");
        stat_eventSent[(int)Command::FlushLineResp] = registerStatistic<uint64_t>("eventSent_FlushLineResp");
        stat_eventSent[(int)Command::Fetch]         = registerStatistic<uint64_t>("eventSent_Fetch");
        stat_eventSent[(int)Command::FetchInv]      = registerStatistic<uint64_t>("eventSent_FetchInv");
        stat_eventSent[(int)Command::ForceInv]      = registerStatistic<uint64_t>("eventSent_ForceInv");
        stat_eventSent[(int)Command::FetchInvX]     = registerStatistic<uint64_t>("eventSent_FetchInvX");
        stat_eventSent[(int)Command::Inv]           = registerStatistic<uint64_t>("eventSent_Inv");
        stat_eventSent[(int)Command::Put]           = registerStatistic<uint64_t>("eventSent_Put");
        stat_eventSent[(int)Command::Get]           = registerStatistic<uint64_t>("eventSent_Get");
        stat_eventSent[(int)Command::AckMove]       = registerStatistic<uint64_t>("eventSent_AckMove");
        stat_eventSent[(int)Command::CustomReq]     = registerStatistic<uint64_t>("eventSent_CustomReq");
        stat_eventSent[(int)Command::CustomResp]    = registerStatistic<uint64_t>("eventSent_CustomResp");
        stat_eventSent[(int)Command::CustomAck]     = registerStatistic<uint64_t>("eventSent_CustomAck");
        stat_latencyGetS[LatType::HIT]       = registerStatistic<uint64_t>("latency_GetS_hit");
        stat_latencyGetS[LatType::MISS]      = registerStatistic<uint64_t>("latency_GetS_miss");
        stat_latencyGetS[LatType::INV]       = registerStatistic<uint64_t>("latency_GetS_inv");
        stat_latencyGetX[LatType::HIT]       = registerStatistic<uint64_t>("latency_GetX_hit");
        stat_latencyGetX[LatType::MISS]      = registerStatistic<uint64_t>("latency_GetX_miss");
        stat_latencyGetX[LatType::INV]       = registerStatistic<uint64_t>("latency_GetX_inv");
        stat_latencyGetX[LatType::UPGRADE]   = registerStatistic<uint64_t>("latency_GetX_upgrade");
        stat_latencyGetSX[LatType::HIT]      = registerStatistic<uint64_t>("latency_GetSX_hit");
        stat_latencyGetSX[LatType::MISS]     = registerStatistic<uint64_t>("latency_GetSX_miss");
        stat_latencyGetSX[LatType::INV]      = registerStatistic<uint64_t>("latency_GetSX_inv");
        stat_latencyGetSX[LatType::UPGRADE]  = registerStatistic<uint64_t>("latency_GetSX_upgrade");
        stat_latencyFlushLine       = registerStatistic<uint64_t>("latency_FlushLine");
        stat_latencyFlushLineInv    = registerStatistic<uint64_t>("latency_FlushLineInv");
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

        /* Prefetch statistics */
        if (prefetch) {
            statPrefetchEvict = registerStatistic<uint64_t>("prefetch_evict");
            statPrefetchInv = registerStatistic<uint64_t>("prefetch_inv");
            statPrefetchHit = registerStatistic<uint64_t>("prefetch_useful");
            statPrefetchUpgradeMiss = registerStatistic<uint64_t>("prefetch_coherence_miss");
            statPrefetchRedundant = registerStatistic<uint64_t>("prefetch_redundant");
        }

        /* Only for caches that expect writeback acks but we don't know yet so always enabled for now (can't register statistics later) */
        stat_eventState[(int)Command::AckPut][I] = registerStatistic<uint64_t>("stateEvent_AckPut_I");

        /* MESI-specific statistics (as opposed to MSI) */
        if (protocol_) {
            stat_evict[E] =         registerStatistic<uint64_t>("evict_E");
            stat_evict[E_Inv] =     registerStatistic<uint64_t>("evict_EInv");
            stat_evict[E_InvX] =    registerStatistic<uint64_t>("evict_EInvX");
            stat_eventState[(int)Command::GetS][E] =        registerStatistic<uint64_t>("stateEvent_GetS_E");
            stat_eventState[(int)Command::GetX][E] =        registerStatistic<uint64_t>("stateEvent_GetX_E");
            stat_eventState[(int)Command::GetSX][E] =       registerStatistic<uint64_t>("stateEvent_GetSX_E");
            stat_eventState[(int)Command::PutS][E] =        registerStatistic<uint64_t>("stateEvent_PutS_E");
            stat_eventState[(int)Command::PutS][E_Inv] =    registerStatistic<uint64_t>("stateEvent_PutS_EInv");
            stat_eventState[(int)Command::PutE][E] =        registerStatistic<uint64_t>("stateEvent_PutE_E");
            stat_eventState[(int)Command::PutE][M] =        registerStatistic<uint64_t>("stateEvent_PutE_M");
            stat_eventState[(int)Command::PutE][M_Inv] =    registerStatistic<uint64_t>("stateEvent_PutE_MInv");
            stat_eventState[(int)Command::PutE][M_InvX] =   registerStatistic<uint64_t>("stateEvent_PutE_MInvX");
            stat_eventState[(int)Command::PutE][E_Inv] =    registerStatistic<uint64_t>("stateEvent_PutE_EInv");
            stat_eventState[(int)Command::PutE][E_InvX] =   registerStatistic<uint64_t>("stateEvent_PutE_EInvX");
            stat_eventState[(int)Command::PutM][E] =        registerStatistic<uint64_t>("stateEvent_PutM_E");
            stat_eventState[(int)Command::PutM][E_Inv] =    registerStatistic<uint64_t>("stateEvent_PutM_EInv");
            stat_eventState[(int)Command::PutM][E_InvX] =   registerStatistic<uint64_t>("stateEvent_PutM_EInvX");
            stat_eventState[(int)Command::PutX][E] =        registerStatistic<uint64_t>("stateEvent_PutX_E");
            stat_eventState[(int)Command::PutX][E_Inv] =    registerStatistic<uint64_t>("stateEvent_PutX_EInv");
            stat_eventState[(int)Command::PutX][E_InvX] =   registerStatistic<uint64_t>("stateEvent_PutX_EInvX");
            stat_eventState[(int)Command::FetchInvX][E] =   registerStatistic<uint64_t>("stateEvent_FetchInvX_E");
            stat_eventState[(int)Command::FetchInvX][E_Inv] =   registerStatistic<uint64_t>("stateEvent_FetchInvX_EInv");
            stat_eventState[(int)Command::FetchInvX][E_InvX] =  registerStatistic<uint64_t>("stateEvent_FetchInvX_EInvX");
            stat_eventState[(int)Command::FetchInv][E] =        registerStatistic<uint64_t>("stateEvent_FetchInv_E");
            stat_eventState[(int)Command::FetchInv][E_Inv] =    registerStatistic<uint64_t>("stateEvent_FetchInv_EInv");
            stat_eventState[(int)Command::FetchInv][E_InvX] =   registerStatistic<uint64_t>("stateEvent_FetchInv_EInvX");
            stat_eventState[(int)Command::ForceInv][E] =        registerStatistic<uint64_t>("stateEvent_ForceInv_E");
            stat_eventState[(int)Command::ForceInv][E_Inv] =    registerStatistic<uint64_t>("stateEvent_ForceInv_EInv");
            stat_eventState[(int)Command::ForceInv][E_InvX] =   registerStatistic<uint64_t>("stateEvent_ForceInv_EInvX");
            stat_eventState[(int)Command::FetchResp][E_Inv] =   registerStatistic<uint64_t>("stateEvent_FetchResp_EInv");
            stat_eventState[(int)Command::FetchXResp][E_InvX] = registerStatistic<uint64_t>("stateEvent_FetchXResp_EInvX");
            stat_eventState[(int)Command::AckInv][E_Inv] =      registerStatistic<uint64_t>("stateEvent_AckInv_EInv");
            stat_eventState[(int)Command::FlushLine][E] =       registerStatistic<uint64_t>("stateEvent_FlushLine_E");
            stat_eventState[(int)Command::FlushLineInv][E] =    registerStatistic<uint64_t>("stateEvent_FlushLineInv_E");
            stat_eventSent[(int)Command::PutE] = registerStatistic<uint64_t>("eventSent_PutE");
        }
    }
    ~MESIInclusive() {}

    /** Event handlers **/
    virtual bool handleGetS(MemEvent * event, bool inMSHR);
    virtual bool handleGetX(MemEvent * event, bool inMSHR);
    virtual bool handleGetSX(MemEvent * event, bool inMSHR);
    virtual bool handleFlushLine(MemEvent * event, bool inMSHR);
    virtual bool handleFlushLineInv(MemEvent * event, bool inMSHR);
    virtual bool handlePutS(MemEvent * event, bool inMSHR);
    virtual bool handlePutX(MemEvent * event, bool inMSHR);
    virtual bool handlePutE(MemEvent * event, bool inMSHR);
    virtual bool handlePutM(MemEvent * event, bool inMSHR);
    virtual bool handleInv(MemEvent * event, bool inMSHR);
    virtual bool handleForceInv(MemEvent * event, bool inMSHR);
    virtual bool handleFetch(MemEvent * event, bool inMSHR);
    virtual bool handleFetchInv(MemEvent * event, bool inMSHR);
    virtual bool handleFetchInvX(MemEvent * event, bool inMSHR);
    virtual bool handleGetSResp(MemEvent * event, bool inMSHR);
    virtual bool handleGetXResp(MemEvent * event, bool inMSHR);
    virtual bool handleFlushLineResp(MemEvent * event, bool inMSHR);
    virtual bool handleFetchResp(MemEvent * event, bool inMSHR);
    virtual bool handleFetchXResp(MemEvent * event, bool inMSHR);
    virtual bool handleAckInv(MemEvent * event, bool inMSHR);
    virtual bool handleAckPut(MemEvent * event, bool inMSHR);
    virtual bool handleNACK(MemEvent * event, bool inMSHR);
    virtual bool handleNULLCMD(MemEvent * event, bool inMSHR);

    /** Cache interface **/
    virtual Addr getBank(Addr addr) { return cacheArray_->getBank(addr); }
    virtual void setSliceAware(uint64_t size, uint64_t step) { cacheArray_->setSliceAware(size, step); }

    /** Initialization **/
    MemEventInitCoherence * getInitCoherenceEvent();

    std::set<Command> getValidReceiveEvents() {
        std::set<Command> cmds = { Command::GetS,
            Command::GetX,
            Command::GetSX,
            Command::Write,
            Command::FlushLine,
            Command::FlushLineInv,
            Command::PutS,
            Command::PutE,
            Command::PutX,
            Command::PutM,
            Command::Inv,
            Command::ForceInv,
            Command::Fetch,
            Command::FetchInv,
            Command::FetchInvX,
            Command::NULLCMD,
            Command::GetSResp,
            Command::GetXResp,
            Command::WriteResp,
            Command::FlushLineResp,
            Command::FetchResp,
            Command::FetchXResp,
            Command::AckInv,
            Command::AckPut,
            Command::NACK };
        return cmds;
    }

    void printStatus(Output &out);

private:

    MemEventStatus processCacheMiss(MemEvent * event, SharedCacheLine * line, bool inMSHR);
    SharedCacheLine * allocateLine(MemEvent * event, SharedCacheLine * line);
    bool handleEviction(Addr addr, SharedCacheLine *& line);
    void cleanUpAfterRequest(MemEvent * event, bool inMSHR);
    void cleanUpAfterResponse(MemEvent * event, bool inMSHR);
    void cleanUpEvent(MemEvent * event, bool inMSHR);
    void retry(Addr addr);

    /** Invalidation **/
    bool invalidateExceptRequestor(MemEvent * event, SharedCacheLine * line, bool inMSHR);
    bool invalidateAll(MemEvent * event, SharedCacheLine * line, bool inMSHR, Command cmd = Command::NULLCMD);
    uint64_t invalidateSharer(std::string shr, MemEvent * event, SharedCacheLine * line, bool inMSHR, Command cmd = Command::Inv);
    bool invalidateOwner(MemEvent * event, SharedCacheLine * line, bool inMSHR, Command cmd = Command::FetchInv);

    /** Forward flush line request, with or without data */
    void forwardFlush(MemEvent * event, SharedCacheLine * line, bool data);

    /** Send response up (towards processor) */
    SimTime_t sendResponseUp(MemEvent * event, vector<uint8_t>* data, bool inMSHR, uint64_t time, Command cmd = Command::NULLCMD, bool success = true);

    /** Send response down (towards memory) */
    void sendResponseDown(MemEvent * event, SharedCacheLine * line, bool data, bool evict);

    /** Send writeback */
    void sendWriteback(Command cmd, SharedCacheLine * line, bool dirty);

    /** Send AckPut */
    void sendAckPut(MemEvent * event);

    /** Downgrade request */
    void downgradeOwner(MemEvent * event, SharedCacheLine * line, bool inMSHR);

    /** Evict a block */
    State doEviction(MemEvent * event, SharedCacheLine * line, State state);

    /** Call through to coherenceController with statistic recording */
    void forwardByAddress(MemEventBase* ev, Cycle_t timestamp);
    void forwardByDestination(MemEventBase* ev, Cycle_t timestamp);

/* Miscellaneous functions */
    /* Record prefetch statistics. Line cannot be null. */
    void recordPrefetchResult(SharedCacheLine * line, Statistic<uint64_t> * stat);

    /* Record latency */
    void recordLatency(Command cmd, int type, uint64_t latency);

    void printLine(Addr addr);

/* Variables */
    CacheArray<SharedCacheLine> * cacheArray_;
    State protocolState_;       // State to transition to on exclusive response to read/shared request
    bool protocol_;             // True for MESI, false for MSI

    std::map<Addr, std::map<std::string, MemEvent::id_type> > responses;

    /* Statistics */
    Statistic<uint64_t>* stat_latencyGetS[3]; // HIT, MISS, INV
    Statistic<uint64_t>* stat_latencyGetX[4];
    Statistic<uint64_t>* stat_latencyGetSX[4];
    Statistic<uint64_t>* stat_latencyFlushLine;
    Statistic<uint64_t>* stat_latencyFlushLineInv;
    Statistic<uint64_t>* stat_hit[3][2];
    Statistic<uint64_t>* stat_miss[3][2];
    Statistic<uint64_t>* stat_hits;
    Statistic<uint64_t>* stat_misses;
};


}}


#endif

