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

#ifndef MESICOHERENCECONTROLLER_H
#define MESICOHERENCECONTROLLER_H

#include <iostream>
#include <array>

#include "sst/elements/memHierarchy/coherencemgr/coherenceController.h"


namespace SST { namespace MemHierarchy {

class MESIController : public CoherenceController {
public:
    SST_ELI_REGISTER_SUBCOMPONENT_DERIVED(MESIController, "memHierarchy", "MESICoherenceController", SST_ELI_ELEMENT_VERSION(1,0,0), 
            "Implements MESI or MSI coherence for a second level or greater cache", SST::MemHierarchy::CoherenceController)

    SST_ELI_DOCUMENT_STATISTICS(
        /* Event sends */
        {"eventSent_GetS",          "Number of GetS requests sent", "events", 2},
        {"eventSent_GetX",          "Number of GetX requests sent", "events", 2},
        {"eventSent_GetSX",         "Number of GetSX requests sent", "events", 2},
        {"eventSent_GetSResp",      "Number of GetSResp responses sent", "events", 2},
        {"eventSent_GetXResp",      "Number of GetXResp responses sent", "events", 2},
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
        {"stateEvent_PutS_I",           "Event/State: Number of times a PutS was seen in state I", "count", 3},
        {"stateEvent_PutS_S",           "Event/State: Number of times a PutS was seen in state S", "count", 3},
        {"stateEvent_PutS_E",           "Event/State: Number of times a PutS was seen in state E", "count", 3},
        {"stateEvent_PutS_M",           "Event/State: Number of times a PutS was seen in state M", "count", 3},
        {"stateEvent_PutS_SI",          "Event/State: Number of times a PutS was seen in state SI", "count", 3},
        {"stateEvent_PutS_EI",          "Event/State: Number of times a PutS was seen in state EI", "count", 3},
        {"stateEvent_PutS_MI",          "Event/State: Number of times a PutS was seen in state MI", "count", 3},
        {"stateEvent_PutS_SInv",        "Event/State: Number of times a PutS was seen in state S_Inv", "count", 3},
        {"stateEvent_PutS_EInv",        "Event/State: Number of times a PutS was seen in state E_Inv", "count", 3},
        {"stateEvent_PutS_MInv",        "Event/State: Number of times a PutS was seen in state M_Inv", "count", 3},
        {"stateEvent_PutS_SMInv",       "Event/State: Number of times a PutS was seen in state SM_Inv", "count", 3},
        {"stateEvent_PutS_IB",          "Event/State: Number of times a PutS was seen in state I_B", "count", 3},
        {"stateEvent_PutS_SB",          "Event/State: Number of times a PutS was seen in state S_B", "count", 3},
        {"stateEvent_PutS_SBInv",       "Event/State: Number of times a PutS was seen in state SB_Inv", "count", 3},
        {"stateEvent_PutE_I",           "Event/State: Number of times a PutE was seen in state I", "count", 3},
        {"stateEvent_PutE_E",           "Event/State: Number of times a PutE was seen in state E", "count", 3},
        {"stateEvent_PutE_M",           "Event/State: Number of times a PutE was seen in state M", "count", 3},
        {"stateEvent_PutE_EI",          "Event/State: Number of times a PutE was seen in state EI", "count", 3},
        {"stateEvent_PutE_MI",          "Event/State: Number of times a PutE was seen in state MI", "count", 3},
        {"stateEvent_PutE_EInv",        "Event/State: Number of times a PutE was seen in state E_Inv", "count", 3},
        {"stateEvent_PutE_MInv",        "Event/State: Number of times a PutE was seen in state M_Inv", "count", 3},
        {"stateEvent_PutE_EInvX",       "Event/State: Number of times a PutE was seen in state E_InvX", "count", 3},
        {"stateEvent_PutE_MInvX",       "Event/State: Number of times a PutE was seen in state M_InvX", "count", 3},
        {"stateEvent_PutM_I",           "Event/State: Number of times a PutM was seen in state I", "count", 3},
        {"stateEvent_PutM_E",           "Event/State: Number of times a PutM was seen in state E", "count", 3},
        {"stateEvent_PutM_M",           "Event/State: Number of times a PutM was seen in state M", "count", 3},
        {"stateEvent_PutM_EI",          "Event/State: Number of times a PutM was seen in state EI", "count", 3},
        {"stateEvent_PutM_MI",          "Event/State: Number of times a PutM was seen in state MI", "count", 3},
        {"stateEvent_PutM_EInv",        "Event/State: Number of times a PutM was seen in state E_Inv", "count", 3},
        {"stateEvent_PutM_MInv",        "Event/State: Number of times a PutM was seen in state M_Inv", "count", 3},
        {"stateEvent_PutM_EInvX",       "Event/State: Number of times a PutM was seen in state E_InvX", "count", 3},
        {"stateEvent_PutM_MInvX",       "Event/State: Number of times a PutM was seen in state M_InvX", "count", 3},
        {"stateEvent_Inv_I",            "Event/State: Number of times an Inv was seen in state I", "count", 3},
        {"stateEvent_Inv_IS",           "Event/State: Number of times an Inv was seen in state IS", "count", 3},
        {"stateEvent_Inv_IM",           "Event/State: Number of times an Inv was seen in state IM", "count", 3},
        {"stateEvent_Inv_IB",           "Event/State: Number of times an Inv was seen in state I_B", "count", 3},
        {"stateEvent_Inv_S",            "Event/State: Number of times an Inv was seen in state S", "count", 3},
        {"stateEvent_Inv_SM",           "Event/State: Number of times an Inv was seen in state SM", "count", 3},
        {"stateEvent_Inv_SInv",         "Event/State: Number of times an Inv was seen in state S_Inv", "count", 3},
        {"stateEvent_Inv_SI",           "Event/State: Number of times an Inv was seen in state SI", "count", 3},
        {"stateEvent_Inv_SMInv",        "Event/State: Number of times an Inv was seen in state SM_Inv", "count", 3},
        {"stateEvent_Inv_SB",           "Event/State: Number of times an Inv was seen in state S_B", "count", 3},
        {"stateEvent_FetchInv_I",       "Event/State: Number of times a FetchInv was seen in state I", "count", 3},
        {"stateEvent_FetchInv_IS",      "Event/State: Number of times a FetchInv was seen in state IS", "count", 3},
        {"stateEvent_FetchInv_IM",      "Event/State: Number of times a FetchInv was seen in state IM", "count", 3},
        {"stateEvent_FetchInv_SM",      "Event/State: Number of times a FetchInv was seen in state SM", "count", 3},
        {"stateEvent_FetchInv_S",       "Event/State: Number of times a FetchInv was seen in state S", "count", 3},
        {"stateEvent_FetchInv_E",       "Event/State: Number of times a FetchInv was seen in state E", "count", 3},
        {"stateEvent_FetchInv_M",       "Event/State: Number of times a FetchInv was seen in state M", "count", 3},
        {"stateEvent_FetchInv_EI",      "Event/State: Number of times a FetchInv was seen in state EI", "count", 3},
        {"stateEvent_FetchInv_MI",      "Event/State: Number of times a FetchInv was seen in state MI", "count", 3},
        {"stateEvent_FetchInv_EInv",    "Event/State: Number of times a FetchInv was seen in state E_Inv", "count", 3},
        {"stateEvent_FetchInv_EInvX",   "Event/State: Number of times a FetchInv was seen in state E_InvX", "count", 3},
        {"stateEvent_FetchInv_MInv",    "Event/State: Number of times a FetchInv was seen in state M_Inv", "count", 3},
        {"stateEvent_FetchInv_MInvX",   "Event/State: Number of times a FetchInv was seen in state M_InvX", "count", 3},
        {"stateEvent_FetchInv_SInv",    "Event/State: Number of times a FetchInv was seen in state S_Inv", "count", 3},
        {"stateEvent_FetchInv_IB",      "Event/State: Number of times a FetchInv was seen in state I_B", "count", 3},
        {"stateEvent_FetchInv_SB",      "Event/State: Number of times a FetchInv was seen in state S_B", "count", 3},
        {"stateEvent_FetchInvX_I",      "Event/State: Number of times a FetchInvX was seen in state I", "count", 3},
        {"stateEvent_FetchInvX_IS",     "Event/State: Number of times a FetchInvX was seen in state IS", "count", 3},
        {"stateEvent_FetchInvX_IM",     "Event/State: Number of times a FetchInvX was seen in state IM", "count", 3},
        {"stateEvent_FetchInvX_E",      "Event/State: Number of times a FetchInvX was seen in state E", "count", 3},
        {"stateEvent_FetchInvX_M",      "Event/State: Number of times a FetchInvX was seen in state M", "count", 3},
        {"stateEvent_FetchInvX_EI",     "Event/State: Number of times a FetchInvX was seen in state EI", "count", 3},
        {"stateEvent_FetchInvX_MI",     "Event/State: Number of times a FetchInvX was seen in state MI", "count", 3},
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
        {"stateEvent_Fetch_SInv",       "Event/State: Number of times a Fetch was seen in state S_Inv", "count", 3},
        {"stateEvent_Fetch_SI",         "Event/State: Number of times a Fetch was seen in state SI", "count", 3},
        {"stateEvent_Fetch_IB",         "Event/State: Number of times a Fetch was seen in state I_B", "count", 3},
        {"stateEvent_Fetch_SB",         "Event/State: Number of times a Fetch was seen in state S_B", "count", 3},
        {"stateEvent_FetchResp_I",      "Event/State: Number of times a FetchResp was seen in state I", "count", 3},
        {"stateEvent_FetchResp_EI",     "Event/State: Number of times a FetchResp was seen in state EI", "count", 3},
        {"stateEvent_FetchResp_MI",     "Event/State: Number of times a FetchResp was seen in state MI", "count", 3},
        {"stateEvent_FetchResp_EInv",   "Event/State: Number of times a FetchResp was seen in state E_Inv", "count", 3},
        {"stateEvent_FetchResp_EInvX",  "Event/State: Number of times a FetchResp was seen in state E_Inv", "count", 3},
        {"stateEvent_FetchResp_MInv",   "Event/State: Number of times a FetchResp was seen in state M_Inv", "count", 3},
        {"stateEvent_FetchResp_MInvX",  "Event/State: Number of times a FetchResp was seen in state M_InvX", "count", 3},
        {"stateEvent_FetchXResp_I",     "Event/State: Number of times a FetchXResp was seen in state I", "count", 3},
        {"stateEvent_FetchXResp_EInv",  "Event/State: Number of times a FetchXResp was seen in state E_Inv", "count", 3},
        {"stateEvent_FetchXResp_EInvX", "Event/State: Number of times a FetchXResp was seen in state E_InvX", "count", 3},
        {"stateEvent_FetchXResp_MInvX", "Event/State: Number of times a FetchXResp was seen in state M_InvX", "count", 3},
        {"stateEvent_FetchXResp_MInv",  "Event/State: Number of times a FetchXResp was seen in state M_Inv", "count", 3},
        {"stateEvent_FetchXResp_MI",    "Event/State: Number of times a FetchXResp was seen in state MI", "count", 3},
        {"stateEvent_FetchXResp_EI",    "Event/State: Number of times a FetchXResp was seen in state EI", "count", 3},
        {"stateEvent_AckInv_I",         "Event/State: Number of times an AckInv was seen in state I", "count", 3},
        {"stateEvent_AckInv_SInv",      "Event/State: Number of times an AckInv was seen in state S_Inv", "count", 3},
        {"stateEvent_AckInv_SMInv",     "Event/State: Number of times an AckInv was seen in state SM_Inv", "count", 3},
        {"stateEvent_AckInv_SI",        "Event/State: Number of times an AckInv was seen in state SI", "count", 3},
        {"stateEvent_AckInv_EI",        "Event/State: Number of times an AckInv was seen in state EI", "count", 3},
        {"stateEvent_AckInv_MI",        "Event/State: Number of times an AckInv was seen in state MI", "count", 3},
        {"stateEvent_AckInv_EInv",      "Event/State: Number of times an AckInv was seen in state E_Inv", "count", 3},
        {"stateEvent_AckInv_EInvX",     "Event/State: Number of times an AckInv was seen in state E_InvX", "count", 3},
        {"stateEvent_AckInv_MInv",      "Event/State: Number of times an AckInv was seen in state M_Inv", "count", 3},
        {"stateEvent_AckInv_MInvX",     "Event/State: Number of times an AckInv was seen in state M_InvX", "count", 3},
        {"stateEvent_AckInv_SBInv",     "Event/State: Number of times an AckInv was seen in state SB_Inv", "count", 3},
        {"stateEvent_AckPut_I",         "Event/State: Number of times an AckPut was seen in state I", "count", 3},
        {"stateEvent_FlushLine_I",      "Event/State: Number of times a FlushLine was seen in state I", "count", 3},
        {"stateEvent_FlushLine_S",      "Event/State: Number of times a FlushLine was seen in state S", "count", 3},
        {"stateEvent_FlushLine_E",      "Event/State: Number of times a FlushLine was seen in state E", "count", 3},
        {"stateEvent_FlushLine_M",      "Event/State: Number of times a FlushLine was seen in state M", "count", 3},
        {"stateEvent_FlushLine_IS",     "Event/State: Number of times a FlushLine was seen in state IS", "count", 3},
        {"stateEvent_FlushLine_IM",     "Event/State: Number of times a FlushLine was seen in state IM", "count", 3},
        {"stateEvent_FlushLine_SM",     "Event/State: Number of times a FlushLine was seen in state SM", "count", 3},
        {"stateEvent_FlushLine_MInv",   "Event/State: Number of times a FlushLine was seen in state M_Inv", "count", 3},
        {"stateEvent_FlushLine_MInvX",  "Event/State: Number of times a FlushLine was seen in state M_InvX", "count", 3},
        {"stateEvent_FlushLine_EInv",   "Event/State: Number of times a FlushLine was seen in state E_Inv", "count", 3},
        {"stateEvent_FlushLine_EInvX",  "Event/State: Number of times a FlushLine was seen in state E_InvX", "count", 3},
        {"stateEvent_FlushLine_SInv",   "Event/State: Number of times a FlushLine was seen in state S_Inv", "count", 3},
        {"stateEvent_FlushLine_SMInv",  "Event/State: Number of times a FlushLine was seen in state SM_Inv", "count", 3},
        {"stateEvent_FlushLine_MI",     "Event/State: Number of times a FlushLine was seen in state MI", "count", 3},
        {"stateEvent_FlushLine_EI",     "Event/State: Number of times a FlushLine was seen in state EI", "count", 3},
        {"stateEvent_FlushLine_SI",     "Event/State: Number of times a FlushLine was seen in state SI", "count", 3},
        {"stateEvent_FlushLine_IB",     "Event/State: Number of times a FlushLine was seen in state I_B", "count", 3},
        {"stateEvent_FlushLine_SB",     "Event/State: Number of times a FlushLine was seen in state S_B", "count", 3},
        {"stateEvent_FlushLineInv_I",       "Event/State: Number of times a FlushLineInv was seen in state I", "count", 3},
        {"stateEvent_FlushLineInv_S",       "Event/State: Number of times a FlushLineInv was seen in state S", "count", 3},
        {"stateEvent_FlushLineInv_E",       "Event/State: Number of times a FlushLineInv was seen in state E", "count", 3},
        {"stateEvent_FlushLineInv_M",       "Event/State: Number of times a FlushLineInv was seen in state M", "count", 3},
        {"stateEvent_FlushLineInv_IS",      "Event/State: Number of times a FlushLineInv was seen in state IS", "count", 3},
        {"stateEvent_FlushLineInv_IM",      "Event/State: Number of times a FlushLineInv was seen in state IM", "count", 3},
        {"stateEvent_FlushLineInv_SM",      "Event/State: Number of times a FlushLineInv was seen in state SM", "count", 3},
        {"stateEvent_FlushLineInv_MInv",    "Event/State: Number of times a FlushLineInv was seen in state M_Inv", "count", 3},
        {"stateEvent_FlushLineInv_MInvX",   "Event/State: Number of times a FlushLineInv was seen in state M_InvX", "count", 3},
        {"stateEvent_FlushLineInv_EInv",    "Event/State: Number of times a FlushLineInv was seen in state E_Inv", "count", 3},
        {"stateEvent_FlushLineInv_EInvX",   "Event/State: Number of times a FlushLineInv was seen in state E_InvX", "count", 3},
        {"stateEvent_FlushLineInv_SInv",    "Event/State: Number of times a FlushLineInv was seen in state S_Inv", "count", 3},
        {"stateEvent_FlushLineInv_SMInv",   "Event/State: Number of times a FlushLineInv was seen in state SM_Inv", "count", 3},
        {"stateEvent_FlushLineInv_MI",      "Event/State: Number of times a FlushLineInv was seen in state MI", "count", 3},
        {"stateEvent_FlushLineInv_EI",      "Event/State: Number of times a FlushLineInv was seen in state EI", "count", 3},
        {"stateEvent_FlushLineInv_SI",      "Event/State: Number of times a FlushLineInv was seen in state SI", "count", 3},
        {"stateEvent_FlushLineInv_IB",      "Event/State: Number of times a FlushLineInv was seen in state I_B", "count", 3},
        {"stateEvent_FlushLineInv_SB",      "Event/State: Number of times a FlushLineInv was seen in state S_B", "count", 3},
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
        {"evict_SI",                "Eviction: Attempted to evict a block in state SI", "count", 3},
        {"evict_IB",                "Eviction: Attempted to evict a block in state S_B", "count", 3},
        {"evict_SB",                "Eviction: Attempted to evict a block in state I_B", "count", 3},
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

/* Class definition */
    /** Constructor for MESIController. Note that MESIController handles both MESI & MSI protocols */
    MESIController(SST::Component* comp, Params& params) : CoherenceController(comp, params) { }
    MESIController(SST::ComponentId_t id, Params& params, Params& ownerParams, bool prefetch) : CoherenceController(id, params, ownerParams, prefetch) {
        params.insert(ownerParams);
        debug->debug(_INFO_,"--------------------------- Initializing [MESI Controller] ... \n\n");
        
        protocol_ = params.find<bool>("protocol", 1);
      
        /* Statistics */
        Statistic<uint64_t>* defStat = registerStatistic<uint64_t>("default_stat");
        for (int i = 0; i < (int)Command::LAST_CMD; i++) {
            stat_eventSent[i] = defStat;
            for (int j = 0; j < LAST_STATE; j++) {
                stat_eventState[i][j] = defStat;
            }
        }

        stat_evict_S =      registerStatistic<uint64_t>("evict_S");
        stat_evict_SM =     registerStatistic<uint64_t>("evict_SM");
        stat_evict_SInv =   registerStatistic<uint64_t>("evict_SInv");
        stat_evict_MInv =   registerStatistic<uint64_t>("evict_MInv");
        stat_evict_SMInv =  registerStatistic<uint64_t>("evict_SMInv");
        stat_evict_MInvX =  registerStatistic<uint64_t>("evict_MInvX");
        stat_evict_SI =     registerStatistic<uint64_t>("evict_SI");
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
        stat_eventState[(int)Command::PutS][I] =        registerStatistic<uint64_t>("stateEvent_PutS_I");
        stat_eventState[(int)Command::PutS][S] =        registerStatistic<uint64_t>("stateEvent_PutS_S");
        stat_eventState[(int)Command::PutS][M] =        registerStatistic<uint64_t>("stateEvent_PutS_M");
        stat_eventState[(int)Command::PutS][M_Inv] =    registerStatistic<uint64_t>("stateEvent_PutS_MInv");
        stat_eventState[(int)Command::PutS][S_Inv] =    registerStatistic<uint64_t>("stateEvent_PutS_SInv");
        stat_eventState[(int)Command::PutS][SM_Inv] =   registerStatistic<uint64_t>("stateEvent_PutS_SMInv");
        stat_eventState[(int)Command::PutS][MI] =       registerStatistic<uint64_t>("stateEvent_PutS_MI");
        stat_eventState[(int)Command::PutS][SI] =       registerStatistic<uint64_t>("stateEvent_PutS_SI");
        stat_eventState[(int)Command::PutS][I_B] =      registerStatistic<uint64_t>("stateEvent_PutS_IB");
        stat_eventState[(int)Command::PutS][S_B] =      registerStatistic<uint64_t>("stateEvent_PutS_SB");
        stat_eventState[(int)Command::PutS][SB_Inv] =   registerStatistic<uint64_t>("stateEvent_PutS_SBInv");
        stat_eventState[(int)Command::PutM][I] =        registerStatistic<uint64_t>("stateEvent_PutM_I");
        stat_eventState[(int)Command::PutM][M] =        registerStatistic<uint64_t>("stateEvent_PutM_M");
        stat_eventState[(int)Command::PutM][M_Inv] =    registerStatistic<uint64_t>("stateEvent_PutM_MInv");
        stat_eventState[(int)Command::PutM][M_InvX] =   registerStatistic<uint64_t>("stateEvent_PutM_MInvX");
        stat_eventState[(int)Command::PutM][MI] =       registerStatistic<uint64_t>("stateEvent_PutM_MI");
        stat_eventState[(int)Command::Inv][I] =         registerStatistic<uint64_t>("stateEvent_Inv_I");
        stat_eventState[(int)Command::Inv][S] =         registerStatistic<uint64_t>("stateEvent_Inv_S");
        stat_eventState[(int)Command::Inv][IS] =        registerStatistic<uint64_t>("stateEvent_Inv_IS");
        stat_eventState[(int)Command::Inv][IM] =        registerStatistic<uint64_t>("stateEvent_Inv_IM");
        stat_eventState[(int)Command::Inv][SM] =        registerStatistic<uint64_t>("stateEvent_Inv_SM");
        stat_eventState[(int)Command::Inv][S_Inv] =     registerStatistic<uint64_t>("stateEvent_Inv_SInv");
        stat_eventState[(int)Command::Inv][SM_Inv] =    registerStatistic<uint64_t>("stateEvent_Inv_SMInv");
        stat_eventState[(int)Command::Inv][SI] =        registerStatistic<uint64_t>("stateEvent_Inv_SI");
        stat_eventState[(int)Command::Inv][S_B] =       registerStatistic<uint64_t>("stateEvent_Inv_SB");
        stat_eventState[(int)Command::Inv][I_B] =       registerStatistic<uint64_t>("stateEvent_Inv_IB");
        stat_eventState[(int)Command::FetchInvX][I] =   registerStatistic<uint64_t>("stateEvent_FetchInvX_I");
        stat_eventState[(int)Command::FetchInvX][M] =   registerStatistic<uint64_t>("stateEvent_FetchInvX_M");
        stat_eventState[(int)Command::FetchInvX][IS] =  registerStatistic<uint64_t>("stateEvent_FetchInvX_IS");
        stat_eventState[(int)Command::FetchInvX][IM] =  registerStatistic<uint64_t>("stateEvent_FetchInvX_IM");
        stat_eventState[(int)Command::FetchInvX][M_Inv] =   registerStatistic<uint64_t>("stateEvent_FetchInvX_MInv");
        stat_eventState[(int)Command::FetchInvX][M_InvX] =  registerStatistic<uint64_t>("stateEvent_FetchInvX_MInvX");
        stat_eventState[(int)Command::FetchInvX][MI] =  registerStatistic<uint64_t>("stateEvent_FetchInvX_MI");
        stat_eventState[(int)Command::FetchInvX][I_B] = registerStatistic<uint64_t>("stateEvent_FetchInvX_IB");
        stat_eventState[(int)Command::FetchInvX][S_B] = registerStatistic<uint64_t>("stateEvent_FetchInvX_SB");
        stat_eventState[(int)Command::Fetch][I] =       registerStatistic<uint64_t>("stateEvent_Fetch_I");
        stat_eventState[(int)Command::Fetch][S] =       registerStatistic<uint64_t>("stateEvent_Fetch_S");
        stat_eventState[(int)Command::Fetch][IS] =      registerStatistic<uint64_t>("stateEvent_Fetch_IS");
        stat_eventState[(int)Command::Fetch][IM] =      registerStatistic<uint64_t>("stateEvent_Fetch_IM");
        stat_eventState[(int)Command::Fetch][SM] =      registerStatistic<uint64_t>("stateEvent_Fetch_SM");
        stat_eventState[(int)Command::Fetch][S_Inv] =   registerStatistic<uint64_t>("stateEvent_Fetch_SInv");
        stat_eventState[(int)Command::Fetch][SI] =      registerStatistic<uint64_t>("stateEvent_Fetch_SI");
        stat_eventState[(int)Command::Fetch][I_B] =     registerStatistic<uint64_t>("stateEvent_Fetch_IB");
        stat_eventState[(int)Command::Fetch][S_B] =     registerStatistic<uint64_t>("stateEvent_Fetch_SB");
        stat_eventState[(int)Command::FetchInv][I] =        registerStatistic<uint64_t>("stateEvent_FetchInv_I");
        stat_eventState[(int)Command::FetchInv][S] =        registerStatistic<uint64_t>("stateEvent_FetchInv_S");
        stat_eventState[(int)Command::FetchInv][M] =        registerStatistic<uint64_t>("stateEvent_FetchInv_M");
        stat_eventState[(int)Command::FetchInv][IS] =       registerStatistic<uint64_t>("stateEvent_FetchInv_IS");
        stat_eventState[(int)Command::FetchInv][IM] =       registerStatistic<uint64_t>("stateEvent_FetchInv_IM");
        stat_eventState[(int)Command::FetchInv][SM] =       registerStatistic<uint64_t>("stateEvent_FetchInv_SM");
        stat_eventState[(int)Command::FetchInv][M_Inv] =    registerStatistic<uint64_t>("stateEvent_FetchInv_MInv");
        stat_eventState[(int)Command::FetchInv][M_InvX] =   registerStatistic<uint64_t>("stateEvent_FetchInv_MInvX");
        stat_eventState[(int)Command::FetchInv][S_Inv] =    registerStatistic<uint64_t>("stateEvent_FetchInv_SInv");
        stat_eventState[(int)Command::FetchInv][MI] =       registerStatistic<uint64_t>("stateEvent_FetchInv_MI");
        stat_eventState[(int)Command::FetchInv][I_B] =      registerStatistic<uint64_t>("stateEvent_FetchInv_IB");
        stat_eventState[(int)Command::FetchInv][S_B] =      registerStatistic<uint64_t>("stateEvent_FetchInv_SB");
        stat_eventState[(int)Command::FetchResp][I] =       registerStatistic<uint64_t>("stateEvent_FetchResp_I");
        stat_eventState[(int)Command::FetchResp][M_Inv] =   registerStatistic<uint64_t>("stateEvent_FetchResp_MInv");
        stat_eventState[(int)Command::FetchResp][M_InvX] =  registerStatistic<uint64_t>("stateEvent_FetchResp_MInvX");
        stat_eventState[(int)Command::FetchResp][MI] =      registerStatistic<uint64_t>("stateEvent_FetchResp_MI");
        stat_eventState[(int)Command::FetchXResp][I] =      registerStatistic<uint64_t>("stateEvent_FetchXResp_I");
        stat_eventState[(int)Command::FetchXResp][M_Inv] =  registerStatistic<uint64_t>("stateEvent_FetchXResp_MInv");
        stat_eventState[(int)Command::FetchXResp][M_InvX] = registerStatistic<uint64_t>("stateEvent_FetchXResp_MInvX");
        stat_eventState[(int)Command::FetchXResp][MI] =     registerStatistic<uint64_t>("stateEvent_FetchXResp_MI");
        stat_eventState[(int)Command::AckInv][I] =          registerStatistic<uint64_t>("stateEvent_AckInv_I");
        stat_eventState[(int)Command::AckInv][M_Inv] =      registerStatistic<uint64_t>("stateEvent_AckInv_MInv");
        stat_eventState[(int)Command::AckInv][M_InvX] =     registerStatistic<uint64_t>("stateEvent_AckInv_MInvX");
        stat_eventState[(int)Command::AckInv][S_Inv] =      registerStatistic<uint64_t>("stateEvent_AckInv_SInv");
        stat_eventState[(int)Command::AckInv][SM_Inv] =     registerStatistic<uint64_t>("stateEvent_AckInv_SMInv");
        stat_eventState[(int)Command::AckInv][MI] =         registerStatistic<uint64_t>("stateEvent_AckInv_MI");
        stat_eventState[(int)Command::AckInv][SI] =         registerStatistic<uint64_t>("stateEvent_AckInv_SI");
        stat_eventState[(int)Command::AckInv][SB_Inv] =     registerStatistic<uint64_t>("stateEvent_AckInv_SBInv");
        stat_eventState[(int)Command::FlushLine][I] =       registerStatistic<uint64_t>("stateEvent_FlushLine_I");
        stat_eventState[(int)Command::FlushLine][S] =       registerStatistic<uint64_t>("stateEvent_FlushLine_S");
        stat_eventState[(int)Command::FlushLine][M] =       registerStatistic<uint64_t>("stateEvent_FlushLine_M");
        stat_eventState[(int)Command::FlushLine][IS] =      registerStatistic<uint64_t>("stateEvent_FlushLine_IS");
        stat_eventState[(int)Command::FlushLine][IM] =      registerStatistic<uint64_t>("stateEvent_FlushLine_IM");
        stat_eventState[(int)Command::FlushLine][SM] =      registerStatistic<uint64_t>("stateEvent_FlushLine_SM");
        stat_eventState[(int)Command::FlushLine][M_Inv] =   registerStatistic<uint64_t>("stateEvent_FlushLine_MInv");
        stat_eventState[(int)Command::FlushLine][M_InvX] =  registerStatistic<uint64_t>("stateEvent_FlushLine_MInvX");
        stat_eventState[(int)Command::FlushLine][S_Inv] =   registerStatistic<uint64_t>("stateEvent_FlushLine_SInv");
        stat_eventState[(int)Command::FlushLine][SM_Inv] =  registerStatistic<uint64_t>("stateEvent_FlushLine_SMInv");
        stat_eventState[(int)Command::FlushLine][MI] =      registerStatistic<uint64_t>("stateEvent_FlushLine_MI");
        stat_eventState[(int)Command::FlushLine][SI] =      registerStatistic<uint64_t>("stateEvent_FlushLine_SI");
        stat_eventState[(int)Command::FlushLine][I_B] =     registerStatistic<uint64_t>("stateEvent_FlushLine_IB");
        stat_eventState[(int)Command::FlushLine][S_B] =     registerStatistic<uint64_t>("stateEvent_FlushLine_SB");
        stat_eventState[(int)Command::FlushLineInv][I] =    registerStatistic<uint64_t>("stateEvent_FlushLineInv_I");
        stat_eventState[(int)Command::FlushLineInv][S] =    registerStatistic<uint64_t>("stateEvent_FlushLineInv_S");
        stat_eventState[(int)Command::FlushLineInv][M] =    registerStatistic<uint64_t>("stateEvent_FlushLineInv_M");
        stat_eventState[(int)Command::FlushLineInv][IS] =   registerStatistic<uint64_t>("stateEvent_FlushLineInv_IS");
        stat_eventState[(int)Command::FlushLineInv][IM] =   registerStatistic<uint64_t>("stateEvent_FlushLineInv_IM");
        stat_eventState[(int)Command::FlushLineInv][SM] =   registerStatistic<uint64_t>("stateEvent_FlushLineInv_SM");
        stat_eventState[(int)Command::FlushLineInv][M_Inv] =    registerStatistic<uint64_t>("stateEvent_FlushLineInv_MInv");
        stat_eventState[(int)Command::FlushLineInv][M_InvX] =   registerStatistic<uint64_t>("stateEvent_FlushLineInv_MInvX");
        stat_eventState[(int)Command::FlushLineInv][S_Inv] =    registerStatistic<uint64_t>("stateEvent_FlushLineInv_SInv");
        stat_eventState[(int)Command::FlushLineInv][SM_Inv] =   registerStatistic<uint64_t>("stateEvent_FlushLineInv_SMInv");
        stat_eventState[(int)Command::FlushLineInv][MI] =       registerStatistic<uint64_t>("stateEvent_FlushLineInv_MI");
        stat_eventState[(int)Command::FlushLineInv][SI] =       registerStatistic<uint64_t>("stateEvent_FlushLineInv_SI");
        stat_eventState[(int)Command::FlushLineInv][I_B] =      registerStatistic<uint64_t>("stateEvent_FlushLineInv_IB");
        stat_eventState[(int)Command::FlushLineInv][S_B] =      registerStatistic<uint64_t>("stateEvent_FlushLineInv_SB");
        stat_eventState[(int)Command::FlushLineResp][I] =       registerStatistic<uint64_t>("stateEvent_FlushLineResp_I");
        stat_eventState[(int)Command::FlushLineResp][I_B] =     registerStatistic<uint64_t>("stateEvent_FlushLineResp_IB");
        stat_eventState[(int)Command::FlushLineResp][S_B] =     registerStatistic<uint64_t>("stateEvent_FlushLineResp_SB");
        stat_eventSent[(int)Command::GetS]          = registerStatistic<uint64_t>("eventSent_GetS");
        stat_eventSent[(int)Command::GetX]          = registerStatistic<uint64_t>("eventSent_GetX");
        stat_eventSent[(int)Command::GetSX]         = registerStatistic<uint64_t>("eventSent_GetSX");
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
            stat_evict_EInv =   registerStatistic<uint64_t>("evict_EInv");
            stat_evict_EInvX =  registerStatistic<uint64_t>("evict_EInvX");
            stat_eventState[(int)Command::GetS][E] =        registerStatistic<uint64_t>("stateEvent_GetS_E");
            stat_eventState[(int)Command::GetX][E] =        registerStatistic<uint64_t>("stateEvent_GetX_E");
            stat_eventState[(int)Command::GetSX][E] =       registerStatistic<uint64_t>("stateEvent_GetSX_E");
            stat_eventState[(int)Command::PutS][E] =        registerStatistic<uint64_t>("stateEvent_PutS_E");
            stat_eventState[(int)Command::PutS][E_Inv] =    registerStatistic<uint64_t>("stateEvent_PutS_EInv");
            stat_eventState[(int)Command::PutS][EI] =       registerStatistic<uint64_t>("stateEvent_PutS_EI");
            stat_eventState[(int)Command::PutE][I] =        registerStatistic<uint64_t>("stateEvent_PutE_I");
            stat_eventState[(int)Command::PutE][E] =        registerStatistic<uint64_t>("stateEvent_PutE_E");
            stat_eventState[(int)Command::PutE][M] =        registerStatistic<uint64_t>("stateEvent_PutE_M");
            stat_eventState[(int)Command::PutE][M_Inv] =    registerStatistic<uint64_t>("stateEvent_PutE_MInv");
            stat_eventState[(int)Command::PutE][M_InvX] =   registerStatistic<uint64_t>("stateEvent_PutE_MInvX");
            stat_eventState[(int)Command::PutE][MI] =       registerStatistic<uint64_t>("stateEvent_PutE_MI");
            stat_eventState[(int)Command::PutE][E_Inv] =    registerStatistic<uint64_t>("stateEvent_PutE_EInv");
            stat_eventState[(int)Command::PutE][E_InvX] =   registerStatistic<uint64_t>("stateEvent_PutE_EInvX");
            stat_eventState[(int)Command::PutE][EI] =       registerStatistic<uint64_t>("stateEvent_PutE_EI");
            stat_eventState[(int)Command::PutM][E] =        registerStatistic<uint64_t>("stateEvent_PutM_E");
            stat_eventState[(int)Command::PutM][E_Inv] =    registerStatistic<uint64_t>("stateEvent_PutM_EInv");
            stat_eventState[(int)Command::PutM][E_InvX] =   registerStatistic<uint64_t>("stateEvent_PutM_EInvX");
            stat_eventState[(int)Command::PutM][EI] =       registerStatistic<uint64_t>("stateEvent_PutM_EI");
            stat_eventState[(int)Command::FetchInvX][E] =   registerStatistic<uint64_t>("stateEvent_FetchInvX_E");
            stat_eventState[(int)Command::FetchInvX][E_Inv] =   registerStatistic<uint64_t>("stateEvent_FetchInvX_EInv");
            stat_eventState[(int)Command::FetchInvX][E_InvX] =  registerStatistic<uint64_t>("stateEvent_FetchInvX_EInvX");
            stat_eventState[(int)Command::FetchInvX][EI] =      registerStatistic<uint64_t>("stateEvent_FetchInvX_EI");
            stat_eventState[(int)Command::FetchInv][E] =        registerStatistic<uint64_t>("stateEvent_FetchInv_E");
            stat_eventState[(int)Command::FetchInv][E_Inv] =    registerStatistic<uint64_t>("stateEvent_FetchInv_EInv");
            stat_eventState[(int)Command::FetchInv][E_InvX] =   registerStatistic<uint64_t>("stateEvent_FetchInv_EInvX");
            stat_eventState[(int)Command::FetchInv][EI] =       registerStatistic<uint64_t>("stateEvent_FetchInv_EI");
            stat_eventState[(int)Command::FetchResp][E_Inv] =   registerStatistic<uint64_t>("stateEvent_FetchResp_EInv");
            stat_eventState[(int)Command::FetchResp][E_InvX] =  registerStatistic<uint64_t>("stateEvent_FetchResp_EInvX");
            stat_eventState[(int)Command::FetchResp][EI] =      registerStatistic<uint64_t>("stateEvent_FetchResp_EI");
            stat_eventState[(int)Command::FetchXResp][E_Inv] =  registerStatistic<uint64_t>("stateEvent_FetchXResp_EInv");
            stat_eventState[(int)Command::FetchXResp][E_InvX] = registerStatistic<uint64_t>("stateEvent_FetchXResp_EInvX");
            stat_eventState[(int)Command::FetchXResp][EI] =     registerStatistic<uint64_t>("stateEvent_FetchXResp_EI");
            stat_eventState[(int)Command::AckInv][E_Inv] =      registerStatistic<uint64_t>("stateEvent_AckInv_EInv");
            stat_eventState[(int)Command::AckInv][E_InvX] =     registerStatistic<uint64_t>("stateEvent_AckInv_EInvX");
            stat_eventState[(int)Command::AckInv][EI] =         registerStatistic<uint64_t>("stateEvent_AckInv_EI");
            stat_eventState[(int)Command::FlushLine][E] =       registerStatistic<uint64_t>("stateEvent_FlushLine_E");
            stat_eventState[(int)Command::FlushLine][E_Inv] =   registerStatistic<uint64_t>("stateEvent_FlushLine_EInv");
            stat_eventState[(int)Command::FlushLine][E_InvX] =  registerStatistic<uint64_t>("stateEvent_FlushLine_EInvX");
            stat_eventState[(int)Command::FlushLine][EI] =      registerStatistic<uint64_t>("stateEvent_FlushLine_EI");
            stat_eventState[(int)Command::FlushLineInv][E] =    registerStatistic<uint64_t>("stateEvent_FlushLineInv_E");
            stat_eventState[(int)Command::FlushLineInv][E_Inv] =    registerStatistic<uint64_t>("stateEvent_FlushLineInv_EInv");
            stat_eventState[(int)Command::FlushLineInv][E_InvX] =   registerStatistic<uint64_t>("stateEvent_FlushLineInv_EInvX");
            stat_eventState[(int)Command::FlushLineInv][EI] =       registerStatistic<uint64_t>("stateEvent_FlushLineInv_EI");
            stat_eventSent[(int)Command::PutE] = registerStatistic<uint64_t>("eventSent_PutE");
        }
    }
    ~MESIController() {}
    
/*----------------------------------------------------------------------------------------------------------------------
 *  Public functions form external interface to the coherence controller
 *---------------------------------------------------------------------------------------------------------------------*/  

/* Event handlers */
    /** Send cacheline data to the lower level caches */
    CacheAction handleEviction(CacheLine* wbCacheLine, string origRqstr, bool ignoredParam=false);

    /** Process cache request:  GetX, GetS, GetSX */
    CacheAction handleRequest(MemEvent* event, bool replay);
    
    /** Process replacement request - PutS, PutE, PutM */
    CacheAction handleReplacement(MemEvent* event, bool replay);
    CacheAction handleFlush(MemEvent* event, CacheLine* cacheLine, MemEvent* reqEvent, bool replay);
    
    /** Process invalidation requests - Inv, FetchInv, FetchInvX */
    CacheAction handleInvalidationRequest(MemEvent *event, bool inMSHR);

    /** Process responses - GetSResp, GetXResp, FetchResp */
    CacheAction handleCacheResponse(MemEvent* event, bool inMSHR);
    CacheAction handleFetchResponse(MemEvent* event, bool inMSHR);
    
    bool handleNACK(MemEvent* event, bool inMSHR);

/* Message send */
    void forwardMessageUp(MemEvent* event);
    
    /** Call through to coherenceController with statistic recording */
    void addToOutgoingQueue(Response& resp);
    void addToOutgoingQueueUp(Response& resp);

/* Miscellaneous */
    /** Determine in advance if a request will miss (and what kind of miss). Used for stats */
    bool isCacheHit(MemEvent* event);
    
/* Temporary */
    void setCacheArray(CacheArray* arrayptr) { cacheArray_ = arrayptr; }
    
    void printLine(Addr addr);

private:
/* Private data members */
    bool        protocol_;  // True for MESI, false for MSI
    CacheArray* cacheArray_;

/* Statistics */
    Statistic<uint64_t>* stat_evict_S;
    Statistic<uint64_t>* stat_evict_SM;
    Statistic<uint64_t>* stat_evict_SInv;
    Statistic<uint64_t>* stat_evict_EInv;
    Statistic<uint64_t>* stat_evict_MInv;
    Statistic<uint64_t>* stat_evict_SMInv;
    Statistic<uint64_t>* stat_evict_EInvX;
    Statistic<uint64_t>* stat_evict_MInvX;
    Statistic<uint64_t>* stat_evict_SI;
    Statistic<uint64_t>* stat_eventSent[(int)Command::LAST_CMD];
    std::array<std::array<Statistic<uint64_t>*, LAST_STATE>, (int)Command::LAST_CMD> stat_eventState;
    Statistic<uint64_t>* stat_latencyGetS[3];
    Statistic<uint64_t>* stat_latencyGetX[4];
    Statistic<uint64_t>* stat_latencyGetSX[4];
    Statistic<uint64_t>* stat_latencyFlushLine;
    Statistic<uint64_t>* stat_latencyFlushLineInv;

/* Private event handlers */
    /** Handle GetX request. Request upgrade if needed */
    CacheAction handleGetXRequest(MemEvent* event, CacheLine* cacheLine, bool replay);

    /** Handle GetS request. Request block if needed */
    CacheAction handleGetSRequest(MemEvent* event, CacheLine* cacheLine, bool replay);
    
    /** Handle FlushLine request. Forward if needed */
    CacheAction handleFlushLineRequest(MemEvent * event, CacheLine * cacheLine, MemEvent * reqEvent, bool replay);

    /** Handle FlushLineInv request. Forward if needed */
    CacheAction handleFlushLineInvRequest(MemEvent * event, CacheLine * cacheLine, MemEvent * reqEvent, bool replay);

    /** Handle PutM request. Write data to cache line.  Update E->M state if necessary */
    CacheAction handlePutMRequest(MemEvent* event, CacheLine* cacheLine, MemEvent * reqEvent);
    
    /** Handle PutS requesst. Update sharer state */ 
    CacheAction handlePutSRequest(MemEvent* event, CacheLine* cacheLine, MemEvent * reqEvent);
    
    /** Handle Inv */
    CacheAction handleInv(MemEvent * event, CacheLine * cacheLine, bool replay);
    
    /** Handle ForceInv */
    CacheAction handleForceInv(MemEvent * event, CacheLine * cacheLine, bool replay);
    
    /** Handle Fetch */
    CacheAction handleFetch(MemEvent * event, CacheLine * cacheLine, bool replay);
    
    /** Handle FetchInv */
    CacheAction handleFetchInv(MemEvent * event, CacheLine * cacheLine, MemEvent * collisionEvent, bool replay);
    
    /** Handle FetchInvX */
    CacheAction handleFetchInvX(MemEvent * event, CacheLine * cacheLine, MemEvent * collisionEvent, bool replay);

    /** Process GetSResp/GetXResp.  Update the cache line */
    CacheAction handleDataResponse(MemEvent* responseEvent, CacheLine * cacheLine, MemEvent * reqEvent);
    
    /** Handle FetchResp */
    CacheAction handleFetchResp(MemEvent * responseEvent, CacheLine* cacheLine, MemEvent * reqEvent);
    
    /** Handle Ack */
    CacheAction handleAckInv(MemEvent * responseEvent, CacheLine* cacheLine, MemEvent * reqEvent);
    
/* Private methods for managing data structures */
    bool allocateLine(Addr addr, MemEvent* event);

/* Private methods for sending events */
    /** Send response to lower level cache */
    void sendResponseDown(MemEvent* event, CacheLine* cacheLine, bool dirty, bool replay);
    
    /** Send response to lower level cache, address is not cached */
    void sendResponseDownFromMSHR(MemEvent* response, MemEvent * request, bool dirty);

    /** Send writeback request to lower level caches */
    void sendWriteback(Command cmd, CacheLine* cacheLine, bool dirty, string origRqstr);
    
    /** Send AckPut to upper level cache */
    void sendWritebackAck(MemEvent * event);

    /** Send AckInv to lower level cache */
    void sendAckInv(MemEvent * event);

    /** Fetch data from owner and invalidate their copy of the line */
    void sendFetchInv(CacheLine * cacheLine, string rqstr, bool replay);
    
    /** Fetch data from owner and downgrade owner to sharer */
    void sendFetchInvX(CacheLine * cacheLine, string rqstr, bool replay);

    /** Force invalidation of line from owner, do not request data */
    void sendForceInv(CacheLine * cacheLine, string rqstr, bool replay);
    /** Invalidate all sharers of a block. Used for invalidations and evictions */
    void invalidateAllSharers(CacheLine * cacheLine, string rqstr, bool replay);
    
    /** Invalidate all sharers of a block except the requestor (rqstr). Used for upgrade requests. */
    bool invalidateSharersExceptRequestor(CacheLine * cacheLine, string rqstr, string origRqstr, bool replay);
    
    /** Send a flush response */
    void sendFlushResponse(MemEvent * reqEent, bool success);
    
    /** Forward a FlushLine request with or without data */
    void forwardFlushLine(Addr baseAddr, string origRqstr, CacheLine * cacheLine, Command cmd);

/* Helper methods */
   
    void printData(vector<uint8_t> * data, bool set);

/* Statistics */
    void recordStateEventCount(Command cmd, State state);
    void recordEvictionState(State state);
    void recordEventSentUp(Command cmd);
    void recordEventSentDown(Command cmd);
    void recordLatency(Command cmd, int type, uint64_t latency);
};


}}


#endif

