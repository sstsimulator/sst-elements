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

#include <sst_config.h>
#include <vector>
#include "coherencemgr/MESI_Shared_Noninclusive.h"

using namespace SST;
using namespace SST::MemHierarchy;

/* Debug macros included from util.h */

/*----------------------------------------------------------------------------------------------------------------------
 * MESI or MSI NonInclusive Coherence Controller for shared caches
 * - Has an inclusive tag directory & noninclusive data array
 * - Unlike the private noninclusive protocol, this one supports prefetchers due to the inclusive tag directory
 *---------------------------------------------------------------------------------------------------------------------*/

 /***********************************************************************************************************
 * Constructor
 ***********************************************************************************************************/
MESISharNoninclusive::MESISharNoninclusive(ComponentId_t id, Params& params, Params& owner_params, bool prefetch) :
    CoherenceController(id, params, owner_params, prefetch)
{
    params.insert(owner_params);

    debug_->debug(_INFO_,"--------------------------- Initializing [MESI + Directory Controller] ... \n\n");

    protocol_ = params.find<bool>("protocol", 1);
    if (protocol_)
        protocol_state_ = E;
    else
        protocol_state_ = S;

    // Data (cache) Array
    uint64_t lines = params.find<uint64_t>("lines");
    uint64_t assoc = params.find<uint64_t>("associativity");

    ReplacementPolicy * rmgr = createReplacementPolicy(lines, assoc, params, false);
    HashFunction * ht = createHashFunction(params);
    data_array_ = new CacheArray<DataLine>(debug_, lines, assoc, line_size_, rmgr, ht);
    data_array_->setBanked(params.find<uint64_t>("banks", 0));

    uint64_t dir_lines = params.find<uint64_t>("dlines");
    uint64_t dir_assoc = params.find<uint64_t>("dassoc");
    params.insert("replacement_policy", params.find<std::string>("drpolicy", "lru"));
    ReplacementPolicy *drmgr = createReplacementPolicy(dir_lines, dir_assoc, params, false, 1);
    dir_array_ = new CacheArray<DirectoryLine>(debug_, dir_lines, dir_assoc, line_size_, drmgr, ht);
    dir_array_->setBanked(params.find<uint64_t>("banks", 0));

    flush_state_ = FlushState::Ready;
    shutdown_flush_counter_ = 0;

    /* Statistics */
    stat_evict_[I] =         registerStatistic<uint64_t>("evict_I");
    stat_evict_[IS] =        registerStatistic<uint64_t>("evict_IS");
    stat_evict_[IM] =        registerStatistic<uint64_t>("evict_IM");
    stat_evict_[I_B] =       registerStatistic<uint64_t>("evict_IB");
    stat_evict_[S] =         registerStatistic<uint64_t>("evict_S");
    stat_evict_[SM] =        registerStatistic<uint64_t>("evict_SM");
    stat_evict_[S_B] =       registerStatistic<uint64_t>("evict_SB");
    stat_evict_[S_Inv] =     registerStatistic<uint64_t>("evict_SInv");
    stat_evict_[SM_Inv] =    registerStatistic<uint64_t>("evict_SMInv");
    stat_evict_[M] =         registerStatistic<uint64_t>("evict_M");
    stat_evict_[MA] =        registerStatistic<uint64_t>("evict_MA");
    stat_evict_[M_Inv] =     registerStatistic<uint64_t>("evict_MInv");
    stat_evict_[M_InvX] =    registerStatistic<uint64_t>("evict_MInvX");
    stat_event_state_[(int)Command::GetS][I] =    registerStatistic<uint64_t>("stateEvent_GetS_I");
    stat_event_state_[(int)Command::GetS][IA] =   registerStatistic<uint64_t>("stateEvent_GetS_IA");
    stat_event_state_[(int)Command::GetS][S] =    registerStatistic<uint64_t>("stateEvent_GetS_S");
    stat_event_state_[(int)Command::GetS][M] =    registerStatistic<uint64_t>("stateEvent_GetS_M");
    stat_event_state_[(int)Command::GetX][I] =    registerStatistic<uint64_t>("stateEvent_GetX_I");
    stat_event_state_[(int)Command::GetX][S] =    registerStatistic<uint64_t>("stateEvent_GetX_S");
    stat_event_state_[(int)Command::GetX][M] =    registerStatistic<uint64_t>("stateEvent_GetX_M");
    stat_event_state_[(int)Command::GetSX][I] =  registerStatistic<uint64_t>("stateEvent_GetSX_I");
    stat_event_state_[(int)Command::GetSX][S] =  registerStatistic<uint64_t>("stateEvent_GetSX_S");
    stat_event_state_[(int)Command::GetSX][M] =  registerStatistic<uint64_t>("stateEvent_GetSX_M");
    stat_event_state_[(int)Command::GetSResp][IS] =   registerStatistic<uint64_t>("stateEvent_GetSResp_IS");
    stat_event_state_[(int)Command::GetXResp][IS] =   registerStatistic<uint64_t>("stateEvent_GetXResp_IS");
    stat_event_state_[(int)Command::GetXResp][IM] =   registerStatistic<uint64_t>("stateEvent_GetXResp_IM");
    stat_event_state_[(int)Command::GetXResp][SM] =   registerStatistic<uint64_t>("stateEvent_GetXResp_SM");
    stat_event_state_[(int)Command::GetXResp][SM_Inv] = registerStatistic<uint64_t>("stateEvent_GetXResp_SMInv");
    stat_event_state_[(int)Command::PutS][S] =    registerStatistic<uint64_t>("stateEvent_PutS_S");
    stat_event_state_[(int)Command::PutS][M] =    registerStatistic<uint64_t>("stateEvent_PutS_M");
    stat_event_state_[(int)Command::PutS][M_Inv] = registerStatistic<uint64_t>("stateEvent_PutS_MInv");
    stat_event_state_[(int)Command::PutS][S_Inv] =     registerStatistic<uint64_t>("stateEvent_PutS_SInv");
    stat_event_state_[(int)Command::PutS][SM_Inv] =    registerStatistic<uint64_t>("stateEvent_PutS_SMInv");
    stat_event_state_[(int)Command::PutS][S_B] =   registerStatistic<uint64_t>("stateEvent_PutS_SB");
    stat_event_state_[(int)Command::PutS][S_D] =   registerStatistic<uint64_t>("stateEvent_PutS_SD");
    stat_event_state_[(int)Command::PutS][SB_D] =  registerStatistic<uint64_t>("stateEvent_PutS_SBD");
    stat_event_state_[(int)Command::PutS][M_D] =   registerStatistic<uint64_t>("stateEvent_PutS_MD");
    stat_event_state_[(int)Command::PutS][M_B] =   registerStatistic<uint64_t>("stateEvent_PutS_MB");
    stat_event_state_[(int)Command::PutX][M] =     registerStatistic<uint64_t>("stateEvent_PutX_M");
    stat_event_state_[(int)Command::PutX][M_Inv] = registerStatistic<uint64_t>("stateEvent_PutX_MInv");
    stat_event_state_[(int)Command::PutM][M] =     registerStatistic<uint64_t>("stateEvent_PutM_M");
    stat_event_state_[(int)Command::PutM][M_Inv] = registerStatistic<uint64_t>("stateEvent_PutM_MInv");
    stat_event_state_[(int)Command::PutM][M_InvX] =    registerStatistic<uint64_t>("stateEvent_PutM_MInvX");
    stat_event_state_[(int)Command::Inv][I] =     registerStatistic<uint64_t>("stateEvent_Inv_I");
    stat_event_state_[(int)Command::Inv][S] =     registerStatistic<uint64_t>("stateEvent_Inv_S");
    stat_event_state_[(int)Command::Inv][SM] =    registerStatistic<uint64_t>("stateEvent_Inv_SM");
    stat_event_state_[(int)Command::Inv][S_Inv] =  registerStatistic<uint64_t>("stateEvent_Inv_SInv");
    stat_event_state_[(int)Command::Inv][SM_Inv] = registerStatistic<uint64_t>("stateEvent_Inv_SMInv");
    stat_event_state_[(int)Command::Inv][S_B] =    registerStatistic<uint64_t>("stateEvent_Inv_SB");
    stat_event_state_[(int)Command::Inv][I_B] =    registerStatistic<uint64_t>("stateEvent_Inv_IB");
    stat_event_state_[(int)Command::FetchInvX][I] =   registerStatistic<uint64_t>("stateEvent_FetchInvX_I");
    stat_event_state_[(int)Command::FetchInvX][M] =   registerStatistic<uint64_t>("stateEvent_FetchInvX_M");
    stat_event_state_[(int)Command::FetchInvX][MA] =  registerStatistic<uint64_t>("stateEvent_FetchInvX_MA");
    stat_event_state_[(int)Command::FetchInvX][M_B] = registerStatistic<uint64_t>("stateEvent_FetchInvX_MB");
    stat_event_state_[(int)Command::FetchInvX][I_B] = registerStatistic<uint64_t>("stateEvent_FetchInvX_IB");
    stat_event_state_[(int)Command::Fetch][I] =       registerStatistic<uint64_t>("stateEvent_Fetch_I");
    stat_event_state_[(int)Command::Fetch][S] =       registerStatistic<uint64_t>("stateEvent_Fetch_S");
    stat_event_state_[(int)Command::Fetch][SM] =      registerStatistic<uint64_t>("stateEvent_Fetch_SM");
    stat_event_state_[(int)Command::Fetch][I_B] =      registerStatistic<uint64_t>("stateEvent_Fetch_IB");
    stat_event_state_[(int)Command::Fetch][S_B] =      registerStatistic<uint64_t>("stateEvent_Fetch_SB");
    stat_event_state_[(int)Command::Fetch][M_B] =      registerStatistic<uint64_t>("stateEvent_Fetch_MB");
    stat_event_state_[(int)Command::Fetch][SA] =      registerStatistic<uint64_t>("stateEvent_Fetch_SA");
    stat_event_state_[(int)Command::ForceInv][I] =        registerStatistic<uint64_t>("stateEvent_ForceInv_I");
    stat_event_state_[(int)Command::ForceInv][I_B] =      registerStatistic<uint64_t>("stateEvent_ForceInv_IB");
    stat_event_state_[(int)Command::ForceInv][S] =        registerStatistic<uint64_t>("stateEvent_ForceInv_S");
    stat_event_state_[(int)Command::ForceInv][S_Inv] =    registerStatistic<uint64_t>("stateEvent_ForceInv_SInv");
    stat_event_state_[(int)Command::ForceInv][SM_Inv] =   registerStatistic<uint64_t>("stateEvent_ForceInv_SMInv");
    stat_event_state_[(int)Command::ForceInv][S_B] =      registerStatistic<uint64_t>("stateEvent_ForceInv_SB");
    stat_event_state_[(int)Command::ForceInv][SM] =       registerStatistic<uint64_t>("stateEvent_ForceInv_SM");
    stat_event_state_[(int)Command::ForceInv][SA] =       registerStatistic<uint64_t>("stateEvent_ForceInv_SA");
    stat_event_state_[(int)Command::ForceInv][M] =        registerStatistic<uint64_t>("stateEvent_ForceInv_M");
    stat_event_state_[(int)Command::ForceInv][M_B] =      registerStatistic<uint64_t>("stateEvent_ForceInv_MB");
    stat_event_state_[(int)Command::ForceInv][M_Inv] =    registerStatistic<uint64_t>("stateEvent_ForceInv_MInv");
    stat_event_state_[(int)Command::ForceInv][MA] =       registerStatistic<uint64_t>("stateEvent_ForceInv_MA");
    stat_event_state_[(int)Command::FetchInv][I] =        registerStatistic<uint64_t>("stateEvent_FetchInv_I");
    stat_event_state_[(int)Command::FetchInv][S] =        registerStatistic<uint64_t>("stateEvent_FetchInv_S");
    stat_event_state_[(int)Command::FetchInv][S_Inv] =    registerStatistic<uint64_t>("stateEvent_FetchInv_SInv");
    stat_event_state_[(int)Command::FetchInv][SM_Inv] =   registerStatistic<uint64_t>("stateEvent_FetchInv_SMInv");
    stat_event_state_[(int)Command::FetchInv][S_B] =      registerStatistic<uint64_t>("stateEvent_FetchInv_SB");
    stat_event_state_[(int)Command::FetchInv][SM] =       registerStatistic<uint64_t>("stateEvent_FetchInv_SM");
    stat_event_state_[(int)Command::FetchInv][SA] =       registerStatistic<uint64_t>("stateEvent_FetchInv_SA");
    stat_event_state_[(int)Command::FetchInv][M] =        registerStatistic<uint64_t>("stateEvent_FetchInv_M");
    stat_event_state_[(int)Command::FetchInv][MA] =       registerStatistic<uint64_t>("stateEvent_FetchInv_MA");
    stat_event_state_[(int)Command::FetchInv][M_B] =      registerStatistic<uint64_t>("stateEvent_FetchInv_MB");
    stat_event_state_[(int)Command::FetchInv][M_Inv] =    registerStatistic<uint64_t>("stateEvent_FetchInv_MInv");
    stat_event_state_[(int)Command::FetchResp][M_Inv] =   registerStatistic<uint64_t>("stateEvent_FetchResp_MInv");
    stat_event_state_[(int)Command::FetchResp][M_InvX] =  registerStatistic<uint64_t>("stateEvent_FetchResp_MInvX");
    stat_event_state_[(int)Command::FetchResp][S_D] =     registerStatistic<uint64_t>("stateEvent_FetchResp_SD");
    stat_event_state_[(int)Command::FetchResp][M_D] =     registerStatistic<uint64_t>("stateEvent_FetchResp_MD");
    stat_event_state_[(int)Command::FetchResp][SM_D] =    registerStatistic<uint64_t>("stateEvent_FetchResp_SMD");
    stat_event_state_[(int)Command::FetchResp][S_Inv] =   registerStatistic<uint64_t>("stateEvent_FetchResp_SInv");
    stat_event_state_[(int)Command::FetchResp][SM_Inv] =  registerStatistic<uint64_t>("stateEvent_FetchResp_SMInv");
    stat_event_state_[(int)Command::FetchResp][SB_D] =    registerStatistic<uint64_t>("stateEvent_FetchResp_SBD");
    stat_event_state_[(int)Command::FetchResp][SB_Inv] =  registerStatistic<uint64_t>("stateEvent_FetchResp_SBInv");
    stat_event_state_[(int)Command::FetchXResp][M_InvX] = registerStatistic<uint64_t>("stateEvent_FetchXResp_MInvX");
    stat_event_state_[(int)Command::AckInv][I] =          registerStatistic<uint64_t>("stateEvent_AckInv_I");
    stat_event_state_[(int)Command::AckInv][M_Inv] =      registerStatistic<uint64_t>("stateEvent_AckInv_MInv");
    stat_event_state_[(int)Command::AckInv][S_Inv] =      registerStatistic<uint64_t>("stateEvent_AckInv_SInv");
    stat_event_state_[(int)Command::AckInv][SM_Inv] =     registerStatistic<uint64_t>("stateEvent_AckInv_SMInv");
    stat_event_state_[(int)Command::AckInv][SB_Inv] =     registerStatistic<uint64_t>("stateEvent_AckInv_SBInv");
    stat_event_state_[(int)Command::AckPut][I] =          registerStatistic<uint64_t>("stateEvent_AckPut_I");
    stat_event_state_[(int)Command::FlushLine][I] =       registerStatistic<uint64_t>("stateEvent_FlushLine_I");
    stat_event_state_[(int)Command::FlushLine][S] =       registerStatistic<uint64_t>("stateEvent_FlushLine_S");
    stat_event_state_[(int)Command::FlushLine][M] =       registerStatistic<uint64_t>("stateEvent_FlushLine_M");
    stat_event_state_[(int)Command::FlushLine][SM_D] =    registerStatistic<uint64_t>("stateEvent_FlushLine_SMD");
    stat_event_state_[(int)Command::FlushLineInv][I] =    registerStatistic<uint64_t>("stateEvent_FlushLineInv_I");
    stat_event_state_[(int)Command::FlushLineInv][S] =    registerStatistic<uint64_t>("stateEvent_FlushLineInv_S");
    stat_event_state_[(int)Command::FlushLineInv][M] =    registerStatistic<uint64_t>("stateEvent_FlushLineInv_M");
    stat_event_state_[(int)Command::FlushLineInv][M_B] =  registerStatistic<uint64_t>("stateEvent_FlushLineInv_MB");
    stat_event_state_[(int)Command::FlushLineResp][I] =   registerStatistic<uint64_t>("stateEvent_FlushLineResp_I");
    stat_event_state_[(int)Command::FlushLineResp][I_B] = registerStatistic<uint64_t>("stateEvent_FlushLineResp_IB");
    stat_event_state_[(int)Command::FlushLineResp][S_B] = registerStatistic<uint64_t>("stateEvent_FlushLineResp_SB");
    stat_event_sent_[(int)Command::GetS]            = registerStatistic<uint64_t>("eventSent_GetS");
    stat_event_sent_[(int)Command::GetX]            = registerStatistic<uint64_t>("eventSent_GetX");
    stat_event_sent_[(int)Command::GetSX]           = registerStatistic<uint64_t>("eventSent_GetSX");
    stat_event_sent_[(int)Command::Write]           = registerStatistic<uint64_t>("eventSent_Write");
    stat_event_sent_[(int)Command::PutS]            = registerStatistic<uint64_t>("eventSent_PutS");
    stat_event_sent_[(int)Command::PutM]            = registerStatistic<uint64_t>("eventSent_PutM");
    stat_event_sent_[(int)Command::FlushLine]       = registerStatistic<uint64_t>("eventSent_FlushLine");
    stat_event_sent_[(int)Command::FlushLineInv]    = registerStatistic<uint64_t>("eventSent_FlushLineInv");
    stat_event_sent_[(int)Command::FlushAll]        = registerStatistic<uint64_t>("eventSent_FlushAll");
    stat_event_sent_[(int)Command::ForwardFlush]    = registerStatistic<uint64_t>("eventSent_ForwardFlush");
    stat_event_sent_[(int)Command::UnblockFlush]    = registerStatistic<uint64_t>("eventSent_UnblockFlush");
    stat_event_sent_[(int)Command::AckFlush]        = registerStatistic<uint64_t>("eventSent_AckFlush");
    stat_event_sent_[(int)Command::FetchResp]       = registerStatistic<uint64_t>("eventSent_FetchResp");
    stat_event_sent_[(int)Command::FetchXResp]      = registerStatistic<uint64_t>("eventSent_FetchXResp");
    stat_event_sent_[(int)Command::AckInv]          = registerStatistic<uint64_t>("eventSent_AckInv");
    stat_event_sent_[(int)Command::NACK]            = registerStatistic<uint64_t>("eventSent_NACK");
    stat_event_sent_[(int)Command::GetSResp]        = registerStatistic<uint64_t>("eventSent_GetSResp");
    stat_event_sent_[(int)Command::GetXResp]        = registerStatistic<uint64_t>("eventSent_GetXResp");
    stat_event_sent_[(int)Command::WriteResp]       = registerStatistic<uint64_t>("eventSent_WriteResp");
    stat_event_sent_[(int)Command::FlushLineResp]   = registerStatistic<uint64_t>("eventSent_FlushLineResp");
    stat_event_sent_[(int)Command::FlushAllResp]    = registerStatistic<uint64_t>("eventSent_FlushAllResp");
    stat_event_sent_[(int)Command::Inv]             = registerStatistic<uint64_t>("eventSent_Inv");
    stat_event_sent_[(int)Command::Fetch]           = registerStatistic<uint64_t>("eventSent_Fetch");
    stat_event_sent_[(int)Command::FetchInv]        = registerStatistic<uint64_t>("eventSent_FetchInv");
    stat_event_sent_[(int)Command::FetchInvX]       = registerStatistic<uint64_t>("eventSent_FetchInvX");
    stat_event_sent_[(int)Command::ForceInv]        = registerStatistic<uint64_t>("eventSent_ForceInv");
    stat_event_sent_[(int)Command::AckPut]          = registerStatistic<uint64_t>("eventSent_AckPut");
    stat_event_sent_[(int)Command::Put]           = registerStatistic<uint64_t>("eventSent_Put");
    stat_event_sent_[(int)Command::Get]           = registerStatistic<uint64_t>("eventSent_Get");
    stat_event_sent_[(int)Command::AckMove]       = registerStatistic<uint64_t>("eventSent_AckMove");
    stat_event_sent_[(int)Command::CustomReq]     = registerStatistic<uint64_t>("eventSent_CustomReq");
    stat_event_sent_[(int)Command::CustomResp]    = registerStatistic<uint64_t>("eventSent_CustomResp");
    stat_event_sent_[(int)Command::CustomAck]     = registerStatistic<uint64_t>("eventSent_CustomAck");
    stat_latency_GetS_[LatType::HIT]       = registerStatistic<uint64_t>("latency_GetS_hit");
    stat_latency_GetS_[LatType::MISS]      = registerStatistic<uint64_t>("latency_GetS_miss");
    stat_latency_GetS_[LatType::INV]       = registerStatistic<uint64_t>("latency_GetS_inv");
    stat_latency_GetX_[LatType::HIT]       = registerStatistic<uint64_t>("latency_GetX_hit");
    stat_latency_GetX_[LatType::MISS]      = registerStatistic<uint64_t>("latency_GetX_miss");
    stat_latency_GetX_[LatType::INV]       = registerStatistic<uint64_t>("latency_GetX_inv");
    stat_latency_GetX_[LatType::UPGRADE]   = registerStatistic<uint64_t>("latency_GetX_upgrade");
    stat_latency_GetSX_[LatType::HIT]      = registerStatistic<uint64_t>("latency_GetSX_hit");
    stat_latency_GetSX_[LatType::MISS]     = registerStatistic<uint64_t>("latency_GetSX_miss");
    stat_latency_GetSX_[LatType::INV]      = registerStatistic<uint64_t>("latency_GetSX_inv");
    stat_latency_GetSX_[LatType::UPGRADE]  = registerStatistic<uint64_t>("latency_GetSX_upgrade");
    stat_latency_FlushLine_       = registerStatistic<uint64_t>("latency_FlushLine");
    stat_latency_FlushLineInv_    = registerStatistic<uint64_t>("latency_FlushLineInv");
    stat_hit_[0][0] = registerStatistic<uint64_t>("GetSHit_Arrival");
    stat_hit_[1][0] = registerStatistic<uint64_t>("GetXHit_Arrival");
    stat_hit_[2][0] = registerStatistic<uint64_t>("GetSXHit_Arrival");
    stat_hit_[0][1] = registerStatistic<uint64_t>("GetSHit_Blocked");
    stat_hit_[1][1] = registerStatistic<uint64_t>("GetXHit_Blocked");
    stat_hit_[2][1] = registerStatistic<uint64_t>("GetSXHit_Blocked");
    stat_miss_[0][0] = registerStatistic<uint64_t>("GetSMiss_Arrival");
    stat_miss_[1][0] = registerStatistic<uint64_t>("GetXMiss_Arrival");
    stat_miss_[2][0] = registerStatistic<uint64_t>("GetSXMiss_Arrival");
    stat_miss_[0][1] = registerStatistic<uint64_t>("GetSMiss_Blocked");
    stat_miss_[1][1] = registerStatistic<uint64_t>("GetXMiss_Blocked");
    stat_miss_[2][1] = registerStatistic<uint64_t>("GetSXMiss_Blocked");
    stat_hits_ = registerStatistic<uint64_t>("CacheHits");
    stat_misses_ = registerStatistic<uint64_t>("CacheMisses");

    /* Prefetch statistics */
    if (prefetch) {
        stat_prefetch_evict_ = registerStatistic<uint64_t>("prefetch_evict");
        stat_prefetch_inv_ = registerStatistic<uint64_t>("prefetch_inv");
        stat_prefetch_hit_ = registerStatistic<uint64_t>("prefetch_useful");
        stat_prefetch_upgrade_miss_ = registerStatistic<uint64_t>("prefetch_coherence_miss");
        stat_prefetch_redundant_ = registerStatistic<uint64_t>("prefetch_redundant");
    }

    /* MESI-specific statistics (as opposed to MSI) */
    if (protocol_) {
        stat_evict_[E] =      registerStatistic<uint64_t>("evict_E");
        stat_evict_[E_D] =    registerStatistic<uint64_t>("evict_ED");
        stat_evict_[E_Inv] =  registerStatistic<uint64_t>("evict_EInv");
        stat_evict_[E_InvX] = registerStatistic<uint64_t>("evict_EInvX");
        stat_event_state_[(int)Command::GetS][E] =        registerStatistic<uint64_t>("stateEvent_GetS_E");
        stat_event_state_[(int)Command::GetX][E] =        registerStatistic<uint64_t>("stateEvent_GetX_E");
        stat_event_state_[(int)Command::GetSX][E] =       registerStatistic<uint64_t>("stateEvent_GetSX_E");
        stat_event_state_[(int)Command::PutS][E] =        registerStatistic<uint64_t>("stateEvent_PutS_E");
        stat_event_state_[(int)Command::PutS][E_Inv] =    registerStatistic<uint64_t>("stateEvent_PutS_EInv");
        stat_event_state_[(int)Command::PutS][E_D] =      registerStatistic<uint64_t>("stateEvent_PutS_ED");
        stat_event_state_[(int)Command::PutS][E_B] =      registerStatistic<uint64_t>("stateEvent_PutS_EB");
        stat_event_state_[(int)Command::PutE][M] =        registerStatistic<uint64_t>("stateEvent_PutE_M");
        stat_event_state_[(int)Command::PutE][M_Inv] =    registerStatistic<uint64_t>("stateEvent_PutE_MInv");
        stat_event_state_[(int)Command::PutE][M_InvX] =   registerStatistic<uint64_t>("stateEvent_PutE_MInvX");
        stat_event_state_[(int)Command::PutE][E] =        registerStatistic<uint64_t>("stateEvent_PutE_E");
        stat_event_state_[(int)Command::PutE][E_Inv] =    registerStatistic<uint64_t>("stateEvent_PutE_EInv");
        stat_event_state_[(int)Command::PutE][E_InvX] =   registerStatistic<uint64_t>("stateEvent_PutE_EInvX");
        stat_event_state_[(int)Command::PutX][E] =        registerStatistic<uint64_t>("stateEvent_PutX_E");
        stat_event_state_[(int)Command::PutX][E_Inv] =    registerStatistic<uint64_t>("stateEvent_PutX_EInv");
        stat_event_state_[(int)Command::PutM][E] =        registerStatistic<uint64_t>("stateEvent_PutM_E");
        stat_event_state_[(int)Command::PutM][E_Inv] =    registerStatistic<uint64_t>("stateEvent_PutM_EInv");
        stat_event_state_[(int)Command::PutM][E_InvX] =   registerStatistic<uint64_t>("stateEvent_PutM_EInvX");
        stat_event_state_[(int)Command::FetchInvX][E] =   registerStatistic<uint64_t>("stateEvent_FetchInvX_E");
        stat_event_state_[(int)Command::FetchInvX][EA] =  registerStatistic<uint64_t>("stateEvent_FetchInvX_EA");
        stat_event_state_[(int)Command::FetchInvX][E_B] = registerStatistic<uint64_t>("stateEvent_FetchInvX_EB");
        stat_event_state_[(int)Command::FetchInv][E] =    registerStatistic<uint64_t>("stateEvent_FetchInv_E");
        stat_event_state_[(int)Command::FetchInv][E_B] =  registerStatistic<uint64_t>("stateEvent_FetchInv_EB");
        stat_event_state_[(int)Command::FetchInv][EA] =   registerStatistic<uint64_t>("stateEvent_FetchInv_EA");
        stat_event_state_[(int)Command::FetchInv][E_Inv] = registerStatistic<uint64_t>("stateEvent_FetchInv_EInv");
        stat_event_state_[(int)Command::Fetch][E_B] =      registerStatistic<uint64_t>("stateEvent_Fetch_EB");
        stat_event_state_[(int)Command::ForceInv][E] =    registerStatistic<uint64_t>("stateEvent_ForceInv_E");
        stat_event_state_[(int)Command::ForceInv][E_B] =  registerStatistic<uint64_t>("stateEvent_ForceInv_EB");
        stat_event_state_[(int)Command::ForceInv][EA] =   registerStatistic<uint64_t>("stateEvent_ForceInv_EA");
        stat_event_state_[(int)Command::ForceInv][E_Inv] =    registerStatistic<uint64_t>("stateEvent_ForceInv_EInv");
        stat_event_state_[(int)Command::FetchResp][E_Inv] =   registerStatistic<uint64_t>("stateEvent_FetchResp_EInv");
        stat_event_state_[(int)Command::FetchResp][E_InvX] =  registerStatistic<uint64_t>("stateEvent_FetchResp_EInvX");
        stat_event_state_[(int)Command::FetchResp][E_D] =     registerStatistic<uint64_t>("stateEvent_FetchResp_ED");
        stat_event_state_[(int)Command::FetchXResp][E_InvX] = registerStatistic<uint64_t>("stateEvent_FetchXResp_EInvX");
        stat_event_state_[(int)Command::AckInv][E_Inv] =      registerStatistic<uint64_t>("stateEvent_AckInv_EInv");
        stat_event_state_[(int)Command::FlushLine][E] =       registerStatistic<uint64_t>("stateEvent_FlushLine_E");
        stat_event_state_[(int)Command::FlushLineInv][E] =    registerStatistic<uint64_t>("stateEvent_FlushLineInv_E");
        stat_event_state_[(int)Command::FlushLineInv][E_B] =  registerStatistic<uint64_t>("stateEvent_FlushLineInv_EB");
        stat_event_sent_[(int)Command::PutE]             =    registerStatistic<uint64_t>("eventSent_PutE");
    }
}

/***********************************************************************************************************
 * Event handlers
 ***********************************************************************************************************/

/* Handle GetS (load/read) requests */
bool MESISharNoninclusive::handleGetS(MemEvent* event, bool in_mshr) {
    Addr addr = event->getBaseAddr();
    DirectoryLine * tag = dir_array_->lookup(addr, true);
    State state = tag ? tag->getState() : I;
    DataLine * data = (tag) ? data_array_->lookup(addr, true) : nullptr;
    if (data && data->getTag() != tag) data = nullptr;

    bool local_prefetch = event->isPrefetch() && (event->getRqstr() == cachename_);
    uint64_t send_time = 0;
    MemEventStatus status = MemEventStatus::OK;
    Command response_command;

    if (mem_h_is_debug_event(event))
        event_debuginfo_.prefill(event->getID(), Command::GetS, (local_prefetch ? "-pref" : ""), addr, state);

    if (in_mshr)
        mshr_->removePendingRetry(addr);

    switch (state) {
        case I:
        case IA:
            status = processDirectoryMiss(event, tag, in_mshr);

            if (status == MemEventStatus::OK) {
                recordLatencyType(event->getID(), LatType::MISS);
                tag = dir_array_->lookup(addr, false);

                if (local_prefetch) { // Also need a data line
                    if (processDataMiss(event, tag, data, true) != MemEventStatus::OK) {
                        tag->setState(IA); // Don't let anyone steal this line, mark that we're waiting for a data line allocation
                        break;
                    } // Status is now inaccurate but it won't be Reject which is the only thing we're checking for
                }

                if (!mshr_->getProfiled(addr)) {
                    stat_event_state_[(int)Command::GetS][state]->addData(1);
                    stat_miss_[0][in_mshr]->addData(1);
                    stat_misses_->addData(1);
                    notifyListenerOfAccess(event, NotifyAccessType::READ, NotifyResultType::MISS);
                    mshr_->setProfiled(addr);
                }

                send_time = forwardMessage(event, line_size_, 0, nullptr);
                tag->setState(IS);
                tag->setTimestamp(send_time);
                mshr_->setInProgress(addr);

                if (mem_h_is_debug_event(event))
                    event_debuginfo_.reason = "miss";
            }
            break;
        case S:
            if (!in_mshr || !mshr_->getProfiled(addr)) {
                stat_event_state_[(int)Command::GetS][S]->addData(1);
                stat_hit_[0][in_mshr]->addData(1);
                stat_hits_->addData(1);
                notifyListenerOfAccess(event, NotifyAccessType::READ, NotifyResultType::HIT);
                if (in_mshr) mshr_->setProfiled(addr);
            }

            if (mem_h_is_debug_event(event))
                event_debuginfo_.reason = "hit";

            if (local_prefetch) {
                stat_prefetch_redundant_->addData(1);
                recordPrefetchLatency(event->getID(), LatType::HIT);
                if (mem_h_is_debug_event(event))
                    event_debuginfo_.action = "Done";
                cleanUpAfterRequest(event, in_mshr);
                break;
            }

            recordPrefetchResult(tag, stat_prefetch_hit_);

            if (data || mshr_->hasData(addr)) {
                tag->addSharer(event->getSrc());
                if (mshr_->hasData(addr))
                    send_time = sendResponseUp(event, &(mshr_->getData(addr)), in_mshr, tag->getTimestamp());
                else
                    send_time = sendResponseUp(event, data->getData(), in_mshr, tag->getTimestamp());
                tag->setTimestamp(send_time-1);
                recordLatencyType(event->getID(), LatType::HIT);
                cleanUpAfterRequest(event, in_mshr);
            } else {
                if (!in_mshr) {
                    status = allocateMSHR(event, false);
                    if (status != MemEventStatus::Reject)
                        mshr_->setProfiled(addr, event->getID());
                }
                if (status == MemEventStatus::OK) {
                    recordLatencyType(event->getID(), LatType::INV);
                    send_time = sendFetch(Command::Fetch, event, *(tag->getSharers()->begin()), in_mshr, tag->getTimestamp());
                    tag->setState(S_D);
                    tag->setTimestamp(send_time - 1);
                    if (mem_h_is_debug_event(event))
                        event_debuginfo_.reason = "data miss";
                    mshr_->setInProgress(addr);
                }
            }
            break;
        case E:
        case M:
            if (!in_mshr || !mshr_->getProfiled(addr)) {
                stat_event_state_[(int)Command::GetS][state]->addData(1);
                stat_hit_[0][in_mshr]->addData(1);
                stat_hits_->addData(1);
                notifyListenerOfAccess(event, NotifyAccessType::READ, NotifyResultType::HIT);
                if (in_mshr) mshr_->setProfiled(addr);
            }

            if (mem_h_is_debug_event(event))
                event_debuginfo_.reason = "hit";

            if (local_prefetch) {
                stat_prefetch_redundant_->addData(1);
                recordPrefetchLatency(event->getID(), LatType::HIT);
                cleanUpAfterRequest(event, in_mshr);
                break;
            }

            recordPrefetchResult(tag, stat_prefetch_hit_);

            if (tag->hasOwner()) {
                if (!in_mshr) {
                    status = allocateMSHR(event, false);
                    if (status != MemEventStatus::Reject)
                        mshr_->setProfiled(addr, event->getID());
                }
                if (status == MemEventStatus::OK) {
                    send_time = sendFetch(Command::FetchInvX, event, tag->getOwner(), in_mshr, tag->getTimestamp());
                    state == E ? tag->setState(E_InvX) : tag->setState(M_InvX);
                    tag->setTimestamp(send_time - 1);
                    recordLatencyType(event->getID(), LatType::INV);
                    mshr_->setInProgress(addr);
                }
            } else if (data || mshr_->hasData(addr)) {
                recordLatencyType(event->getID(), LatType::HIT);
                if (tag->hasSharers() || !protocol_) {
                    response_command = Command::GetSResp;
                    tag->addSharer(event->getSrc());
                } else {
                    response_command = Command::GetXResp;
                    tag->setOwner(event->getSrc());
                }
                if (mshr_->hasData(addr))
                    send_time = sendResponseUp(event, &(mshr_->getData(addr)), in_mshr, tag->getTimestamp(), response_command);
                else
                    send_time = sendResponseUp(event, data->getData(), in_mshr, tag->getTimestamp(), response_command);
                tag->setTimestamp(send_time - 1);
                cleanUpAfterRequest(event, in_mshr);
            } else {
                if (!in_mshr) {
                    status = allocateMSHR(event, false);
                    if (status != MemEventStatus::Reject)
                        mshr_->setProfiled(addr, event->getID());
                }
                if (status == MemEventStatus::OK) {
                    send_time = sendFetch(Command::Fetch, event, *(tag->getSharers()->begin()), in_mshr, tag->getTimestamp());
                    state == E ? tag->setState(E_D) : tag->setState(M_D);
                    tag->setTimestamp(send_time - 1);
                    if (mem_h_is_debug_event(event))
                        event_debuginfo_.reason = "data miss";
                    recordLatencyType(event->getID(), LatType::INV);
                    mshr_->setInProgress(addr);
                }
            }
            break;
        default:
            if (!in_mshr)
                status = allocateMSHR(event, false);
            break;
    }

    if (mem_h_is_debug_addr(addr) && tag) {
        event_debuginfo_.new_state = tag->getState();
        event_debuginfo_.verbose_line = tag->getString();
        if (data)
            event_debuginfo_.verbose_line += "/" + data->getString();
    }

    if (status == MemEventStatus::Reject) {
        if (local_prefetch)
            return false;
        else
            sendNACK(event);
    }

    return true;
}


bool MESISharNoninclusive::handleGetX(MemEvent * event, bool in_mshr) {
    Addr addr = event->getBaseAddr();
    DirectoryLine * tag = dir_array_->lookup(addr, true);
    State state = tag ? tag->getState() : I;
    DataLine * data = (tag) ? data_array_->lookup(addr, true) : nullptr;
    if (data && data->getTag() != tag) data = nullptr;

    uint64_t send_time = 0;
    MemEventStatus status = MemEventStatus::OK;
    Command response_command;

    if (in_mshr)
        mshr_->removePendingRetry(addr);

    if (mem_h_is_debug_event(event))
        event_debuginfo_.prefill(event->getID(), event->getCmd(), "", addr, state);

    switch (state) {
        case I:
            status = processDirectoryMiss(event, tag, in_mshr);

            if (status == MemEventStatus::OK) {
                recordLatencyType(event->getID(), LatType::MISS);
                tag = dir_array_->lookup(addr, false);

                if (!mshr_->getProfiled(addr)) {
                    stat_event_state_[(int)event->getCmd()][I]->addData(1);
                    stat_miss_[(event->getCmd() == Command::GetX ? 1 : 2)][in_mshr]->addData(1);
                    stat_misses_->addData(1);
                    notifyListenerOfAccess(event, NotifyAccessType::WRITE, NotifyResultType::MISS);
                    mshr_->setProfiled(addr);
                }
                send_time = forwardMessage(event, line_size_, 0, nullptr);
                tag->setState(IM);
                tag->setTimestamp(send_time);
                mshr_->setInProgress(addr);
                if (mem_h_is_debug_event(event))
                    event_debuginfo_.reason = "miss";
            }
            break;
        case S:
            if (!last_level_) { // If last level, upgrade silently by falling through to next case
                status = in_mshr ? MemEventStatus::OK : allocateMSHR(event, false);

                if (status == MemEventStatus::OK) {
                    if (!mshr_->getProfiled(addr)) {
                        stat_event_state_[(int)event->getCmd()][S]->addData(1);
                        stat_miss_[(event->getCmd() == Command::GetX ? 1 : 2)][in_mshr]->addData(1);
                        stat_misses_->addData(1);
                        notifyListenerOfAccess(event, NotifyAccessType::WRITE, NotifyResultType::MISS);
                        mshr_->setProfiled(addr);
                    }
                    recordPrefetchResult(tag, stat_prefetch_upgrade_miss_);
                    recordLatencyType(event->getID(), LatType::UPGRADE);

                    send_time = forwardMessage(event, line_size_, 0, nullptr);

                    if (invalidateExceptRequestor(event, tag, in_mshr, data == nullptr)) {
                        tag->setState(SM_Inv);
                    } else {
                        tag->setState(SM);
                        tag->setTimestamp(send_time);
                    }
                    mshr_->setInProgress(addr);

                    if (mem_h_is_debug_event(event))
                        event_debuginfo_.reason = "miss";
                }
                break;
            }
        case E:
        case M:
            if (!tag->hasOtherSharers(event->getSrc()) && !tag->hasOwner()) {
                if (mem_h_is_debug_event(event))
                    event_debuginfo_.reason = "hit";
                if (!in_mshr || !mshr_->getProfiled(addr)) {
                    notifyListenerOfAccess(event, NotifyAccessType::WRITE, NotifyResultType::HIT);
                    stat_event_state_[(int)event->getCmd()][state]->addData(1);
                    stat_hit_[(event->getCmd() == Command::GetX ? 1 : 2)][in_mshr]->addData(1);
                    stat_hits_->addData(1);
                }
                tag->setOwner(event->getSrc());
                if (state != M) {
                    tag->setState(M);
                }
                if (tag->isSharer(event->getSrc())) {
                    tag->removeSharer(event->getSrc());
                    send_time = sendResponseUp(event, nullptr, in_mshr, tag->getTimestamp(), Command::GetXResp);
                } else if (mshr_->hasData(addr))
                    send_time = sendResponseUp(event, &(mshr_->getData(addr)), in_mshr, tag->getTimestamp(), Command::GetXResp);
                else
                    send_time = sendResponseUp(event, data->getData(), in_mshr, tag->getTimestamp(), Command::GetXResp);
                tag->setTimestamp(send_time - 1);
                recordLatencyType(event->getID(), LatType::HIT);
                cleanUpAfterRequest(event, in_mshr);
                break;
            }
            if (!in_mshr)
                status = allocateMSHR(event, false);
            if (status == MemEventStatus::OK) {
                if (!mshr_->getProfiled(addr)) {
                    notifyListenerOfAccess(event, NotifyAccessType::WRITE, NotifyResultType::HIT);
                    stat_event_state_[(int)event->getCmd()][state]->addData(1);
                    stat_hit_[(event->getCmd() == Command::GetX ? 1 : 2)][in_mshr]->addData(1);
                    stat_hits_->addData(1);
                    mshr_->setProfiled(addr);
                }
                recordLatencyType(event->getID(), LatType::INV);
                if (tag->hasOtherSharers(event->getSrc())) {
                    invalidateExceptRequestor(event, tag, in_mshr, !data && !tag->isSharer(event->getSrc()));
                } else {
                    invalidateOwner(event, tag, in_mshr, Command::FetchInv);
                }
                tag->setState(M_Inv);
                mshr_->setInProgress(addr);
            }
            break;
        default:
            if (!in_mshr)
                status = allocateMSHR(event, false);
            else if (mem_h_is_debug_event(event))
                event_debuginfo_.action = "Stall";
    }

    if (mem_h_is_debug_addr(addr) && tag) {
        event_debuginfo_.new_state = tag->getState();
        event_debuginfo_.verbose_line = tag->getString();
        if (data)
            event_debuginfo_.verbose_line += "/" + data->getString();
    }

    if (status == MemEventStatus::Reject)
        sendNACK(event);

    return true;

}


bool MESISharNoninclusive::handleGetSX(MemEvent * event, bool in_mshr) {
    return handleGetX(event, in_mshr);
}


/* Flush a line from the cache but retain a clean copy */
bool MESISharNoninclusive::handleFlushLine(MemEvent* event, bool in_mshr) {
    Addr addr = event->getBaseAddr();
    DirectoryLine * tag = dir_array_->lookup(addr, false);
    State state = tag ? tag->getState() : I;
    DataLine * data = (tag) ? data_array_->lookup(addr, false) : nullptr;
    if (data && data->getTag() != tag) data = nullptr;

    if (mem_h_is_debug_event(event))
        event_debuginfo_.prefill(event->getID(), Command::FlushLine, "", addr, state);

    if (in_mshr)
        mshr_->removePendingRetry(addr);

    // Always need an MSHR for a flush
    MemEventStatus status = MemEventStatus::OK;
    if (in_mshr) {
        if (mshr_->getFrontEvent(addr) != event) {
            if (mem_h_is_debug_event(event))
                event_debuginfo_.action = "Stall";
            status = MemEventStatus::Stall;
        }
    } else {
        status = allocateMSHR(event, false);
    }

    recordLatencyType(event->getID(), LatType::HIT);

    switch (state) {
        case I:
            if (status == MemEventStatus::OK) {
                if (!mshr_->getProfiled(addr)) {
                    stat_event_state_[(int)Command::FlushLine][state]->addData(1);
                    mshr_->setProfiled(addr);
                }
                // event, evict, *data, dirty, time)
                forwardFlush(event, false, nullptr, false, 0);
                mshr_->setInProgress(addr);
            }
            break;
        case S:
            if (status == MemEventStatus::OK) {
                if (!mshr_->getProfiled(addr)) {
                    stat_event_state_[(int)Command::FlushLine][state]->addData(1);
                    mshr_->setProfiled(addr);
                }
                forwardFlush(event, false, nullptr, false, tag->getTimestamp());
                tag->setState(S_B);
                mshr_->setInProgress(addr);
            }
            break;
        case E:
        case M:
            if (status == MemEventStatus::OK) {
                if (!mshr_->getProfiled(addr)) {
                    stat_event_state_[(int)Command::FlushLine][state]->addData(1);
                    mshr_->setProfiled(addr);
                }
                if (event->getEvict()) {
                    removeOwnerViaInv(event, tag, data, false);
                    tag->addSharer(event->getSrc());
                    event->setEvict(false); // Don't stall
                } else if (tag->hasOwner()) {
                    uint64_t send_time = sendFetch(Command::FetchInvX, event, tag->getOwner(), in_mshr, tag->getTimestamp());
                    tag->setTimestamp(send_time - 1);
                    state == E ? tag->setState(E_InvX) : tag->setState(M_InvX);
                    break;
                }
                if (data)
                    forwardFlush(event, true, data->getData(), tag->getState() == M, tag->getTimestamp());
                else
                    forwardFlush(event, true, &(mshr_->getData(addr)), tag->getState() == M, tag->getTimestamp());
                tag->getState() == E ? tag->setState(E_B) : tag->setState(M_B);
                mshr_->setInProgress(addr);
            }
            break;
        case E_InvX:
        case M_InvX:
            if (event->getEvict()) {
                removeOwnerViaInv(event, tag, data, true);
                tag->addSharer(event->getSrc());

                mshr_->decrementAcksNeeded(addr);
                tag->setState(NextState[tag->getState()]);

                retry(addr);
                event->setEvict(false); // Don't replay the eviction part of this flush
            }
            break;
        case E_Inv:
        case M_Inv:
            if (event->getEvict()) {
                removeOwnerViaInv(event, tag, data, false);
                tag->addSharer(event->getSrc());
                event->setEvict(false);
            }
            break;
        default:
            break;
    }

    if (mem_h_is_debug_addr(addr) && tag) {
        event_debuginfo_.new_state = tag->getState();
        event_debuginfo_.verbose_line = tag->getString();
        if (data)
            event_debuginfo_.verbose_line += "/" + data->getString();
    }

    if (status == MemEventStatus::Reject)
        sendNACK(event);

    return true;
}

/* Flush a line from cache & invalidate it */
bool MESISharNoninclusive::handleFlushLineInv(MemEvent* event, bool in_mshr) {
    Addr addr = event->getBaseAddr();
    DirectoryLine * tag = dir_array_->lookup(addr, false);
    State state = tag ? tag->getState() : I;
    DataLine * data = (tag) ? data_array_->lookup(addr, false) : nullptr;
    if (data && data->getTag() != tag) data = nullptr;
    MemEventStatus status = in_mshr ? MemEventStatus::OK : allocateMSHR(event, false);

    if (mem_h_is_debug_event(event))
        event_debuginfo_.prefill(event->getID(), Command::FlushLineInv, "", addr, state);

    if (in_mshr)
        mshr_->removePendingRetry(addr);

    vector<uint8_t>* datavec = nullptr;
    recordLatencyType(event->getID(), LatType::HIT);

    switch (state) {
        case I:
            if (status == MemEventStatus::OK) {
                forwardFlush(event, false, nullptr, false, 0);
                mshr_->setInProgress(addr);
                if (!mshr_->getProfiled(addr)) {
                    stat_event_state_[(int)Command::FlushLineInv][I]->addData(1);
                    mshr_->setProfiled(addr);
                }
            }
            break;
        case S:
            if (status == MemEventStatus::OK) {
                if (!mshr_->getProfiled(addr)) {
                    stat_event_state_[(int)Command::FlushLineInv][S]->addData(1);
                    mshr_->setProfiled(addr);
                }
                if (event->getEvict()) {
                    removeSharerViaInv(event, tag, data, false);
                    event->setEvict(false); // Don't replay eviction
                }
                if (tag->hasSharers()) {
                    invalidateSharers(event, tag, in_mshr, !data && !mshr_->hasData(addr), Command::Inv);
                    tag->setState(S_Inv);
                    break;
                }

                if (data)
                    forwardFlush(event, true, data->getData(), false, tag->getTimestamp());
                else
                    forwardFlush(event, true, &(mshr_->getData(addr)), false, tag->getTimestamp());
                mshr_->setInProgress(addr);
                tag->setState(I_B);
            }
            break;
        case E:
        case M:
            if (status == MemEventStatus::OK) {
                if (!mshr_->getProfiled(addr)) {
                    stat_event_state_[(int)Command::FlushLineInv][state]->addData(1);
                    mshr_->setProfiled(addr);
                }
                if (event->getEvict()) {
                    if (tag->hasOwner())
                        removeOwnerViaInv(event, tag, data, false);
                    else
                        removeSharerViaInv(event, tag, data, false);
                    event->setEvict(false);
                }

                if (tag->hasOwner()) {
                    invalidateOwner(event, tag, in_mshr, Command::FetchInv);
                    tag->getState() == E ? tag->setState(E_Inv) : tag->setState(M_Inv);
                } else if (tag->hasSharers()) {
                    invalidateSharers(event, tag, in_mshr, !data && !mshr_->hasData(addr), Command::Inv);
                    tag->getState() == E ? tag->setState(E_Inv) : tag->setState(M_Inv);
                } else {
                    if (data)
                        forwardFlush(event, true, data->getData(), tag->getState() == M, tag->getTimestamp());
                    else
                        forwardFlush(event, true, &(mshr_->getData(addr)), tag->getState() == M, tag->getTimestamp());
                    mshr_->setInProgress(addr);
                    tag->setState(I_B);
                }
            }
            break;
        case SM_Inv:
        case S_Inv:
        case E_Inv:
        case M_Inv:
        case E_InvX:
        case M_InvX:
            if (event->getEvict()) {
                if (tag->hasOwner())
                    removeOwnerViaInv(event, tag, data, true);
                else
                    removeSharerViaInv(event, tag, data, true);
                event->setEvict(false);

                if (mshr_->decrementAcksNeeded(addr)) {
                    tag->setState(NextState[tag->getState()]);
                    retry(addr);
                }
            }
            break;
        case S_D:
        case E_D:
        case M_D:
        case SM_D:
        case SB_D:
            if (event->getEvict()) {
                if (*(tag->getSharers()->begin()) == event->getSrc()) {
                    removeSharerViaInv(event, tag, data, true);
                    mshr_->decrementAcksNeeded(addr);
                    tag->setState(NextState[tag->getState()]);
                    retry(addr);
                } else
                    removeSharerViaInv(event, tag, data, false);
                event->setEvict(false);
            }
            break;
        default:
            break;
    }

    if (mem_h_is_debug_addr(addr) && tag) {
        event_debuginfo_.new_state = tag->getState();
        event_debuginfo_.verbose_line = tag->getString();
        if (data)
            event_debuginfo_.verbose_line += "/" + data->getString();
    }

    if (status == MemEventStatus::Reject)
        sendNACK(event);

    return true;
}


bool MESISharNoninclusive::handleFlushAll(MemEvent* event, bool in_mshr) {
    event_debuginfo_.prefill(event->getID(), Command::FlushAll, "", 0, State::NP);

    if (!flush_manager_) {
        if (!in_mshr) {
            MemEventStatus status = mshr_->insertFlush(event, false, true);
            if (status == MemEventStatus::Reject) {
                sendNACK(event);
                return true;
            } else if (status == MemEventStatus::Stall) {
                event_debuginfo_.action = "Stall";
                // Don't forward if there's already a waiting FlushAll so we don't risk re-ordering
                return true;
            }
        }
        // Forward flush to flush manager
        MemEvent* flush = new MemEvent(*event); // Copy event for forwarding
        flush->setDst(flush_dest_);
        forwardByDestination(flush, timestamp_ + mshr_latency_);
        event_debuginfo_.action = "Forward";
        return true;
    }

    if (!in_mshr) {
        MemEventStatus status = mshr_->insertFlush(event, false);
        if (status == MemEventStatus::Reject) { /* No room for flush in MSHR */
            sendNACK(event);
            return true;
        } else if (status == MemEventStatus::Stall) { /* Stall for current flush */
            event_debuginfo_.action = "Stall";
            event_debuginfo_.reason = "Flush in progress";
            return true;
        }
    }

    switch (flush_state_) {
        case FlushState::Ready:
        {
            /* Forward requests up (and, if flush manager to peers as well), transition to FlushState::Forward */
            // Broadcast ForwardFlush to all sources
            // Broadcast ForwardFlush to all peers (if flush_manager)
            //      Wait for Ack from all sources & peers -> retry when count == 0
            int count = broadcastMemEventToSources(Command::ForwardFlush, event, timestamp_ + 1);
            mshr_->incrementFlushCount(count);
            flush_state_ = FlushState::Forward;
            event_debuginfo_.action = "Begin";
            break;
        }
        case FlushState::Forward:
        {
            int count = broadcastMemEventToPeers(Command::ForwardFlush, event, timestamp_ + 1);
            mshr_->incrementFlushCount(count);

            if (mshr_->getSize() != mshr_->getFlushSize()) {
                mshr_->incrementFlushCount(mshr_->getSize() - mshr_->getFlushSize());
                event_debuginfo_.action = "Drain";
                flush_state_ = FlushState::Drain;
                break;
            } /* else fall-thru */
        }
        case FlushState::Drain:
        {
            /* Have received all acks and drained MSHR if needed, do local flush */
            /* OK if peers have not all ack'd yet */
            for (auto it : *dir_array_) {
                if (it->getState() == I) continue;
                if (it->getState() == S || it->getState() == E || it->getState() == M) {
                        MemEvent * ev = new MemEvent(cachename_, it->getAddr(), it->getAddr(), Command::NULLCMD);
                        retry_buffer_.push_back(ev);
                        mshr_->incrementFlushCount();
                } else {
                    debug_->fatal(CALL_INFO, -1, "%s, Error: Attempting to flush a cache line that is in a transient state '%s'. Addr = 0x%" PRIx64 ". Event: %s. Time: %" PRIu64 "ns\n",
                            cachename_.c_str(), StateString[it->getState()], it->getAddr(), event->getVerboseString().c_str(), getCurrentSimTimeNano());
                }
            }
            if (mshr_->getFlushCount() != 0) {
                event_debuginfo_.action = "Flush";
                flush_state_ = FlushState::Invalidate;
                break;
            } /* else fall-thru */
        }
        case FlushState::Invalidate:
            /* Have finished invalidating and all peers have flushed */
            // Unblock/respond to sources & peers
            sendResponseUp(event, nullptr, true, timestamp_);
            broadcastMemEventToSources(Command::UnblockFlush, event, timestamp_ + 1);
            mshr_->removeFlush();
            delete event;
            if (mshr_->getFlush() != nullptr) {
                retry_buffer_.push_back(mshr_->getFlush());
            }
            flush_state_ = FlushState::Ready;
            break;
    }

    return true;


}

bool MESISharNoninclusive::handleForwardFlush(MemEvent* event, bool in_mshr) {
    /* Flushes are ordered by the FlushManager and coordinated by the FlushHelper at each level
    * of the hierarchy. Only one cache in a set of distributed caches is the FlushHelper;
    * whereas private caches and monolithic shared caches are the FlushHelper.
    *
    * If FlushHelper - propagate Flush upwards and notify peers when done
    * If not FlushHelper - wait to be contacted by FlushHelper before flushing locally
    */
    event_debuginfo_.prefill(event->getID(), Command::ForwardFlush, "", 0, State::NP);

    if (!in_mshr) {
        MemEventStatus status = mshr_->insertFlush(event, true);
        if (status == MemEventStatus::Reject) { /* No room for flush in MSHR */
            sendNACK(event);
            return true;
        }
    }

    if ( flush_helper_ ) {
        switch (flush_state_) {
            case FlushState::Ready:
            {
                int count = broadcastMemEventToSources(Command::ForwardFlush, event, timestamp_ + 1);
                flush_state_ = FlushState::Forward;
                mshr_->incrementFlushCount(count);
                event_debuginfo_.action = "Begin";
                break;
            }
            case FlushState::Forward:
            {
                int count = broadcastMemEventToPeers(Command::ForwardFlush, event, timestamp_ + 1);
                mshr_->incrementFlushCount(count);

                if (mshr_->getSize() != mshr_->getFlushSize()) {
                    mshr_->incrementFlushCount(mshr_->getSize() - mshr_->getFlushSize());
                    event_debuginfo_.action = "Drain";
                    flush_state_ = FlushState::Drain;
                    break;
                } /* else fall-thru */
            }
            case FlushState::Drain:
            {
                /* Have received all acks and drained MSHR if needed, do local flush */
                /* OK if peers have not all ack'd yet */
                for (auto it : *dir_array_) {
                    if (it->getState() == I) continue;
                    if (it->getState() == S || it->getState() == E || it->getState() == M) {
                        MemEvent * ev = new MemEvent(cachename_, it->getAddr(), it->getAddr(), Command::NULLCMD);
                        retry_buffer_.push_back(ev);
                        mshr_->incrementFlushCount();
                    } else {
                        debug_->fatal(CALL_INFO, -1, "%s, Error: Attempting to flush a cache line that is in a transient state '%s'. Addr = 0x%" PRIx64 ". Event: %s. Time: %" PRIu64 "ns\n",
                                cachename_.c_str(), StateString[it->getState()], it->getAddr(), event->getVerboseString().c_str(), getCurrentSimTimeNano());
                    }
                }
                if (mshr_->getFlushCount()) {
                    event_debuginfo_.action = "Flush";
                    flush_state_ = FlushState::Invalidate;
                    break;
                } /* else fall-thru */
            }
            case FlushState::Invalidate:
                /* All blocks written back and peers have ack'd - respond */
                sendResponseDown(event, nullptr, false, false);
                mshr_->removeFlush();
                delete event;
                flush_state_ = FlushState::Ready;
                break;
        } /* End switch */
        return true;

    /* Not the flush helper; Event is from flush helper */
    } else if ( isPeer(event->getSrc()) ) {
        if (mshr_->getSize() != mshr_->getFlushSize()) { // Drain outstanding Put* before invalidating cache
            mshr_->incrementFlushCount(mshr_->getSize() - mshr_->getFlushSize());
            event_debuginfo_.action = "Drain";
            flush_state_ = FlushState::Drain;

        } else {
            bool evictionNeeded = false;
            for (auto it : *dir_array_) {
                if (it->getState() == I) continue;
                if (it->getState() == S || it->getState() == E || it->getState() == M) {
                    MemEvent * ev = new MemEvent(cachename_, it->getAddr(), it->getAddr(), Command::NULLCMD);
                    retry_buffer_.push_back(ev);
                    evictionNeeded = true;
                    mshr_->incrementFlushCount();
                } else {
                    debug_->fatal(CALL_INFO, -1, "%s, Error: Attempting to flush a cache line that is in a transient state '%s'. Addr = 0x%" PRIx64 ". Event: %s. Time: %" PRIu64 "ns\n",
                        cachename_.c_str(), StateString[it->getState()], it->getAddr(), event->getVerboseString().c_str(), getCurrentSimTimeNano());
                }
            }
            if (evictionNeeded) {
                event_debuginfo_.action = "Flush";
                flush_state_ = FlushState::Invalidate;
            } else {
                sendResponseUp(event, nullptr, true, timestamp_);
                mshr_->removeFlush();
                delete event;
                flush_state_ = FlushState::Forward;
                /* A bit backwards from flush helper/manager - here Forward means OK to execute another ForwardFlush */
                if ( mshr_->getFlush() != nullptr ) {
                    retry_buffer_.push_back(mshr_->getFlush());
                }
            }
        }

    /* Already handled ForwardFlush from flush helper/manager, retire ForwardFlush from peer */
    } else if (flush_state_ == FlushState::Forward) {
        if (in_mshr) mshr_->removeFlush();
        sendResponseDown(event, nullptr, false, false);
        delete event;
        flush_state_ = FlushState::Ready;
        return true;
    }
    /* Remaining case: This is a peer to the flush helper/manager and event is not from the helper/manager
    * Wait until we've handled the event from peer helper/manager before handling this event
    */

    return true;
}


bool MESISharNoninclusive::handlePutS(MemEvent * event, bool in_mshr) {
    Addr addr = event->getBaseAddr();
    DirectoryLine * tag = dir_array_->lookup(addr, false);
    State state = tag ? tag->getState() : I;
    DataLine * data = (tag) ? data_array_->lookup(addr, false) : nullptr;
    if (data && data->getTag() != tag) data = nullptr;
    MemEventStatus status = MemEventStatus::OK;

    if (mem_h_is_debug_event(event))
        event_debuginfo_.prefill(event->getID(), Command::PutS, "", addr, state);

    if (in_mshr)
        mshr_->removePendingRetry(addr);

    MemEventBase* entry;

    switch (state) {
        case S:
        case E:
        case M:
            if (!data && tag->numSharers() == 1) { // Need to allocate line
                status = in_mshr ? MemEventStatus::OK : allocateMSHR(event, false, -1);
                if (status != MemEventStatus::OK) {
                    break;
                }
                status = processDataMiss(event, tag, data, true);
                if (status != MemEventStatus::OK) {
                    if (!mshr_->getProfiled(addr)) {
                        stat_event_state_[(int)Command::PutS][state]->addData(1);
                        mshr_->setProfiled(addr);
                    }
                    if (state == S) tag->setState(SA);
                    else if (state == E) tag->setState(EA);
                    else tag->setState(MA);
                    break;
                }
                data = data_array_->lookup(addr, true);
                data->setData(event->getPayload(), 0);
                if (mem_h_is_debug_addr(addr))
                    printDataValue(addr, &(event->getPayload()), true);
                in_mshr = true;
            }
            if (!in_mshr || !mshr_->getProfiled(addr)) {
                stat_event_state_[(int)Command::PutS][state]->addData(1);
            }
            tag->removeSharer(event->getSrc());
            sendWritebackAck(event);
            cleanUpAfterRequest(event, in_mshr);
            break;
        case S_Inv:
        case E_Inv:
        case M_Inv:
        case SM_Inv:
            removeSharerViaInv(event, tag, data, true);
            sendWritebackAck(event);
            if (in_mshr || !mshr_->getProfiled(addr)) {
                stat_event_state_[(int)Command::PutS][state]->addData(1);
            }
            if (mshr_->decrementAcksNeeded(addr)) {
                tag->setState(NextState[state]);
                retry(addr);
                cleanUpEvent(event, in_mshr);
            } else {
                cleanUpAfterRequest(event, in_mshr);
            }
            break;
        case S_B:
        case E_B:
        case M_B:
            if (!data && tag->numSharers() == 1) {
                status = in_mshr ? MemEventStatus::OK : allocateMSHR(event, false, 1);   // Put just after the Flush, will handle next
                break;
            }
            tag->removeSharer(event->getSrc());
            sendWritebackAck(event);
            if (in_mshr || !mshr_->getProfiled(addr)) {
                stat_event_state_[(int)Command::PutS][state]->addData(1);
            }
            cleanUpEvent(event, in_mshr);
            break;
        case S_D:
        case E_D:
        case M_D:
        case SB_D:
            if (event->getSrc() == *(tag->getSharers()->begin())) { // Sent fetch to this requestor
                // Retry the pending fetch
                mshr_->decrementAcksNeeded(addr);
                mshr_->setData(addr, event->getPayload());
                responses_.find(addr)->second.erase(event->getSrc());
                if (responses_.find(addr)->second.empty())
                    responses_.erase(addr);
                tag->setState(NextState[state]);
                retry(addr);

                // Handle PutS now if we can, later if not
                if (tag->numSharers() > 1) {
                    tag->removeSharer(event->getSrc());
                    sendWritebackAck(event);
                    if (in_mshr || !mshr_->getProfiled(addr)) {
                        stat_event_state_[(int)Command::PutS][state]->addData(1);
                    }
                    cleanUpEvent(event, in_mshr);
                } else {
                    if (in_mshr)
                        mshr_->removeFront(addr); // Need to reinsert after the conflicting request
                    entry = mshr_->getEntryEvent(addr, 1);
                    if (entry && (CommandClassArr[(int)entry->getCmd()] == CommandClass::ForwardRequest || mshr_->getEntry(addr, 1).getInProgress()))
                        status = allocateMSHR(event, false, 2); // Retry the waiting event and waiting forward request, then handle this replacement
                    else
                        status = allocateMSHR(event, false, 1); // Retry the waiting event, then handle this replacement
                }
                break;
            }
            tag->removeSharer(event->getSrc());
            sendWritebackAck(event);
            if (in_mshr || !mshr_->getProfiled(addr)) {
                stat_event_state_[(int)Command::PutS][state]->addData(1);
            }
            cleanUpEvent(event, in_mshr);
            break;
        default:
            debug_->fatal(CALL_INFO,-1,"%s, Error: Received PutS in unhandled state '%s'. Event: %s. Time = %" PRIu64 "ns\n",
                    getName().c_str(), StateString[state], event->getVerboseString().c_str(), getCurrentSimTimeNano());
    }

    if (mem_h_is_debug_addr(addr) && tag) {
        event_debuginfo_.new_state = tag->getState();
        event_debuginfo_.verbose_line = tag->getString();
        if (data)
            event_debuginfo_.verbose_line += "/" + data->getString();
    }

    if (status == MemEventStatus::Reject)
        sendNACK(event);

    return true;
}


bool MESISharNoninclusive::handlePutE(MemEvent * event, bool in_mshr) {
    Addr addr = event->getBaseAddr();
    DirectoryLine * tag = dir_array_->lookup(addr, false);
    State state = tag ? tag->getState() : I;
    DataLine * data = (tag) ? data_array_->lookup(addr, false) : nullptr;
    if (data && data->getTag() != tag) data = nullptr;

    MemEventStatus status = MemEventStatus::OK;
    if (mem_h_is_debug_event(event))
        event_debuginfo_.prefill(event->getID(), Command::PutE, "", addr, state);

    if (in_mshr)
        mshr_->removePendingRetry(addr);

    switch (state) {
        case E:
        case M:
            if (!data) { // Need to allocate line
                status = in_mshr ? MemEventStatus::OK : allocateMSHR(event, false, -1);
                if (status != MemEventStatus::OK)
                    break;
                status = processDataMiss(event, tag, data, true);
                if (status != MemEventStatus::OK) {
                    if (!in_mshr || !mshr_->getProfiled(addr)) {
                        stat_event_state_[(int)Command::PutE][state]->addData(1);
                        mshr_->setProfiled(addr);
                    }
                    state == E ? tag->setState(EA) : tag->setState(MA);
                    break;
                }
                data = data_array_->lookup(addr, true);
                data->setData(event->getPayload(), 0);
                if (mem_h_is_debug_addr(addr))
                    printDataValue(addr, &(event->getPayload()), true);
                in_mshr = true;
            }
            tag->removeOwner();
            sendWritebackAck(event);
            if (!in_mshr || !mshr_->getProfiled(addr)) {
                stat_event_state_[(int)Command::PutE][state]->addData(1);
            }
            cleanUpAfterRequest(event, in_mshr);
            break;
        case E_InvX:
        case M_InvX:
            tag->removeOwner();
            mshr_->decrementAcksNeeded(addr);
            if (!data && !mshr_->hasData(addr))
                mshr_->setData(addr, event->getPayload());
            responses_.find(addr)->second.erase(event->getSrc());
            if (responses_.find(addr)->second.empty())
                responses_.erase(addr);
            tag->setState(NextState[state]);
            // Handle PutE now if possible, later if not
            if (data) {
                sendWritebackAck(event);
                cleanUpEvent(event, in_mshr);
                if (!in_mshr || !mshr_->getProfiled(addr)) {
                    stat_event_state_[(int)Command::PutE][state]->addData(1);
                }
            } else {
                tag->addSharer(event->getSrc());
                event->setCmd(Command::PutS);
                if (in_mshr)
                    mshr_->removeFront(addr); // Need to reinsert after the conflicting request
                MemEventBase* entry = mshr_->getEntryEvent(addr, 1);
                if (entry && (CommandClassArr[(int)entry->getCmd()] == CommandClass::ForwardRequest || mshr_->getEntry(addr, 1).getInProgress()))
                    status = allocateMSHR(event, false, 2); // Retry the waiting event and waiting forward request, then handle this replacement
                else
                    status = allocateMSHR(event, false, 1); // Retry the waiting event, then handle this replacement
                retry(addr);
            }
            break;
        case E_Inv:
        case M_Inv:
            tag->removeOwner();
            if (!data && !mshr_->hasData(addr))
                mshr_->setData(addr, event->getPayload());
            responses_.find(addr)->second.erase(event->getSrc());
            if (responses_.find(addr)->second.empty())
                responses_.erase(addr);
            sendWritebackAck(event);
            tag->setState(NextState[state]);

            if (mshr_->decrementAcksNeeded(addr)) {
                retry(addr);
                cleanUpEvent(event, in_mshr);
            } else {
                cleanUpAfterRequest(event, in_mshr);
            }

            if (!in_mshr || !mshr_->getProfiled(addr)) {
                stat_event_state_[(int)Command::PutE][state]->addData(1);
            }
            break;
        default:
            debug_->fatal(CALL_INFO,-1,"%s, Error: Received PutE in unhandled state '%s'. Event: %s. Time = %" PRIu64 "ns\n",
                    getName().c_str(), StateString[state], event->getVerboseString().c_str(), getCurrentSimTimeNano());
    }

    if (mem_h_is_debug_addr(addr) && tag) {
        event_debuginfo_.new_state = tag->getState();
        event_debuginfo_.verbose_line = tag->getString();
        if (data)
            event_debuginfo_.verbose_line += "/" + data->getString();
    }
    if (status == MemEventStatus::Reject)
        sendNACK(event);

    return true;
}

bool MESISharNoninclusive::handlePutM(MemEvent * event, bool in_mshr) {
    Addr addr = event->getBaseAddr();
    DirectoryLine * tag = dir_array_->lookup(addr, false);
    State state = tag ? tag->getState() : I;
    DataLine * data = (tag) ? data_array_->lookup(addr, false) : nullptr;
    if (data && data->getTag() != tag) data = nullptr;

    MemEventStatus status = MemEventStatus::OK;
    if (mem_h_is_debug_event(event))
        event_debuginfo_.prefill(event->getID(), Command::PutM, "", addr, state);

    if (in_mshr)
        mshr_->removePendingRetry(addr);

    switch (state) {
        case E:
        case M:
            if (!data) { // Need to allocate line
                status = in_mshr ? MemEventStatus::OK : allocateMSHR(event, false, -1); // In case we need to stall dataline allocation
                if (status == MemEventStatus::Reject) {
                    break;
                } else if (status == MemEventStatus::Stall) {
                    if (mem_h_is_debug_event(event)) {
                        event_debuginfo_.action = "Stall";
                        event_debuginfo_.reason = "MSHR conflict";
                    }
                    break;
                }
                if (!in_mshr || !mshr_->getProfiled(addr)) {
                    stat_event_state_[(int)Command::PutM][state]->addData(1);
                    mshr_->setProfiled(addr);
                }
                status = processDataMiss(event, tag, data, true);
                if (status != MemEventStatus::OK) {
                    tag->setState(MA);
                    if (mem_h_is_debug_event(event)) {
                        event_debuginfo_.action = "Stall";
                        event_debuginfo_.reason = "Data line miss";
                    }
                    break;
                }
                in_mshr = true;
            } else if (!in_mshr || !mshr_->getProfiled(addr)) {
                stat_event_state_[(int)Command::PutM][state]->addData(1);
            }
            if (mem_h_is_debug_addr(addr))
                printDataValue(addr, &(event->getPayload()), true);
            data = data_array_->lookup(addr, true);
            data->setData(event->getPayload(), 0);
            if (mem_h_is_debug_event(event))
                event_debuginfo_.reason = "hit";

            tag->removeOwner();
            tag->setState(M);
            sendWritebackAck(event);
            cleanUpAfterRequest(event, in_mshr);
            break;
        case E_InvX:
        case M_InvX:
        {
            tag->removeOwner();
            mshr_->decrementAcksNeeded(addr);
            responses_.find(addr)->second.erase(event->getSrc());
            if (responses_.find(addr)->second.empty())
                responses_.erase(addr);
            tag->setState(M);

            // Handle PutM now if possible, later if not
            if (data) {
                if (!in_mshr || !mshr_->getProfiled(addr)) {
                    stat_event_state_[(int)Command::PutM][state]->addData(1);
                }
                data->setData(event->getPayload(), 0);
                if (mem_h_is_debug_addr(addr))
                    printDataValue(addr, &(event->getPayload()), true);
                sendWritebackAck(event);
                cleanUpEvent(event, in_mshr);
            } else {
                tag->addSharer(event->getSrc());
                event->setCmd(Command::PutS);
                mshr_->setData(addr, event->getPayload());
                if (in_mshr)
                    mshr_->removeFront(addr); // Need to reinsert after the conflicting request
                MemEventBase* entry = mshr_->getEntryEvent(addr, 1);
                if (entry && (CommandClassArr[(int)entry->getCmd()] == CommandClass::ForwardRequest || mshr_->getEntry(addr, 1).getInProgress()))
                    status = allocateMSHR(event, false, 2); // Retry the waiting event and waiting forward request, then handle this replacement
                else
                    status = allocateMSHR(event, false, 1); // Retry the waiting event, then handle this replacement
            }
            retry(addr);
            break;
        }
        case E_Inv:
        case M_Inv:
            if (!in_mshr || !mshr_->getProfiled(addr)) {
                stat_event_state_[(int)Command::PutM][state]->addData(1);
            }
            // Handle the coherence state part and buffer the data in the MSHR or dataline if we have it
            // We won't need to allocate a line because we're either deallocating the data or one of our children wants it
            tag->removeOwner();

            if (!data)
                mshr_->setData(addr, event->getPayload());
            else
                data->setData(event->getPayload(), 0);
            responses_.find(addr)->second.erase(event->getSrc());
            if (responses_.find(addr)->second.empty())
                responses_.erase(addr);
            sendWritebackAck(event);
            tag->setState(M);

            if (mshr_->decrementAcksNeeded(addr)) {
                retry(addr);
                cleanUpEvent(event, in_mshr);
            } else {
                cleanUpAfterRequest(event, in_mshr);
            }
            break;
        default:
            debug_->fatal(CALL_INFO,-1,"%s, Error: Received PutM in unhandled state '%s'. Event: %s. Time = %" PRIu64 "ns\n",
                    getName().c_str(), StateString[state], event->getVerboseString().c_str(), getCurrentSimTimeNano());
    }

    if (mem_h_is_debug_addr(addr) && tag) {
        event_debuginfo_.new_state = tag->getState();
        event_debuginfo_.verbose_line = tag->getString();
        if (data)
            event_debuginfo_.verbose_line += "/" + data->getString();
    }
    if (status == MemEventStatus::Reject)
        sendNACK(event);

    return true;
}

bool MESISharNoninclusive::handlePutX(MemEvent * event, bool in_mshr) {
    Addr addr = event->getBaseAddr();
    DirectoryLine * tag = dir_array_->lookup(addr, false);
    State state = tag ? tag->getState() : I;
    DataLine * data = (tag) ? data_array_->lookup(addr, false) : nullptr;
    if (data && data->getTag() != tag) data = nullptr;

    if (mem_h_is_debug_event(event))
        event_debuginfo_.prefill(event->getID(), Command::PutX, "", addr, state);

    if (in_mshr)
        mshr_->removePendingRetry(addr);

    tag->removeOwner();
    tag->addSharer(event->getSrc());

    sendWritebackAck(event);

    if (!in_mshr || !mshr_->getProfiled(addr)) {
        stat_event_state_[(int)Command::PutX][state]->addData(1);
    }

    switch (state) {
        case E:
        case M:
            if (event->getDirty())
                tag->setState(M);

            if (data) {
                data->setData(event->getPayload(), 0);
                if (mem_h_is_debug_addr(addr))
                    printDataValue(addr, &(event->getPayload()), true);
            }
            cleanUpAfterRequest(event, in_mshr);
            break;
        case E_InvX:
        case M_InvX:
            if (event->getDirty() || state == M_InvX)
                tag->setState(M);
            else
                tag->setState(E);

            if (data)
                data->setData(event->getPayload(), 0);
            else
                mshr_->setData(addr, event->getPayload());

            if (mem_h_is_debug_addr(addr))
                printDataValue(addr, &(event->getPayload()), true);

            mshr_->decrementAcksNeeded(addr);

            cleanUpAfterRequest(event, in_mshr);
            break;
        case E_Inv:
        case M_Inv:
            if (event->getDirty())
                tag->setState(M_Inv);

            if (data)
                data->setData(event->getPayload(), 0);
            else
                mshr_->setData(addr, event->getPayload());

            if (mem_h_is_debug_addr(addr))
                printDataValue(addr, &(event->getPayload()), true);

            cleanUpEvent(event, in_mshr);
            break;
        default:
            debug_->fatal(CALL_INFO,-1,"%s, Error: Received PutX in unhandled state '%s'. Event: %s. Time = %" PRIu64 "ns\n",
                    getName().c_str(), StateString[state], event->getVerboseString().c_str(), getCurrentSimTimeNano());
    }

    if (mem_h_is_debug_addr(addr) && tag) {
        event_debuginfo_.new_state = tag->getState();
        event_debuginfo_.verbose_line = tag->getString();
        if (data)
            event_debuginfo_.verbose_line += "/" + data->getString();
    }

    return true;

}

bool MESISharNoninclusive::handleFetch(MemEvent * event, bool in_mshr) {
    Addr addr = event->getBaseAddr();
    DirectoryLine * tag = dir_array_->lookup(addr, false);
    State state = tag ? tag->getState() : I;
    DataLine * data = (tag) ? data_array_->lookup(addr, false) : nullptr;
    if (data && data->getTag() != tag) data = nullptr;

    MemEventStatus status = MemEventStatus::OK;
    if (mem_h_is_debug_event(event))
        event_debuginfo_.prefill(event->getID(), Command::Fetch, "", addr, state);

    if (in_mshr)
        mshr_->removePendingRetry(addr);

    uint64_t send_time;
    MemEvent* put;
    switch (state) {
        case I: // Happens if we replaced and are waiting for a writeback ack or we are replaying the event after an eviction -> drop the fetch
        case I_B: // Happens if we sent a FlushLineInv and it raced with a Fetch
        case E_B: // Happens if we sent a FlushLine and it raced with Fetch
        case M_B: // Happens if we sent a FlushLine and it raced with Fetch
            stat_event_state_[(int)Command::Fetch][state]->addData(1);
            delete event;
            break;
        case S:
            if (!in_mshr || !mshr_->getProfiled(addr)) {
                stat_event_state_[(int)Command::Fetch][state]->addData(1);
            }
            if (data) {
                sendResponseDown(event, data->getData(), false, false);
                cleanUpEvent(event, in_mshr);
            } else if (mshr_->hasData(addr)) {
                sendResponseDown(event, &(mshr_->getData(addr)), false, false);
                cleanUpEvent(event, in_mshr);
            } else {
                status = in_mshr ? MemEventStatus::OK : allocateMSHR(event, true, 0);
                if (status == MemEventStatus::OK) {
                    mshr_->setProfiled(addr);
                    tag->setState(S_D);
                    if (!applyPendingReplacement(addr))
                        send_time = sendFetch(Command::Fetch, event, *(tag->getSharers()->begin()), in_mshr, tag->getTimestamp());
                }
            }
            break;
        case SA:
            //Look for a PutS in the MSHR
            put = static_cast<MemEvent*>(mshr_->getFirstEventEntry(addr, Command::PutS));
            sendResponseDown(event, &(put->getPayload()), false, false);
            stat_event_state_[(int)Command::Fetch][state]->addData(1);
            cleanUpEvent(event, in_mshr);
            break;
        case SM:
            if (!in_mshr || !mshr_->getProfiled(addr)) {
                stat_event_state_[(int)Command::Fetch][state]->addData(1);
            }
            if (data) {
                sendResponseDown(event, data->getData(), false, false);
                cleanUpEvent(event, in_mshr);
            } else if (mshr_->hasData(addr)) {
                sendResponseDown(event, &(mshr_->getData(addr)), false, false);
                cleanUpEvent(event, in_mshr);
            } else {
                status = in_mshr ? MemEventStatus::OK : allocateMSHR(event, true, 0);
                if (status == MemEventStatus::OK) {
                    mshr_->setProfiled(addr);
                    tag->setState(SM_D);
                    send_time = sendFetch(Command::Fetch, event, *(tag->getSharers()->begin()), in_mshr, tag->getTimestamp());
                }
            }
            break;
        case S_D:
        case S_Inv:
            if (!in_mshr)
                status = allocateMSHR(event, true, 1);
            break;
        case S_B:
            if (!in_mshr || !mshr_->getProfiled(addr)) {
                stat_event_state_[(int)Command::Fetch][state]->addData(1);
            }
            if (data) {
                sendResponseDown(event, data->getData(), false, false);
                cleanUpEvent(event, in_mshr);
            } else if (mshr_->hasData(addr)) {
                sendResponseDown(event, &(mshr_->getData(addr)), false, false);
                cleanUpEvent(event, in_mshr);
            } else {
                status = in_mshr ? MemEventStatus::OK : allocateMSHR(event, true, 0);
                if (status == MemEventStatus::OK) {
                    mshr_->setProfiled(addr);
                    tag->setState(SB_D);
                    send_time = sendFetch(Command::Fetch, event, *(tag->getSharers()->begin()), in_mshr, tag->getTimestamp());
                }
            }
            break;
        default:
            debug_->fatal(CALL_INFO,-1,"%s, Error: Received Fetch in unhandled state '%s'. Event: %s. Time = %" PRIu64 "ns\n",
                    getName().c_str(), StateString[state], event->getVerboseString().c_str(), getCurrentSimTimeNano());
    }

    if (mem_h_is_debug_addr(addr) && tag) {
        event_debuginfo_.new_state = tag->getState();
        event_debuginfo_.verbose_line = tag->getString();
        if (data)
            event_debuginfo_.verbose_line += "/" + data->getString();
    }

    if (status == MemEventStatus::Reject)
        sendNACK(event);

    return true;
}


bool MESISharNoninclusive::handleInv(MemEvent * event, bool in_mshr) {
    Addr addr = event->getBaseAddr();
    DirectoryLine * tag = dir_array_->lookup(addr, false);
    State state = tag ? tag->getState() : I;
    DataLine * data = (tag) ? data_array_->lookup(addr, false) : nullptr;
    if (data && data->getTag() != tag) data = nullptr;
    MemEventStatus status = MemEventStatus::OK;

    if (mem_h_is_debug_event(event))
        event_debuginfo_.prefill(event->getID(), Command::Inv, "", addr, state);

    if (in_mshr)
        mshr_->removePendingRetry(addr);

    MemEvent * put;
    switch (state) {
        case I_B:
            dir_array_->deallocate(tag);
            if (data)
                data_array_->deallocate(data);
        case I:
            stat_event_state_[(int)Command::Inv][state]->addData(1);
            delete event;
            break;
        case S:
            if (tag->hasSharers() && !in_mshr) {
                status = allocateMSHR(event, true, 0);
                in_mshr = status != MemEventStatus::Reject;
            }

            if (status == MemEventStatus::OK) {
                if (!in_mshr || !mshr_->getProfiled(addr)) {
                    recordPrefetchResult(tag, stat_prefetch_inv_);
                    stat_event_state_[(int)Command::Inv][state]->addData(1);
                    if (in_mshr) mshr_->setProfiled(addr);
                }

                if (tag->hasSharers()) {
                    if (!applyPendingReplacement(addr))
                        invalidateSharers(event, tag, in_mshr, false, Command::Inv);
                    tag->setState(S_Inv);
                } else {
                    sendResponseDown(event, nullptr, false, true);
                    dir_array_->deallocate(tag);
                    if (data)
                        data_array_->deallocate(data);
                    cleanUpAfterRequest(event, in_mshr);
                    if (mshr_->hasData(addr))
                        mshr_->clearData(addr);
                }
            }
            break;
        case SA: // Merge with PutS and drop
            // TODO make sure the pending eviction won't mess anything up when it tries to replay
            put = static_cast<MemEvent*>(mshr_->getFrontEvent(addr));
            sendWritebackAck(put);
            sendResponseDown(event, nullptr, false, true);
            dir_array_->deallocate(tag);
            if (mshr_->hasData(addr))
                mshr_->clearData(addr);
            if (!in_mshr || !mshr_->getProfiled(addr)) {
                stat_event_state_[(int)Command::Inv][state]->addData(1);
            }
            cleanUpEvent(event, in_mshr);
            cleanUpAfterRequest(put, true);
            break;
        case S_D:
            if (!in_mshr)
                status = allocateMSHR(event, true, 1);
            break;
        case S_Inv: // Evict
            if (!in_mshr) {
                status = allocateMSHR(event, true, 0);
                if (status == MemEventStatus::OK) {
                    mshr_->setProfiled(addr);
                    stat_event_state_[(int)Command::Inv][state]->addData(1);
                }
            }
            break;
        case SM:
            if (tag->hasSharers() && !in_mshr)
                status = allocateMSHR(event, true, 0);

            if (status == MemEventStatus::OK) {
                if (!in_mshr || !mshr_->getProfiled(addr)) {
                    stat_event_state_[(int)Command::Inv][state]->addData(1);
                }
                if (tag->hasSharers()) {
                    invalidateSharers(event, tag, in_mshr, false, Command::Inv);
                    tag->setState(SM_Inv);
                    mshr_->setProfiled(addr);
                } else {
                    sendResponseDown(event, nullptr, false, true);
                    tag->setState(IM);
                    cleanUpAfterRequest(event, in_mshr);
                    if (mshr_->hasData(addr))
                        mshr_->clearData(addr);
                }
            }
            break;
        case SM_Inv:
            if (!in_mshr) {
                status = allocateMSHR(event, true, 0);
                if (status == MemEventStatus::OK) {
                    mshr_->setProfiled(addr);
                    stat_event_state_[(int)Command::Inv][state]->addData(1);
                }
            }
            break;
        case S_B:
            if (tag->hasSharers() && !in_mshr)
                status = allocateMSHR(event, true, 0);

            if (status == MemEventStatus::OK) {
                if (!in_mshr || !mshr_->getProfiled(addr)) {
                    stat_event_state_[(int)Command::Inv][state]->addData(1);
                }
                if (tag->hasSharers()) {
                    invalidateSharers(event, tag, in_mshr, false, Command::Inv);
                    tag->setState(SB_Inv);
                    mshr_->setProfiled(addr);
                } else {
                    sendResponseDown(event, nullptr, false, true);
                    dir_array_->deallocate(tag);
                    if (data)
                        data_array_->deallocate(data);
                    if (mshr_->hasData(addr))
                        mshr_->clearData(addr);
                    cleanUpAfterRequest(event, in_mshr);
                }
            }
            break;
        default:
            debug_->fatal(CALL_INFO,-1,"%s, Error: Received Inv in unhandled state '%s'. Event: %s. Time = %" PRIu64 "ns\n",
                    getName().c_str(), StateString[state], event->getVerboseString().c_str(), getCurrentSimTimeNano());
    }

    if (mem_h_is_debug_addr(addr) && tag) {
        event_debuginfo_.new_state = tag->getState();
        event_debuginfo_.verbose_line = tag->getString();
        if (data)
            event_debuginfo_.verbose_line += "/" + data->getString();
    }

    if (status == MemEventStatus::Reject)
        sendNACK(event);

    return true;
}


bool MESISharNoninclusive::handleForceInv(MemEvent * event, bool in_mshr) {
    Addr addr = event->getBaseAddr();
    DirectoryLine * tag = dir_array_->lookup(addr, false);
    State state = tag ? tag->getState() : I;
    DataLine * data = (tag) ? data_array_->lookup(addr, false) : nullptr;
    if (data && data->getTag() != tag) data = nullptr;
    MemEventStatus status = MemEventStatus::OK;

    if (mem_h_is_debug_event(event))
        event_debuginfo_.prefill(event->getID(), Command::ForceInv, "", addr, state);

    if (in_mshr)
        mshr_->removePendingRetry(addr);

    MemEvent * put;
    switch (state) {
        case I_B:
            dir_array_->deallocate(tag);
            if (data)
                data_array_->deallocate(data);
        case I:
            stat_event_state_[(int)Command::ForceInv][state]->addData(1);
            delete event;
            break;
        case S:
            if (tag->hasSharers() && !in_mshr)
                status = allocateMSHR(event, true, 0);

            if (status == MemEventStatus::OK) {
                if (!in_mshr || !mshr_->getProfiled(addr)) {
                    recordPrefetchResult(tag, stat_prefetch_inv_);
                    stat_event_state_[(int)Command::ForceInv][state]->addData(1);
                    if (tag->hasSharers()) mshr_->setProfiled(addr);
                }
                if (tag->hasSharers()) {
                    if (!applyPendingReplacement(addr))
                        invalidateSharers(event, tag, in_mshr, false, Command::ForceInv);
                    tag->setState(S_Inv);
                } else {
                    sendResponseDown(event, nullptr, false, true);
                    dir_array_->deallocate(tag);
                    if (data) {
                        data_array_->deallocate(data);
                    }
                    cleanUpAfterRequest(event, in_mshr);
                    if (mshr_->hasData(addr))
                        mshr_->clearData(addr);
                }
            }
            break;
        case E:
        case M:
            if ((tag->hasSharers() || tag->hasOwner()) && !in_mshr)
                status = allocateMSHR(event, true, 0);

            if (status == MemEventStatus::OK) {
                if (!in_mshr || !mshr_->getProfiled(addr)) {
                    stat_event_state_[(int)Command::ForceInv][state]->addData(1);
                    recordPrefetchResult(tag, stat_prefetch_inv_);
                }
                if (tag->hasSharers()) {
                    if (!applyPendingReplacement(addr))
                        invalidateSharers(event, tag, in_mshr, false, Command::ForceInv);
                    state == E ? tag->setState(E_Inv) : tag->setState(M_Inv);
                    mshr_->setProfiled(addr);
                } else if (tag->hasOwner()) {
                    if (!applyPendingReplacement(addr))
                        invalidateOwner(event, tag, in_mshr, Command::ForceInv);
                    state == E ? tag->setState(E_Inv) : tag->setState(M_Inv);
                    mshr_->setProfiled(addr);
                } else {
                    sendResponseDown(event, nullptr, false, true);
                    dir_array_->deallocate(tag);
                    if (data) {
                        data_array_->deallocate(data);
                    }
                    cleanUpAfterRequest(event, in_mshr);
                    if (mshr_->hasData(addr))
                        mshr_->clearData(addr);
                }
            }
            break;
        case S_B:
        case E_B:
        case M_B:
            if ((tag->hasSharers() || tag->hasOwner()) && !in_mshr)
                status = allocateMSHR(event, true, 0);

            if (status == MemEventStatus::OK) {
                if (!in_mshr || !mshr_->getProfiled(addr))  {
                    stat_event_state_[(int)Command::ForceInv][state]->addData(1);
                }
                if (tag->hasSharers()) {
                    invalidateSharers(event, tag, in_mshr, false, Command::ForceInv);
                    tag->setState(SB_Inv);
                    mshr_->setProfiled(addr);
                } else {
                    sendResponseDown(event, nullptr, false, true);
                    dir_array_->deallocate(tag);
                    if (data)
                        data_array_->deallocate(data);
                    if (mshr_->hasData(addr))
                        mshr_->clearData(addr);
                    cleanUpAfterRequest(event, in_mshr);
                }
            }
            break;
        case S_D:
        case E_D:
        case M_D:
        case E_InvX:
        case M_InvX:
            if (!in_mshr)
                status = allocateMSHR(event, true, 1);
            break;
        case S_Inv:     // Evict, FlushLineInv
        case E_Inv:     // Evict, FlushLineInv
        case M_Inv:     // Evict, FlushlIneInv, Internal GetX
            if (!in_mshr) {
                if (mshr_->getFrontType(addr) == MSHREntryType::Evict || mshr_->getFrontEvent(addr)->getCmd() == Command::FlushLineInv) {
                    status = allocateMSHR(event, true, 0);
                    if (status == MemEventStatus::OK) {
                        mshr_->setProfiled(addr);
                        stat_event_state_[(int)Command::ForceInv][state]->addData(1);
                    }
                } else { // In a race with GetX/GetSX, let the other event complete first since it always can and this will avoid repeatedly losing the block before the Get* can complete
                    status = allocateMSHR(event, true, 1);
                }
            }
            break;
        case SM:
            if (!in_mshr || !mshr_->getProfiled(addr)) {
                stat_event_state_[(int)Command::ForceInv][state]->addData(1);
            }
            if (!tag->hasSharers()) {
                sendResponseDown(event, nullptr, false, true);
                tag->setState(IM);
                cleanUpEvent(event, in_mshr);
            } else {
                if (!in_mshr)
                    status = allocateMSHR(event, true, 0);
                if (status == MemEventStatus::OK) {
                    invalidateSharers(event, tag, in_mshr, false, Command::Inv);
                    tag->setState(SM_Inv);
                    mshr_->setProfiled(addr);
                }
            }
            break;
        case SM_Inv:
            if (!in_mshr)
                status = allocateMSHR(event, true, 0);
            if (status == MemEventStatus::OK) {
                stat_event_state_[(int)Command::ForceInv][state]->addData(1);
                mshr_->setProfiled(addr);
            }
            break;
        case SA:
        case EA:
        case MA:
            if (!in_mshr || !mshr_->getProfiled(addr)) {
                stat_event_state_[(int)Command::ForceInv][state]->addData(1);
            }
            // TODO make sure the pending eviction won't mess anything up when it tries to replay
            put = static_cast<MemEvent*>(mshr_->getFrontEvent(addr));
            sendWritebackAck(put);
            sendResponseDown(event, nullptr, state == MA, true);
            dir_array_->deallocate(tag);
            if (mshr_->hasData(addr))
                mshr_->clearData(addr);
            cleanUpEvent(event, in_mshr);
            cleanUpAfterRequest(put, true);
            break;
        default:
            debug_->fatal(CALL_INFO,-1,"%s, Error: Received ForceInv in unhandled state '%s'. Event: %s. Time = %" PRIu64 "ns\n",
                    getName().c_str(), StateString[state], event->getVerboseString().c_str(), getCurrentSimTimeNano());
    }

    if (mem_h_is_debug_addr(addr) && tag) {
        event_debuginfo_.new_state = tag->getState();
        event_debuginfo_.verbose_line = tag->getString();
        if (data)
            event_debuginfo_.verbose_line += "/" + data->getString();
    }

    if (status == MemEventStatus::Reject)
        sendNACK(event);

    return true;
}


bool MESISharNoninclusive::handleFetchInv(MemEvent * event, bool in_mshr){
    Addr addr = event->getBaseAddr();
    DirectoryLine * tag = dir_array_->lookup(addr, false);
    State state = tag ? tag->getState() : I;
    DataLine * data = (tag) ? data_array_->lookup(addr, false) : nullptr;
    if (data && data->getTag() != tag) data = nullptr;
    MemEventStatus status = MemEventStatus::OK;

    if (mem_h_is_debug_event(event))
        event_debuginfo_.prefill(event->getID(), Command::FetchInv, "", addr, state);

    if (in_mshr)
        mshr_->removePendingRetry(addr);

    MemEvent * put;
    switch (state) {
        case I:
            stat_event_state_[(int)Command::FetchInv][state]->addData(1);
            delete event;
            break;
        case S:
            if (tag->hasSharers() && !in_mshr)
                status = allocateMSHR(event, true, 0);

            if (status == MemEventStatus::OK) {
                if (!in_mshr || !mshr_->getProfiled(addr)) {
                    recordPrefetchResult(tag, stat_prefetch_inv_);
                    stat_event_state_[(int)Command::FetchInv][state]->addData(1);
                    if (tag->hasSharers()) mshr_->setProfiled(addr);
                }
                if (tag->hasSharers()) {
                    tag->setState(S_Inv);
                    if (!applyPendingReplacement(addr))
                        invalidateSharers(event, tag, in_mshr, !(data || mshr_->hasData(addr)), Command::Inv);
                } else {
                    if (data)
                        sendResponseDown(event, data->getData(), false, true);
                    else {
                        sendResponseDown(event, &(mshr_->getData(addr)), false, true);
                        mshr_->clearData(addr);
                    }
                    dir_array_->deallocate(tag);
                    if (data) {
                        data_array_->deallocate(data);
                    }
                    cleanUpAfterRequest(event, in_mshr);
                }
            }
            break;
        case E:
        case M:
            if ((tag->hasSharers() || tag->hasOwner())&& !in_mshr)
                status = allocateMSHR(event, true, 0);

            if (status == MemEventStatus::OK) {
                if (!in_mshr || !mshr_->getProfiled(addr)) {
                    stat_event_state_[(int)Command::FetchInv][state]->addData(1);
                    if (tag->hasOwner() || tag->hasSharers()) mshr_->setProfiled(addr);
                    recordPrefetchResult(tag, stat_prefetch_inv_);
                }
                if (applyPendingReplacement(addr)) {
                    state == E ? tag->setState(E_Inv) : tag->setState(M_Inv);
                } else if (tag->hasSharers()) {
                    invalidateSharers(event, tag, in_mshr, !data, Command::Inv);
                    state == E ? tag->setState(E_Inv) : tag->setState(M_Inv);
                } else if (tag->hasOwner()) {
                    invalidateOwner(event, tag, in_mshr, Command::FetchInv);
                    state == E ? tag->setState(E_Inv) : tag->setState(M_Inv);
                } else {
                    if (data)
                        sendResponseDown(event, data->getData(), state == M, true);
                    else {
                        sendResponseDown(event, &(mshr_->getData(addr)), state == M, true);
                        mshr_->clearData(addr);
                    }
                    dir_array_->deallocate(tag);
                    if (data) {
                        data_array_->deallocate(data);
                    }
                    cleanUpAfterRequest(event, in_mshr);
                }
            }
            break;
        case S_B:
        case E_B:
        case M_B:
            if (tag->hasSharers() && !in_mshr)
                status = allocateMSHR(event, true, 0);

            if (status == MemEventStatus::OK) {
                if (!in_mshr || !mshr_->getProfiled(addr)) {
                    stat_event_state_[(int)Command::FetchInv][state]->addData(1);
                    if (tag->hasSharers()) mshr_->setProfiled(addr);
                }
                if (tag->hasSharers()) {
                    invalidateSharers(event, tag, in_mshr, !data, Command::Inv);
                    tag->setState(SB_Inv);
                } else {
                    if (data) {
                        sendResponseDown(event, data->getData(), false, true);
                        data_array_->deallocate(data);
                    } else {
                        sendResponseDown(event, &(mshr_->getData(addr)), false, true);
                        mshr_->clearData(addr);
                    }
                    dir_array_->deallocate(tag);
                    cleanUpAfterRequest(event, in_mshr);
                }
            }
            break;
        case S_D:
        case E_D:
        case M_D:
            if (!in_mshr)
                status = allocateMSHR(event, true, 1);
            break;
        case SA:
        case EA:
        case MA:
            if (!in_mshr || !mshr_->getProfiled(addr)) {
                stat_event_state_[(int)Command::FetchInv][state]->addData(1);
            }
            // TODO make sure the pending eviction won't mess anything up when it tries to replay
            put = static_cast<MemEvent*>(mshr_->getFrontEvent(addr));
            sendWritebackAck(put);
            sendResponseDown(event, &(put->getPayload()), state == MA, true);
            dir_array_->deallocate(tag);
            if (mshr_->hasData(addr))
                mshr_->clearData(addr);
            cleanUpEvent(event, in_mshr);
            cleanUpAfterRequest(put, true);
            break;
        case SM:
            if (!tag->hasSharers()) {
                if (!in_mshr || !mshr_->getProfiled(addr)) {
                    stat_event_state_[(int)Command::FetchInv][state]->addData(1);
                }
                tag->setState(IM);
                if (data)
                    sendResponseDown(event, data->getData(), false, true);
                else
                    sendResponseDown(event, &(mshr_->getData(addr)), false, true);
                tag->setState(IM);
                if (mshr_->hasData(addr))
                    mshr_->clearData(addr);
                cleanUpEvent(event, in_mshr);
                break;
            }
            if (!in_mshr)
                status = allocateMSHR(event, true, 0);
            if (status == MemEventStatus::OK) {
                if (!in_mshr || !mshr_->getProfiled(addr)) {
                    mshr_->setProfiled(addr);
                    stat_event_state_[(int)Command::FetchInv][state]->addData(1);
                }
                if (tag->hasSharers()) {
                    invalidateSharers(event, tag, in_mshr, !data && !mshr_->hasData(addr), Command::Inv);
                    tag->setState(SM_Inv);
                    break;
                }
            }
            break;
        case SM_Inv:
            if (!in_mshr) {
                status = allocateMSHR(event, true, 0);
                if (status == MemEventStatus::OK) {
                    mshr_->setProfiled(addr);
                    stat_event_state_[(int)Command::FetchInv][state]->addData(1);
                }
            }
            break;
        case S_Inv:
        case E_Inv:
        case M_Inv:
            if (mshr_->getFrontType(addr) == MSHREntryType::Evict || mshr_->getFrontEvent(addr)->getCmd() == Command::FlushLineInv) {
                status = allocateMSHR(event, true, 0);
                if (status == MemEventStatus::OK) {
                    mshr_->setProfiled(addr);
                    stat_event_state_[(int)Command::FetchInv][state]->addData(1);
                }
            } else if (!in_mshr) {
                status = allocateMSHR(event, true, 1);
            }
            break;
        case E_InvX:
        case M_InvX:
            if (!in_mshr)
                status = allocateMSHR(event, true, 1);
            break;
        default:
            debug_->fatal(CALL_INFO,-1,"%s, Error: Received FetchInv in unhandled state '%s'. Event: %s. Time = %" PRIu64 "ns\n",
                    getName().c_str(), StateString[state], event->getVerboseString().c_str(), getCurrentSimTimeNano());
    }

    if (mem_h_is_debug_addr(addr) && tag) {
        event_debuginfo_.new_state = tag->getState();
        event_debuginfo_.verbose_line = tag->getString();
        if (data)
            event_debuginfo_.verbose_line += "/" + data->getString();
    }

    if (status == MemEventStatus::Reject)
        sendNACK(event);

    return true;
}


bool MESISharNoninclusive::handleFetchInvX(MemEvent * event, bool in_mshr) {
    Addr addr = event->getBaseAddr();
    DirectoryLine * tag = dir_array_->lookup(addr, false);
    State state = tag ? tag->getState() : I;
    DataLine * data = (tag) ? data_array_->lookup(addr, false) : nullptr;
    if (data && data->getTag() != tag) data = nullptr;
    MemEventStatus status = MemEventStatus::OK;

    if (mem_h_is_debug_event(event))
        event_debuginfo_.prefill(event->getID(), Command::FetchInvX, "", addr, state);

    if (in_mshr)
        mshr_->removePendingRetry(addr);

    uint64_t send_time;

    MemEvent * req;

    switch (state) {
        case I_B:
            dir_array_->deallocate(tag);
            if (data) data_array_->deallocate(data);
        case I:
            if (!in_mshr || !mshr_->getProfiled(addr)) {
                stat_event_state_[(int)Command::FetchInvX][state]->addData(1);
            }
            if (in_mshr) {
                cleanUpAfterRequest(event, in_mshr);
            } else {
                delete event;
            }
            break;
        case E_B:
        case M_B:
            tag->setState(S_B);
            if (!in_mshr || !mshr_->getProfiled(addr)) {
                stat_event_state_[(int)Command::FetchInvX][state]->addData(1);
            }
            delete event;
            break;
        case E:
        case M:
            if ((tag->hasOwner()|| (!data && !mshr_->hasData(addr))) && !in_mshr)
                status = allocateMSHR(event, true, 0);
            if (status != MemEventStatus::OK)
                break;
            if (!in_mshr || !mshr_->getProfiled(addr)) {
                stat_event_state_[(int)Command::FetchInvX][state]->addData(1);
            }
            if (tag->hasOwner()) { // Get data from owner
                if (!applyPendingReplacement(addr)) {
                    send_time = sendFetch(Command::FetchInvX, event, tag->getOwner(), in_mshr, tag->getTimestamp());
                    tag->setTimestamp(send_time-1);
                }
                state == E ? tag->setState(E_InvX) : tag->setState(M_InvX);
                mshr_->setProfiled(addr);
            } else if (!data && !mshr_->hasData(addr)) {
                if (!applyPendingReplacement(addr)) {
                    send_time = sendFetch(Command::Fetch, event, *(tag->getSharers()->begin()), in_mshr, tag->getTimestamp());
                    tag->setTimestamp(send_time-1);
                }
                state == E ? tag->setState(E_D) : tag->setState(M_D);
                mshr_->setProfiled(addr);
            } else {
                tag->setState(S);
                if (data)
                    sendResponseDown(event, data->getData(), state == M, true); // TODO Double check that a downgrade counts as an evict
                else {
                    sendResponseDown(event, &(mshr_->getData(addr)), state == M, true);
                }
                cleanUpAfterRequest(event, in_mshr);
            }
            break;
        case EA:
        case MA:
            if (!in_mshr || mshr_->getProfiled(addr)) {
                stat_event_state_[(int)Command::FetchInvX][state]->addData(1);
            }
            req = static_cast<MemEvent*>(mshr_->getFrontEvent(addr));
            sendResponseDown(event, &(req->getPayload()), state == M, true); // TODO Double check that a downgrade counts as an evict
            // Clean up so that when we replay the replacement we get the right downgraded state
            req->setCmd(Command::PutS);
            tag->removeOwner();
            tag->addSharer(req->getSrc());
            tag->setState(SA);
            delete event;
            break;
        case E_InvX:
        case M_InvX:
        case E_D:
        case M_D:
        case E_Inv:
        case M_Inv:
            if (!in_mshr)
                status = allocateMSHR(event, true, 1);
            break;
        default:
            debug_->fatal(CALL_INFO,-1,"%s, Error: Received FetchInvX in unhandled state '%s'. Event: %s. Time = %" PRIu64 "ns\n",
                    getName().c_str(), StateString[state], event->getVerboseString().c_str(), getCurrentSimTimeNano());
    }

    if (mem_h_is_debug_addr(addr) && tag) {
        event_debuginfo_.new_state = tag->getState();
        event_debuginfo_.verbose_line = tag->getString();
        if (data)
            event_debuginfo_.verbose_line += "/" + data->getString();
    }

    if (status == MemEventStatus::Reject)
        sendNACK(event);

    return true;
}

bool MESISharNoninclusive::handleGetSResp(MemEvent * event, bool in_mshr) {
    Addr addr = event->getBaseAddr();
    DirectoryLine * tag = dir_array_->lookup(addr, false);
    State state = tag ? tag->getState() : I;
    DataLine * data = (tag) ? data_array_->lookup(addr, false) : nullptr;
    if (data && data->getTag() != tag) data = nullptr;

    // Find matching request in MSHR
    MemEvent * req = static_cast<MemEvent*>(mshr_->getFrontEvent(addr));

    bool local_prefetch = req->isPrefetch() && (req->getRqstr() == cachename_);
    req->setFlags(event->getMemFlags());

    if (mem_h_is_debug_event(event))
        event_debuginfo_.prefill(event->getID(), Command::GetSResp, (local_prefetch ? "-pref" : ""), addr, state);

    stat_event_state_[(int)Command::GetSResp][state]->addData(1);

    tag->setState(S);
    if (data) {
        data->setData(event->getPayload(), 0);
        if (mem_h_is_debug_addr(addr))
            printDataValue(addr, &(event->getPayload()), true);
    }

    if (local_prefetch) {
        tag->setPrefetch(true);
        if (mem_h_is_debug_event(event))
            event_debuginfo_.action = "Done";
    } else {
        tag->addSharer(req->getSrc());
        uint64_t send_time = sendResponseUp(req, &(event->getPayload()), true, tag->getTimestamp(), Command::GetSResp);
        tag->setTimestamp(send_time-1);
    }

    cleanUpAfterResponse(event, in_mshr);

    if (mem_h_is_debug_addr(addr) && tag) {
        event_debuginfo_.new_state = tag->getState();
        event_debuginfo_.verbose_line = tag->getString();
        if (data)
            event_debuginfo_.verbose_line += "/" + data->getString();
    }

    return true;
}


bool MESISharNoninclusive::handleGetXResp(MemEvent * event, bool in_mshr) {
    Addr addr = event->getBaseAddr();
    DirectoryLine * tag = dir_array_->lookup(addr, false);
    State state = tag ? tag->getState() : I;
    DataLine * data = (tag) ? data_array_->lookup(addr, false) : nullptr;
    if (data && data->getTag() != tag) data = nullptr;

    // Get matching request
    MemEvent * req = static_cast<MemEvent*>(mshr_->getFrontEvent(event->getBaseAddr()));

    bool local_prefetch = req->isPrefetch() && (req->getRqstr() == cachename_);
    req->setFlags(event->getMemFlags());

    if (mem_h_is_debug_event(event))
        event_debuginfo_.prefill(event->getID(), Command::GetXResp, (local_prefetch ? "-pref" : ""), addr, state);

    stat_event_state_[(int)Command::GetXResp][state]->addData(1);

    switch (state) {
        case IS:
        {
            // Update line if we have it locally
            if (data) {
                data->setData(event->getPayload(), 0);
                if (mem_h_is_debug_addr(addr))
                    printDataValue(addr, &(event->getPayload()), true);
            }
            // Update state
            if (event->getDirty())
                tag->setState(M);
            else
                tag->setState(protocol_state_); // E for MESI, S for MSI

            if (local_prefetch) {
                tag->setPrefetch(true);
                if (mem_h_is_debug_event(event))
                    event_debuginfo_.action = "Done";
            } else {
                if (tag->getState() == S || !protocol_ || mshr_->getSize(addr) > 1) {
                    tag->addSharer(req->getSrc());
                    uint64_t send_time = sendResponseUp(req, &(event->getPayload()), true, tag->getTimestamp(), Command::GetSResp);
                    tag->setTimestamp(send_time - 1);
                } else {
                    tag->setOwner(req->getSrc());
                    uint64_t send_time = sendResponseUp(req, &(event->getPayload()), true, tag->getTimestamp(), Command::GetXResp);
                    tag->setTimestamp(send_time - 1);
                }
            }

            cleanUpAfterResponse(event, in_mshr);
            break;
        }
        case IM:
            if (data) {
                data->setData(event->getPayload(), 0);
                if (mem_h_is_debug_addr(addr))
                    printDataValue(addr, &(event->getPayload()), true);
            } // fall-thru
        case SM:
        {
            tag->setState(M);
            tag->setOwner(req->getSrc());
            uint64_t send_time = 0;

            if (tag->isSharer(req->getSrc())) {
                tag->removeSharer(req->getSrc());
                send_time = sendResponseUp(req, nullptr, true, tag->getTimestamp(), Command::GetXResp);
            } else {
                // Data could come from an allocated data line, mshr (prior Fetch for ex.) or this event
                MemEventBase::dataVec* value = data ? data->getData() : (event->getPayloadSize() != 0 ? &(event->getPayload()) : &(mshr_->getData(addr)));
                send_time = sendResponseUp(req, value, true, tag->getTimestamp(), Command::GetXResp);
            }
            tag->setTimestamp(send_time - 1);

            if (mshr_->hasData(addr))
                mshr_->clearData(addr);

            cleanUpAfterResponse(event, in_mshr);
            break;
        }
        case SM_Inv:
            tag->setState(M_Inv);
            mshr_->setInProgress(addr, false);
            if (event->getPayloadSize() != 0) {
                if (data) {
                    data->setData(event->getPayload(), 0);
                } else {
                    mshr_->setData(addr, event->getPayload());
                }
                if (mem_h_is_debug_addr(addr))
                    printDataValue(addr, &(event->getPayload()), true);
            }
            if (mem_h_is_debug_event(event)) {
                event_debuginfo_.action = "Stall";
                event_debuginfo_.reason = "Acks needed";
            }
            cleanUpEvent(event, in_mshr);
            break;
        default:
            debug_->fatal(CALL_INFO,-1,"%s, Error: Received GetXResp in unhandled state '%s'. Event: %s. Time = %" PRIu64 "ns\n",
                    getName().c_str(), StateString[state], event->getVerboseString().c_str(), getCurrentSimTimeNano());
    }

    if (mem_h_is_debug_addr(addr) && tag) {
        event_debuginfo_.new_state = tag->getState();
        event_debuginfo_.verbose_line = tag->getString();
        if (data)
            event_debuginfo_.verbose_line += "/" + data->getString();
    }
    return true;
}


bool MESISharNoninclusive::handleFlushLineResp(MemEvent * event, bool in_mshr) {
    Addr addr = event->getBaseAddr();
    DirectoryLine * tag = dir_array_->lookup(addr, false);
    State state = tag ? tag->getState() : I;
    DataLine * data = (tag) ? data_array_->lookup(addr, false) : nullptr;
    if (data && data->getTag() != tag) data = nullptr;

    if (mem_h_is_debug_event(event))
        event_debuginfo_.prefill(event->getID(), Command::FlushLineResp, "", addr, state);

    stat_event_state_[(int)Command::FlushLineResp][state]->addData(1);

    MemEvent * req = static_cast<MemEvent*>(mshr_->getFrontEvent(event->getBaseAddr()));

    switch (state) {
        case I:
            break;
        case I_B:
            dir_array_->deallocate(tag);
            if (data)  {
                data_array_->deallocate(data);
            }
            break;
        case S_B:
        case E_B:
        case M_B:
            tag->setState(S);
            break;
        default:
            debug_->fatal(CALL_INFO,-1,"%s, Error: Received FlushLineResp in unhandled state '%s'. Event: %s. Time = %" PRIu64 "ns\n",
                    getName().c_str(), StateString[state], event->getVerboseString().c_str(), getCurrentSimTimeNano());
    }

    sendResponseUp(req, nullptr, true, timestamp_, Command::FlushLineResp, event->success());

    if (mem_h_is_debug_addr(addr) && tag) {
        event_debuginfo_.new_state = tag->getState();
        event_debuginfo_.verbose_line = tag->getString();
        if (data)
            event_debuginfo_.verbose_line += "/" + data->getString();
    }
    cleanUpAfterResponse(event, in_mshr);
    return true;
}


bool MESISharNoninclusive::handleFlushAllResp(MemEvent * event, bool in_mshr) {
    event_debuginfo_.prefill(event->getID(), Command::FlushAllResp, "", 0, State::NP);

    MemEvent* flush_request = static_cast<MemEvent*>(mshr_->getFlush());
    mshr_->removeFlush(); // Remove FlushAll

    event_debuginfo_.action = "Respond";

    sendResponseUp(flush_request, nullptr, true, timestamp_);

    delete flush_request;
    delete event;

    if (mshr_->getFlush() != nullptr) {
        retry_buffer_.push_back(mshr_->getFlush());
    }

    return true;
}


bool MESISharNoninclusive::handleFetchResp(MemEvent * event, bool in_mshr) {
    Addr addr = event->getBaseAddr();
    DirectoryLine * tag = dir_array_->lookup(addr, false);
    State state = tag ? tag->getState() : I;
    DataLine * data = (tag) ? data_array_->lookup(addr, false) : nullptr;
    if (data && data->getTag() != tag) data = nullptr;

    if (mem_h_is_debug_event(event))
        event_debuginfo_.prefill(event->getID(), Command::FetchResp, "", addr, state);

    // Check acks needed
    bool done = mshr_->decrementAcksNeeded(addr);

    // Remove response from expected response list & extract payload
    responses_.find(addr)->second.erase(event->getSrc());
    if (responses_.find(addr)->second.empty())
        responses_.erase(addr);

    if (data)
        data->setData(event->getPayload(), 0);
    else
        mshr_->setData(addr, event->getPayload(), event->getDirty());

    if (mem_h_is_debug_addr(addr))
        printDataValue(addr, &(event->getPayload()), true);

    stat_event_state_[(int)Command::FetchResp][state]->addData(1);

    switch (state) {
        case S_D:
        case M_D:
        case SM_D:
        case SB_D:
            tag->setState(NextState[state]); // S or E or M or SM or S_B
            retry(addr);
            break;
        case E_D:
            event->getDirty() ? tag->setState(M) : tag->setState(E);
            retry(addr);
            break;
        case S_Inv:
        case SB_Inv:
            tag->removeSharer(event->getSrc());
            if (done) {
                tag->setState(S);
                retry(addr);
            }
            break;
        case SM_Inv:
            tag->removeSharer(event->getSrc());
            if (done) {
                tag->setState(SM);
                if (!mshr_->getInProgress(addr))
                    retry(addr);
            }
            break;
        case E_InvX:
        case M_InvX:
            tag->removeOwner();
            tag->addSharer(event->getSrc());
            event->getDirty() ? tag->setState(M) : tag->setState(NextState[state]); // E or M
            retry(addr);
            break;
        case E_Inv:
        case M_Inv:
            if (tag->hasOwner())
                tag->removeOwner();
            else
                tag->removeSharer(event->getSrc());
            if (done) {
                event->getDirty() ? tag->setState(M) : tag->setState(NextState[state]);    // E or M
                retry(addr);
            }
            break;
        default:
            debug_->fatal(CALL_INFO,-1,"%s, Error: Received FetchResp in unhandled state '%s'. Event: %s. Time = %" PRIu64 "ns\n",
                    getName().c_str(), StateString[state], event->getVerboseString().c_str(), getCurrentSimTimeNano());
    }

    if (mem_h_is_debug_event(event)) {
        event_debuginfo_.action = done ? "Retry" : "Stall";
        if (tag) {
            event_debuginfo_.new_state = tag->getState();
            event_debuginfo_.verbose_line = tag->getString();
            if (data)
                event_debuginfo_.verbose_line += "/" + data->getString();
        }
    }

    delete event;

    return true;
}


bool MESISharNoninclusive::handleFetchXResp(MemEvent * event, bool in_mshr) {
    Addr addr = event->getBaseAddr();
    DirectoryLine * tag = dir_array_->lookup(addr, false);
    State state = tag ? tag->getState() : I;
    DataLine * data = (tag) ? data_array_->lookup(addr, false) : nullptr;
    if (data && data->getTag() != tag) data = nullptr;

    if (mem_h_is_debug_event(event)) {
        event_debuginfo_.prefill(event->getID(), Command::FetchXResp, "", addr, state);
        event_debuginfo_.action = "Retry";
    }

    stat_event_state_[(int)Command::FetchXResp][state]->addData(1);

    mshr_->decrementAcksNeeded(addr);

    // Clear expected responses
    responses_.find(addr)->second.erase(event->getSrc());
    if (responses_.find(addr)->second.empty()) responses_.erase(addr);

    // Update coherence state
    tag->removeOwner();
    tag->addSharer(event->getSrc());

    if (state == M_InvX || event->getDirty())
        tag->setState(M);
    else
        tag->setState(E);

    // Save data
    if (data)
        data->setData(event->getPayload(), 0);
    else
        mshr_->setData(addr, event->getPayload(), event->getDirty());

    if (mem_h_is_debug_addr(addr))
        printDataValue(addr, &(event->getPayload()), true);

    // Clean up and retry
    retry(addr);
    delete event;

    if (mem_h_is_debug_addr(addr) && tag) {
        event_debuginfo_.new_state = tag->getState();
        event_debuginfo_.verbose_line = tag->getString();
        if (data)
            event_debuginfo_.verbose_line += "/" + data->getString();
    }
    return true;
}


bool MESISharNoninclusive::handleAckFlush(MemEvent* event, bool in_mshr) {
    event_debuginfo_.prefill(event->getID(), Command::AckFlush, "", 0, State::NP);

    mshr_->decrementFlushCount();
    if (mshr_->getFlushCount() == 0) {
        retry_buffer_.push_back(mshr_->getFlush());
    }

    delete event;
    return true;
}


bool MESISharNoninclusive::handleUnblockFlush(MemEvent * event, bool in_mshr) {
    event_debuginfo_.prefill(event->getID(), Command::UnblockFlush, "", 0, State::NP);

    if (flush_helper_) {
        broadcastMemEventToSources(Command::UnblockFlush, event, timestamp_ + 1);
    }
    delete event;

    return true;
}


bool MESISharNoninclusive::handleAckInv(MemEvent * event, bool in_mshr) {
    Addr addr = event->getAddr();
    DirectoryLine * tag = dir_array_->lookup(addr, false);
    State state = tag ? tag->getState() : I;
    DataLine * data = (tag) ? data_array_->lookup(addr, false) : nullptr;
    if (data && data->getTag() != tag) data = nullptr;
    if (mem_h_is_debug_event(event))
        event_debuginfo_.prefill(event->getID(), Command::AckInv, "", addr, state);

    stat_event_state_[(int)Command::AckInv][state]->addData(1);

    if (tag->isSharer(event->getSrc()))
        tag->removeSharer(event->getSrc());
    else
        tag->removeOwner();

    responses_.find(addr)->second.erase(event->getSrc());
    if (responses_.find(addr)->second.empty()) responses_.erase(addr);

    bool done = mshr_->decrementAcksNeeded(addr);

    if (done) {
        tag->setState(NextState[state]);
        retry(addr);
    }

    if (mem_h_is_debug_addr(addr)) {
        event_debuginfo_.action = done ? "Retry" : "DecAcks";
        if (tag) {
            event_debuginfo_.new_state = tag->getState();
            event_debuginfo_.verbose_line = tag->getString();
            if (data)
                event_debuginfo_.verbose_line += "/" + data->getString();
        }
    }

    delete event;
    return true;
}


bool MESISharNoninclusive::handleAckPut(MemEvent * event, bool in_mshr) {
    DirectoryLine * tag = dir_array_->lookup(event->getBaseAddr(), false);
    State state = tag ? tag->getState() : I;
    stat_event_state_[(int)Command::AckPut][state]->addData(1);
    if (mem_h_is_debug_event(event)) {
        event_debuginfo_.prefill(event->getID(), Command::AckPut, "", event->getBaseAddr(), state);
        event_debuginfo_.action = "Done";
        if (tag)
            event_debuginfo_.verbose_line = tag->getString();
    }

    cleanUpAfterResponse(event, in_mshr);
    return true;
}


/* We're using NULLCMD to signal an internally generated event - in this case an eviction */
bool MESISharNoninclusive::handleNULLCMD(MemEvent* event, bool in_mshr) {
    Addr oldAddr = event->getAddr();
    Addr newAddr = event->getBaseAddr();

    // Is this a data eviction or directory eviction?
    bool dirEvict = oldAddr != newAddr ? eviction_type_.find(std::make_pair(oldAddr,newAddr))->second : false;
    bool evicted;

    // Tag/directory array eviction
    if (dirEvict) {
        DirectoryLine * tag = dir_array_->lookup(oldAddr, false);

        evicted = handleDirEviction(newAddr, tag);

        if (mem_h_is_debug_addr(newAddr) || mem_h_is_debug_addr(evict_debuginfo_.addr)) {
            event_debuginfo_.prefill(event->getID(), Command::NULLCMD, "", evict_debuginfo_.addr, evict_debuginfo_.old_state);
            event_debuginfo_.new_state = tag->getState();
            event_debuginfo_.verbose_line = tag->getString();
        }

        if (evicted) {
            notifyListenerOfEvict(tag->getAddr(), line_size_, event->getInstructionPointer());
            retry_buffer_.push_back(mshr_->getFrontEvent(newAddr));
            mshr_->addPendingRetry(newAddr);
            if (mshr_->removeEvictPointer(oldAddr, newAddr))
                retry(oldAddr);
            eviction_type_.erase(std::make_pair(oldAddr, newAddr));
            if (mem_h_is_debug_event(event))
                event_debuginfo_.action = "Retry";
        } else {
            if (mem_h_is_debug_event(event)) {
                event_debuginfo_.action = "Stall";
                event_debuginfo_.reason = "Dir evict failed";
            }
            if (oldAddr != tag->getAddr()) { // Now evicting a different address than we were before
                if (mem_h_is_debug_event(event)) {
                    std::stringstream reason;
                    reason << "New dir evict targ: 0x" << std::hex << tag->getAddr();
                    event_debuginfo_.reason = reason.str();
                }
                if (mshr_->removeEvictPointer(oldAddr, newAddr))
                    retry(oldAddr);
                eviction_type_.erase(std::make_pair(oldAddr, newAddr));
                std::pair<Addr,Addr> evictpair = std::make_pair(tag->getAddr(),newAddr);
                if (eviction_type_.find(evictpair) == eviction_type_.end()) {
                    mshr_->insertEviction(tag->getAddr(), newAddr);
                    eviction_type_.insert(std::make_pair(evictpair, true));
                } else {
                    eviction_type_[evictpair] = true;
                }
            }
        }
    } else {
    // Data array eviction
    // Races can mean that this eviction is no longer necessary but we can't tell if we haven't started the eviction
    // or if we're finishing. So finish it, but only replace if we currently need an eviction
    // (the event waiting for eviction now and the one that triggered this eviction might not be the same)
        DirectoryLine * tag = dir_array_->lookup(newAddr, false);
        DataLine * data = data_array_->lookup(oldAddr, false);
        evicted = handleDataEviction(newAddr, data);

        if (mem_h_is_debug_addr(newAddr) || mem_h_is_debug_addr(evict_debuginfo_.addr)) {
            event_debuginfo_.prefill(event->getID(), Command::NULLCMD, "", evict_debuginfo_.addr, evict_debuginfo_.old_state);
            event_debuginfo_.new_state = data ? data->getState() : I;
            event_debuginfo_.verbose_line = data ? data->getString() : "";
        }
        if (evicted) {
            if (tag && (tag->getState() == IA || tag->getState() == SA || tag->getState() == EA || tag->getState() == MA)) {
                data_array_->replace(newAddr, data);
                if (mem_h_is_debug_addr(newAddr))
                    printDebugAlloc(true, newAddr, "Data");

                data->setTag(tag);
                if (tag->getState() == IA)
                    tag->setState(I);
                else if (tag->getState() == SA)
                    tag->setState(S);
                else if (tag->getState() == EA)
                    tag->setState(E);
                else if (tag->getState() == MA)
                    tag->setState(M);
                retry_buffer_.push_back(mshr_->getFrontEvent(newAddr));
                mshr_->addPendingRetry(newAddr);
            } else {
                data_array_->deallocate(data);
            }
            if (oldAddr != newAddr) {
                if (mshr_->removeEvictPointer(oldAddr, newAddr))
                    retry(oldAddr);
                eviction_type_.erase(std::make_pair(oldAddr, newAddr));
                if (mem_h_is_debug_event(event)) {
                    event_debuginfo_.action = "Retry";
                    event_debuginfo_.new_state = tag->getState();
                }
            } else {
                mshr_->decrementFlushCount();
                if (mshr_->getFlushCount() == 0) {
                    retry_buffer_.push_back(mshr_->getFlush());
                    event_debuginfo_.action = "Retry";
                }
            }

        } else {
            if (mem_h_is_debug_event(event)) {
                event_debuginfo_.action = "Stall";
                event_debuginfo_.reason = "Data evict failed";
            }
            if (oldAddr != data->getAddr()) { // Now evicting a different address than we were before
                if (mem_h_is_debug_event(event) || mem_h_is_debug_addr(data->getAddr())) {
                    std::stringstream reason;
                    reason << "New data evict targ: 0x" << std::hex << data->getAddr();
                    event_debuginfo_.reason = reason.str();
                }

                if (mshr_->removeEvictPointer(oldAddr, newAddr))
                    retry(oldAddr);
                eviction_type_.erase(std::make_pair(oldAddr, newAddr));
                std::pair<Addr,Addr> evictpair = std::make_pair(data->getAddr(), newAddr);
                if (eviction_type_.find(evictpair) == eviction_type_.end()) {
                    mshr_->insertEviction(data->getAddr(), newAddr);
                    eviction_type_.insert(std::make_pair(evictpair, false));
                } else {
                    eviction_type_[evictpair] = false;
                }
            }
        }
    }

    delete event;
    return true;
}


bool MESISharNoninclusive::handleNACK(MemEvent* event, bool in_mshr) {
    MemEvent * nackedEvent = event->getNACKedEvent();
    Command cmd = nackedEvent->getCmd();
    Addr addr = nackedEvent->getBaseAddr();

    if (mem_h_is_debug_event(event) || mem_h_is_debug_event(nackedEvent)) {
        DirectoryLine * tag = dir_array_->lookup(addr, false);
        event_debuginfo_.prefill(event->getID(), Command::NACK, "", addr, tag ? tag->getState() : I);
    }

    delete event;

    switch (cmd) {
        case Command::GetS:
        case Command::GetX:
        case Command::GetSX:
        case Command::FlushLine:
        case Command::FlushLineInv:
        case Command::PutS:
        case Command::PutE:
        case Command::PutM:
        case Command::FlushAll:
            resendEvent(nackedEvent, false); // Resend towards memory
            break;
        case Command::FetchInv:
        case Command::FetchInvX:
        case Command::Fetch:
        case Command::Inv:
        case Command::ForceInv:
            if (mem_h_is_debug_addr(addr)) {
            }
            if (responses_.find(addr) != responses_.end()
                    && responses_.find(addr)->second.find(nackedEvent->getDst()) != responses_.find(addr)->second.end()
                    && responses_.find(addr)->second.find(nackedEvent->getDst())->second == nackedEvent->getID()) {

                resendEvent(nackedEvent, true); // Resend towards CPU
            } else {
                if (mem_h_is_debug_event(nackedEvent))
                    event_debuginfo_.action = "Drop";
                delete nackedEvent;
            }
            break;
        case Command::ForwardFlush:
            resendEvent(nackedEvent, true); // Resend towards CPU
            break;
        default:
            debug_->fatal(CALL_INFO,-1,"%s, Error: Received NACK with unhandled command type. Event: %s. Time = %" PRIu64 "ns\n",
                    getName().c_str(), nackedEvent->getVerboseString().c_str(), getCurrentSimTimeNano());
    }
    return true;
}

/***********************************************************************************************************
    * MSHR & CacheArray management
    ***********************************************************************************************************/

/**
    * Handle a cache miss
    */
MemEventStatus MESISharNoninclusive::processDirectoryMiss(MemEvent * event, DirectoryLine * tag, bool in_mshr) {
    MemEventStatus status = in_mshr ? MemEventStatus::OK : allocateMSHR(event, false); // Miss means we need an MSHR entry
    if (in_mshr && mshr_->getFrontEvent(event->getBaseAddr()) != event) {
        if (mem_h_is_debug_event(event))
            event_debuginfo_.action = "Stall";
        return MemEventStatus::Stall;
    }
    if (status == MemEventStatus::OK && !tag) { // Need a cache line too
        tag = allocateDirLine(event, tag);
        status = tag ? MemEventStatus::OK : MemEventStatus::Stall;
    }
    return status;
}

MemEventStatus MESISharNoninclusive::processDataMiss(MemEvent * event, DirectoryLine * tag, DataLine * data, bool in_mshr) {
    // Evict a data line, if that requires evicting a dirline, evict a dirline too
    bool evicted = handleDataEviction(event->getBaseAddr(), data);

    if (evicted) {
        if (mem_h_is_debug_event(event))
            printDebugAlloc(true, event->getBaseAddr(), "Data");
        data_array_->replace(event->getBaseAddr(), data);
        data->setTag(tag);
        return MemEventStatus::OK;
    } else {
        if (mem_h_is_debug_event(event)) {
            event_debuginfo_.action = "Stall";
            std::stringstream reason;
            reason << "evict data 0x" << std::hex << tag->getAddr();
            event_debuginfo_.reason = reason.str();
        }
        std::pair<Addr,Addr> evictpair = std::make_pair(data->getAddr(), event->getBaseAddr());
        if (eviction_type_.find(evictpair) == eviction_type_.end()) {
            mshr_->insertEviction(data->getAddr(), event->getBaseAddr());
            eviction_type_.insert(std::make_pair(evictpair, false));
        } else {
            eviction_type_[evictpair] = false;
        }
        return MemEventStatus::Stall;
    }
}


/**
    * Allocate a new directory line
    */
DirectoryLine* MESISharNoninclusive::allocateDirLine(MemEvent * event, DirectoryLine * tag) {
    bool evicted = handleDirEviction(event->getBaseAddr(), tag);
    if (evicted) {
        notifyListenerOfEvict(tag->getAddr(), line_size_, event->getInstructionPointer());
        dir_array_->replace(event->getBaseAddr(), tag);
        if (mem_h_is_debug_event(event))
            printDebugAlloc(true, event->getBaseAddr(), "Dir");
        return tag;
    } else {
        if (mem_h_is_debug_event(event)) {
            event_debuginfo_.action = "Stall";
            stringstream reason;
            reason << "evict dir 0x" << std::hex << tag->getAddr();
            event_debuginfo_.reason = reason.str();
        }

        std::pair<Addr,Addr> evictpair = std::make_pair(tag->getAddr(), event->getBaseAddr());
        if (eviction_type_.find(evictpair) == eviction_type_.end()) {
            mshr_->insertEviction(tag->getAddr(), event->getBaseAddr());
            eviction_type_.insert(std::make_pair(evictpair, true));
        } else {
            eviction_type_[evictpair] = true;
        }
        return nullptr;
    }
}

bool MESISharNoninclusive::handleDirEviction(Addr addr, DirectoryLine*& tag) {
    if (!tag)
        tag = dir_array_->findReplacementCandidate(addr);

    DataLine* data = data_array_->lookup(tag->getAddr(), false);
    if (data && data->getTag() != tag) data = nullptr; // Dataline not valid

    State state = tag->getState();

    if (mem_h_is_debug_addr(tag->getAddr())) {
        evict_debuginfo_.old_state = tag->getState();
        evict_debuginfo_.addr = tag->getAddr();
    }

    stat_evict_[state]->addData(1);

    bool evict = false;
    bool wbSent = false;
    switch (state) {
        case I:
            return true;
        case S:
            if (!mshr_->getPendingRetries(tag->getAddr())) {
                if (invalidateAll(nullptr, tag, false, data != nullptr ? false : true)) {
                    evict = false;
                    tag->setState(S_Inv);
                    if (mem_h_is_debug_addr(tag->getAddr()))
                        printDebugAlloc(false, tag->getAddr(), "Dir, InProg, S_Inv");
                    break;
                } else if (!silent_evict_clean_) {
                    if (data) {
                        sendWritebackFromCache(Command::PutS, tag, data, false);
                    } else {
                        sendWritebackFromMSHR(Command::PutS, tag, false);
                    }
                    wbSent = true;
                    if (mem_h_is_debug_addr(tag->getAddr()))
                        printDebugAlloc(false, tag->getAddr(), "Dir, Writeback");
                } else if (mem_h_is_debug_addr(tag->getAddr()))
                    printDebugAlloc(false, tag->getAddr(), "Dir, Drop");
                if (data) {
                    if (mem_h_is_debug_addr(data->getAddr()))
                        printDebugAlloc(false, data->getAddr(), "Data");
                    data_array_->deallocate(data);
                }
                tag->setState(I);
                evict = true;
                break;
            }
        case E:
            if (!mshr_->getPendingRetries(tag->getAddr())) {
                if (invalidateAll(nullptr, tag, false, data != nullptr ? false : true)) {
                    evict = false;
                    tag->setState(E_Inv);
                    if (mem_h_is_debug_addr(tag->getAddr()))
                        printDebugAlloc(false, tag->getAddr(), "Dir, InProg, E_Inv");
                    break;
                } else if (!silent_evict_clean_) {
                    if (data) {
                        sendWritebackFromCache(Command::PutE, tag, data, false);
                    } else {
                        sendWritebackFromMSHR(Command::PutE, tag, false);
                    }
                    wbSent = true;
                    if (mem_h_is_debug_addr(tag->getAddr()))
                        printDebugAlloc(false, tag->getAddr(), "Dir, Writeback");
                } else if (mem_h_is_debug_addr(tag->getAddr()))
                    printDebugAlloc(false, tag->getAddr(), "Dir, Drop");
                tag->setState(I);

                if (data) {
                    if (mem_h_is_debug_addr(data->getAddr()))
                        printDebugAlloc(false, data->getAddr(), "Data");
                    data_array_->deallocate(data);
                }
                evict = true;
                break;
            }
        case M:
            if (!mshr_->getPendingRetries(tag->getAddr())) {
                if (invalidateAll(nullptr, tag, false, data != nullptr ? false : true)) {
                    evict = false;
                    tag->setState(M_Inv);
                    if (mem_h_is_debug_addr(tag->getAddr()))
                        printDebugAlloc(false, tag->getAddr(), "Dir, InProg, M_Inv");
                } else {
                    if (mem_h_is_debug_addr(tag->getAddr()))
                        printDebugAlloc(false, tag->getAddr(), "Dir, Writeback");
                    if (data) {
                        sendWritebackFromCache(Command::PutM, tag, data, true);
                    if (mem_h_is_debug_addr(data->getAddr()))
                        printDebugAlloc(false, data->getAddr(), "Data");
                        data_array_->deallocate(data);
                    } else {
                        sendWritebackFromMSHR(Command::PutM, tag, true);
                    }
                    wbSent = true;
                    tag->setState(I);
                    evict = true;
                }
                break;
            }
        default:
            if (mem_h_is_debug_addr(tag->getAddr())) {
                std::stringstream note;
                note << "InProg, " << StateString[state];
                printDebugAlloc(false, tag->getAddr(), note.str());
            }
            return false;
    }

    if (wbSent && recv_writeback_ack_) {
        mshr_->insertWriteback(tag->getAddr(), false);
    }

    recordPrefetchResult(tag, stat_prefetch_evict_);
    return evict;
}

bool MESISharNoninclusive::handleDataEviction(Addr addr, DataLine *&data) {
    bool evict = false;

    if (!data)
        data = data_array_->findReplacementCandidate(addr);
    State state = data->getState();

    if (mem_h_is_debug_addr(data->getAddr())) {
        evict_debuginfo_.old_state = data->getState();
        evict_debuginfo_.addr = data->getAddr();
    }

    DirectoryLine* tag;

    switch (state) {
        case I:
            return true;
        case S:
            if (!mshr_->getPendingRetries(data->getAddr())) {
                tag = data->getTag();
                if (!(tag->hasSharers())) {
                    if (mem_h_is_debug_addr(data->getAddr())) {
                        printDebugAlloc(false, tag->getAddr(), "Dir, Writeback");
                        printDebugAlloc(false, data->getAddr(), "Data");
                    }
                    sendWritebackFromCache(Command::PutS, tag, data, false);
                    if (recv_writeback_ack_)
                        mshr_->insertWriteback(tag->getAddr(), false);
                    recordPrefetchResult(tag, stat_prefetch_evict_);
                    notifyListenerOfEvict(data->getAddr(), line_size_, 0);
                    tag->setState(I);
                    dir_array_->deallocate(tag);
                } else if (mem_h_is_debug_addr(data->getAddr())) {
                    printDebugAlloc(false, data->getAddr(), "Data, Drop");
                }
                data_array_->deallocate(data);
                return true;
            }
        case E:
            if (!mshr_->getPendingRetries(data->getAddr())) {
                tag = data->getTag();
                if (!(tag->hasOwner() || tag->hasSharers())) {
                    if (mem_h_is_debug_addr(data->getAddr())) {
                        printDebugAlloc(false, tag->getAddr(), "Dir, Writeback");
                        printDebugAlloc(false, data->getAddr(), "Data");
                    }
                    sendWritebackFromCache(Command::PutE, tag, data, false);
                    if (recv_writeback_ack_)
                        mshr_->insertWriteback(tag->getAddr(), false);
                    recordPrefetchResult(tag, stat_prefetch_evict_);
                    notifyListenerOfEvict(data->getAddr(), line_size_, 0);
                    tag->setState(I);
                    dir_array_->deallocate(tag);
                } else if (mem_h_is_debug_addr(data->getAddr())) {
                    printDebugAlloc(false, data->getAddr(), "Data, Drop");
                }
                data_array_->deallocate(data);

                return true;
            }
        case M:
            if (!mshr_->getPendingRetries(data->getAddr())) {
                tag = data->getTag();
                if (!(tag->hasOwner() || tag->hasSharers())) {
                    if (mem_h_is_debug_addr(data->getAddr())) {
                        printDebugAlloc(false, tag->getAddr(), "Dir, Writeback");
                        printDebugAlloc(false, data->getAddr(), "Data");
                    }
                    sendWritebackFromCache(Command::PutM, tag, data, true);
                    if (recv_writeback_ack_)
                        mshr_->insertWriteback(tag->getAddr(), false);
                    recordPrefetchResult(tag, stat_prefetch_evict_);
                    notifyListenerOfEvict(data->getAddr(), line_size_, 0);
                    tag->setState(I);
                    dir_array_->deallocate(tag);
                } else if (mem_h_is_debug_addr(data->getAddr())) {
                    printDebugAlloc(false, data->getAddr(), "Data, Drop");
                }
                data_array_->deallocate(data);
                return true;
            }
        default:
            if (mem_h_is_debug_addr(data->getAddr())) {
                std::stringstream reason;
                reason << "InProg, " << StateString[state];
                printDebugAlloc(false, data->getAddr(), reason.str());
            }
            return false;
    }

    return false;
}


void MESISharNoninclusive::cleanUpEvent(MemEvent* event, bool in_mshr) {
    Addr addr = event->getBaseAddr();
    Command cmd = event->getCmd();

    /* Remove from MSHR */
    if (in_mshr) {
        if (event->isPrefetch() && event->getRqstr() == cachename_) outstanding_prefetch_count_--;
        mshr_->removeFront(addr);

        if (flush_state_ == FlushState::Drain) {
            mshr_->decrementFlushCount();
            if (mshr_->getFlushCount() == 0) {
                retry_buffer_.push_back(mshr_->getFlush());
            }
        }
    }

    delete event;
}


void MESISharNoninclusive::cleanUpAfterRequest(MemEvent* event, bool in_mshr) {
    Addr addr = event->getBaseAddr();
    Command cmd = event->getCmd();

    cleanUpEvent(event, in_mshr);

    /* Replay any waiting events */
    if (mshr_->exists(addr)) {
        if (mshr_->getFrontType(addr) == MSHREntryType::Event) {
            if (!mshr_->getInProgress(addr) && mshr_->getAcksNeeded(addr) == 0) {
                retry_buffer_.push_back(mshr_->getFrontEvent(addr));
                mshr_->addPendingRetry(addr);
            }
        } else { // Pointer -> either we're waiting for a writeback ACK or another address is waiting for this one
            if (mshr_->getFrontType(addr) == MSHREntryType::Evict && mshr_->getAcksNeeded(addr) == 0) {
                std::list<Addr>* evictPointers = mshr_->getEvictPointers(addr);
                for (std::list<Addr>::iterator it = evictPointers->begin(); it != evictPointers->end(); it++) {
                    MemEvent * ev = new MemEvent(cachename_, addr, *it, Command::NULLCMD);
                    retry_buffer_.push_back(ev);
                }
            }
        }
    }
}


void MESISharNoninclusive::cleanUpAfterResponse(MemEvent* event, bool in_mshr) {
    Addr addr = event->getBaseAddr();

    /* Clean up MSHR */
    MemEvent * req = nullptr;
    if (mshr_->getFrontType(addr) == MSHREntryType::Event) {
        req = static_cast<MemEvent*>(mshr_->getFrontEvent(addr));
    }

    mshr_->removeFront(addr);
    delete event;

    if (req) {
        if (req->isPrefetch() && req->getRqstr() == cachename_) outstanding_prefetch_count_--;
        delete req;
    }

    if (mshr_->exists(addr)) {
        if (mshr_->getFrontType(addr) == MSHREntryType::Event) {
            if (!mshr_->getInProgress(addr) && mshr_->getAcksNeeded(addr) == 0) {
                retry_buffer_.push_back(mshr_->getFrontEvent(addr));
                mshr_->addPendingRetry(addr);
            }
        } else {
            if (mshr_->getAcksNeeded(addr) == 0) {
                std::list<Addr>* evictPointers = mshr_->getEvictPointers(addr);
                for (std::list<Addr>::iterator it = evictPointers->begin(); it != evictPointers->end(); it++) {
                    MemEvent * ev = new MemEvent(cachename_, addr, *it, Command::NULLCMD);
                    retry_buffer_.push_back(ev);
                }
            }
        }
    }
}


void MESISharNoninclusive::retry(Addr addr) {
    if (mshr_->exists(addr)) {
        if (mshr_->getFrontType(addr) == MSHREntryType::Event) {
            retry_buffer_.push_back(mshr_->getFrontEvent(addr));
            mshr_->addPendingRetry(addr);
            if (mem_h_is_debug_addr(addr)) {
                if (event_debuginfo_.reason != "")
                    event_debuginfo_.reason = event_debuginfo_.reason + ",retry";
                else
                    event_debuginfo_.reason = "retry";
            }
        } else if (!(mshr_->pendingWriteback(addr))) {
            std::list<Addr>* evictPointers = mshr_->getEvictPointers(addr);
            for (std::list<Addr>::iterator it = evictPointers->begin(); it != evictPointers->end(); it++) {
                MemEvent * ev = new MemEvent(cachename_, addr, *it, Command::NULLCMD);
                retry_buffer_.push_back(ev);
            }
            if (mem_h_is_debug_addr(addr)) {
                if (event_debuginfo_.reason != "")
                    event_debuginfo_.reason = event_debuginfo_.reason + ",retry";
                else
                    event_debuginfo_.reason = "retry";
            }
        }
    }
}


/***********************************************************************************************************
    * Protocol helper functions
    ***********************************************************************************************************/

uint64_t MESISharNoninclusive::sendResponseUp(MemEvent * event, vector<uint8_t> * data, bool in_mshr, uint64_t time, Command cmd, bool success) {
    MemEvent * responseEvent = event->makeResponse();
    if (cmd != Command::NULLCMD)
        responseEvent->setCmd(cmd);

    /* Only return the desired word */
    if (data) {
        responseEvent->setPayload(*data);
        responseEvent->setSize(data->size()); // Return size that was written
        if (mem_h_is_debug_event(event)) {
            printDataValue(event->getBaseAddr(), data, false);
        }
    }

    if (!success)
        responseEvent->setFail();

    // Compute latency, accounting for serialization of requests to the address
    if (time < timestamp_) time = timestamp_;
    uint64_t delivery_time = time + (in_mshr ? mshr_latency_ : access_latency_);
    forwardByDestination(responseEvent, delivery_time);

    if (mem_h_is_debug_event(event))
        event_debuginfo_.action = "Respond";

    return delivery_time;
}

void MESISharNoninclusive::sendResponseDown(MemEvent * event, std::vector<uint8_t> * data, bool dirty, bool evict) {
    MemEvent * responseEvent = event->makeResponse();

    if (data) {
        responseEvent->setPayload(*data);
        responseEvent->setDirty(dirty);
    }

    responseEvent->setEvict(evict);

    responseEvent->setSize(line_size_);

    uint64_t delivery_time = timestamp_ + (data ? access_latency_ : tag_latency_);
    forwardByDestination(responseEvent, delivery_time);

    if (mem_h_is_debug_event(event))
        event_debuginfo_.action = "Respond";
}


uint64_t MESISharNoninclusive::forwardFlush(MemEvent * event, bool evict, std::vector<uint8_t>* data, bool dirty, uint64_t time) {
    MemEvent * flush = new MemEvent(*event);

    uint64_t latency = tag_latency_;
    if (evict) {
        flush->setEvict(true);
        // TODO only send payload when needed
        flush->setPayload(*data);
        flush->setDirty(dirty);
        latency = access_latency_;
    } else {
        flush->setPayload(0, nullptr);
    }

    uint64_t base_time = (time > timestamp_) ? time : timestamp_;
    uint64_t delivery_time = base_time + latency;
    forwardByAddress(flush, delivery_time);

    if (mem_h_is_debug_event(event))
        event_debuginfo_.action = "Forward";

    return delivery_time-1;
}

/*---------------------------------------------------------------------------------------------------
* Helper Functions
*--------------------------------------------------------------------------------------------------*/

/*********************************************
    *  Methods for sending & receiving messages
    *********************************************/

/*
*  Handles: sending writebacks
*  Latency: cache access + tag to read data that is being written back and update coherence state
*/
void MESISharNoninclusive::sendWritebackFromCache(Command cmd, DirectoryLine* tag, DataLine* data, bool dirty) {
    MemEvent* writeback = new MemEvent(cachename_, tag->getAddr(), tag->getAddr(), cmd);
    writeback->setSize(line_size_);

    uint64_t latency = tag_latency_;

    /* Writeback data */
    if (dirty || writeback_clean_blocks_) {
        writeback->setPayload(*(data->getData()));
        writeback->setDirty(dirty);

        if (mem_h_is_debug_addr(tag->getAddr())) {
            printDataValue(tag->getAddr(), data->getData(), false);
        }

        latency = access_latency_;
    }

    writeback->setRqstr(cachename_);

    uint64_t base_time = (timestamp_ > tag->getTimestamp()) ? timestamp_ : tag->getTimestamp();
    uint64_t delivery_time = base_time + latency;
    forwardByAddress(writeback, delivery_time);
    tag->setTimestamp(delivery_time-1);

}

void MESISharNoninclusive::sendWritebackFromMSHR(Command cmd, DirectoryLine* tag, bool dirty) {
    MemEvent* writeback = new MemEvent(cachename_, tag->getAddr(), tag->getAddr(), cmd);
    writeback->setSize(line_size_);

    uint64_t latency = tag_latency_;

    /* Writeback data */
    if (dirty || writeback_clean_blocks_) {
        writeback->setPayload(mshr_->getData(tag->getAddr()));
        writeback->setDirty(dirty);

        if (mem_h_is_debug_addr(tag->getAddr())) {
            printDataValue(tag->getAddr(), &(mshr_->getData(tag->getAddr())), false);
        }

        latency = access_latency_;
    }

    writeback->setRqstr(cachename_);

    uint64_t base_time = (timestamp_ > tag->getTimestamp()) ? timestamp_ : tag->getTimestamp();
    uint64_t delivery_time = base_time + latency;
    forwardByAddress(writeback, delivery_time);
    tag->setTimestamp(delivery_time-1);

}


void MESISharNoninclusive::sendWritebackAck(MemEvent * event) {
    MemEvent * ack = event->makeResponse();

    uint64_t delivery_time = timestamp_ + tag_latency_;
    forwardByDestination(ack, delivery_time);

    if (mem_h_is_debug_event(event))
        event_debuginfo_.action = "Ack";
}

uint64_t MESISharNoninclusive::sendFetch(Command cmd, MemEvent * event, std::string dst, bool in_mshr, uint64_t ts) {
    Addr addr = event->getBaseAddr();
    MemEvent * fetch = new MemEvent(cachename_, addr, addr, cmd);
    fetch->copyMetadata(event);
    fetch->setDst(dst);
    fetch->setSize(line_size_);

    mshr_->incrementAcksNeeded(addr);

    if (responses_.find(addr) != responses_.end()) {
        responses_.find(addr)->second.insert(std::make_pair(dst, fetch->getID())); // Record events we're waiting for to avoid trying to figure out what happened if we get a NACK
    } else {
        std::map<std::string,MemEvent::id_type> respid;
        respid.insert(std::make_pair(dst, fetch->getID()));
        responses_.insert(std::make_pair(addr, respid));
    }

    uint64_t base_time = timestamp_ > ts ? timestamp_ : ts;
    uint64_t delivery_time = (in_mshr) ? base_time + mshr_latency_ : base_time + tag_latency_;
    forwardByDestination(fetch, delivery_time);

    if (mem_h_is_debug_addr(event->getBaseAddr())) {
        event_debuginfo_.action = "Stall";
        event_debuginfo_.reason = (cmd == Command::Fetch) ? "fetch data" : "Dgr owner";
    }
    return delivery_time;
}


bool MESISharNoninclusive::invalidateExceptRequestor(MemEvent * event, DirectoryLine * tag, bool in_mshr, bool needData) {
    uint64_t delivery_time = 0;
    std::string rqstr = event->getSrc();

    bool getData = needData;
    if (getData && tag->isSharer(event->getSrc()))
        getData = false;

    for (set<std::string>::iterator it = tag->getSharers()->begin(); it != tag->getSharers()->end(); it++) {
        if (*it == rqstr) continue;

        if (getData) { // FetchInv
            getData = false;
            delivery_time =  invalidateSharer(*it, event, tag, in_mshr, Command::FetchInv);
        } else { // Inv
            delivery_time =  invalidateSharer(*it, event, tag, in_mshr);
        }
    }

    if (delivery_time != 0) tag->setTimestamp(delivery_time);

    return delivery_time != 0;
}


bool MESISharNoninclusive::invalidateAll(MemEvent * event, DirectoryLine * tag, bool in_mshr, bool needData) {
    uint64_t delivery_time = 0;
    if (invalidateOwner(event, tag, in_mshr, Command::FetchInv)) {
        return true;
    } else {
        bool data_requested = !needData;
        for (std::set<std::string>::iterator it = tag->getSharers()->begin(); it != tag->getSharers()->end(); it++) {
            if (!data_requested) {
                delivery_time = invalidateSharer(*it, event, tag, in_mshr, Command::FetchInv);
                data_requested = true;
            } else {
                delivery_time = invalidateSharer(*it, event, tag, in_mshr, Command::Inv);
            }
        }
        if (delivery_time != 0) {
            tag->setTimestamp(delivery_time);
            return true;
        }
    }
    return false;
}

void MESISharNoninclusive::invalidateSharers(MemEvent * event, DirectoryLine * tag, bool in_mshr, bool needData, Command cmd) {
    uint64_t delivery_time = 0;
    for (std::set<std::string>::iterator it = tag->getSharers()->begin(); it != tag->getSharers()->end(); it++) {
        if (needData) {
            delivery_time = invalidateSharer(*it, event, tag, in_mshr, Command::FetchInv);
            needData = false;
        } else {
            delivery_time = invalidateSharer(*it, event, tag, in_mshr, cmd);
        }
    }
    tag->setTimestamp(delivery_time);

}

uint64_t MESISharNoninclusive::invalidateSharer(std::string shr, MemEvent * event, DirectoryLine * tag, bool in_mshr, Command cmd) {
    if (tag->isSharer(shr)) {
        Addr addr = tag->getAddr();
        MemEvent * inv = new MemEvent(cachename_, addr, addr, cmd);
        if (event) {
            inv->copyMetadata(event);
        } else {
            inv->setRqstr(cachename_);
        }
        inv->setDst(shr);
        inv->setSize(line_size_);
        if (responses_.find(addr) != responses_.end()) {
            responses_.find(addr)->second.insert(std::make_pair(shr, inv->getID())); // Record events we're waiting for to avoid trying to figure out what happened if we get a NACK
        } else {
            std::map<std::string,MemEvent::id_type> respid;
            respid.insert(std::make_pair(shr, inv->getID()));
            responses_.insert(std::make_pair(addr, respid));
        }

        if (mem_h_is_debug_addr(addr)) {
            event_debuginfo_.action = "Stall";
            event_debuginfo_.reason = "Inv sharer(s)";
        }

        uint64_t base_time = timestamp_ > tag->getTimestamp() ? timestamp_ : tag->getTimestamp();
        uint64_t delivery_time = (in_mshr) ? base_time + mshr_latency_ : base_time + tag_latency_;
        forwardByDestination(inv, delivery_time);

        mshr_->incrementAcksNeeded(addr);

        return delivery_time;
    }
    return 0;
}


bool MESISharNoninclusive::invalidateOwner(MemEvent * metaEvent, DirectoryLine * tag, bool in_mshr, Command cmd) {
    Addr addr = tag->getAddr();
    if (tag->getOwner() == "")
        return false;

    if (mem_h_is_debug_addr(addr)) {
        event_debuginfo_.action = "Stall";
        event_debuginfo_.reason = "Inv owner";
    }

    MemEvent * inv = new MemEvent(cachename_, addr, addr, cmd);
    if (metaEvent) {
        inv->copyMetadata(metaEvent);
    } else {
        inv->setRqstr(cachename_);
    }
    inv->setDst(tag->getOwner());
    inv->setSize(line_size_);

    mshr_->incrementAcksNeeded(addr);

    // Record events we're waiting for to avoid trying to figure out what happened if we get a NACK
    if (responses_.find(addr) != responses_.end()) {
        responses_.find(addr)->second.insert(std::make_pair(inv->getDst(), inv->getID()));
    } else {
        std::map<std::string,MemEvent::id_type> respid;
        respid.insert(std::make_pair(inv->getDst(), inv->getID()));
        responses_.insert(std::make_pair(addr,respid));
    }

    uint64_t base_time = timestamp_ > tag->getTimestamp() ? timestamp_ : tag->getTimestamp();
    uint64_t delivery_time = (in_mshr) ? base_time + mshr_latency_ : base_time + tag_latency_;
    forwardByDestination(inv, delivery_time);
    tag->setTimestamp(delivery_time);

    return true;
}


// Search MSHR for a stalled replacement, move it to the front, and retry
// It'll be retried in the context of whatever request is currently stalled
bool MESISharNoninclusive::applyPendingReplacement(Addr addr) {
    for (int i = mshr_->getSize(addr) - 1; i > 0; i--) {
        MemEventBase * evb = mshr_->getEntryEvent(addr, i);
        if (evb && CommandWriteback[(int)(evb->getCmd())]) {
            mshr_->incrementAcksNeeded(addr);
            mshr_->moveEntryToFront(addr, i);
            if (responses_.find(addr) != responses_.end()) {
                responses_.find(addr)->second.insert(std::make_pair(evb->getSrc(), evb->getID()));
            } else {
                std::map<std::string,MemEvent::id_type> respid;
                respid.insert(std::make_pair(evb->getSrc(), evb->getID()));
                responses_.insert(std::make_pair(addr,respid));
            }
            retry(addr);
            return true;
        }
    }
    return false;
}

/*----------------------------------------------------------------------------------------------------------------------
*  Override message send functions with versions that record statistics & call parent class
*---------------------------------------------------------------------------------------------------------------------*/
void MESISharNoninclusive::forwardByAddress(MemEventBase* ev, Cycle_t timestamp) {
    stat_event_sent_[(int)ev->getCmd()]->addData(1);
    CoherenceController::forwardByAddress(ev, timestamp);
}

void MESISharNoninclusive::forwardByDestination(MemEventBase* ev, Cycle_t timestamp) {
    stat_event_sent_[(int)ev->getCmd()]->addData(1);
    CoherenceController::forwardByDestination(ev, timestamp);
}

/********************
    * Helper functions
    ********************/


void MESISharNoninclusive::removeSharerViaInv(MemEvent * event, DirectoryLine * tag, DataLine * data, bool remove) {
    Addr addr = event->getBaseAddr();
    tag->removeSharer(event->getSrc());
    if (!data && !mshr_->hasData(addr))
        mshr_->setData(addr, event->getPayload());

    if (remove) {
        responses_.find(addr)->second.erase(event->getSrc());
        if (responses_.find(addr)->second.empty())
            responses_.erase(addr);
    }
}

void MESISharNoninclusive::removeOwnerViaInv(MemEvent * event, DirectoryLine * tag, DataLine * data, bool remove) {
    Addr addr = event->getBaseAddr();
    tag->removeOwner();
    if (data)
        data->setData(event->getPayload(), 0);
    else
        mshr_->setData(addr, event->getPayload());

    if (mem_h_is_debug_addr(addr))
        printDataValue(addr, &(event->getPayload()), true);

    if (event->getDirty()) {
        if (tag->getState() == E)
            tag->setState(M);
        else if (tag->getState() == E_Inv)
            tag->setState(M_Inv);
        else if (tag->getState() == E_InvX)
            tag->setState(M_InvX);
    }

    if (remove) {
        responses_.find(addr)->second.erase(event->getSrc());
        if (responses_.find(addr)->second.empty())
            responses_.erase(addr);
    }
}

void MESISharNoninclusive::recordPrefetchResult(DirectoryLine * tag, Statistic<uint64_t> * stat) {
    if (tag->getPrefetch()) {
        stat->addData(1);
        tag->setPrefetch(false);
    }
}

MemEventInitCoherence* MESISharNoninclusive::getInitCoherenceEvent() {
    // Source, Endpoint type, inclusive, sends WB Acks, line size, tracks block presence
    return new MemEventInitCoherence(cachename_, Endpoint::Cache, false, true, false, line_size_, true);
}

void MESISharNoninclusive::printLine(Addr addr) {
    return;
    if (!mem_h_is_debug_addr(addr)) return;
    DirectoryLine * tag = dir_array_->lookup(addr, false);
    DataLine * data = data_array_->lookup(addr, false);
    std::string state = (tag == nullptr) ? "NP" : tag->getString();
    debug_->debug(_L8_, "  Line 0x%" PRIx64 ": %s Data Present: (%s)\n", addr, state.c_str(), data && data->getTag() ? "Y" : "N");
    if (data && data->getTag() && data->getTag() != tag) {
        if (data->getTag() != tag) {
            debug_->fatal(CALL_INFO, -1, "Error: Data has a tag but it does not match the tag found in the directory. Addr 0x%" PRIx64 ". Data = (%" PRIu32 ",0x%" PRIx64 "), Tag = (%" PRIu32 ",0x%" PRIx64 ")\n",
                    addr, data->getIndex(), data->getAddr(), tag->getIndex(), tag->getAddr());
        }
    }
}

void MESISharNoninclusive::printStatus(Output &out) {
    out.output("    Directory Array\n");
    dir_array_->printCacheArray(out);
    out.output("    Data Array\n");
    data_array_->printCacheArray(out);
}

void MESISharNoninclusive::recordLatency(Command cmd, int type, uint64_t latency) {
    if (type == -1)
        return;

    switch (cmd) {
        case Command::GetS:
            stat_latency_GetS_[type]->addData(latency);
            break;
        case Command::GetX:
            stat_latency_GetX_[type]->addData(latency);
            break;
        case Command::GetSX:
            stat_latency_GetSX_[type]->addData(latency);
            break;
        case Command::FlushLine:
            stat_latency_FlushLine_->addData(latency);
            break;
        case Command::FlushLineInv:
            stat_latency_FlushLineInv_->addData(latency);
            break;
        default:
            break;
    }
}


/***********************************************************************************************************
    * Cache flush at simulation shutdown
    ***********************************************************************************************************/
void MESISharNoninclusive::beginCompleteStage() {
    flush_state_ = FlushState::Ready;
    shutdown_flush_counter_ = 0;
}

void MESISharNoninclusive::processCompleteEvent(MemEventInit* event, MemLinkBase* highlink, MemLinkBase* lowlink) {
    if (event->getInitCmd() == MemEventInit::InitCommand::Flush) {
        MemEventUntimedFlush* flush = static_cast<MemEventUntimedFlush*>(event);

        if ( flush->request() ) {
            if ( flush_state_ == FlushState::Ready ) {
                flush_state_ = FlushState::Invalidate;
                std::set<MemLinkBase::EndpointInfo>* src = highlink->getSources();
                shutdown_flush_counter_ += src->size();
                for (auto it = src->begin(); it != src->end(); it++) {
                    MemEventUntimedFlush* forward = new MemEventUntimedFlush(getName());
                    forward->setDst(it->name);
                    highlink->sendUntimedData(forward, false, false);
                }
            }
            if (shutdown_flush_counter_ != 0) {
                delete event;
                return;
            }
        } else {
            shutdown_flush_counter_--;
            if (shutdown_flush_counter_ != 0) {
                delete event;
                return;
            }
        }

        // Ready to flush - all other caches above hae already flushed
        for (auto it : *data_array_) {
            // Only flush dirty data, no need to fix coherence state
            switch (it->getState()) {
                case I:
                case S:
                case E:
                    break;
                case M:
                    {
                    // Double check dataline is actually valid
                    if (it->getTag() != dir_array_->lookup(it->getAddr(), false)) {
                        break;
                    }

                    // Flush if valid
                    MemEventInit * ev = new MemEventInit(cachename_, Command::Write, it->getAddr(), *(it->getData()));
                    lowlink->sendUntimedData(ev, false, true); // Don't broadcast, route by addr
                    break;
                    }
            default:
                // TODO handle case where sim ends and state is not stable
                std::string valstr = getDataString(it->getData());
                debug_->output("%s, NOTICE: Unable to flush cache line that is in transient state '%s'. Addr = 0x%" PRIx64 ". Value = %s\n",
                    cachename_.c_str(), StateString[it->getState()], it->getAddr(), valstr.c_str());
                break;
            };
        }

        if (!flush_manager_) {
            std::set<MemLinkBase::EndpointInfo>* dst = lowlink->getDests();
            for (auto it = dst->begin(); it != dst->end(); it++) {
                MemEventUntimedFlush* response = new MemEventUntimedFlush(getName(), false);
                response->setDst(it->name);
                lowlink->sendUntimedData(response, false, false);
            }
        }
        delete event;
    } else if (event->getCmd() == Command::Write ) {
        Addr addr = event->getAddr();
        DirectoryLine * tag = dir_array_->lookup(addr, false);
        DataLine * data = data_array_->lookup(addr, true);
        if (data && data->getTag() != tag) data = nullptr;

        if (!data) {
            event->setSrc(getName());
            lowlink->sendUntimedData(event, false, true);
        } else {
            data->setData(event->getPayload(), 0);
            delete event;
            tag->setState(M); // Make sure data gets flushed
        }
    } else {
        delete event; // Nothing for now
    }
}

std::set<Command> MESISharNoninclusive::getValidReceiveEvents() {
    std::set<Command> cmds = { Command::GetS,
        Command::GetX,
        Command::GetSX,
        Command::FlushLine,
        Command::FlushLineInv,
        Command::FlushAll,
        Command::ForwardFlush,
        Command::PutS,
        Command::PutE,
        Command::PutM,
        Command::PutX,
        Command::Inv,
        Command::ForceInv,
        Command::Fetch,
        Command::FetchInv,
        Command::FetchInvX,
        Command::NULLCMD,
        Command::GetSResp,
        Command::GetXResp,
        Command::FlushLineResp,
        Command::FlushAllResp,
        Command::FetchResp,
        Command::FetchXResp,
        Command::UnblockFlush,
        Command::AckFlush,
        Command::AckInv,
        Command::AckPut,
        Command::NACK };
    return cmds;
}

void MESISharNoninclusive::serialize_order(SST::Core::Serialization::serializer& ser) {
    CoherenceController::serialize_order(ser);

    SST_SER(data_array_);
    SST_SER(dir_array_);
    SST_SER(protocol_);
    SST_SER(protocol_state_);
    SST_SER(responses_);
    SST_SER(eviction_type_);
    SST_SER(flush_state_);
    SST_SER(shutdown_flush_counter_);
    SST_SER(stat_latency_GetS_);
    SST_SER(stat_latency_GetX_);
    SST_SER(stat_latency_GetSX_);
    SST_SER(stat_latency_FlushLine_);
    SST_SER(stat_latency_FlushLineInv_);
    SST_SER(stat_hit_);
    SST_SER(stat_miss_);
    SST_SER(stat_hits_);
    SST_SER(stat_misses_);
}