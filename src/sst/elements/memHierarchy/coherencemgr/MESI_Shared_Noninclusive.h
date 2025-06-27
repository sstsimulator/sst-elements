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


#ifndef MESIINTERNALDIRCONTROLLER_H
#define MESIINTERNALDIRCONTROLLER_H

#include <iostream>
#include <array>

#include "sst/elements/memHierarchy/coherencemgr/coherenceController.h"
#include "sst/elements/memHierarchy/lineTypes.h"
#include "sst/elements/memHierarchy/cacheArray.h"

namespace SST { namespace MemHierarchy {

class MESISharNoninclusive : public CoherenceController {
public:
    SST_ELI_REGISTER_SUBCOMPONENT(MESISharNoninclusive, "memHierarchy", "coherence.mesi_shared_noninclusive", SST_ELI_ELEMENT_VERSION(1,0,0),
            "Implements MESI or MSI coherence for cache that is co-located with a directory, for noninclusive last-level caches", SST::MemHierarchy::CoherenceController)

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
        {"eventSent_PutX",          "Number of PutX requests sent", "events", 2},
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
        {"eventSent_FlushAll",      "Number of FlushAll requests sent", "events", 2},
        {"eventSent_ForwardFlush",  "Number of ForwardFlush requests sent", "events", 2},
        {"eventSent_FlushAllResp",  "Number of FlushAllResp requests sent", "events", 2},
        {"eventSent_UnblockFlush",  "Number of UnblockFlush requests sent", "events", 2},
        {"eventSent_AckFlush",      "Number of AckFlush requests sent", "events", 2},
        {"eventSent_Put",           "Number of Put requests sent", "events", 6},
        {"eventSent_Get",           "Number of Get requests sent", "events", 6},
        {"eventSent_AckMove",       "Number of AckMove responses sent", "events", 6},
        {"eventSent_CustomReq",     "Number of CustomReq requests sent", "events", 4},
        {"eventSent_CustomResp",    "Number of CustomResp responses sent", "events", 4},
        {"eventSent_CustomAck",     "Number of CustomAck responses sent", "events", 4},
        /* Event/State combinations - Count how many times an event was seen in particular state */
        {"stateEvent_GetS_I",           "Event/State: Number of times a GetS was seen in state I (Miss)", "count", 3},
        {"stateEvent_GetS_IA",          "Event/State: Number of times a GetS was seen in state IA (Miss)", "count", 3},
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
        {"stateEvent_PutS_SD",          "Event/State: Number of times a PutS was seen in state S_D", "count", 3},
        {"stateEvent_PutS_ED",          "Event/State: Number of times a PutS was seen in state E_D", "count", 3},
        {"stateEvent_PutS_MD",          "Event/State: Number of times a PutS was seen in state M_D", "count", 3},
        {"stateEvent_PutS_EB",          "Event/State: Number of times a PutS was seen in state E_B", "count", 3},
        {"stateEvent_PutS_MB",          "Event/State: Number of times a PutS was seen in state M_B", "count", 3},
        {"stateEvent_PutS_SBD",         "Event/State: Number of times a PutS was seen in state SB_D", "count", 3},
        {"stateEvent_PutS_SInv",        "Event/State: Number of times a PutS was seen in state S_Inv", "count", 3},
        {"stateEvent_PutS_EInv",        "Event/State: Number of times a PutS was seen in state E_Inv", "count", 3},
        {"stateEvent_PutS_MInv",        "Event/State: Number of times a PutS was seen in state M_Inv", "count", 3},
        {"stateEvent_PutS_SMInv",       "Event/State: Number of times a PutS was seen in state SM_Inv", "count", 3},
        {"stateEvent_PutS_SB",          "Event/State: Number of times a PutS was seen in state S_B", "count", 3},
        {"stateEvent_PutE_E",           "Event/State: Number of times a PutE was seen in state E", "count", 3},
        {"stateEvent_PutE_M",           "Event/State: Number of times a PutE was seen in state M", "count", 3},
        {"stateEvent_PutE_EInv",        "Event/State: Number of times a PutE was seen in state E_Inv", "count", 3},
        {"stateEvent_PutE_MInv",        "Event/State: Number of times a PutE was seen in state M_Inv", "count", 3},
        {"stateEvent_PutE_EInvX",       "Event/State: Number of times a PutE was seen in state E_InvX", "count", 3},
        {"stateEvent_PutE_MInvX",       "Event/State: Number of times a PutE was seen in state M_InvX", "count", 3},
        {"stateEvent_PutX_E",           "Event/State: Number of times a PutX was seen in state E", "count", 3},
        {"stateEvent_PutX_M",           "Event/State: Number of times a PutX was seen in state M", "count", 3},
        {"stateEvent_PutX_EInv",        "Event/State: Number of times a PutX was seen in state EInv", "count", 3},
        {"stateEvent_PutX_MInv",        "Event/State: Number of times a PutX was seen in state MInv", "count", 3},
        {"stateEvent_PutM_E",           "Event/State: Number of times a PutM was seen in state E", "count", 3},
        {"stateEvent_PutM_M",           "Event/State: Number of times a PutM was seen in state M", "count", 3},
        {"stateEvent_PutM_EInv",        "Event/State: Number of times a PutM was seen in state E_Inv", "count", 3},
        {"stateEvent_PutM_MInv",        "Event/State: Number of times a PutM was seen in state M_Inv", "count", 3},
        {"stateEvent_PutM_EInvX",       "Event/State: Number of times a PutM was seen in state E_InvX", "count", 3},
        {"stateEvent_PutM_MInvX",       "Event/State: Number of times a PutM was seen in state M_InvX", "count", 3},
        {"stateEvent_Inv_I",            "Event/State: Number of times an Inv was seen in state I", "count", 3},
        {"stateEvent_Inv_IB",           "Event/State: Number of times an Inv was seen in state I_B", "count", 3},
        {"stateEvent_Inv_S",            "Event/State: Number of times an Inv was seen in state S", "count", 3},
        {"stateEvent_Inv_SM",           "Event/State: Number of times an Inv was seen in state SM", "count", 3},
        {"stateEvent_Inv_SInv",         "Event/State: Number of times an Inv was seen in state S_Inv", "count", 3},
        {"stateEvent_Inv_SMInv",        "Event/State: Number of times an Inv was seen in state SM_Inv", "count", 3},
        {"stateEvent_Inv_SB",           "Event/State: Number of times an Inv was seen in state S_B", "count", 3},
        {"stateEvent_FetchInv_I",       "Event/State: Number of times a FetchInv was seen in state I", "count", 3},
        {"stateEvent_FetchInv_S",       "Event/State: Number of times a FetchInv was seen in state S", "count", 3},
        {"stateEvent_FetchInv_E",       "Event/State: Number of times a FetchInv was seen in state E", "count", 3},
        {"stateEvent_FetchInv_M",       "Event/State: Number of times a FetchInv was seen in state M", "count", 3},
        {"stateEvent_FetchInv_SInv",    "Event/State: Number of times a FetchInv was seen in state S_Inv", "count", 3},
        {"stateEvent_FetchInv_EInv",    "Event/State: Number of times a FetchInv was seen in state E_Inv", "count", 3},
        {"stateEvent_FetchInv_MInv",    "Event/State: Number of times a FetchInv was seen in state M_Inv", "count", 3},
        {"stateEvent_FetchInv_SM",      "Event/State: Number of times a FetchInv was seen in state SM", "count", 3},
        {"stateEvent_FetchInv_SMInv",   "Event/State: Number of times a FetchInv was seen in state SM_Inv", "count", 3},
        {"stateEvent_FetchInv_SB",      "Event/State: Number of times a FetchInv was seen in state S_B", "count", 3},
        {"stateEvent_FetchInv_EB",      "Event/State: Number of times a FetchInv was seen in state E_B", "count", 3},
        {"stateEvent_FetchInv_MB",      "Event/State: Number of times a FetchInv was seen in state M_B", "count", 3},
        {"stateEvent_FetchInv_SA",      "Event/State: Number of times a FetchInv was seen in state S_A", "count", 3},
        {"stateEvent_FetchInv_EA",      "Event/State: Number of times a FetchInv was seen in state E_A", "count", 3},
        {"stateEvent_FetchInv_MA",      "Event/State: Number of times a FetchInv was seen in state M_A", "count", 3},
        {"stateEvent_FetchInvX_I",      "Event/State: Number of times a FetchInvX was seen in state I", "count", 3},
        {"stateEvent_FetchInvX_E",      "Event/State: Number of times a FetchInvX was seen in state E", "count", 3},
        {"stateEvent_FetchInvX_M",      "Event/State: Number of times a FetchInvX was seen in state M", "count", 3},
        {"stateEvent_FetchInvX_IB",     "Event/State: Number of times a FetchInvX was seen in state I_B", "count", 3},
        {"stateEvent_FetchInvX_EB",     "Event/State: Number of times a FetchInvX was seen in state E_B", "count", 3},
        {"stateEvent_FetchInvX_MB",     "Event/State: Number of times a FetchInvX was seen in state M_B", "count", 3},
        {"stateEvent_FetchInvX_EA",     "Event/State: Number of times a FetchInvX was seen in state E_A", "count", 3},
        {"stateEvent_FetchInvX_MA",     "Event/State: Number of times a FetchInvX was seen in state M_A", "count", 3},
        {"stateEvent_ForceInv_I",       "Event/State: Number of times a ForceInv was seen in state I", "count", 3},
        {"stateEvent_ForceInv_IB",      "Event/State: Number of times a ForceInv was seen in state I_B", "count", 3},
        {"stateEvent_ForceInv_S",       "Event/State: Number of times a ForceInv was seen in state S", "count", 3},
        {"stateEvent_ForceInv_E",       "Event/State: Number of times a ForceInv was seen in state E", "count", 3},
        {"stateEvent_ForceInv_M",       "Event/State: Number of times a ForceInv was seen in state M", "count", 3},
        {"stateEvent_ForceInv_SB",      "Event/State: Number of times a ForceInv was seen in state S_B", "count", 3},
        {"stateEvent_ForceInv_EB",      "Event/State: Number of times a ForceInv was seen in state E_B", "count", 3},
        {"stateEvent_ForceInv_MB",      "Event/State: Number of times a ForceInv was seen in state M_B", "count", 3},
        {"stateEvent_ForceInv_SInv",    "Event/State: Number of times a ForceInv was seen in state S_Inv", "count", 3},
        {"stateEvent_ForceInv_EInv",    "Event/State: Number of times a ForceInv was seen in state E_Inv", "count", 3},
        {"stateEvent_ForceInv_MInv",    "Event/State: Number of times a ForceInv was seen in state M_Inv", "count", 3},
        {"stateEvent_ForceInv_SM",      "Event/State: Number of times a ForceInv was seen in state SM", "count", 3},
        {"stateEvent_ForceInv_SMInv",   "Event/State: Number of times a ForceInv was seen in state SM_Inv", "count", 3},
        {"stateEvent_ForceInv_SA",      "Event/State: Number of times a ForceInv was seen in state S_A", "count", 3},
        {"stateEvent_ForceInv_EA",      "Event/State: Number of times a ForceInv was seen in state E_A", "count", 3},
        {"stateEvent_ForceInv_MA",      "Event/State: Number of times a ForceInv was seen in state M_A", "count", 3},
        {"stateEvent_Fetch_I",          "Event/State: Number of times a Fetch was seen in state I", "count", 3},
        {"stateEvent_Fetch_S",          "Event/State: Number of times a Fetch was seen in state S", "count", 3},
        {"stateEvent_Fetch_SM",         "Event/State: Number of times a Fetch was seen in state SM", "count", 3},
        {"stateEvent_Fetch_IB",         "Event/State: Number of times a Fetch was seen in state I_B", "count", 3},
        {"stateEvent_Fetch_SB",         "Event/State: Number of times a Fetch was seen in state S_B", "count", 3},
        {"stateEvent_Fetch_EB",         "Event/State: Number of times a Fetch was seen in state E_B", "count", 3},
        {"stateEvent_Fetch_MB",         "Event/State: Number of times a Fetch was seen in state M_B", "count", 3},
        {"stateEvent_Fetch_SA",         "Event/State: Number of times a Fetch was seen in state S_A", "count", 3},
        {"stateEvent_FetchResp_SInv",   "Event/State: Number of times a FetchResp was seen in state S_Inv", "count", 3},
        {"stateEvent_FetchResp_SMInv",  "Event/State: Number of times a FetchResp was seen in state SM_Inv", "count", 3},
        {"stateEvent_FetchResp_EInv",   "Event/State: Number of times a FetchResp was seen in state E_Inv", "count", 3},
        {"stateEvent_FetchResp_EInvX",  "Event/State: Number of times a FetchResp was seen in state E_Inv", "count", 3},
        {"stateEvent_FetchResp_MInv",   "Event/State: Number of times a FetchResp was seen in state M_Inv", "count", 3},
        {"stateEvent_FetchResp_MInvX",  "Event/State: Number of times a FetchResp was seen in state M_InvX", "count", 3},
        {"stateEvent_FetchResp_SD",     "Event/State: Number of times a FetchResp was seen in state S_D", "count", 3},
        {"stateEvent_FetchResp_SMD",    "Event/State: Number of times a FetchResp was seen in state SM_D", "count", 3},
        {"stateEvent_FetchResp_ED",     "Event/State: Number of times a FetchResp was seen in state E_D", "count", 3},
        {"stateEvent_FetchResp_MD",     "Event/State: Number of times a FetchResp was seen in state M_D", "count", 3},
        {"stateEvent_FetchResp_SBD",    "Event/State: Number of times a FetchResp was seen in state SB_D", "count", 3},
        {"stateEvent_FetchResp_SBInv",  "Event/State: Number of times a FetchResp was seen in state SB_Inv", "count", 3},
        {"stateEvent_FetchXResp_EInvX", "Event/State: Number of times a FetchXResp was seen in state E_InvX", "count", 3},
        {"stateEvent_FetchXResp_MInvX", "Event/State: Number of times a FetchXResp was seen in state M_InvX", "count", 3},
        {"stateEvent_AckInv_I",         "Event/State: Number of times an AckInv was seen in state I", "count", 3},
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
        {"stateEvent_FlushLine_SMD",    "Event/State: Number of times a FlushLine was seen in state SM_D", "count", 3},
        {"stateEvent_FlushLineInv_I",       "Event/State: Number of times a FlushLineInv was seen in state I", "count", 3},
        {"stateEvent_FlushLineInv_S",       "Event/State: Number of times a FlushLineInv was seen in state S", "count", 3},
        {"stateEvent_FlushLineInv_E",       "Event/State: Number of times a FlushLineInv was seen in state E", "count", 3},
        {"stateEvent_FlushLineInv_M",       "Event/State: Number of times a FlushLineInv was seen in state M", "count", 3},
        {"stateEvent_FlushLineInv_EB",      "Event/State: Number of times a FlushLineInv was seen in state E_B", "count", 3},
        {"stateEvent_FlushLineInv_MB",      "Event/State: Number of times a FlushLineInv was seen in state M_B", "count", 3},
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
        {"evict_ED",                "Eviction: Attempted to evict a block in state E_D", "count", 3},
        {"evict_MA",                "Eviction: Attempted to evict a block in state MA", "count", 3},
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
    MESISharNoninclusive(ComponentId_t id, Params& params, Params& ownerParams, bool prefetch);
    MESISharNoninclusive() = default; // Serialization
    ~MESISharNoninclusive() {}

    /** Event handlers - called by controller */
    virtual bool handleGetS(MemEvent* event, bool inMSHR) override;
    virtual bool handleGetX(MemEvent* event, bool inMSHR) override;
    virtual bool handleGetSX(MemEvent* event, bool inMSHR) override;
    virtual bool handleFlushLine(MemEvent* event, bool inMSHR) override;
    virtual bool handleFlushLineInv(MemEvent* event, bool inMSHR) override;
    virtual bool handleFlushAll(MemEvent* event, bool inMSHR) override;
    virtual bool handleForwardFlush(MemEvent* event, bool inMSHR) override;
    virtual bool handlePutS(MemEvent* event, bool inMSHR) override;
    virtual bool handlePutE(MemEvent* event, bool inMSHR) override;
    virtual bool handlePutX(MemEvent* event, bool inMSHR) override;
    virtual bool handlePutM(MemEvent* event, bool inMSHR) override;
    virtual bool handleInv(MemEvent * event, bool inMSHR) override;
    virtual bool handleForceInv(MemEvent * event, bool inMSHR) override;
    virtual bool handleFetch(MemEvent * event, bool inMSHR) override;
    virtual bool handleFetchInv(MemEvent * event, bool inMSHR) override;
    virtual bool handleFetchInvX(MemEvent * event, bool inMSHR) override;
    virtual bool handleNULLCMD(MemEvent* event, bool inMSHR) override;
    virtual bool handleGetSResp(MemEvent* event, bool inMSHR) override;
    virtual bool handleGetXResp(MemEvent* event, bool inMSHR) override;
    virtual bool handleFlushLineResp(MemEvent* event, bool inMSHR) override;
    virtual bool handleFlushAllResp(MemEvent* event, bool inMSHR) override;
    virtual bool handleFetchResp(MemEvent* event, bool inMSHR) override;
    virtual bool handleFetchXResp(MemEvent* event, bool inMSHR) override;
    virtual bool handleAckFlush(MemEvent* event, bool inMSHR) override;
    virtual bool handleUnblockFlush(MemEvent* event, bool inMSHR) override;
    virtual bool handleAckInv(MemEvent* event, bool inMSHR) override;
    virtual bool handleAckPut(MemEvent* event, bool inMSHR) override;
    virtual bool handleNACK(MemEvent* event, bool inMSHR) override;
    
    /** Bank conflict detection - used by controller */
    Addr getBank(Addr addr) override { return dirArray_->getBank(addr); }

    /** Configuration */
    MemEventInitCoherence* getInitCoherenceEvent() override;
    std::set<Command> getValidReceiveEvents() override;
    void setSliceAware(uint64_t size, uint64_t step) override {
        dirArray_->setSliceAware(size, step);
        dataArray_->setSliceAware(size, step);
    }

    /** Status output */
    //void printStatus(Output &out);

    /** Serialization */
    void serialize_order(SST::Core::Serialization::serializer& ser) override {}
    ImplementSerializable(SST::MemHierarchy::MESISharNoninclusive);

private:
    /** Cache and MSHR management */
    MemEventStatus processDirectoryMiss(MemEvent * event, DirectoryLine * line, bool inMSHR);
    MemEventStatus processDataMiss(MemEvent * event, DirectoryLine * tag, DataLine * data, bool inMSHR);
    DirectoryLine * allocateDirLine(MemEvent * event, DirectoryLine * line);
    DataLine * allocateDataLine(MemEvent * event, DataLine * line);
    bool handleDirEviction(Addr addr, DirectoryLine* &line);
    bool handleDataEviction(Addr addr, DataLine* &line);
    void cleanUpAfterRequest(MemEvent * event, bool inMSHR);
    void cleanUpAfterResponse(MemEvent * event, bool inMSHR);
    void cleanUpEvent(MemEvent * event, bool inMSHR);
    void retry(Addr addr);

    /** Invalidate sharers and/or owner; returns either the new line timestamp (or 0 if no invalidation) or a bool indicating whether anything was invalidated */
    bool invalidateExceptRequestor(MemEvent * event, DirectoryLine * line, bool inMSHR, bool needData);
    bool invalidateAll(MemEvent * event, DirectoryLine * line, bool inMSHR, bool needData);
    uint64_t invalidateSharer(std::string shr, MemEvent * event, DirectoryLine * line, bool inMSHR, Command cmd = Command::Inv);
    void invalidateSharers(MemEvent * event, DirectoryLine * line, bool inMSHR, bool needData, Command cmd);
    bool invalidateOwner(MemEvent * event, DirectoryLine * line, bool inMSHR, Command cmd = Command::FetchInv);

    /** Forward a flush line request, with or without data */
    uint64_t forwardFlush(MemEvent* event, bool evict, std::vector<uint8_t>* data, bool dirty, uint64_t time);

    /** Send response up (to processor) */
    uint64_t sendResponseUp(MemEvent * event, vector<uint8_t>* data, bool inMSHR, uint64_t baseTime, Command cmd = Command::NULLCMD, bool success = true);

    /** Send response down (towards memory) */
    void sendResponseDown(MemEvent* event, std::vector<uint8_t>* data, bool dirty, bool evict);

    /** Send writeback request to lower level caches */
    void sendWritebackFromCache(Command cmd, DirectoryLine* tag, DataLine* data, bool dirty);
    void sendWritebackFromMSHR(Command cmd, DirectoryLine* tag, bool dirty);
    void sendWritebackAck(MemEvent* event);

    uint64_t sendFetch(Command cmd, MemEvent * event, std::string dst, bool inMSHR, uint64_t ts);

    /** Call through to coherenceController with statistic recording */
    void forwardByAddress(MemEventBase* ev, Cycle_t timestamp) override;
    void forwardByDestination(MemEventBase* ev, Cycle_t timestamp) override;

    /** Helpers */
    void removeSharerViaInv(MemEvent* event, DirectoryLine * tag, DataLine * data, bool remove);
    void removeOwnerViaInv(MemEvent* event, DirectoryLine * tag, DataLine * data, bool remove);

    bool applyPendingReplacement(Addr addr);

/* Miscellaneous */
    void printLine(Addr addr);
    void printStatus(Output &out) override;
    void beginCompleteStage() override;
    void processCompleteEvent(MemEventInit* event, MemLinkBase* highlink, MemLinkBase* lowlink) override;

/* Statistics */
    void recordLatency(Command cmd, int type, uint64_t latency) override;
    void recordPrefetchResult(DirectoryLine * line, Statistic<uint64_t> * stat);

/* Private data members */
    CacheArray<DataLine>* dataArray_;
    CacheArray<DirectoryLine>* dirArray_;

    bool protocol_;  // True for MESI, false for MSI
    State protocolState_;

    std::map<Addr, std::map<std::string, MemEvent::id_type> > responses;

    // Map an outstanding eviction (key = replaceAddr,newAddr) to whether it is a directory eviction (true) or data eviction (false)
    std::map<std::pair<Addr,Addr>, bool> eviction_type_;

    FlushState flush_state_;
    int shutdown_flush_counter_;

/* Statistics */
    Statistic<uint64_t>* stat_latencyGetS[3];
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

