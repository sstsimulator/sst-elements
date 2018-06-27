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


#ifndef MESIINTERNALDIRCONTROLLER_H
#define MESIINTERNALDIRCONTROLLER_H

#include <iostream>

#include <sst/core/elementinfo.h>

#include "sst/elements/memHierarchy/coherencemgr/coherenceController.h"


namespace SST { namespace MemHierarchy {

class MESIInternalDirectory : public CoherenceController {
public:
    SST_ELI_REGISTER_SUBCOMPONENT(MESIInternalDirectory, "memHierarchy", "MESICacheDirectoryCoherenceController", SST_ELI_ELEMENT_VERSION(1,0,0), 
            "Implements MESI or MSI coherence for cache that is co-located with a directory, for noninclusive last-level caches", "SST::MemHierarchy::CoherenceController")

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
        {"eventSent_FetchResp",     "Number of FetchResp requests sent", "events", 2},
        {"eventSent_FetchXResp",    "Number of FetchXResp requests sent", "events", 2},
        {"eventSent_AckInv",        "Number of AckInvs sent", "events", 2},
        {"eventSent_AckPut",        "Number of AckPuts sent", "events", 2},
        {"eventSent_NACK_up",       "Number of NACKs sent up (towards CPU)", "events", 2},
        {"eventSent_NACK_down",     "Number of NACKs sent down (towards main memory)", "events", 2},
        {"eventSent_FlushLine",     "Number of FlushLine requests sent", "events", 2},
        {"eventSent_FlushLineInv",  "Number of FlushLineInv requests sent", "events", 2},
        {"eventSent_FlushLineResp", "Number of FlushLineResp responses sent", "events", 2},
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
        {"stateEvent_PutS_SD",          "Event/State: Number of times a PutS was seen in state S_D", "count", 3},
        {"stateEvent_PutS_ED",          "Event/State: Number of times a PutS was seen in state E_D", "count", 3},
        {"stateEvent_PutS_MD",          "Event/State: Number of times a PutS was seen in state M_D", "count", 3},
        {"stateEvent_PutS_SMD",         "Event/State: Number of times a PutS was seen in state SM_D", "count", 3},
        {"stateEvent_PutS_SI",          "Event/State: Number of times a PutS was seen in state SI", "count", 3},
        {"stateEvent_PutS_EI",          "Event/State: Number of times a PutS was seen in state EI", "count", 3},
        {"stateEvent_PutS_MI",          "Event/State: Number of times a PutS was seen in state MI", "count", 3},
        {"stateEvent_PutS_SInv",        "Event/State: Number of times a PutS was seen in state S_Inv", "count", 3},
        {"stateEvent_PutS_EInv",        "Event/State: Number of times a PutS was seen in state E_Inv", "count", 3},
        {"stateEvent_PutS_MInv",        "Event/State: Number of times a PutS was seen in state M_Inv", "count", 3},
        {"stateEvent_PutS_SMInv",       "Event/State: Number of times a PutS was seen in state SM_Inv", "count", 3},
        {"stateEvent_PutS_EInvX",       "Event/State: Number of times a PutS was seen in state E_InvX", "count", 3},
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
        {"stateEvent_Inv_IB",           "Event/State: Number of times an Inv was seen in state I_B", "count", 3},
        {"stateEvent_Inv_S",            "Event/State: Number of times an Inv was seen in state S", "count", 3},
        {"stateEvent_Inv_SM",           "Event/State: Number of times an Inv was seen in state SM", "count", 3},
        {"stateEvent_Inv_SInv",         "Event/State: Number of times an Inv was seen in state S_Inv", "count", 3},
        {"stateEvent_Inv_SI",           "Event/State: Number of times an Inv was seen in state SI", "count", 3},
        {"stateEvent_Inv_SMInv",        "Event/State: Number of times an Inv was seen in state SM_Inv", "count", 3},
        {"stateEvent_Inv_SD",           "Event/State: Number of times an Inv was seen in state S_D", "count", 3},
        {"stateEvent_Inv_SB",           "Event/State: Number of times an Inv was seen in state S_B", "count", 3},
        {"stateEvent_Inv_ED",           "Event/State: Number of times an Inv was seen in state E_D", "count", 3},
        {"stateEvent_Inv_EI",           "Event/State: Number of times an Inv was seen in state EI", "count", 3},
        {"stateEvent_Inv_EInv",         "Event/State: Number of times an Inv was seen in state E_Inv", "count", 3},
        {"stateEvent_Inv_EInvX",        "Event/State: Number of times an Inv was seen in state E_InvX", "count", 3},
        {"stateEvent_Inv_MD",           "Event/State: Number of times an Inv was seen in state M_D", "count", 3},
        {"stateEvent_Inv_MI",           "Event/State: Number of times an Inv was seen in state MI", "count", 3},
        {"stateEvent_Inv_MInv",         "Event/State: Number of times an Inv was seen in state M_Inv", "count", 3},
        {"stateEvent_Inv_MInvX",        "Event/State: Number of times an Inv was seen in state M_InvX", "count", 3},
        {"stateEvent_FetchInv_I",       "Event/State: Number of times a FetchInv was seen in state I", "count", 3},
        {"stateEvent_FetchInv_IS",      "Event/State: Number of times a FetchInv was seen in state IS", "count", 3},
        {"stateEvent_FetchInv_IM",      "Event/State: Number of times a FetchInv was seen in state IM", "count", 3},
        {"stateEvent_FetchInv_E",       "Event/State: Number of times a FetchInv was seen in state E", "count", 3},
        {"stateEvent_FetchInv_M",       "Event/State: Number of times a FetchInv was seen in state M", "count", 3},
        {"stateEvent_FetchInv_EI",      "Event/State: Number of times a FetchInv was seen in state EI", "count", 3},
        {"stateEvent_FetchInv_MI",      "Event/State: Number of times a FetchInv was seen in state MI", "count", 3},
        {"stateEvent_FetchInv_EInv",    "Event/State: Number of times a FetchInv was seen in state E_Inv", "count", 3},
        {"stateEvent_FetchInv_EInvX",   "Event/State: Number of times a FetchInv was seen in state E_InvX", "count", 3},
        {"stateEvent_FetchInv_MInv",    "Event/State: Number of times a FetchInv was seen in state M_Inv", "count", 3},
        {"stateEvent_FetchInv_MInvX",   "Event/State: Number of times a FetchInv was seen in state M_InvX", "count", 3},
        {"stateEvent_FetchInv_SD",      "Event/State: Number of times a FetchInv was seen in state S_D", "count", 3},
        {"stateEvent_FetchInv_ED",      "Event/State: Number of times a FetchInv was seen in state E_D", "count", 3},
        {"stateEvent_FetchInv_MD",      "Event/State: Number of times a FetchInv was seen in state M_D", "count", 3},
        {"stateEvent_FetchInv_IB",      "Event/State: Number of times a FetchInv was seen in state I_B", "count", 3},
        {"stateEvent_FetchInvX_I",      "Event/State: Number of times a FetchInvX was seen in state I", "count", 3},
        {"stateEvent_FetchInvX_IS",     "Event/State: Number of times a FetchInvX was seen in state IS", "count", 3},
        {"stateEvent_FetchInvX_IM",     "Event/State: Number of times a FetchInvX was seen in state IM", "count", 3},
        {"stateEvent_FetchInvX_E",      "Event/State: Number of times a FetchInvX was seen in state E", "count", 3},
        {"stateEvent_FetchInvX_M",      "Event/State: Number of times a FetchInvX was seen in state M", "count", 3},
        {"stateEvent_FetchInvX_IB",     "Event/State: Number of times a FetchInvX was seen in state I_B", "count", 3},
        {"stateEvent_FetchInvX_SB",     "Event/State: Number of times a FetchInvX was seen in state S_B", "count", 3},
        {"stateEvent_Fetch_I",          "Event/State: Number of times a Fetch was seen in state I", "count", 3},
        {"stateEvent_Fetch_IS",         "Event/State: Number of times a Fetch was seen in state IS", "count", 3},
        {"stateEvent_Fetch_IM",         "Event/State: Number of times a Fetch was seen in state IM", "count", 3},
        {"stateEvent_Fetch_S",          "Event/State: Number of times a Fetch was seen in state S", "count", 3},
        {"stateEvent_Fetch_SM",         "Event/State: Number of times a Fetch was seen in state SM", "count", 3},
        {"stateEvent_Fetch_SInv",       "Event/State: Number of times a Fetch was seen in state S_Inv", "count", 3},
        {"stateEvent_Fetch_SI",         "Event/State: Number of times a Fetch was seen in state SI", "count", 3},
        {"stateEvent_Fetch_SD",         "Event/State: Number of times a Fetch was seen in state S_D", "count", 3},
        {"stateEvent_FetchResp_SI",     "Event/State: Number of times a FetchResp was seen in state SI", "count", 3},
        {"stateEvent_FetchResp_EI",     "Event/State: Number of times a FetchResp was seen in state EI", "count", 3},
        {"stateEvent_FetchResp_MI",     "Event/State: Number of times a FetchResp was seen in state MI", "count", 3},
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
        {"stateEvent_FetchXResp_EInv",  "Event/State: Number of times a FetchXResp was seen in state E_Inv", "count", 3},
        {"stateEvent_FetchXResp_EInvX", "Event/State: Number of times a FetchXResp was seen in state E_InvX", "count", 3},
        {"stateEvent_FetchXResp_MInvX", "Event/State: Number of times a FetchXResp was seen in state M_InvX", "count", 3},
        {"stateEvent_FetchXResp_MInv",  "Event/State: Number of times a FetchXResp was seen in state M_Inv", "count", 3},
        {"stateEvent_FetchXResp_MI",    "Event/State: Number of times a FetchXResp was seen in state MI", "count", 3},
        {"stateEvent_FetchXResp_EI",    "Event/State: Number of times a FetchXResp was seen in state EI", "count", 3},
        {"stateEvent_FetchXResp_SI",    "Event/State: Number of times a FetchXResp was seen in state SI", "count", 3},
        {"stateEvent_FetchXResp_SD",    "Event/State: Number of times a FetchXResp was seen in state S_D", "count", 3},
        {"stateEvent_FetchXResp_ED",    "Event/State: Number of times a FetchXResp was seen in state E_D", "count", 3},
        {"stateEvent_FetchXResp_MD",    "Event/State: Number of times a FetchXResp was seen in state M_D", "count", 3},
        {"stateEvent_FetchXResp_SMD",   "Event/State: Number of times a FetchXResp was seen in state SM_D", "count", 3},
        {"stateEvent_FetchXResp_SInv",  "Event/State: Number of times a FetchXResp was seen in state S_Inv", "count", 3},
        {"stateEvent_FetchXResp_SMInv", "Event/State: Number of times a FetchXResp was seen in state SM_Inv", "count", 3},
        {"stateEvent_AckInv_I",         "Event/State: Number of times an AckInv was seen in state I", "count", 3},
        {"stateEvent_AckInv_SInv",      "Event/State: Number of times an AckInv was seen in state S_Inv", "count", 3},
        {"stateEvent_AckInv_SMInv",     "Event/State: Number of times an AckInv was seen in state SM_Inv", "count", 3},
        {"stateEvent_AckInv_SI",        "Event/State: Number of times an AckInv was seen in state SI", "count", 3},
        {"stateEvent_AckInv_EI",        "Event/State: Number of times an AckInv was seen in state EI", "count", 3},
        {"stateEvent_AckInv_MI",        "Event/State: Number of times an AckInv was seen in state MI", "count", 3},
        {"stateEvent_AckInv_EInv",      "Event/State: Number of times an AckInv was seen in state E_Inv", "count", 3},
        {"stateEvent_AckInv_MInv",      "Event/State: Number of times an AckInv was seen in state M_Inv", "count", 3},
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
        {"stateEvent_FlushLine_SD",     "Event/State: Number of times a FlushLine was seen in state S_D", "count", 3},
        {"stateEvent_FlushLine_ED",     "Event/State: Number of times a FlushLine was seen in state E_D", "count", 3},
        {"stateEvent_FlushLine_MD",     "Event/State: Number of times a FlushLine was seen in state M_D", "count", 3},
        {"stateEvent_FlushLine_SMD",    "Event/State: Number of times a FlushLine was seen in state SM_D", "count", 3},
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
        {"stateEvent_FlushLineInv_SD",      "Event/State: Number of times a FlushLineInv was seen in state S_D", "count", 3},
        {"stateEvent_FlushLineInv_ED",      "Event/State: Number of times a FlushLineInv was seen in state E_D", "count", 3},
        {"stateEvent_FlushLineInv_MD",      "Event/State: Number of times a FlushLineInv was seen in state M_D", "count", 3},
        {"stateEvent_FlushLineInv_SMD",     "Event/State: Number of times a FlushLineInv was seen in state SM_D", "count", 3},
        {"stateEvent_FlushLineInv_MI",      "Event/State: Number of times a FlushLineInv was seen in state MI", "count", 3},
        {"stateEvent_FlushLineInv_EI",      "Event/State: Number of times a FlushLineInv was seen in state EI", "count", 3},
        {"stateEvent_FlushLineInv_SI",      "Event/State: Number of times a FlushLineInv was seen in state SI", "count", 3},
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
        {"prefetch_inv",            "Prefetched block was invalidated before being accessed", "count", 2},
        {"prefetch_coherence_miss", "Prefetched block incurred a coherence miss (upgrade) on its first access", "count", 2},
        {"prefetch_redundant",      "Prefetch issued for a block that was already in cache", "count", 2})

/* Class definition */
    /** Constructor for MESIInternalDirectory. */
    MESIInternalDirectory(Component * comp, Params& params) : CoherenceController(comp, params) {
        
        debug->debug(_INFO_,"--------------------------- Initializing [MESI + Directory Controller] ... \n\n");
        
        protocol_ = params.find<bool>("protocol", 1);
        
        /* Statistics */
        stat_evict_S =      registerStatistic<uint64_t>("evict_S");
        stat_evict_SM =     registerStatistic<uint64_t>("evict_SM");
        stat_evict_SInv =   registerStatistic<uint64_t>("evict_SInv");
        stat_evict_MInv =   registerStatistic<uint64_t>("evict_MInv");
        stat_evict_SMInv =  registerStatistic<uint64_t>("evict_SMInv");
        stat_evict_MInvX =  registerStatistic<uint64_t>("evict_MInvX");
        stat_evict_SI =     registerStatistic<uint64_t>("evict_SI");
        stat_stateEvent_GetS_I =    registerStatistic<uint64_t>("stateEvent_GetS_I");
        stat_stateEvent_GetS_S =    registerStatistic<uint64_t>("stateEvent_GetS_S");
        stat_stateEvent_GetS_M =    registerStatistic<uint64_t>("stateEvent_GetS_M");
        stat_stateEvent_GetX_I =    registerStatistic<uint64_t>("stateEvent_GetX_I");
        stat_stateEvent_GetX_S =    registerStatistic<uint64_t>("stateEvent_GetX_S");
        stat_stateEvent_GetX_M =    registerStatistic<uint64_t>("stateEvent_GetX_M");
        stat_stateEvent_GetSX_I =  registerStatistic<uint64_t>("stateEvent_GetSX_I");
        stat_stateEvent_GetSX_S =  registerStatistic<uint64_t>("stateEvent_GetSX_S");
        stat_stateEvent_GetSX_M =  registerStatistic<uint64_t>("stateEvent_GetSX_M");
        stat_stateEvent_GetSResp_IS =   registerStatistic<uint64_t>("stateEvent_GetSResp_IS");
        stat_stateEvent_GetXResp_IS =   registerStatistic<uint64_t>("stateEvent_GetXResp_IS");
        stat_stateEvent_GetXResp_IM =   registerStatistic<uint64_t>("stateEvent_GetXResp_IM");
        stat_stateEvent_GetXResp_SM =   registerStatistic<uint64_t>("stateEvent_GetXResp_SM");
        stat_stateEvent_GetXResp_SMInv = registerStatistic<uint64_t>("stateEvent_GetXResp_SMInv");
        stat_stateEvent_PutS_I =    registerStatistic<uint64_t>("stateEvent_PutS_I");
        stat_stateEvent_PutS_S =    registerStatistic<uint64_t>("stateEvent_PutS_S");
        stat_stateEvent_PutS_M =    registerStatistic<uint64_t>("stateEvent_PutS_M");
        stat_stateEvent_PutS_MInv = registerStatistic<uint64_t>("stateEvent_PutS_MInv");
        stat_stateEvent_PutS_SInv =     registerStatistic<uint64_t>("stateEvent_PutS_SInv");
        stat_stateEvent_PutS_SMInv =    registerStatistic<uint64_t>("stateEvent_PutS_SMInv");
        stat_stateEvent_PutS_MI =   registerStatistic<uint64_t>("stateEvent_PutS_MI");
        stat_stateEvent_PutS_SI =   registerStatistic<uint64_t>("stateEvent_PutS_SI");
        stat_stateEvent_PutS_IB =   registerStatistic<uint64_t>("stateEvent_PutS_IB");
        stat_stateEvent_PutS_SB =   registerStatistic<uint64_t>("stateEvent_PutS_SB");
        stat_stateEvent_PutS_SBInv = registerStatistic<uint64_t>("stateEvent_PutS_SBInv");
        stat_stateEvent_PutS_SD =   registerStatistic<uint64_t>("stateEvent_PutS_SD");
        stat_stateEvent_PutS_MD =   registerStatistic<uint64_t>("stateEvent_PutS_MD");
        stat_stateEvent_PutS_SMD =  registerStatistic<uint64_t>("stateEvent_PutS_SMD");
        stat_stateEvent_PutM_I =    registerStatistic<uint64_t>("stateEvent_PutM_I");
        stat_stateEvent_PutM_M =    registerStatistic<uint64_t>("stateEvent_PutM_M");
        stat_stateEvent_PutM_MInv = registerStatistic<uint64_t>("stateEvent_PutM_MInv");
        stat_stateEvent_PutM_MInvX =    registerStatistic<uint64_t>("stateEvent_PutM_MInvX");
        stat_stateEvent_PutM_MI =   registerStatistic<uint64_t>("stateEvent_PutM_MI");
        stat_stateEvent_Inv_I =     registerStatistic<uint64_t>("stateEvent_Inv_I");
        stat_stateEvent_Inv_S =     registerStatistic<uint64_t>("stateEvent_Inv_S");
        stat_stateEvent_Inv_SM =    registerStatistic<uint64_t>("stateEvent_Inv_SM");
        stat_stateEvent_Inv_MInv =  registerStatistic<uint64_t>("stateEvent_Inv_MInv");
        stat_stateEvent_Inv_MInvX = registerStatistic<uint64_t>("stateEvent_Inv_MInvX");
        stat_stateEvent_Inv_SInv =  registerStatistic<uint64_t>("stateEvent_Inv_SInv");
        stat_stateEvent_Inv_SMInv = registerStatistic<uint64_t>("stateEvent_Inv_SMInv");
        stat_stateEvent_Inv_SI =    registerStatistic<uint64_t>("stateEvent_Inv_SI");
        stat_stateEvent_Inv_MI =    registerStatistic<uint64_t>("stateEvent_Inv_MI");
        stat_stateEvent_Inv_SB =    registerStatistic<uint64_t>("stateEvent_Inv_SB");
        stat_stateEvent_Inv_IB =    registerStatistic<uint64_t>("stateEvent_Inv_IB");
        stat_stateEvent_Inv_SD =    registerStatistic<uint64_t>("stateEvent_Inv_SD");
        stat_stateEvent_Inv_MD =    registerStatistic<uint64_t>("stateEvent_Inv_MD");
        stat_stateEvent_FetchInvX_I =   registerStatistic<uint64_t>("stateEvent_FetchInvX_I");
        stat_stateEvent_FetchInvX_M =   registerStatistic<uint64_t>("stateEvent_FetchInvX_M");
        stat_stateEvent_FetchInvX_IS =  registerStatistic<uint64_t>("stateEvent_FetchInvX_IS");
        stat_stateEvent_FetchInvX_IM =  registerStatistic<uint64_t>("stateEvent_FetchInvX_IM");
        stat_stateEvent_FetchInvX_IB =  registerStatistic<uint64_t>("stateEvent_FetchInvX_IB");
        stat_stateEvent_FetchInvX_SB =  registerStatistic<uint64_t>("stateEvent_FetchInvX_SB");
        stat_stateEvent_Fetch_I =       registerStatistic<uint64_t>("stateEvent_Fetch_I");
        stat_stateEvent_Fetch_S =       registerStatistic<uint64_t>("stateEvent_Fetch_S");
        stat_stateEvent_Fetch_IS =      registerStatistic<uint64_t>("stateEvent_Fetch_IS");
        stat_stateEvent_Fetch_IM =      registerStatistic<uint64_t>("stateEvent_Fetch_IM");
        stat_stateEvent_Fetch_SM =      registerStatistic<uint64_t>("stateEvent_Fetch_SM");
        stat_stateEvent_Fetch_SD =      registerStatistic<uint64_t>("stateEvent_Fetch_SD");
        stat_stateEvent_Fetch_SInv =    registerStatistic<uint64_t>("stateEvent_Fetch_SInv");
        stat_stateEvent_Fetch_SI =      registerStatistic<uint64_t>("stateEvent_Fetch_SI");
        stat_stateEvent_FetchInv_I =        registerStatistic<uint64_t>("stateEvent_FetchInv_I");
        stat_stateEvent_FetchInv_M =        registerStatistic<uint64_t>("stateEvent_FetchInv_M");
        stat_stateEvent_FetchInv_IS =       registerStatistic<uint64_t>("stateEvent_FetchInv_IS");
        stat_stateEvent_FetchInv_IM =       registerStatistic<uint64_t>("stateEvent_FetchInv_IM");
        stat_stateEvent_FetchInv_MInv =     registerStatistic<uint64_t>("stateEvent_FetchInv_MInv");
        stat_stateEvent_FetchInv_MInvX =    registerStatistic<uint64_t>("stateEvent_FetchInv_MInvX");
        stat_stateEvent_FetchInv_MI =       registerStatistic<uint64_t>("stateEvent_FetchInv_MI");
        stat_stateEvent_FetchInv_IB =       registerStatistic<uint64_t>("stateEvent_FetchInv_IB");
        stat_stateEvent_FetchInv_SD =       registerStatistic<uint64_t>("stateEvent_FetchInv_SD");
        stat_stateEvent_FetchInv_MD =       registerStatistic<uint64_t>("stateEvent_FetchInv_MD");
        stat_stateEvent_FetchResp_MInv =    registerStatistic<uint64_t>("stateEvent_FetchResp_MInv");
        stat_stateEvent_FetchResp_MInvX =   registerStatistic<uint64_t>("stateEvent_FetchResp_MInvX");
        stat_stateEvent_FetchResp_SD =      registerStatistic<uint64_t>("stateEvent_FetchResp_SD");
        stat_stateEvent_FetchResp_MD =      registerStatistic<uint64_t>("stateEvent_FetchResp_MD");
        stat_stateEvent_FetchResp_SMD =     registerStatistic<uint64_t>("stateEvent_FetchResp_SMD");
        stat_stateEvent_FetchResp_SInv =    registerStatistic<uint64_t>("stateEvent_FetchResp_SInv");
        stat_stateEvent_FetchResp_SMInv =   registerStatistic<uint64_t>("stateEvent_FetchResp_SMInv");
        stat_stateEvent_FetchResp_MI =      registerStatistic<uint64_t>("stateEvent_FetchResp_MI");
        stat_stateEvent_FetchResp_SI =      registerStatistic<uint64_t>("stateEvent_FetchResp_SI");
        stat_stateEvent_FetchXResp_MInv =   registerStatistic<uint64_t>("stateEvent_FetchXResp_MInv");
        stat_stateEvent_FetchXResp_MInvX =  registerStatistic<uint64_t>("stateEvent_FetchXResp_MInvX");
        stat_stateEvent_FetchXResp_SD =     registerStatistic<uint64_t>("stateEvent_FetchXResp_SD");
        stat_stateEvent_FetchXResp_MD =     registerStatistic<uint64_t>("stateEvent_FetchXResp_MD");
        stat_stateEvent_FetchXResp_SMD =    registerStatistic<uint64_t>("stateEvent_FetchXResp_SMD");
        stat_stateEvent_FetchXResp_SInv =   registerStatistic<uint64_t>("stateEvent_FetchXResp_SInv");
        stat_stateEvent_FetchXResp_SMInv =  registerStatistic<uint64_t>("stateEvent_FetchXResp_SMInv");
        stat_stateEvent_FetchXResp_MI =     registerStatistic<uint64_t>("stateEvent_FetchXResp_MI");
        stat_stateEvent_FetchXResp_SI =     registerStatistic<uint64_t>("stateEvent_FetchXResp_SI");
        stat_stateEvent_AckInv_I =      registerStatistic<uint64_t>("stateEvent_AckInv_I");
        stat_stateEvent_AckInv_MInv =   registerStatistic<uint64_t>("stateEvent_AckInv_MInv");
        stat_stateEvent_AckInv_SInv =   registerStatistic<uint64_t>("stateEvent_AckInv_SInv");
        stat_stateEvent_AckInv_SMInv =  registerStatistic<uint64_t>("stateEvent_AckInv_SMInv");
        stat_stateEvent_AckInv_MI =     registerStatistic<uint64_t>("stateEvent_AckInv_MI");
        stat_stateEvent_AckInv_SI =     registerStatistic<uint64_t>("stateEvent_AckInv_SI");
        stat_stateEvent_AckInv_SBInv =  registerStatistic<uint64_t>("stateEvent_AckInv_SBInv");
        stat_stateEvent_AckPut_I =      registerStatistic<uint64_t>("stateEvent_AckPut_I");
        stat_stateEvent_FlushLine_I =       registerStatistic<uint64_t>("stateEvent_FlushLine_I");
        stat_stateEvent_FlushLine_S =       registerStatistic<uint64_t>("stateEvent_FlushLine_S");
        stat_stateEvent_FlushLine_M =       registerStatistic<uint64_t>("stateEvent_FlushLine_M");
        stat_stateEvent_FlushLine_IS =      registerStatistic<uint64_t>("stateEvent_FlushLine_IS");
        stat_stateEvent_FlushLine_IM =      registerStatistic<uint64_t>("stateEvent_FlushLine_IM");
        stat_stateEvent_FlushLine_SM =      registerStatistic<uint64_t>("stateEvent_FlushLine_SM");
        stat_stateEvent_FlushLine_MInv =    registerStatistic<uint64_t>("stateEvent_FlushLine_MInv");
        stat_stateEvent_FlushLine_MInvX =   registerStatistic<uint64_t>("stateEvent_FlushLine_MInvX");
        stat_stateEvent_FlushLine_SInv =    registerStatistic<uint64_t>("stateEvent_FlushLine_SInv");
        stat_stateEvent_FlushLine_SMInv =   registerStatistic<uint64_t>("stateEvent_FlushLine_SMInv");
        stat_stateEvent_FlushLine_SD =      registerStatistic<uint64_t>("stateEvent_FlushLine_SD");
        stat_stateEvent_FlushLine_MD =      registerStatistic<uint64_t>("stateEvent_FlushLine_MD");
        stat_stateEvent_FlushLine_SMD =     registerStatistic<uint64_t>("stateEvent_FlushLine_SMD");
        stat_stateEvent_FlushLine_MI =      registerStatistic<uint64_t>("stateEvent_FlushLine_MI");
        stat_stateEvent_FlushLine_SI =      registerStatistic<uint64_t>("stateEvent_FlushLine_SI");
        stat_stateEvent_FlushLine_IB =      registerStatistic<uint64_t>("stateEvent_FlushLine_IB");
        stat_stateEvent_FlushLine_SB =      registerStatistic<uint64_t>("stateEvent_FlushLine_SB");
        stat_stateEvent_FlushLineInv_I =    registerStatistic<uint64_t>("stateEvent_FlushLineInv_I");
        stat_stateEvent_FlushLineInv_S =    registerStatistic<uint64_t>("stateEvent_FlushLineInv_S");
        stat_stateEvent_FlushLineInv_M =    registerStatistic<uint64_t>("stateEvent_FlushLineInv_M");
        stat_stateEvent_FlushLineInv_IS =   registerStatistic<uint64_t>("stateEvent_FlushLineInv_IS");
        stat_stateEvent_FlushLineInv_IM =   registerStatistic<uint64_t>("stateEvent_FlushLineInv_IM");
        stat_stateEvent_FlushLineInv_SM =   registerStatistic<uint64_t>("stateEvent_FlushLineInv_SM");
        stat_stateEvent_FlushLineInv_MInv =     registerStatistic<uint64_t>("stateEvent_FlushLineInv_MInv");
        stat_stateEvent_FlushLineInv_MInvX =    registerStatistic<uint64_t>("stateEvent_FlushLineInv_MInvX");
        stat_stateEvent_FlushLineInv_SInv =     registerStatistic<uint64_t>("stateEvent_FlushLineInv_SInv");
        stat_stateEvent_FlushLineInv_SMInv =    registerStatistic<uint64_t>("stateEvent_FlushLineInv_SMInv");
        stat_stateEvent_FlushLineInv_SD =   registerStatistic<uint64_t>("stateEvent_FlushLineInv_SD");
        stat_stateEvent_FlushLineInv_MD =   registerStatistic<uint64_t>("stateEvent_FlushLineInv_MD");
        stat_stateEvent_FlushLineInv_SMD =  registerStatistic<uint64_t>("stateEvent_FlushLineInv_SMD");
        stat_stateEvent_FlushLineInv_MI =   registerStatistic<uint64_t>("stateEvent_FlushLineInv_MI");
        stat_stateEvent_FlushLineInv_SI =   registerStatistic<uint64_t>("stateEvent_FlushLineInv_SI");
        stat_stateEvent_FlushLineResp_I =   registerStatistic<uint64_t>("stateEvent_FlushLineResp_I");
        stat_stateEvent_FlushLineResp_IB =  registerStatistic<uint64_t>("stateEvent_FlushLineResp_IB");
        stat_stateEvent_FlushLineResp_SB =  registerStatistic<uint64_t>("stateEvent_FlushLineResp_SB");
        stat_eventSent_GetS             = registerStatistic<uint64_t>("eventSent_GetS");
        stat_eventSent_GetX             = registerStatistic<uint64_t>("eventSent_GetX");
        stat_eventSent_GetSX           = registerStatistic<uint64_t>("eventSent_GetSX");
        stat_eventSent_PutS             = registerStatistic<uint64_t>("eventSent_PutS");
        stat_eventSent_PutM             = registerStatistic<uint64_t>("eventSent_PutM");
        stat_eventSent_FlushLine        = registerStatistic<uint64_t>("eventSent_FlushLine");
        stat_eventSent_FlushLineInv     = registerStatistic<uint64_t>("eventSent_FlushLineInv");
        stat_eventSent_FetchResp        = registerStatistic<uint64_t>("eventSent_FetchResp");
        stat_eventSent_FetchXResp       = registerStatistic<uint64_t>("eventSent_FetchXResp");
        stat_eventSent_AckInv           = registerStatistic<uint64_t>("eventSent_AckInv");
        stat_eventSent_NACK_down        = registerStatistic<uint64_t>("eventSent_NACK_down");
        stat_eventSent_GetSResp         = registerStatistic<uint64_t>("eventSent_GetSResp");
        stat_eventSent_GetXResp         = registerStatistic<uint64_t>("eventSent_GetXResp");
        stat_eventSent_FlushLineResp    = registerStatistic<uint64_t>("eventSent_FlushLineResp");
        stat_eventSent_Inv              = registerStatistic<uint64_t>("eventSent_Inv");
        stat_eventSent_Fetch            = registerStatistic<uint64_t>("eventSent_Fetch");
        stat_eventSent_FetchInv         = registerStatistic<uint64_t>("eventSent_FetchInv");
        stat_eventSent_FetchInvX        = registerStatistic<uint64_t>("eventSent_FetchInvX");
        stat_eventSent_AckPut           = registerStatistic<uint64_t>("eventSent_AckPut");
        stat_eventSent_NACK_up          = registerStatistic<uint64_t>("eventSent_NACK_up");
        
        /* Prefetch statistics */
        if (!params.find<std::string>("prefetcher", "").empty()) {
            statPrefetchEvict = registerStatistic<uint64_t>("prefetch_evict");
            statPrefetchInv = registerStatistic<uint64_t>("prefetch_inv");
            statPrefetchHit = registerStatistic<uint64_t>("prefetch_useful");
            statPrefetchUpgradeMiss = registerStatistic<uint64_t>("prefetch_coherence_miss");
            statPrefetchRedundant = registerStatistic<uint64_t>("prefetch_redundant");
        }

        /* MESI-specific statistics (as opposed to MSI) */
        if (protocol_) {
            stat_evict_EInv =   registerStatistic<uint64_t>("evict_EInv");
            stat_evict_EInvX =  registerStatistic<uint64_t>("evict_EInvX");
            stat_stateEvent_GetS_E =    registerStatistic<uint64_t>("stateEvent_GetS_E");
            stat_stateEvent_GetX_E =    registerStatistic<uint64_t>("stateEvent_GetX_E");
            stat_stateEvent_GetSX_E =  registerStatistic<uint64_t>("stateEvent_GetSX_E");
            stat_stateEvent_PutS_E =    registerStatistic<uint64_t>("stateEvent_PutS_E");
            stat_stateEvent_PutS_EInv = registerStatistic<uint64_t>("stateEvent_PutS_EInv");
            stat_stateEvent_PutS_EInvX =    registerStatistic<uint64_t>("stateEvent_PutS_EInvX");
            stat_stateEvent_PutS_EI =   registerStatistic<uint64_t>("stateEvent_PutS_EI");
            stat_stateEvent_PutS_ED =   registerStatistic<uint64_t>("stateEvent_PutS_ED");
            stat_stateEvent_PutE_I =    registerStatistic<uint64_t>("stateEvent_PutE_I");
            stat_stateEvent_PutE_M =    registerStatistic<uint64_t>("stateEvent_PutE_M");
            stat_stateEvent_PutE_MInv = registerStatistic<uint64_t>("stateEvent_PutE_MInv");
            stat_stateEvent_PutE_MInvX =    registerStatistic<uint64_t>("stateEvent_PutE_MInvX");
            stat_stateEvent_PutE_E =    registerStatistic<uint64_t>("stateEvent_PutE_E");
            stat_stateEvent_PutE_EInv =     registerStatistic<uint64_t>("stateEvent_PutE_EInv");
            stat_stateEvent_PutE_EInvX =    registerStatistic<uint64_t>("stateEvent_PutE_EInvX");
            stat_stateEvent_PutE_EI =   registerStatistic<uint64_t>("stateEvent_PutE_EI");
            stat_stateEvent_PutE_MI =   registerStatistic<uint64_t>("stateEvent_PutE_MI");
            stat_stateEvent_PutM_E =    registerStatistic<uint64_t>("stateEvent_PutM_E");
            stat_stateEvent_PutM_EInv =     registerStatistic<uint64_t>("stateEvent_PutM_EInv");
            stat_stateEvent_PutM_EInvX =    registerStatistic<uint64_t>("stateEvent_PutM_EInvX");
            stat_stateEvent_PutM_EI =   registerStatistic<uint64_t>("stateEvent_PutM_EI");
            stat_stateEvent_Inv_EInv =  registerStatistic<uint64_t>("stateEvent_Inv_EInv");
            stat_stateEvent_Inv_EInvX = registerStatistic<uint64_t>("stateEvent_Inv_EInvX");
            stat_stateEvent_Inv_EI =    registerStatistic<uint64_t>("stateEvent_Inv_EI");
            stat_stateEvent_Inv_ED =    registerStatistic<uint64_t>("stateEvent_Inv_ED");
            stat_stateEvent_FetchInvX_E =   registerStatistic<uint64_t>("stateEvent_FetchInvX_E");
            stat_stateEvent_FetchInv_E =        registerStatistic<uint64_t>("stateEvent_FetchInv_E");
            stat_stateEvent_FetchInv_EInv =     registerStatistic<uint64_t>("stateEvent_FetchInv_EInv");
            stat_stateEvent_FetchInv_EInvX =    registerStatistic<uint64_t>("stateEvent_FetchInv_EInvX");
            stat_stateEvent_FetchInv_EI =       registerStatistic<uint64_t>("stateEvent_FetchInv_EI");
            stat_stateEvent_FetchInv_ED =       registerStatistic<uint64_t>("stateEvent_FetchInv_ED");
            stat_stateEvent_FetchResp_EInv =    registerStatistic<uint64_t>("stateEvent_FetchResp_EInv");
            stat_stateEvent_FetchResp_EInvX =   registerStatistic<uint64_t>("stateEvent_FetchResp_EInvX");
            stat_stateEvent_FetchResp_ED =      registerStatistic<uint64_t>("stateEvent_FetchResp_ED");
            stat_stateEvent_FetchResp_EI =      registerStatistic<uint64_t>("stateEvent_FetchResp_EI");
            stat_stateEvent_FetchXResp_EInv =   registerStatistic<uint64_t>("stateEvent_FetchXResp_EInv");
            stat_stateEvent_FetchXResp_EInvX =  registerStatistic<uint64_t>("stateEvent_FetchXResp_EInvX");
            stat_stateEvent_FetchXResp_ED =     registerStatistic<uint64_t>("stateEvent_FetchXResp_ED");
            stat_stateEvent_FetchXResp_EI =     registerStatistic<uint64_t>("stateEvent_FetchXResp_EI");
            stat_stateEvent_AckInv_EInv =   registerStatistic<uint64_t>("stateEvent_AckInv_EInv");
            stat_stateEvent_AckInv_EI =     registerStatistic<uint64_t>("stateEvent_AckInv_EI");
            stat_stateEvent_FlushLine_E =       registerStatistic<uint64_t>("stateEvent_FlushLine_E");
            stat_stateEvent_FlushLine_EInv =    registerStatistic<uint64_t>("stateEvent_FlushLine_EInv");
            stat_stateEvent_FlushLine_EInvX =   registerStatistic<uint64_t>("stateEvent_FlushLine_EInvX");
            stat_stateEvent_FlushLine_ED =      registerStatistic<uint64_t>("stateEvent_FlushLine_ED");
            stat_stateEvent_FlushLine_EI =      registerStatistic<uint64_t>("stateEvent_FlushLine_EI");
            stat_stateEvent_FlushLineInv_E =    registerStatistic<uint64_t>("stateEvent_FlushLineInv_E");
            stat_stateEvent_FlushLineInv_EInv =     registerStatistic<uint64_t>("stateEvent_FlushLineInv_EInv");
            stat_stateEvent_FlushLineInv_EInvX =    registerStatistic<uint64_t>("stateEvent_FlushLineInv_EInvX");
            stat_stateEvent_FlushLineInv_ED =   registerStatistic<uint64_t>("stateEvent_FlushLineInv_ED");
            stat_stateEvent_FlushLineInv_EI =   registerStatistic<uint64_t>("stateEvent_FlushLineInv_EI");
            stat_eventSent_PutE             = registerStatistic<uint64_t>("eventSent_PutE");
        }
    }

    ~MESIInternalDirectory() {}
    
/*----------------------------------------------------------------------------------------------------------------------
 *  Public functions form external interface to the coherence controller
 *---------------------------------------------------------------------------------------------------------------------*/  

/* Event handlers */
    /** Send cache line data to the lower level caches */
    CacheAction handleEviction(CacheLine* replacementLine, string origRqstr, bool fromDataCache);

    /** Process cache request:  GetX, GetS, GetSX */
    CacheAction handleRequest(MemEvent* event, CacheLine* dirLine, bool replay);
    
    /** Process replacement request - PutS, PutE, PutM. May also resolve an outstanding/racing request event */
    CacheAction handleReplacement(MemEvent* event, CacheLine* dirLine, MemEvent * reqEvent, bool replay);
    
    /** Process invalidation requests - Inv, FetchInv, FetchInvX */
    CacheAction handleInvalidationRequest(MemEvent *event, CacheLine* dirLine, MemEvent * collisionEvent, bool replay);

    /** Process responses - GetSResp, GetXResp, FetchResp */
    CacheAction handleResponse(MemEvent* responseEvent, CacheLine* dirLine, MemEvent* origRequest);
    

/* Miscellaneous */
    
    /** Determine in advance if a request will miss (and what kind of miss). Used for stats */
    int isCoherenceMiss(MemEvent* event, CacheLine* dirLine);

    /** Determine whether a NACKed event should be retried */
    bool isRetryNeeded(MemEvent * event, CacheLine * cacheLine);

    /** Message send: Call through to coherenceController with statistic recording */
    void addToOutgoingQueue(Response& resp);
    void addToOutgoingQueueUp(Response& resp);

private:
/* Private data members */
    bool                protocol_;  // True for MESI, false for MSI

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
    Statistic<uint64_t>* stat_stateEvent_GetS_I;
    Statistic<uint64_t>* stat_stateEvent_GetS_S;
    Statistic<uint64_t>* stat_stateEvent_GetS_E;
    Statistic<uint64_t>* stat_stateEvent_GetS_M;
    Statistic<uint64_t>* stat_stateEvent_GetX_I;
    Statistic<uint64_t>* stat_stateEvent_GetX_S;
    Statistic<uint64_t>* stat_stateEvent_GetX_E;
    Statistic<uint64_t>* stat_stateEvent_GetX_M;
    Statistic<uint64_t>* stat_stateEvent_GetSX_I;
    Statistic<uint64_t>* stat_stateEvent_GetSX_S;
    Statistic<uint64_t>* stat_stateEvent_GetSX_E;
    Statistic<uint64_t>* stat_stateEvent_GetSX_M;
    Statistic<uint64_t>* stat_stateEvent_GetSResp_IS;
    Statistic<uint64_t>* stat_stateEvent_GetXResp_IS;
    Statistic<uint64_t>* stat_stateEvent_GetXResp_IM;
    Statistic<uint64_t>* stat_stateEvent_GetXResp_SM;
    Statistic<uint64_t>* stat_stateEvent_GetXResp_SMInv;
    Statistic<uint64_t>* stat_stateEvent_PutS_I;
    Statistic<uint64_t>* stat_stateEvent_PutS_S;
    Statistic<uint64_t>* stat_stateEvent_PutS_E;
    Statistic<uint64_t>* stat_stateEvent_PutS_M;
    Statistic<uint64_t>* stat_stateEvent_PutS_MInv;
    Statistic<uint64_t>* stat_stateEvent_PutS_EInv;
    Statistic<uint64_t>* stat_stateEvent_PutS_EInvX;
    Statistic<uint64_t>* stat_stateEvent_PutS_SInv;
    Statistic<uint64_t>* stat_stateEvent_PutS_SMInv;
    Statistic<uint64_t>* stat_stateEvent_PutS_MI;
    Statistic<uint64_t>* stat_stateEvent_PutS_EI;
    Statistic<uint64_t>* stat_stateEvent_PutS_SI;
    Statistic<uint64_t>* stat_stateEvent_PutS_IB;
    Statistic<uint64_t>* stat_stateEvent_PutS_SB;
    Statistic<uint64_t>* stat_stateEvent_PutS_SBInv;
    Statistic<uint64_t>* stat_stateEvent_PutS_SD;
    Statistic<uint64_t>* stat_stateEvent_PutS_ED;
    Statistic<uint64_t>* stat_stateEvent_PutS_MD;
    Statistic<uint64_t>* stat_stateEvent_PutS_SMD;
    Statistic<uint64_t>* stat_stateEvent_PutE_I;
    Statistic<uint64_t>* stat_stateEvent_PutE_E;
    Statistic<uint64_t>* stat_stateEvent_PutE_M;
    Statistic<uint64_t>* stat_stateEvent_PutE_MInv;
    Statistic<uint64_t>* stat_stateEvent_PutE_MInvX;
    Statistic<uint64_t>* stat_stateEvent_PutE_EInv;
    Statistic<uint64_t>* stat_stateEvent_PutE_EInvX;
    Statistic<uint64_t>* stat_stateEvent_PutE_MI;
    Statistic<uint64_t>* stat_stateEvent_PutE_EI;
    Statistic<uint64_t>* stat_stateEvent_PutM_I;
    Statistic<uint64_t>* stat_stateEvent_PutM_E;
    Statistic<uint64_t>* stat_stateEvent_PutM_M;
    Statistic<uint64_t>* stat_stateEvent_PutM_MInv;
    Statistic<uint64_t>* stat_stateEvent_PutM_MInvX;
    Statistic<uint64_t>* stat_stateEvent_PutM_EInv;
    Statistic<uint64_t>* stat_stateEvent_PutM_EInvX;
    Statistic<uint64_t>* stat_stateEvent_PutM_MI;
    Statistic<uint64_t>* stat_stateEvent_PutM_EI;
    Statistic<uint64_t>* stat_stateEvent_Inv_I;
    Statistic<uint64_t>* stat_stateEvent_Inv_S;
    Statistic<uint64_t>* stat_stateEvent_Inv_SM;
    Statistic<uint64_t>* stat_stateEvent_Inv_MInv;
    Statistic<uint64_t>* stat_stateEvent_Inv_MInvX;
    Statistic<uint64_t>* stat_stateEvent_Inv_EInv;
    Statistic<uint64_t>* stat_stateEvent_Inv_EInvX;
    Statistic<uint64_t>* stat_stateEvent_Inv_SInv;
    Statistic<uint64_t>* stat_stateEvent_Inv_SMInv;
    Statistic<uint64_t>* stat_stateEvent_Inv_SI;
    Statistic<uint64_t>* stat_stateEvent_Inv_EI;
    Statistic<uint64_t>* stat_stateEvent_Inv_MI;
    Statistic<uint64_t>* stat_stateEvent_Inv_SB;
    Statistic<uint64_t>* stat_stateEvent_Inv_IB;
    Statistic<uint64_t>* stat_stateEvent_Inv_SD;
    Statistic<uint64_t>* stat_stateEvent_Inv_ED;
    Statistic<uint64_t>* stat_stateEvent_Inv_MD;
    Statistic<uint64_t>* stat_stateEvent_FetchInvX_I;
    Statistic<uint64_t>* stat_stateEvent_FetchInvX_E;
    Statistic<uint64_t>* stat_stateEvent_FetchInvX_M;
    Statistic<uint64_t>* stat_stateEvent_FetchInvX_IS;
    Statistic<uint64_t>* stat_stateEvent_FetchInvX_IM;
    Statistic<uint64_t>* stat_stateEvent_FetchInvX_IB;
    Statistic<uint64_t>* stat_stateEvent_FetchInvX_SB;
    Statistic<uint64_t>* stat_stateEvent_Fetch_I;
    Statistic<uint64_t>* stat_stateEvent_Fetch_S;
    Statistic<uint64_t>* stat_stateEvent_Fetch_IS;
    Statistic<uint64_t>* stat_stateEvent_Fetch_IM;
    Statistic<uint64_t>* stat_stateEvent_Fetch_SM;
    Statistic<uint64_t>* stat_stateEvent_Fetch_SD;
    Statistic<uint64_t>* stat_stateEvent_Fetch_SInv;
    Statistic<uint64_t>* stat_stateEvent_Fetch_SI;
    Statistic<uint64_t>* stat_stateEvent_FetchInv_I;
    Statistic<uint64_t>* stat_stateEvent_FetchInv_E;
    Statistic<uint64_t>* stat_stateEvent_FetchInv_M;
    Statistic<uint64_t>* stat_stateEvent_FetchInv_IS;
    Statistic<uint64_t>* stat_stateEvent_FetchInv_IM;
    Statistic<uint64_t>* stat_stateEvent_FetchInv_MInv;
    Statistic<uint64_t>* stat_stateEvent_FetchInv_MInvX;
    Statistic<uint64_t>* stat_stateEvent_FetchInv_EInv;
    Statistic<uint64_t>* stat_stateEvent_FetchInv_EInvX;
    Statistic<uint64_t>* stat_stateEvent_FetchInv_MI;
    Statistic<uint64_t>* stat_stateEvent_FetchInv_EI;
    Statistic<uint64_t>* stat_stateEvent_FetchInv_IB;
    Statistic<uint64_t>* stat_stateEvent_FetchInv_SD;
    Statistic<uint64_t>* stat_stateEvent_FetchInv_ED;
    Statistic<uint64_t>* stat_stateEvent_FetchInv_MD;
    Statistic<uint64_t>* stat_stateEvent_FetchResp_MInv;
    Statistic<uint64_t>* stat_stateEvent_FetchResp_MInvX;
    Statistic<uint64_t>* stat_stateEvent_FetchResp_EInv;
    Statistic<uint64_t>* stat_stateEvent_FetchResp_EInvX;
    Statistic<uint64_t>* stat_stateEvent_FetchResp_SD;
    Statistic<uint64_t>* stat_stateEvent_FetchResp_ED;
    Statistic<uint64_t>* stat_stateEvent_FetchResp_MD;
    Statistic<uint64_t>* stat_stateEvent_FetchResp_SMD;
    Statistic<uint64_t>* stat_stateEvent_FetchResp_SInv;
    Statistic<uint64_t>* stat_stateEvent_FetchResp_SMInv;
    Statistic<uint64_t>* stat_stateEvent_FetchResp_MI;
    Statistic<uint64_t>* stat_stateEvent_FetchResp_EI;
    Statistic<uint64_t>* stat_stateEvent_FetchResp_SI;
    Statistic<uint64_t>* stat_stateEvent_FetchXResp_MInv;
    Statistic<uint64_t>* stat_stateEvent_FetchXResp_MInvX;
    Statistic<uint64_t>* stat_stateEvent_FetchXResp_EInv;
    Statistic<uint64_t>* stat_stateEvent_FetchXResp_EInvX;
    Statistic<uint64_t>* stat_stateEvent_FetchXResp_SD;
    Statistic<uint64_t>* stat_stateEvent_FetchXResp_ED;
    Statistic<uint64_t>* stat_stateEvent_FetchXResp_MD;
    Statistic<uint64_t>* stat_stateEvent_FetchXResp_SMD;
    Statistic<uint64_t>* stat_stateEvent_FetchXResp_SInv;
    Statistic<uint64_t>* stat_stateEvent_FetchXResp_SMInv;
    Statistic<uint64_t>* stat_stateEvent_FetchXResp_MI;
    Statistic<uint64_t>* stat_stateEvent_FetchXResp_EI;
    Statistic<uint64_t>* stat_stateEvent_FetchXResp_SI;
    Statistic<uint64_t>* stat_stateEvent_AckInv_I;
    Statistic<uint64_t>* stat_stateEvent_AckInv_MInv;
    Statistic<uint64_t>* stat_stateEvent_AckInv_EInv;
    Statistic<uint64_t>* stat_stateEvent_AckInv_SInv;
    Statistic<uint64_t>* stat_stateEvent_AckInv_SMInv;
    Statistic<uint64_t>* stat_stateEvent_AckInv_MI;
    Statistic<uint64_t>* stat_stateEvent_AckInv_EI;
    Statistic<uint64_t>* stat_stateEvent_AckInv_SI;
    Statistic<uint64_t>* stat_stateEvent_AckInv_SBInv;
    Statistic<uint64_t>* stat_stateEvent_AckPut_I;
    Statistic<uint64_t>* stat_stateEvent_FlushLine_I;
    Statistic<uint64_t>* stat_stateEvent_FlushLine_S;
    Statistic<uint64_t>* stat_stateEvent_FlushLine_E;
    Statistic<uint64_t>* stat_stateEvent_FlushLine_M;
    Statistic<uint64_t>* stat_stateEvent_FlushLine_IS;
    Statistic<uint64_t>* stat_stateEvent_FlushLine_IM;
    Statistic<uint64_t>* stat_stateEvent_FlushLine_SM;
    Statistic<uint64_t>* stat_stateEvent_FlushLine_MInv;
    Statistic<uint64_t>* stat_stateEvent_FlushLine_MInvX;
    Statistic<uint64_t>* stat_stateEvent_FlushLine_EInv;
    Statistic<uint64_t>* stat_stateEvent_FlushLine_EInvX;
    Statistic<uint64_t>* stat_stateEvent_FlushLine_SInv;
    Statistic<uint64_t>* stat_stateEvent_FlushLine_SMInv;
    Statistic<uint64_t>* stat_stateEvent_FlushLine_SD;
    Statistic<uint64_t>* stat_stateEvent_FlushLine_ED;
    Statistic<uint64_t>* stat_stateEvent_FlushLine_MD;
    Statistic<uint64_t>* stat_stateEvent_FlushLine_SMD;
    Statistic<uint64_t>* stat_stateEvent_FlushLine_MI;
    Statistic<uint64_t>* stat_stateEvent_FlushLine_EI;
    Statistic<uint64_t>* stat_stateEvent_FlushLine_SI;
    Statistic<uint64_t>* stat_stateEvent_FlushLine_IB;
    Statistic<uint64_t>* stat_stateEvent_FlushLine_SB;
    Statistic<uint64_t>* stat_stateEvent_FlushLineInv_I;
    Statistic<uint64_t>* stat_stateEvent_FlushLineInv_S;
    Statistic<uint64_t>* stat_stateEvent_FlushLineInv_E;
    Statistic<uint64_t>* stat_stateEvent_FlushLineInv_M;
    Statistic<uint64_t>* stat_stateEvent_FlushLineInv_IS;
    Statistic<uint64_t>* stat_stateEvent_FlushLineInv_IM;
    Statistic<uint64_t>* stat_stateEvent_FlushLineInv_SM;
    Statistic<uint64_t>* stat_stateEvent_FlushLineInv_MInv;
    Statistic<uint64_t>* stat_stateEvent_FlushLineInv_MInvX;
    Statistic<uint64_t>* stat_stateEvent_FlushLineInv_EInv;
    Statistic<uint64_t>* stat_stateEvent_FlushLineInv_EInvX;
    Statistic<uint64_t>* stat_stateEvent_FlushLineInv_SInv;
    Statistic<uint64_t>* stat_stateEvent_FlushLineInv_SMInv;
    Statistic<uint64_t>* stat_stateEvent_FlushLineInv_SD;
    Statistic<uint64_t>* stat_stateEvent_FlushLineInv_ED;
    Statistic<uint64_t>* stat_stateEvent_FlushLineInv_MD;
    Statistic<uint64_t>* stat_stateEvent_FlushLineInv_SMD;
    Statistic<uint64_t>* stat_stateEvent_FlushLineInv_MI;
    Statistic<uint64_t>* stat_stateEvent_FlushLineInv_EI;
    Statistic<uint64_t>* stat_stateEvent_FlushLineInv_SI;
    Statistic<uint64_t>* stat_stateEvent_FlushLineResp_I;
    Statistic<uint64_t>* stat_stateEvent_FlushLineResp_IB;
    Statistic<uint64_t>* stat_stateEvent_FlushLineResp_SB;
    Statistic<uint64_t>* stat_eventSent_GetS;
    Statistic<uint64_t>* stat_eventSent_GetX;
    Statistic<uint64_t>* stat_eventSent_GetSX;
    Statistic<uint64_t>* stat_eventSent_PutS;
    Statistic<uint64_t>* stat_eventSent_PutE;
    Statistic<uint64_t>* stat_eventSent_PutM;
    Statistic<uint64_t>* stat_eventSent_FlushLine;
    Statistic<uint64_t>* stat_eventSent_FlushLineInv;
    Statistic<uint64_t>* stat_eventSent_FetchResp;
    Statistic<uint64_t>* stat_eventSent_FetchXResp;
    Statistic<uint64_t>* stat_eventSent_AckInv;
    Statistic<uint64_t>* stat_eventSent_NACK_down;
    Statistic<uint64_t>* stat_eventSent_GetSResp;
    Statistic<uint64_t>* stat_eventSent_GetXResp;
    Statistic<uint64_t>* stat_eventSent_FlushLineResp;
    Statistic<uint64_t>* stat_eventSent_Inv;
    Statistic<uint64_t>* stat_eventSent_Fetch;
    Statistic<uint64_t>* stat_eventSent_FetchInv;
    Statistic<uint64_t>* stat_eventSent_FetchInvX;
    Statistic<uint64_t>* stat_eventSent_AckPut;
    Statistic<uint64_t>* stat_eventSent_NACK_up;

/* Private event handlers */
    /** Handle GetX request. Request upgrade if needed */
    CacheAction handleGetXRequest(MemEvent* event, CacheLine* dirLine, bool replay);

    /** Handle GetS request. Request block if needed */
    CacheAction handleGetSRequest(MemEvent* event, CacheLine* dirLine, bool replay);
    
    /** Handle PutS request. Possibly complete a waiting request if it raced with the PutS */
    CacheAction handlePutSRequest(MemEvent* event, CacheLine * dirLine, MemEvent * origReq);
    
    /** Handle PutM request. Possibly complete a waiting request if it raced with the PutM */
    CacheAction handlePutMRequest(MemEvent* event, CacheLine * dirLine, MemEvent * origReq);

    /** Handle FlushLine request. */
    CacheAction handleFlushLineRequest(MemEvent* event, CacheLine * dirLine, MemEvent * origReq, bool replay);

    /** Handle FlushLineInv request. */
    CacheAction handleFlushLineInvRequest(MemEvent* event, CacheLine * dirLine, MemEvent * origReq, bool replay);

    /** Handle Inv */
    CacheAction handleInv(MemEvent * event, CacheLine * dirLine, bool replay, MemEvent * collisionEvent);
    
    /** Handle ForceInv */
    CacheAction handleForceInv(MemEvent * event, CacheLine * dirLine, bool replay, MemEvent * collisionEvent);
    
    /** Handle Fetch */
    CacheAction handleFetch(MemEvent * event, CacheLine * dirLine, bool replay, MemEvent * collisionEvent);
    
    /** Handle FetchInv */
    CacheAction handleFetchInv(MemEvent * event, CacheLine * dirLine, bool replay, MemEvent * collisionEvent);
    
    /** Handle FetchInvX */
    CacheAction handleFetchInvX(MemEvent * event, CacheLine * dirLine, bool replay, MemEvent * collisionEvent);

    /** Process GetSResp/GetXResp */
    CacheAction handleDataResponse(MemEvent* responseEvent, CacheLine * dirLine, MemEvent * reqEvent);
    
    /** Handle FetchResp */
    CacheAction handleFetchResp(MemEvent * responseEvent, CacheLine* dirLine, MemEvent * reqEvent);
    
    /** Handle Ack */
    CacheAction handleAckInv(MemEvent * responseEvent, CacheLine* dirLine, MemEvent * reqEvent);

/* Private methods for sending events */
    void sendResponseDown(MemEvent* event, CacheLine* dirLine, std::vector<uint8_t>* data, bool dirty, bool replay);
   
    /** Send response to lower level cache using 'event' instead of dirLine */
    void sendResponseDownFromMSHR(MemEvent* event, bool dirty);

    /** Send writeback request to lower level caches */
    void sendWriteback(Command cmd, CacheLine* dirLine, string origRqstr);
    
    /** Send writeback request to lower level cache using data from cache */
    void sendWritebackFromCache(Command cmd, CacheLine* dirLine, string origRqstr);

    /** Send writeback request to lower level cache using data from MSHR */
    void sendWritebackFromMSHR(Command cmd, CacheLine* dirLine, string origRqstr, std::vector<uint8_t>* data);
    
    /** Send writeback ack */
    void sendWritebackAck(MemEvent * event);

    /** Send AckInv to lower level cache */
    void sendAckInv(MemEvent * event);

    /** Fetch data from owner and invalidate their copy of the line */
    void sendFetchInv(CacheLine * dirLine, string rqstr, bool replay);
    
    /** Fetch data from owner and downgrade owner to sharer */
    void sendFetchInvX(CacheLine * dirLine, string rqstr, bool replay);

    /** Fetch data from sharer */
    void sendFetch(CacheLine * dirLine, string rqstr, bool replay);

    /** Send ForceInv to owner */
    void sendForceInv(CacheLine * dirLine, string rqstr, bool replay);

    /** Send a flush response */
    void sendFlushResponse(MemEvent * reqEvent, bool success);
    
    /** Forward a FlushLine request with or without data */
    void forwardFlushLine(MemEvent * origFlush, CacheLine * dirLine, bool dirty, Command cmd);

    /** Invalidate all sharers of a block. Used for invalidations and evictions */
    void invalidateAllSharers(CacheLine * dirLine, string rqstr, bool replay);
    
    /** Invalidate all sharers of a block and fetch block from one of them. Used for invalidations and evictions */
    void invalidateAllSharersAndFetch(CacheLine * dirLine, string rqstr, bool replay);
    
    /** Invalidate all sharers of a block except the requestor (rqstr). If requestor is not a sharer, may fetch data from a sharer. Used for upgrade requests. */
    bool invalidateSharersExceptRequestor(CacheLine * dirLine, string rqstr, string origRqstr, bool replay, bool checkFetch);


/* Miscellaneous */
   
    void printData(vector<uint8_t> * data, bool set);

/* Statistics */
    //void recordStateEventCount(Command cmd, State state);
    void recordEvictionState(State state);
    void recordStateEventCount(Command cmd, State state);
    void recordEventSentDown(Command cmd);
    void recordEventSentUp(Command cmd);
};


}}


#endif

