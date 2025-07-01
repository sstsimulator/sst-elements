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

// General
#include <vector>

// SST-Core
#include <sst_config.h>

// SST-Elements
#include "coherencemgr/MESI_Inclusive.h"

using namespace SST;
using namespace SST::MemHierarchy;

/* Debug macros included from util.h */

/*----------------------------------------------------------------------------------------------------------------------
 * MESI Coherence Controller Implementation
 * Provides MESI & MSI coherence for inclusive, non-L1 caches
 *
 * MESI:
 *  M   Exclusive, dirty
 *  E   Exclusive, clean
 *  S   Shared, clean
 *  I   Invalid / not present
 *
 * MSI:
 *  M   Exclusive (treated as dirty)
 *  S   Shared, clean
 *  I   Invalid / not present
 *---------------------------------------------------------------------------------------------------------------------*/

/***********************************************************************************************************
 * Constructor
 ***********************************************************************************************************/
MESIInclusive::MESIInclusive(SST::ComponentId_t id, Params& params, Params& owner_params, bool prefetch) :
    CoherenceController(id, params, owner_params, prefetch)
{
    params.insert(owner_params);

    protocol_ = params.find<bool>("protocol", 1);
    if (protocol_)
        protocol_state_ = E;
    else
        protocol_state_ = S;

    flush_state_ = FlushState::Ready;

    shutdown_flush_counter_ = 0;

    // Cache Array
    uint64_t lines = params.find<uint64_t>("lines");
    uint64_t assoc = params.find<uint64_t>("associativity");

    ReplacementPolicy * rmgr = createReplacementPolicy(lines, assoc, params, false);
    HashFunction * ht = createHashFunction(params);
    cache_array_ = new CacheArray<SharedCacheLine>(debug_, lines, assoc, line_size_, rmgr, ht);
    cache_array_->setBanked(params.find<uint64_t>("banks", 0));

    /* Statistics */
    stat_evict_[I] =         registerStatistic<uint64_t>("evict_I");
    stat_evict_[IS] =        registerStatistic<uint64_t>("evict_IS");
    stat_evict_[IM] =        registerStatistic<uint64_t>("evict_IM");
    stat_evict_[S] =         registerStatistic<uint64_t>("evict_S");
    stat_evict_[SM] =        registerStatistic<uint64_t>("evict_SM");
    stat_evict_[S_Inv] =     registerStatistic<uint64_t>("evict_SInv");
    stat_evict_[M] =         registerStatistic<uint64_t>("evict_M");
    stat_evict_[M_Inv] =     registerStatistic<uint64_t>("evict_MInv");
    stat_evict_[SM_Inv] =    registerStatistic<uint64_t>("evict_SMInv");
    stat_evict_[M_InvX] =    registerStatistic<uint64_t>("evict_MInvX");
    stat_evict_[I_B] =       registerStatistic<uint64_t>("evict_IB");
    stat_evict_[S_B] =       registerStatistic<uint64_t>("evict_SB");
    stat_evict_[SB_Inv] =    registerStatistic<uint64_t>("evict_SBInv");
    stat_event_state_[(int)Command::GetS][I] =    registerStatistic<uint64_t>("stateEvent_GetS_I");
    stat_event_state_[(int)Command::GetS][S] =    registerStatistic<uint64_t>("stateEvent_GetS_S");
    stat_event_state_[(int)Command::GetS][M] =    registerStatistic<uint64_t>("stateEvent_GetS_M");
    stat_event_state_[(int)Command::GetX][I] =    registerStatistic<uint64_t>("stateEvent_GetX_I");
    stat_event_state_[(int)Command::GetX][S] =    registerStatistic<uint64_t>("stateEvent_GetX_S");
    stat_event_state_[(int)Command::GetX][M] =    registerStatistic<uint64_t>("stateEvent_GetX_M");
    stat_event_state_[(int)Command::GetSX][I] =   registerStatistic<uint64_t>("stateEvent_GetSX_I");
    stat_event_state_[(int)Command::GetSX][S] =   registerStatistic<uint64_t>("stateEvent_GetSX_S");
    stat_event_state_[(int)Command::GetSX][M] =   registerStatistic<uint64_t>("stateEvent_GetSX_M");
    stat_event_state_[(int)Command::GetSResp][IS] =       registerStatistic<uint64_t>("stateEvent_GetSResp_IS");
    stat_event_state_[(int)Command::GetXResp][IS] =       registerStatistic<uint64_t>("stateEvent_GetXResp_IS");
    stat_event_state_[(int)Command::GetXResp][IM] =       registerStatistic<uint64_t>("stateEvent_GetXResp_IM");
    stat_event_state_[(int)Command::GetXResp][SM] =       registerStatistic<uint64_t>("stateEvent_GetXResp_SM");
    stat_event_state_[(int)Command::GetXResp][SM_Inv] =   registerStatistic<uint64_t>("stateEvent_GetXResp_SMInv");
    stat_event_state_[(int)Command::PutS][S] =        registerStatistic<uint64_t>("stateEvent_PutS_S");
    stat_event_state_[(int)Command::PutS][M] =        registerStatistic<uint64_t>("stateEvent_PutS_M");
    stat_event_state_[(int)Command::PutS][M_Inv] =    registerStatistic<uint64_t>("stateEvent_PutS_MInv");
    stat_event_state_[(int)Command::PutS][S_Inv] =    registerStatistic<uint64_t>("stateEvent_PutS_SInv");
    stat_event_state_[(int)Command::PutS][SM_Inv] =   registerStatistic<uint64_t>("stateEvent_PutS_SMInv");
    stat_event_state_[(int)Command::PutS][S_B] =      registerStatistic<uint64_t>("stateEvent_PutS_SB");
    stat_event_state_[(int)Command::PutS][SB_Inv] =   registerStatistic<uint64_t>("stateEvent_PutS_SBInv");
    stat_event_state_[(int)Command::PutM][M] =        registerStatistic<uint64_t>("stateEvent_PutM_M");
    stat_event_state_[(int)Command::PutM][M_Inv] =    registerStatistic<uint64_t>("stateEvent_PutM_MInv");
    stat_event_state_[(int)Command::PutM][M_InvX] =   registerStatistic<uint64_t>("stateEvent_PutM_MInvX");
    stat_event_state_[(int)Command::PutX][M] =        registerStatistic<uint64_t>("stateEvent_PutX_M");
    stat_event_state_[(int)Command::PutX][M_Inv] =    registerStatistic<uint64_t>("stateEvent_PutX_MInv");
    stat_event_state_[(int)Command::PutX][M_InvX] =   registerStatistic<uint64_t>("stateEvent_PutX_MInvX");
    stat_event_state_[(int)Command::Inv][I] =         registerStatistic<uint64_t>("stateEvent_Inv_I");
    stat_event_state_[(int)Command::Inv][S] =         registerStatistic<uint64_t>("stateEvent_Inv_S");
    stat_event_state_[(int)Command::Inv][IS] =        registerStatistic<uint64_t>("stateEvent_Inv_IS");
    stat_event_state_[(int)Command::Inv][IM] =        registerStatistic<uint64_t>("stateEvent_Inv_IM");
    stat_event_state_[(int)Command::Inv][SM] =        registerStatistic<uint64_t>("stateEvent_Inv_SM");
    stat_event_state_[(int)Command::Inv][S_B] =       registerStatistic<uint64_t>("stateEvent_Inv_SB");
    stat_event_state_[(int)Command::Inv][I_B] =       registerStatistic<uint64_t>("stateEvent_Inv_IB");
    stat_event_state_[(int)Command::FetchInvX][I] =   registerStatistic<uint64_t>("stateEvent_FetchInvX_I");
    stat_event_state_[(int)Command::FetchInvX][M] =   registerStatistic<uint64_t>("stateEvent_FetchInvX_M");
    stat_event_state_[(int)Command::FetchInvX][IS] =  registerStatistic<uint64_t>("stateEvent_FetchInvX_IS");
    stat_event_state_[(int)Command::FetchInvX][IM] =  registerStatistic<uint64_t>("stateEvent_FetchInvX_IM");
    stat_event_state_[(int)Command::FetchInvX][M_Inv] =   registerStatistic<uint64_t>("stateEvent_FetchInvX_MInv");
    stat_event_state_[(int)Command::FetchInvX][M_InvX] =  registerStatistic<uint64_t>("stateEvent_FetchInvX_MInvX");
    stat_event_state_[(int)Command::FetchInvX][I_B] = registerStatistic<uint64_t>("stateEvent_FetchInvX_IB");
    stat_event_state_[(int)Command::FetchInvX][S_B] = registerStatistic<uint64_t>("stateEvent_FetchInvX_SB");
    stat_event_state_[(int)Command::Fetch][I] =       registerStatistic<uint64_t>("stateEvent_Fetch_I");
    stat_event_state_[(int)Command::Fetch][S] =       registerStatistic<uint64_t>("stateEvent_Fetch_S");
    stat_event_state_[(int)Command::Fetch][IS] =      registerStatistic<uint64_t>("stateEvent_Fetch_IS");
    stat_event_state_[(int)Command::Fetch][IM] =      registerStatistic<uint64_t>("stateEvent_Fetch_IM");
    stat_event_state_[(int)Command::Fetch][SM] =      registerStatistic<uint64_t>("stateEvent_Fetch_SM");
    stat_event_state_[(int)Command::Fetch][SM_Inv] =  registerStatistic<uint64_t>("stateEvent_Fetch_SMInv");
    stat_event_state_[(int)Command::Fetch][S_Inv] =   registerStatistic<uint64_t>("stateEvent_Fetch_SInv");
    stat_event_state_[(int)Command::Fetch][I_B] =     registerStatistic<uint64_t>("stateEvent_Fetch_IB");
    stat_event_state_[(int)Command::Fetch][S_B] =     registerStatistic<uint64_t>("stateEvent_Fetch_SB");
    stat_event_state_[(int)Command::FetchInv][I] =        registerStatistic<uint64_t>("stateEvent_FetchInv_I");
    stat_event_state_[(int)Command::FetchInv][S] =        registerStatistic<uint64_t>("stateEvent_FetchInv_S");
    stat_event_state_[(int)Command::FetchInv][M] =        registerStatistic<uint64_t>("stateEvent_FetchInv_M");
    stat_event_state_[(int)Command::FetchInv][IS] =       registerStatistic<uint64_t>("stateEvent_FetchInv_IS");
    stat_event_state_[(int)Command::FetchInv][IM] =       registerStatistic<uint64_t>("stateEvent_FetchInv_IM");
    stat_event_state_[(int)Command::FetchInv][SM] =       registerStatistic<uint64_t>("stateEvent_FetchInv_SM");
    stat_event_state_[(int)Command::FetchInv][SM_Inv] =   registerStatistic<uint64_t>("stateEvent_FetchInv_SMInv");
    stat_event_state_[(int)Command::FetchInv][M_Inv] =    registerStatistic<uint64_t>("stateEvent_FetchInv_MInv");
    stat_event_state_[(int)Command::FetchInv][M_InvX] =   registerStatistic<uint64_t>("stateEvent_FetchInv_MInvX");
    stat_event_state_[(int)Command::FetchInv][I_B] =      registerStatistic<uint64_t>("stateEvent_FetchInv_IB");
    stat_event_state_[(int)Command::FetchInv][S_B] =      registerStatistic<uint64_t>("stateEvent_FetchInv_SB");
    stat_event_state_[(int)Command::ForceInv][I] =        registerStatistic<uint64_t>("stateEvent_ForceInv_I");
    stat_event_state_[(int)Command::ForceInv][S] =        registerStatistic<uint64_t>("stateEvent_ForceInv_S");
    stat_event_state_[(int)Command::ForceInv][M] =        registerStatistic<uint64_t>("stateEvent_ForceInv_M");
    stat_event_state_[(int)Command::ForceInv][IS] =       registerStatistic<uint64_t>("stateEvent_ForceInv_IS");
    stat_event_state_[(int)Command::ForceInv][IM] =       registerStatistic<uint64_t>("stateEvent_ForceInv_IM");
    stat_event_state_[(int)Command::ForceInv][SM] =       registerStatistic<uint64_t>("stateEvent_ForceInv_SM");
    stat_event_state_[(int)Command::ForceInv][S_Inv] =    registerStatistic<uint64_t>("stateEvent_ForceInv_SInv");
    stat_event_state_[(int)Command::ForceInv][SM_Inv] =   registerStatistic<uint64_t>("stateEvent_ForceInv_SMInv");
    stat_event_state_[(int)Command::ForceInv][M_Inv] =    registerStatistic<uint64_t>("stateEvent_ForceInv_MInv");
    stat_event_state_[(int)Command::ForceInv][M_InvX] =   registerStatistic<uint64_t>("stateEvent_ForceInv_MInvX");
    stat_event_state_[(int)Command::ForceInv][I_B] =      registerStatistic<uint64_t>("stateEvent_ForceInv_IB");
    stat_event_state_[(int)Command::ForceInv][S_B] =      registerStatistic<uint64_t>("stateEvent_ForceInv_SB");
    stat_event_state_[(int)Command::FetchResp][M_Inv] =   registerStatistic<uint64_t>("stateEvent_FetchResp_MInv");
    stat_event_state_[(int)Command::FetchXResp][M_InvX] = registerStatistic<uint64_t>("stateEvent_FetchXResp_MInvX");
    stat_event_state_[(int)Command::AckInv][M_Inv] =      registerStatistic<uint64_t>("stateEvent_AckInv_MInv");
    stat_event_state_[(int)Command::AckInv][S_Inv] =      registerStatistic<uint64_t>("stateEvent_AckInv_SInv");
    stat_event_state_[(int)Command::AckInv][SM_Inv] =     registerStatistic<uint64_t>("stateEvent_AckInv_SMInv");
    stat_event_state_[(int)Command::AckInv][SB_Inv] =     registerStatistic<uint64_t>("stateEvent_AckInv_SBInv");
    stat_event_state_[(int)Command::FlushLine][I] =       registerStatistic<uint64_t>("stateEvent_FlushLine_I");
    stat_event_state_[(int)Command::FlushLine][S] =       registerStatistic<uint64_t>("stateEvent_FlushLine_S");
    stat_event_state_[(int)Command::FlushLine][M] =       registerStatistic<uint64_t>("stateEvent_FlushLine_M");
    stat_event_state_[(int)Command::FlushLineInv][I] =    registerStatistic<uint64_t>("stateEvent_FlushLineInv_I");
    stat_event_state_[(int)Command::FlushLineInv][S] =    registerStatistic<uint64_t>("stateEvent_FlushLineInv_S");
    stat_event_state_[(int)Command::FlushLineInv][M] =    registerStatistic<uint64_t>("stateEvent_FlushLineInv_M");
    stat_event_state_[(int)Command::FlushLineResp][I] =       registerStatistic<uint64_t>("stateEvent_FlushLineResp_I");
    stat_event_state_[(int)Command::FlushLineResp][I_B] =     registerStatistic<uint64_t>("stateEvent_FlushLineResp_IB");
    stat_event_state_[(int)Command::FlushLineResp][S_B] =     registerStatistic<uint64_t>("stateEvent_FlushLineResp_SB");
    stat_event_sent_[(int)Command::GetS]          = registerStatistic<uint64_t>("eventSent_GetS");
    stat_event_sent_[(int)Command::GetX]          = registerStatistic<uint64_t>("eventSent_GetX");
    stat_event_sent_[(int)Command::GetSX]         = registerStatistic<uint64_t>("eventSent_GetSX");
    stat_event_sent_[(int)Command::Write]         = registerStatistic<uint64_t>("eventSent_Write");
    stat_event_sent_[(int)Command::PutS]          = registerStatistic<uint64_t>("eventSent_PutS");
    stat_event_sent_[(int)Command::PutM]          = registerStatistic<uint64_t>("eventSent_PutM");
    stat_event_sent_[(int)Command::AckPut]        = registerStatistic<uint64_t>("eventSent_AckPut");
    stat_event_sent_[(int)Command::FlushLine]     = registerStatistic<uint64_t>("eventSent_FlushLine");
    stat_event_sent_[(int)Command::FlushLineInv]  = registerStatistic<uint64_t>("eventSent_FlushLineInv");
    stat_event_sent_[(int)Command::FlushAll]      = registerStatistic<uint64_t>("eventSent_FlushAll");
    stat_event_sent_[(int)Command::ForwardFlush]  = registerStatistic<uint64_t>("eventSent_ForwardFlush");
    stat_event_sent_[(int)Command::FetchResp]     = registerStatistic<uint64_t>("eventSent_FetchResp");
    stat_event_sent_[(int)Command::FetchXResp]    = registerStatistic<uint64_t>("eventSent_FetchXResp");
    stat_event_sent_[(int)Command::AckInv]        = registerStatistic<uint64_t>("eventSent_AckInv");
    stat_event_sent_[(int)Command::NACK]          = registerStatistic<uint64_t>("eventSent_NACK");
    stat_event_sent_[(int)Command::GetSResp]      = registerStatistic<uint64_t>("eventSent_GetSResp");
    stat_event_sent_[(int)Command::GetXResp]      = registerStatistic<uint64_t>("eventSent_GetXResp");
    stat_event_sent_[(int)Command::WriteResp]     = registerStatistic<uint64_t>("eventSent_WriteResp");
    stat_event_sent_[(int)Command::FlushLineResp] = registerStatistic<uint64_t>("eventSent_FlushLineResp");
    stat_event_sent_[(int)Command::FlushAllResp]  = registerStatistic<uint64_t>("eventSent_FlushAllResp");
    stat_event_sent_[(int)Command::AckFlush]      = registerStatistic<uint64_t>("eventSent_AckFlush");
    stat_event_sent_[(int)Command::UnblockFlush]  = registerStatistic<uint64_t>("eventSent_UnblockFlush");
    stat_event_sent_[(int)Command::Fetch]         = registerStatistic<uint64_t>("eventSent_Fetch");
    stat_event_sent_[(int)Command::FetchInv]      = registerStatistic<uint64_t>("eventSent_FetchInv");
    stat_event_sent_[(int)Command::ForceInv]      = registerStatistic<uint64_t>("eventSent_ForceInv");
    stat_event_sent_[(int)Command::FetchInvX]     = registerStatistic<uint64_t>("eventSent_FetchInvX");
    stat_event_sent_[(int)Command::Inv]           = registerStatistic<uint64_t>("eventSent_Inv");
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
    stat_latency_FlushAll_        = registerStatistic<uint64_t>("latency_FlushAll");
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

    /* Only for caches that expect writeback acks but we don't know yet so always enabled for now (can't register statistics later) */
    stat_event_state_[(int)Command::AckPut][I] = registerStatistic<uint64_t>("stateEvent_AckPut_I");

    /* MESI-specific statistics (as opposed to MSI) */
    if (protocol_) {
        stat_evict_[E] =         registerStatistic<uint64_t>("evict_E");
        stat_evict_[E_Inv] =     registerStatistic<uint64_t>("evict_EInv");
        stat_evict_[E_InvX] =    registerStatistic<uint64_t>("evict_EInvX");
        stat_event_state_[(int)Command::GetS][E] =        registerStatistic<uint64_t>("stateEvent_GetS_E");
        stat_event_state_[(int)Command::GetX][E] =        registerStatistic<uint64_t>("stateEvent_GetX_E");
        stat_event_state_[(int)Command::GetSX][E] =       registerStatistic<uint64_t>("stateEvent_GetSX_E");
        stat_event_state_[(int)Command::PutS][E] =        registerStatistic<uint64_t>("stateEvent_PutS_E");
        stat_event_state_[(int)Command::PutS][E_Inv] =    registerStatistic<uint64_t>("stateEvent_PutS_EInv");
        stat_event_state_[(int)Command::PutE][E] =        registerStatistic<uint64_t>("stateEvent_PutE_E");
        stat_event_state_[(int)Command::PutE][M] =        registerStatistic<uint64_t>("stateEvent_PutE_M");
        stat_event_state_[(int)Command::PutE][M_Inv] =    registerStatistic<uint64_t>("stateEvent_PutE_MInv");
        stat_event_state_[(int)Command::PutE][M_InvX] =   registerStatistic<uint64_t>("stateEvent_PutE_MInvX");
        stat_event_state_[(int)Command::PutE][E_Inv] =    registerStatistic<uint64_t>("stateEvent_PutE_EInv");
        stat_event_state_[(int)Command::PutE][E_InvX] =   registerStatistic<uint64_t>("stateEvent_PutE_EInvX");
        stat_event_state_[(int)Command::PutM][E] =        registerStatistic<uint64_t>("stateEvent_PutM_E");
        stat_event_state_[(int)Command::PutM][E_Inv] =    registerStatistic<uint64_t>("stateEvent_PutM_EInv");
        stat_event_state_[(int)Command::PutM][E_InvX] =   registerStatistic<uint64_t>("stateEvent_PutM_EInvX");
        stat_event_state_[(int)Command::PutX][E] =        registerStatistic<uint64_t>("stateEvent_PutX_E");
        stat_event_state_[(int)Command::PutX][E_Inv] =    registerStatistic<uint64_t>("stateEvent_PutX_EInv");
        stat_event_state_[(int)Command::PutX][E_InvX] =   registerStatistic<uint64_t>("stateEvent_PutX_EInvX");
        stat_event_state_[(int)Command::FetchInvX][E] =   registerStatistic<uint64_t>("stateEvent_FetchInvX_E");
        stat_event_state_[(int)Command::FetchInvX][E_Inv] =   registerStatistic<uint64_t>("stateEvent_FetchInvX_EInv");
        stat_event_state_[(int)Command::FetchInvX][E_InvX] =  registerStatistic<uint64_t>("stateEvent_FetchInvX_EInvX");
        stat_event_state_[(int)Command::FetchInv][E] =        registerStatistic<uint64_t>("stateEvent_FetchInv_E");
        stat_event_state_[(int)Command::FetchInv][E_Inv] =    registerStatistic<uint64_t>("stateEvent_FetchInv_EInv");
        stat_event_state_[(int)Command::FetchInv][E_InvX] =   registerStatistic<uint64_t>("stateEvent_FetchInv_EInvX");
        stat_event_state_[(int)Command::ForceInv][E] =        registerStatistic<uint64_t>("stateEvent_ForceInv_E");
        stat_event_state_[(int)Command::ForceInv][E_Inv] =    registerStatistic<uint64_t>("stateEvent_ForceInv_EInv");
        stat_event_state_[(int)Command::ForceInv][E_InvX] =   registerStatistic<uint64_t>("stateEvent_ForceInv_EInvX");
        stat_event_state_[(int)Command::FetchResp][E_Inv] =   registerStatistic<uint64_t>("stateEvent_FetchResp_EInv");
        stat_event_state_[(int)Command::FetchXResp][E_InvX] = registerStatistic<uint64_t>("stateEvent_FetchXResp_EInvX");
        stat_event_state_[(int)Command::AckInv][E_Inv] =      registerStatistic<uint64_t>("stateEvent_AckInv_EInv");
        stat_event_state_[(int)Command::FlushLine][E] =       registerStatistic<uint64_t>("stateEvent_FlushLine_E");
        stat_event_state_[(int)Command::FlushLineInv][E] =    registerStatistic<uint64_t>("stateEvent_FlushLineInv_E");
        stat_event_sent_[(int)Command::PutE] = registerStatistic<uint64_t>("eventSent_PutE");
    }
}

/***********************************************************************************************************
 * Event handlers
 ***********************************************************************************************************/

bool MESIInclusive::handleGetS(MemEvent * event, bool in_mshr) {
    Addr addr = event->getBaseAddr();
    SharedCacheLine * line = cache_array_->lookup(addr, true);
    bool local_prefetch = event->isPrefetch() && (event->getRqstr() == cachename_);
    State state = line ? line->getState() : I;

    MemEventStatus status = MemEventStatus::OK;
    uint64_t send_time = 0;
    vector<uint8_t>* data;
    Command response_command;

    if (mem_h_is_debug_addr(addr))
        event_debuginfo_.prefill(event->getID(), Command::GetS, (local_prefetch ? "-pref" : ""), addr, state);

    if (in_mshr)
        mshr_->removePendingRetry(addr);

    switch (state) {
        case I:
            status = processCacheMiss(event, line, in_mshr);

            if (status == MemEventStatus::OK) { // Both MSHR insert and cache line allocation succeeded and there's no MSHR conflict
                line = cache_array_->lookup(addr, false);
                if (!mshr_->getProfiled(addr)) {
                    stat_event_state_[(int)Command::GetS][I]->addData(1);
                    stat_miss_[0][in_mshr]->addData(1);
                    stat_misses_->addData(1);
                    notifyListenerOfAccess(event, NotifyAccessType::READ, NotifyResultType::MISS);
                    recordLatencyType(event->getID(), LatType::MISS);
                    mshr_->setProfiled(addr);
                }
                send_time = forwardMessage(event, line_size_, 0, nullptr);
                line->setState(IS);
                line->setTimestamp(send_time);
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
                if (local_prefetch) {
                    stat_prefetch_redundant_->addData(1);
                    recordPrefetchLatency(event->getID(), LatType::HIT);
                } else {
                    recordLatencyType(event->getID(), LatType::HIT);
                }
            }

            if (mem_h_is_debug_event(event))
                event_debuginfo_.reason = "hit";

            if (local_prefetch) {
                if (mem_h_is_debug_event(event))
                    event_debuginfo_.action = "Done";
                cleanUpAfterRequest(event, in_mshr);
                break;
            }

            recordPrefetchResult(line, stat_prefetch_hit_);
            line->addSharer(event->getSrc());

            send_time = sendResponseUp(event, line->getData(), in_mshr, line->getTimestamp());
            line->setTimestamp(send_time - 1);
            cleanUpAfterRequest(event, in_mshr);

            break;
        case E:
        case M:
            if (mem_h_is_debug_event(event))
                event_debuginfo_.reason = "hit";

            // Local prefetch -> drop
            if (local_prefetch) {
                if (!in_mshr || !mshr_->getProfiled(addr)) {
                    stat_event_state_[(int)Command::GetS][state]->addData(1);
                    stat_hit_[0][in_mshr]->addData(1);
                    stat_hits_->addData(1);
                    notifyListenerOfAccess(event, NotifyAccessType::PREFETCH, NotifyResultType::HIT);
                    stat_prefetch_redundant_->addData(1);
                    recordPrefetchLatency(event->getID(), LatType::HIT);
                }
                if (mem_h_is_debug_event(event))
                    event_debuginfo_.action = "Done";
                cleanUpAfterRequest(event, in_mshr);
                break;
            }

            recordPrefetchResult(line, stat_prefetch_hit_); // Accessed a prefetched line

            if (line->hasOwner()) {
                if (!in_mshr)
                    status = allocateMSHR(event, false);

                if (status == MemEventStatus::OK) {
                    if (!mshr_->getProfiled(addr)) {
                        stat_event_state_[(int)Command::GetS][state]->addData(1);
                        stat_hit_[0][in_mshr]->addData(1);
                        stat_hits_->addData(1);
                        notifyListenerOfAccess(event, NotifyAccessType::READ, NotifyResultType::HIT);
                        recordLatencyType(event->getID(), LatType::INV);
                        mshr_->setProfiled(addr);
                    }
                    downgradeOwner(event, line, in_mshr);
                    if (state == E)
                        line->setState(E_InvX);
                    else
                        line->setState(M_InvX);
                    mshr_->setInProgress(addr);
                }
                break;
            } else {
                if (!in_mshr || !mshr_->getProfiled(addr)) {
                    stat_event_state_[(int)Command::GetS][state]->addData(1);
                    stat_hit_[0][in_mshr]->addData(1);
                    stat_hits_->addData(1);
                    notifyListenerOfAccess(event, NotifyAccessType::READ, NotifyResultType::HIT);
                    recordLatencyType(event->getID(), LatType::HIT);
                    if (in_mshr) mshr_->setProfiled(addr);
                }
                if (!line->hasSharers() && protocol_) {
                    line->setOwner(event->getSrc());
                    response_command = Command::GetXResp;
                } else {
                    line->addSharer(event->getSrc());
                    response_command = Command::GetSResp;
                }
            }

            send_time = sendResponseUp(event, line->getData(), in_mshr, line->getTimestamp(), response_command);
            line->setTimestamp(send_time);
            cleanUpAfterRequest(event, in_mshr);

            break;
        default:
            if (!in_mshr) {
                status = allocateMSHR(event, false);
            }
    }

    if (mem_h_is_debug_addr(addr) && line) {
        event_debuginfo_.newst = line->getState();
        event_debuginfo_.verboseline = line->getString();
    }

    if (status == MemEventStatus::Reject) {
        if (!local_prefetch)
            sendNACK(event);
        else
            return false; /* Cannot NACK a prefetch, this will cause controller to drop it */
    }

    return true;
}


bool MESIInclusive::handleGetX(MemEvent * event, bool in_mshr) {
    Addr addr = event->getBaseAddr();
    SharedCacheLine * line = cache_array_->lookup(addr, true);
    State state = line ? line->getState() : I;

    MemEventStatus status = MemEventStatus::OK;
    uint64_t send_time = 0;

    if (mem_h_is_debug_event(event))
        event_debuginfo_.prefill(event->getID(), event->getCmd(), "", addr, state);

    if (in_mshr)
        mshr_->removePendingRetry(addr);

    switch (state) {
        case I:
            status = processCacheMiss(event, line, in_mshr);
            if (status == MemEventStatus::OK) {
                line = cache_array_->lookup(addr, false);
                if (!mshr_->getProfiled(addr)) {
                    recordMiss(event->getID());
                    recordLatencyType(event->getID(), LatType::MISS);
                    stat_event_state_[(int)event->getCmd()][I]->addData(1);
                    stat_miss_[(event->getCmd() == Command::GetX ? 1 : 2)][in_mshr]->addData(1);
                    stat_misses_->addData(1);
                    notifyListenerOfAccess(event, NotifyAccessType::WRITE, NotifyResultType::MISS);
                    mshr_->setProfiled(addr);
                }
                send_time = forwardMessage(event, line_size_, 0, nullptr);
                line->setState(IM);
                line->setTimestamp(send_time);
                mshr_->setInProgress(addr); // Keeps us from retrying too early due to certain race conditions between events
                if (mem_h_is_debug_event(event))
                    event_debuginfo_.reason = "miss";
            }
            break;
        case S:
            if (!last_level_) { // Otherwise can silently upgrade to M by falling through to next case
                status = in_mshr ? MemEventStatus::OK : allocateMSHR(event, false);
                if (status == MemEventStatus::OK) {
                    if (!mshr_->getProfiled(addr)) {
                        stat_event_state_[(int)event->getCmd()][state]->addData(1);
                        stat_miss_[(event->getCmd() == Command::GetX ? 1 : 2)][in_mshr]->addData(1);
                        stat_misses_->addData(1);
                        notifyListenerOfAccess(event, NotifyAccessType::WRITE, NotifyResultType::MISS);
                        mshr_->setProfiled(addr);
                    }
                    recordPrefetchResult(line, stat_prefetch_upgrade_miss_);
                    recordLatencyType(event->getID(), LatType::UPGRADE);

                    send_time = forwardMessage(event, line_size_, 0, nullptr);

                    if (invalidateExceptRequestor(event, line, in_mshr)) {
                        line->setState(SM_Inv);
                    } else {
                        line->setState(SM);
                        line->setTimestamp(send_time);
                        if (mem_h_is_debug_event(event))
                            event_debuginfo_.reason = "miss";
                    }
                    mshr_->setInProgress(addr);
                }
                break;
            }
        case E:
        case M:
            if (!in_mshr || !mshr_->getProfiled(addr)) {
                notifyListenerOfAccess(event, NotifyAccessType::WRITE, NotifyResultType::HIT);
                stat_event_state_[(int)event->getCmd()][state]->addData(1);
                stat_hit_[(event->getCmd() == Command::GetX ? 1 : 2)][in_mshr]->addData(1);
                stat_hits_->addData(1);
                if (in_mshr)
                    mshr_->setProfiled(addr);
            }

            recordPrefetchResult(line, stat_prefetch_hit_);

            if (line->hasOtherSharers(event->getSrc())) {
                if (!in_mshr)
                    status = allocateMSHR(event, false);
                if (status == MemEventStatus::OK) {
                    recordLatencyType(event->getID(), LatType::INV);
                    mshr_->setProfiled(addr);
                    invalidateExceptRequestor(event, line, in_mshr);
                    if (state == M)
                        line->setState(M_Inv);
                    else
                        line->setState(E_Inv);
                    mshr_->setInProgress(addr);
                }
                break;
            } else if (line->hasOwner()) {
                if (!in_mshr)
                    status = allocateMSHR(event, false);
                if (status == MemEventStatus::OK) {
                    mshr_->setProfiled(addr);
                    recordLatencyType(event->getID(), LatType::INV);
                    invalidateOwner(event, line, in_mshr, Command::FetchInv);
                    if (state == M)
                        line->setState(M_Inv);
                    else
                        line->setState(E_Inv);
                    mshr_->setInProgress(addr);
                }
                break;
            }

            if (state == S) {
                if (protocol_)
                    line->setState(E);
                else
                    line->setState(M);
            }

            line->setOwner(event->getSrc());
            if (line->isSharer(event->getSrc()))
                line->removeSharer(event->getSrc());
            send_time = sendResponseUp(event, line->getData(), in_mshr, line->getTimestamp());
            line->setTimestamp(send_time);

            if (mem_h_is_debug_event(event))
                event_debuginfo_.reason = "hit";

            if (!in_mshr || !mshr_->getProfiled(addr))
                recordLatencyType(event->getID(), LatType::HIT);

            cleanUpAfterRequest(event, in_mshr);
            break;
        default:
            if (!in_mshr)
                status = allocateMSHR(event, false);
    }

    if (mem_h_is_debug_addr(addr) && line) {
        event_debuginfo_.newst = line->getState();
        event_debuginfo_.verboseline = line->getString();
    }

    if (status == MemEventStatus::Reject)
        sendNACK(event);

    return true;
}


bool MESIInclusive::handleGetSX(MemEvent * event, bool in_mshr) {
    return handleGetX(event, in_mshr);
}


bool MESIInclusive::handleFlushLine(MemEvent * event, bool in_mshr) {
    Addr addr = event->getBaseAddr();
    SharedCacheLine * line = cache_array_->lookup(addr, false);
    State state = line ? line->getState() : I;

    if (mem_h_is_debug_event(event))
        event_debuginfo_.prefill(event->getID(), Command::FlushLine, "", addr, state);

    // Always need an MSHR for a flush
    MemEventStatus status = MemEventStatus::OK;
    if (!in_mshr) {
        status = allocateMSHR(event, false);
    } else {
        mshr_->removePendingRetry(addr);
    }

    bool ack = false;

    /*
     * To avoid races with Invs, we're going to strip the
     * evict part of a FlushLine out before we handle it
     * Then if it gets NACKed, we don't have to worry about
     * whether to retry the eviction too
     */
    recordLatencyType(event->getID(), LatType::HIT);

    if (event->getEvict()) {
        state = doEviction(event, line, state);
        line->addSharer(event->getSrc());
        ack = true;
    }

    switch (state) {
        case I:
        case S:
            break;
        case E:
        case M:
            if (status == MemEventStatus::OK && line->hasOwner()) {
                if (!mshr_->getProfiled(addr)) {
                    stat_event_state_[(int)Command::FlushLine][state]->addData(1);
                    mshr_->setProfiled(addr);
                }
                downgradeOwner(event, line, in_mshr);
                state == M ? line->setState(M_InvX) : line->setState(E_InvX);
                status = MemEventStatus::Stall;
            }
            break;
        case E_Inv:
        case M_Inv:
            if (mshr_->getFrontType(addr) == MSHREntryType::Event) {
                MemEvent * headEvent = static_cast<MemEvent*>(mshr_->getFrontEvent(addr));
                if (headEvent->getCmd() == Command::FetchInvX && !line->hasOwner()) { // Resolve race between downgrade request & this flush
                    responses_.find(addr)->second.erase(event->getSrc());
                    if (responses_.find(addr)->second.empty()) responses_.erase(addr);
                    retry(addr);
                }
            }
            if (status == MemEventStatus::OK)
                status = MemEventStatus::Stall;
            break;
        case E_InvX:
        case M_InvX:
            if (ack) {
                responses_.find(addr)->second.erase(event->getSrc());
                if (responses_.find(addr)->second.empty()) responses_.erase(addr);
                mshr_->decrementAcksNeeded(addr);
                state == E_InvX ? line->setState(E) : line->setState(M);
                retry(addr);
            }
            // Fall through (stall the Flush/replay the waiting request)
        default:
            if (status == MemEventStatus::OK)
               status = MemEventStatus::Stall;
            break;
    }

    if (status == MemEventStatus::OK) {
        if (!mshr_->getProfiled(addr)) {
            stat_event_state_[(int)Command::FlushLine][state]->addData(1);
            mshr_->setProfiled(addr);
        }
        bool downgrade = (state == E || state == M);
        forwardFlush(event, line, downgrade);
        if (line) {
            recordPrefetchResult(line, stat_prefetch_evict_);
            if (state != I)
                line->setState(S_B);
        }
        mshr_->setInProgress(addr);
    }

    //printLine(addr);
    if (mem_h_is_debug_addr(addr) && line) {
        event_debuginfo_.newst = line->getState();
        event_debuginfo_.verboseline = line->getString();
    }

    if (status == MemEventStatus::Reject)
        sendNACK(event);

    return true;
}


bool MESIInclusive::handleFlushLineInv(MemEvent * event, bool in_mshr) {
    Addr addr = event->getBaseAddr();
    SharedCacheLine * line = cache_array_->lookup(addr, false);
    State state = line ? line->getState() : I;
    MemEventStatus status = MemEventStatus::OK;

    if (mem_h_is_debug_event(event))
        event_debuginfo_.prefill(event->getID(), Command::FlushLineInv, "", addr, state);
    //printLine(addr);

    // Always need an MSHR entry
    if (!in_mshr) {
        status = allocateMSHR(event, false);
    } else {
        mshr_->removePendingRetry(addr);
    }

    bool done = (mshr_->getAcksNeeded(addr) == 0);
    if (event->getEvict()) {
        state = doEviction(event, line, state);
        if (responses_.find(addr) != responses_.end() && responses_.find(addr)->second.find(event->getSrc()) != responses_.find(addr)->second.end()) {
            responses_.find(addr)->second.erase(event->getSrc());
            if (responses_.find(addr)->second.empty()) responses_.erase(addr);
        }
        if (!done) {
            done = mshr_->decrementAcksNeeded(addr);
        }
    }

    recordLatencyType(event->getID(), LatType::HIT);

    switch (state) {
        case I:
            break;
        case S:
            if (status == MemEventStatus::OK && invalidateAll(event, line, in_mshr)) {
                line->setState(S_Inv);
                status = MemEventStatus::Stall;
            }
            break;
        case E:
            if (status == MemEventStatus::OK && invalidateAll(event, line, in_mshr)) {
                line->setState(E_Inv);
                status = MemEventStatus::Stall;
            }
            break;
        case M:
            if (status == MemEventStatus::OK && invalidateAll(event, line, in_mshr)) {
                line->setState(M_Inv);
                status = MemEventStatus::Stall;
            }
            break;
        case S_Inv:
            if (done) {
                line->setState(S);
                retry(addr);
            }
            if (status == MemEventStatus::OK)
                status = MemEventStatus::Stall;
            break;
        case E_Inv:
        case E_InvX:
            if (done) {
                line->setState(E);
                retry(addr);
            }
            if (status == MemEventStatus::OK)
                status = MemEventStatus::Stall;
            break;
        case M_Inv:
        case M_InvX:
            if (done) {
                line->setState(M);
                retry(addr);
            }
            if (status == MemEventStatus::OK)
                status = MemEventStatus::Stall;
            break;
        case SB_Inv:
            if (done) {
                line->setState(S_B);
                retry(addr);
            }
            if (status == MemEventStatus::OK)
                status = MemEventStatus::Stall;
            break;
        case SM_Inv:
            if (done) {
                line->setState(SM);
                retry(addr);
            }
            if (status == MemEventStatus::OK)
                status = MemEventStatus::Stall;
            break;
        default:
            if (status == MemEventStatus::OK)
                status = MemEventStatus::Stall;
    }

    if (status == MemEventStatus::OK) {
        if (!mshr_->getProfiled(addr)) {
            stat_event_state_[(int)Command::FlushLineInv][state]->addData(1);
            mshr_->setProfiled(addr);
        }
        mshr_->setInProgress(addr);
        if (line)
            recordPrefetchResult(line, stat_prefetch_evict_);
        forwardFlush(event, line, state != I);

        if (state != I)
            line->setState(I_B);
    }

    //printLine(addr);
    if (mem_h_is_debug_addr(addr) && line) {
        event_debuginfo_.newst = line->getState();
        event_debuginfo_.verboseline = line->getString();
    }

    if (status == MemEventStatus::Reject)
        sendNACK(event);

    return true;
}

/*
 *  1 source -> always send
 *  2+ sources + peers -> only the min peer sends
 *  2+ sources + no peers -> always send
 *
 * 1. Flush Manager contacts *all* coherence entities and forces a transition to flush
 *  -> as soon as contacted, L1s do flush. They DO NOT transition out of flush state when done
 *  -> Private cache -> flush when ack received.
 *  -> Shared cache -> forward flush
 */
bool MESIInclusive::handleFlushAll(MemEvent * event, bool in_mshr) {
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
        case FlushState::Drain: // Unused state in this coherence protocol, fall-thru with Forward
        {
            /* Have received all acks, do local flush and have all peers flush as well */
            int count = broadcastMemEventToPeers(Command::ForwardFlush, event, timestamp_ + 1);

            for (auto it : *cache_array_) {
                if (it->getState() == I) continue;
                if (it->getState() == S || it->getState() == E || it->getState() == M) {
                        MemEvent * ev = new MemEvent(cachename_, it->getAddr(), it->getAddr(), Command::NULLCMD);
                        retry_buffer_.push_back(ev);
                        count++;
                    } else {
                        debug_->fatal(CALL_INFO, -1, "%s, Error: Attempting to flush a cache line that is in a transient state '%s'. Addr = 0x%" PRIx64 ". Event: %s. Time: %" PRIu64 "ns\n",
                            cachename_.c_str(), StateString[it->getState()], it->getAddr(), event->getVerboseString().c_str(), getCurrentSimTimeNano());
                    }
            }
            if (count > 0) {
                mshr_->incrementFlushCount(count);
                event_debuginfo_.action = "Flush";
                flush_state_ = FlushState::Invalidate;
                break;
            } /* else fall-thru */
        }
        case FlushState::Invalidate:
            /* Have finished invalidating */
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


bool MESIInclusive::handleForwardFlush(MemEvent * event, bool in_mshr) {
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
            case FlushState::Drain: // Unused state in this coherence protocol, fall-thru with Forward
            case FlushState::Forward:
            {
                /* Have received all acks, do local flush and have all peers flush as well */
                int count = broadcastMemEventToPeers(Command::ForwardFlush, event, timestamp_ + 1);
                mshr_->incrementFlushCount(count);
                bool evictionNeeded = (count != 0);
                for (auto it : *cache_array_) {
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
        bool evictionNeeded = false;
        for (auto it : *cache_array_) {
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


bool MESIInclusive::handlePutS(MemEvent * event, bool in_mshr) {
    Addr addr = event->getBaseAddr();
    SharedCacheLine * line = cache_array_->lookup(addr, false);
    State state = line ? line->getState() : I;

    if (mem_h_is_debug_event(event)) {
        event_debuginfo_.prefill(event->getID(), Command::PutS, "", addr, state);
        event_debuginfo_.action = "Done";
    }

    if (in_mshr)
        mshr_->removePendingRetry(addr);

    state = doEviction(event, line, state);
    stat_event_state_[(int)Command::PutS][state]->addData(1);

    if (responses_.find(addr) != responses_.end() && responses_.find(addr)->second.find(event->getSrc()) != responses_.find(addr)->second.end()) {
        responses_.find(addr)->second.erase(event->getSrc());
        if (responses_.find(addr)->second.empty()) responses_.erase(addr);
    }

    if (send_writeback_ack_)
       sendAckPut(event);

    bool done = (mshr_->getAcksNeeded(addr) == 0);
    if (!done)
        done = mshr_->decrementAcksNeeded(addr);

    switch (state) {
        case S:
        case E:
        case M:
            break;
        case S_Inv:
            if (done) {
                line->setState(S);
                retry(addr);
            }
            break;
        case E_Inv:
            if (done) {
                line->setState(E);
                retry(addr);
            }
            break;
        case M_Inv:
            if (done) {
                line->setState(M);
                retry(addr);
            }
            break;
        case SM_Inv:
            if (done) {
                line->setState(SM);
                if (!mshr_->getInProgress(addr))
                    retry(addr);    // An Inv raced with a GetX, replay the Inv
            }
            break;
        case SB_Inv:
            if (done) {
                line->setState(S_B);
                retry(addr);
            }
            break;
        case S_B:
            break;
        default:
            debug_->fatal(CALL_INFO,-1,"%s, Error: Received PutS in unhandled state '%s'. Event: %s. Time = %" PRIu64 "ns\n",
                    getName().c_str(), StateString[state], event->getVerboseString().c_str(), getCurrentSimTimeNano());
    }

    //printLine(addr);
    if (mem_h_is_debug_addr(addr) && line) {
        event_debuginfo_.newst = line->getState();
        event_debuginfo_.verboseline = line->getString();
    }

    delete event;

    return true;
}


bool MESIInclusive::handlePutE(MemEvent * event, bool in_mshr) {
    Addr addr = event->getBaseAddr();
    SharedCacheLine * line = cache_array_->lookup(addr, false);
    State state = line ? line->getState() : I;

    if (mem_h_is_debug_event(event)) {
        event_debuginfo_.prefill(event->getID(), Command::PutE, "", addr, state);
        event_debuginfo_.action = "Done";
    }

    if (in_mshr)
        mshr_->removePendingRetry(addr);

    stat_event_state_[(int)Command::PutE][state]->addData(1);

    state = doEviction(event, line, state);
    if (responses_.find(addr) != responses_.end() && responses_.find(addr)->second.find(event->getSrc()) != responses_.find(addr)->second.end()) {
        responses_.find(addr)->second.erase(event->getSrc());
        if (responses_.find(addr)->second.empty()) responses_.erase(addr);
    }


    if (send_writeback_ack_)
       sendAckPut(event);

    bool done = (mshr_->getAcksNeeded(addr) == 0);
    if (!done)
        done = mshr_->decrementAcksNeeded(addr);

    switch (state) {
        case E:
        case M:
            break;
        case E_Inv:
        case E_InvX:
            if (done) {
                line->setState(E);
                retry(addr);
            }
            break;
        case M_Inv:
        case M_InvX:
            if (done) {
                line->setState(M);
                retry(addr);
            }
            break;
        default:
            debug_->fatal(CALL_INFO,-1,"%s, Error: Received PutE in unhandled state '%s'. Event: %s. Time = %" PRIu64 "ns\n",
                    getName().c_str(), StateString[state], event->getVerboseString().c_str(), getCurrentSimTimeNano());
    }

    //printLine(addr);
    if (mem_h_is_debug_addr(addr) && line) {
        event_debuginfo_.newst = line->getState();
        event_debuginfo_.verboseline = line->getString();
    }

    delete event;

    return true;
}


bool MESIInclusive::handlePutM(MemEvent * event, bool in_mshr) {
    Addr addr = event->getBaseAddr();
    SharedCacheLine * line = cache_array_->lookup(addr, false);
    State state = line ? line->getState() : I;

    if (mem_h_is_debug_event(event)) {
        event_debuginfo_.prefill(event->getID(), Command::PutM, "", addr, state);
        event_debuginfo_.action = "Done";
    }

    if (in_mshr)
        mshr_->removePendingRetry(addr);

    stat_event_state_[(int)Command::PutM][state]->addData(1);

    state = doEviction(event, line, state);
    if (responses_.find(addr) != responses_.end() && responses_.find(addr)->second.find(event->getSrc()) != responses_.find(addr)->second.end()) {
        responses_.find(addr)->second.erase(event->getSrc());
        if (responses_.find(addr)->second.empty()) responses_.erase(addr);
    }

    if (send_writeback_ack_)
       sendAckPut(event);

    bool done = (mshr_->getAcksNeeded(addr) == 0);
    if (!done)
        done = mshr_->decrementAcksNeeded(addr);

    switch (state) {
        case E:
        case M:
            break;
        case E_Inv:
        case E_InvX:
            if (done) {
                line->setState(E);
                retry(addr);
            }
            break;
        case M_Inv:
        case M_InvX:
            if (done) {
                line->setState(M);
                retry(addr);
            }
            break;
        default:
            debug_->fatal(CALL_INFO,-1,"%s, Error: Received PutM in unhandled state '%s'. Event: %s. Time = %" PRIu64 "ns\n",
                    getName().c_str(), StateString[state], event->getVerboseString().c_str(), getCurrentSimTimeNano());
    }

    if (mem_h_is_debug_addr(addr) && line) {
        event_debuginfo_.newst = line->getState();
        event_debuginfo_.verboseline = line->getString();
    }

    delete event;

    return true;
}


bool MESIInclusive::handlePutX(MemEvent * event, bool in_mshr) {
    Addr addr = event->getBaseAddr();
    SharedCacheLine * line = cache_array_->lookup(addr, false);
    State state = line ? line->getState() : I;

    if (mem_h_is_debug_event(event))
        event_debuginfo_.prefill(event->getID(), Command::PutX, "", addr, state);

    if (in_mshr)
        mshr_->removePendingRetry(addr);

    stat_event_state_[(int)Command::PutX][state]->addData(1);

    state = doEviction(event, line, state);
    line->addSharer(event->getSrc());

    if (send_writeback_ack_)
       sendAckPut(event);

    switch (state) {
        case E:
        case M:
            break;
        case E_Inv:
        case M_Inv:
            if (mshr_->getFrontType(addr) == MSHREntryType::Event && mshr_->getFrontEvent(addr)->getCmd() == Command::FetchInvX) {
                responses_.find(addr)->second.erase(event->getSrc());
                if (responses_.find(addr)->second.empty()) responses_.erase(addr);
                retry(addr);
            }
            break;
        case E_InvX:
            responses_.find(addr)->second.erase(event->getSrc());
            if (responses_.find(addr)->second.empty()) responses_.erase(addr);
            if (mshr_->getAcksNeeded(addr) && mshr_->decrementAcksNeeded(addr)) {
                line->setState(E);
                retry(addr);
            }
            break;
        case M_InvX:
            responses_.find(addr)->second.erase(event->getSrc());
            if (responses_.find(addr)->second.empty()) responses_.erase(addr);
            if (mshr_->getAcksNeeded(addr) && mshr_->decrementAcksNeeded(addr)) {
                line->setState(M);
                retry(addr);
            }
            break;
        default:
            debug_->fatal(CALL_INFO,-1,"%s, Error: Received PutX in unhandled state '%s'. Event: %s. Time = %" PRIu64 "ns\n",
                    getName().c_str(), StateString[state], event->getVerboseString().c_str(), getCurrentSimTimeNano());
    }

    //printLine(addr);
    if (mem_h_is_debug_addr(addr) && line) {
        event_debuginfo_.newst = line->getState();
        event_debuginfo_.verboseline = line->getString();
    }

    delete event;
    return true;
}


bool MESIInclusive::handleFetch(MemEvent * event, bool in_mshr) {
    Addr addr = event->getBaseAddr();
    SharedCacheLine * line = cache_array_->lookup(addr, false);
    State state = line ? line->getState() : I;

    if (mem_h_is_debug_event(event))
        event_debuginfo_.prefill(event->getID(), Command::Fetch, "", addr, state);

    if (in_mshr)
        mshr_->removePendingRetry(addr);

    stat_event_state_[(int)Command::Fetch][state]->addData(1);

    switch (state) {
        case S:
        case SM:
        case S_B:
        case S_Inv:
        case SM_Inv:
            sendResponseDown(event, line, true, false);
            break;
        case I:
        case IS:
        case IM:
        case I_B:
            break;
        default:
            debug_->fatal(CALL_INFO,-1,"%s, Error: Received Fetch in unhandled state '%s'. Event: %s. Time = %" PRIu64 "ns\n",
                    getName().c_str(), StateString[state], event->getVerboseString().c_str(), getCurrentSimTimeNano());
    }

    if (mem_h_is_debug_addr(addr) && line) {
        event_debuginfo_.newst = line->getState();
        event_debuginfo_.verboseline = line->getString();
    }

    cleanUpEvent(event, in_mshr); // No replay since state doesn't change
    return true;
}


bool MESIInclusive::handleInv(MemEvent * event, bool in_mshr) {
    Addr addr = event->getBaseAddr();
    SharedCacheLine * line = cache_array_->lookup(addr, false);
    State state = line ? line->getState() : I;
    MemEventStatus status = MemEventStatus::OK;

    if (mem_h_is_debug_event(event))
        event_debuginfo_.prefill(event->getID(), Command::Inv, "", addr, state);

    if (in_mshr)
        mshr_->removePendingRetry(addr);

    bool handle = false;
    State state1, state2;
    switch (state) {
        case S:     // If sharers -> S_Inv & stall, if no sharers -> I & respond (done).
            state1 = S_Inv;
            state2 = I;
            handle = true;
            break;
        case S_B:   // If sharers -> SB_Inv & stall, if no sharers -> I & respond (done)
            state1 = SB_Inv;
            state2 = I;
            handle = true;
            break;
        case SM:    // If requestor is sharer, send inv -> SM_Inv, & stall. If no sharers, -> IM & respond(done)
            state1 = SM_Inv;
            state2 = IM;
            handle = true;
            break;
        case S_Inv:
        case SM_Inv:
            status = in_mshr ? MemEventStatus::OK : allocateMSHR(event, true, 0);
            break;
        case I_B:
            line->setState(I);
        case I:
        case IS:
        case IM:
            if (mem_h_is_debug_event(event))
                event_debuginfo_.action = "Drop";
            cleanUpEvent(event, in_mshr); // No replay since state doesn't change
            stat_event_state_[(int)Command::Inv][state]->addData(1);
            break;
        default:
            debug_->fatal(CALL_INFO,-1,"%s, Error: Received Inv in unhandled state '%s'. Event: %s. Time = %" PRIu64 "ns\n",
                    getName().c_str(), StateString[state], event->getVerboseString().c_str(), getCurrentSimTimeNano());
    }

    if (handle) {
        if (!in_mshr || mshr_->getProfiled(addr)) {
            stat_event_state_[(int)Command::Inv][state]->addData(1);
            recordPrefetchResult(line, stat_prefetch_inv_);
            if (in_mshr) mshr_->setProfiled(addr);
        }
        if (line->hasSharers() && !in_mshr)
            status = allocateMSHR(event, true, 0);
        if (status != MemEventStatus::Reject) {
            if (invalidateAll(event, line, in_mshr)) {
                line->setState(state1);
                status = MemEventStatus::Stall;
            } else {
                sendResponseDown(event, line, false, true);
                line->setState(state2);
                cleanUpAfterRequest(event, in_mshr);
            }
        }
    }

    if (mem_h_is_debug_addr(addr) && line) {
        event_debuginfo_.newst = line->getState();
        event_debuginfo_.verboseline = line->getString();
    }

    if (status == MemEventStatus::Reject)
        sendNACK(event);

    return true;
}


bool MESIInclusive::handleForceInv(MemEvent * event, bool in_mshr) {
    Addr addr = event->getBaseAddr();
    SharedCacheLine * line = cache_array_->lookup(addr, false);
    State state = line ? line->getState() : I;
    MemEventStatus status = MemEventStatus::OK;

    if (mem_h_is_debug_event(event))
        event_debuginfo_.prefill(event->getID(), Command::ForceInv, "", addr, state);

    if (in_mshr)
        mshr_->removePendingRetry(addr);

    bool handle = false;
    bool profile = false;
    State state1, state2;
    switch (state) {
        case S: // Ack if unshared, else forward ForceInv and S_Inv
            state1 = S_Inv;
            state2 = I;
            handle = true;
            break;
        case E: // Ack if unshared/owned. Else forward ForceInv and E_Inv
            state1 = E_Inv;
            state2 = I;
            handle = true;
            break;
        case M: // Ack if unshared/owned. Else forward ForceInv and M_Inv
            state1 = M_Inv;
            state2 = I;
            handle = true;
            break;
        case SM: // ForceInv if un-inv'd sharer & SM_Inv, else IM & ack
            state1 = SM_Inv;
            state2 = IM;
            handle = true;
            break;
        case S_B: // if shared, forward & SB_Inv, else ack & I
            state1 = SB_Inv;
            state2 = I;
            handle = true;
            break;
        case S_Inv: // in mshr & stall
        case E_Inv: // in mshr & stall
            status = in_mshr ? MemEventStatus::OK : allocateMSHR(event, true, 0);
            if (status != MemEventStatus::Reject) {
                profile = true;
                status = MemEventStatus::Stall;
            }
            break;
        case M_Inv: // in mshr & stall
        case E_InvX: // in mshr & stall
        case M_InvX: // in mshr & stall
            if (mshr_->getFrontType(addr) == MSHREntryType::Event &&
                    (mshr_->getFrontEvent(addr)->getCmd() == Command::GetX || mshr_->getFrontEvent(addr)->getCmd() == Command::GetS || mshr_->getFrontEvent(addr)->getCmd() == Command::GetSX)) {
                status = in_mshr ? MemEventStatus::OK : allocateMSHR(event, true, 1);
            } else {
                status = in_mshr ? MemEventStatus::OK : allocateMSHR(event, true, 0);
                profile = true;
            }
            if (status != MemEventStatus::Reject) {
                status = MemEventStatus::Stall;
            } else profile = false;
            break;
        case I_B:
            line->setState(I);
        case IS:
        case IM:
        case I:
            stat_event_state_[(int)Command::ForceInv][state]->addData(1);
            cleanUpEvent(event, in_mshr); // No replay since state doesn't change
            break;
        case SM_Inv: { // ForceInv if there's an un-inv'd sharer, else in mshr & stall
            std::string src = mshr_->getFrontEvent(addr)->getSrc();
            status = in_mshr ? MemEventStatus::OK : allocateMSHR(event, true, 0);
            if (status != MemEventStatus::Reject) {
                profile = true;
                if (line->isSharer(src)) {
                    line->setTimestamp(invalidateSharer(src, event, line, in_mshr, Command::ForceInv));
                }
                status = MemEventStatus::Stall;
            }
            break;
            }
        default:
            debug_->fatal(CALL_INFO,-1,"%s, Error: Received ForceInv in unhandled state '%s'. Event: %s. Time = %" PRIu64 "ns\n",
                    getName().c_str(), StateString[state], event->getVerboseString().c_str(), getCurrentSimTimeNano());
    }

    if ((handle || profile) && (!in_mshr || !mshr_->getProfiled(addr))) {
        stat_event_state_[(int)Command::ForceInv][state]->addData(1);
        recordPrefetchResult(line, stat_prefetch_inv_);
        if (in_mshr || profile) mshr_->setProfiled(addr);
    }

    if (handle) {
        if (line->hasSharers() || line->hasOwner()) {
            if (!in_mshr)
                status = allocateMSHR(event, true, 0);
            if (status != MemEventStatus::Reject) {
                invalidateAll(event, line, in_mshr, Command::ForceInv);
                line->setState(state1);
                status = MemEventStatus::Stall;
                mshr_->setProfiled(addr);
            }
        } else {
            sendResponseDown(event, line, false, true);
            line->setState(state2);
            cleanUpAfterRequest(event, in_mshr);
        }
    }

    if (mem_h_is_debug_addr(addr) && line) {
        event_debuginfo_.newst = line->getState();
        event_debuginfo_.verboseline = line->getString();
    }

    if (status == MemEventStatus::Reject)
        sendNACK(event);

    return true;
}


bool MESIInclusive::handleFetchInv(MemEvent * event, bool in_mshr) {
    Addr addr = event->getBaseAddr();
    SharedCacheLine * line = cache_array_->lookup(addr, false);
    State state = line ? line->getState() : I;
    MemEventStatus status = MemEventStatus::OK;

    if (mem_h_is_debug_event(event))
        event_debuginfo_.prefill(event->getID(), Command::FetchInv, "", addr, state);

    if (in_mshr)
        mshr_->removePendingRetry(addr);

    State state1, state2;
    bool handle = false;
    bool profile = false;
    switch (state) {
        case I_B:
            line->setState(I);
        case I: // Drop
        case IS:
        case IM:
            if (mem_h_is_debug_event(event))
                event_debuginfo_.action = "Drop";
            cleanUpEvent(event, in_mshr); // No replay since state doesn't change
            stat_event_state_[(int)Command::FetchInv][state]->addData(1);
            break;
        case S:
            state1 = S_Inv;
            state2 = I;
            handle = true;
            break;
        case E:
            state1 = E_Inv;
            state2 = I;
            handle = true;
            break;
        case M:
            state1 = M_Inv;
            state2 = I;
            handle = true;
            break;
        case SM:
            state1 = SM_Inv;
            state2 = IM;
            handle = true;
            break;
        case S_B:
            state1 = SB_Inv;
            state2 = I;
            handle = true;
            break;
        case S_Inv:
            status = in_mshr ? MemEventStatus::OK : allocateMSHR(event, true, 1);
            if (status != MemEventStatus::Reject)
                status = MemEventStatus::Stall;
            break;
        case E_Inv:
        case M_Inv:
        case E_InvX:
        case M_InvX:
            if (mshr_->getFrontType(addr) == MSHREntryType::Event &&
                    (mshr_->getFrontEvent(addr)->getCmd() == Command::GetX || mshr_->getFrontEvent(addr)->getCmd() == Command::GetS || mshr_->getFrontEvent(addr)->getCmd() == Command::GetSX)) {
                status = in_mshr ? MemEventStatus::OK : allocateMSHR(event, true, 1);
            } else {
                status = in_mshr ? MemEventStatus::OK : allocateMSHR(event, true, 0);
                profile = true;
            }
            if (status != MemEventStatus::Reject)
                status = MemEventStatus::Stall;
            else profile = false;
            break;
        case SM_Inv:
            status = in_mshr ? MemEventStatus::OK : allocateMSHR(event, true, 0);
            if (status != MemEventStatus::Reject) {
                profile = true;
                std::string shr = mshr_->getFrontEvent(addr)->getSrc();
                if (line->isSharer(shr)) {
                    invalidateSharer(shr, event, line, in_mshr);
                }
                status = MemEventStatus::Stall;
            }
            break;
        default:
            debug_->fatal(CALL_INFO,-1,"%s, Error: Received FetchInv in unhandled state '%s'. Event: %s. Time = %" PRIu64 "ns\n",
                    getName().c_str(), StateString[state], event->getVerboseString().c_str(), getCurrentSimTimeNano());
    }

    if ((handle || profile) && (!in_mshr || !mshr_->getProfiled(addr))) {
        stat_event_state_[(int)Command::FetchInv][state]->addData(1);
        recordPrefetchResult(line, stat_prefetch_inv_);
        if (in_mshr || profile) mshr_->setProfiled(addr);
    }

    if (handle) {
        if (line->hasSharers() || line->hasOwner()) {
            status = in_mshr ? MemEventStatus::OK : allocateMSHR(event, true, 0);
            if (status != MemEventStatus::Reject) {
                invalidateAll(event, line, in_mshr);
                line->setState(state1);
                status = MemEventStatus::Stall;
                mshr_->setProfiled(addr);
            }
        } else {
            sendResponseDown(event, line, true, true);
            line->setState(state2);
            cleanUpAfterRequest(event, in_mshr);
        }
    }

    if (mem_h_is_debug_addr(addr) && line) {
        event_debuginfo_.newst = line->getState();
        event_debuginfo_.verboseline = line->getString();
    }

    if (status == MemEventStatus::Reject)
        sendNACK(event);

    return true;
}


bool MESIInclusive::handleFetchInvX(MemEvent * event, bool in_mshr) {
    Addr addr = event->getBaseAddr();
    SharedCacheLine * line = cache_array_->lookup(addr, false);
    State state = line ? line->getState() : I;
    MemEventStatus status = MemEventStatus::OK;

    if (mem_h_is_debug_event(event))
        event_debuginfo_.prefill(event->getID(), Command::FetchInvX, "", addr, state);

    if (in_mshr)
        mshr_->removePendingRetry(addr);

    switch (state) {
        case E:
        case M:
            if (line->hasOwner()) {
                status = in_mshr ? MemEventStatus::OK : allocateMSHR(event, true, 0);
                if (status == MemEventStatus::OK) {
                    downgradeOwner(event, line, in_mshr);
                    mshr_->setInProgress(addr);
                    state == E ? line->setState(E_InvX) : line->setState(M_InvX);
                    status = MemEventStatus::Stall;
                    mshr_->setProfiled(addr);
                    stat_event_state_[(int)Command::FetchInvX][state]->addData(1);
                }
                break;
            }
            sendResponseDown(event, line, true, true);
            line->setState(S);
            cleanUpAfterRequest(event, in_mshr);
            stat_event_state_[(int)Command::FetchInvX][state]->addData(1);
            break;
        case M_Inv:
        case E_Inv:
        case E_InvX:
        case M_InvX:
            // GetX is handled internally so the FetchInvX should stall for it
            if (mshr_->getFrontType(addr) == MSHREntryType::Event &&
                    (mshr_->getFrontEvent(addr)->getCmd() == Command::GetX || mshr_->getFrontEvent(addr)->getCmd() == Command::GetSX || mshr_->getFrontEvent(addr)->getCmd() == Command::GetS)) {
                status = in_mshr ? MemEventStatus::Stall : allocateMSHR(event, true, 1);
            } else if (line->hasOwner()) {
                status = in_mshr ? MemEventStatus::OK : allocateMSHR(event, true, 0);
                stat_event_state_[(int)Command::FetchInvX][state]->addData(1);
                mshr_->setProfiled(addr);
                if (status != MemEventStatus::Reject)
                    status = MemEventStatus::Stall;
            } else { // E_InvX/M_InvX never reach this bit
                line->setState(S_Inv);
                sendResponseDown(event, line, true, true);
                cleanUpAfterRequest(event, in_mshr);
                stat_event_state_[(int)Command::FetchInvX][state]->addData(1);
            }
            break;
        case S_B:
        case I_B:
        case IS:
        case IM:
        case I:
            if (mem_h_is_debug_event(event))
                event_debuginfo_.action = "Drop";
            cleanUpEvent(event, in_mshr); // No replay since state doesn't change
            stat_event_state_[(int)Command::FetchInvX][state]->addData(1);
            break;
        default:
            debug_->fatal(CALL_INFO,-1,"%s, Error: Received FetchInvX in unhandled state '%s'. Event: %s. Time = %" PRIu64 "ns\n",
                    getName().c_str(), StateString[state], event->getVerboseString().c_str(), getCurrentSimTimeNano());
    }

    if (mem_h_is_debug_addr(addr) && line) {
        event_debuginfo_.newst = line->getState();
        event_debuginfo_.verboseline = line->getString();
    }

    if (status == MemEventStatus::Reject)
        sendNACK(event);

    return true;
}

bool MESIInclusive::handleGetSResp(MemEvent * event, bool in_mshr) {
    Addr addr = event->getBaseAddr();
    SharedCacheLine * line = cache_array_->lookup(addr, false);
    State state = line ? line->getState() : I;

    if (mem_h_is_debug_event(event))
        event_debuginfo_.prefill(event->getID(), Command::GetSResp, "", addr, state);

    stat_event_state_[(int)Command::GetSResp][state]->addData(1);

    // Find matching request in MSHR
    MemEvent * req = static_cast<MemEvent*>(mshr_->getFrontEvent(event->getBaseAddr()));
    //if (mem_h_is_debug_addr(addr))
        //debug_->debug(_L5_, "    Request: %s\n", req->getBriefString().c_str());
    bool local_prefetch = req->isPrefetch() && (req->getRqstr() == cachename_);
    req->setFlags(event->getMemFlags());

    // Sanity check line state
    if (state != IS) {
        debug_->fatal(CALL_INFO,-1,"%s, Error: Received GetSResp in unhandled state '%s'. Event: %s. Time = %" PRIu64 "ns\n",
                getName().c_str(), StateString[state], event->getVerboseString().c_str(), getCurrentSimTimeNano());
    }

    // Update line
    line->setData(event->getPayload(), 0);
    line->setState(S);

    if (mem_h_is_debug_addr(addr))
        printDataValue(addr, line->getData(), true);

    if (local_prefetch) {
        line->setPrefetch(true);
        if (mem_h_is_debug_event(event))
            event_debuginfo_.action = "Done";
    } else {
        line->addSharer(req->getSrc());
        Addr offset = req->getAddr() - req->getBaseAddr();
        uint64_t send_time = sendResponseUp(req, line->getData(), true, line->getTimestamp());
        line->setTimestamp(send_time-1);
    }

    cleanUpAfterResponse(event, in_mshr);

    if (mem_h_is_debug_addr(addr) && line) {
        event_debuginfo_.newst = line->getState();
        event_debuginfo_.verboseline = line->getString();
    }

    return true;
}


bool MESIInclusive::handleGetXResp(MemEvent * event, bool in_mshr) {
    Addr addr = event->getBaseAddr();
    SharedCacheLine * line = cache_array_->lookup(event->getBaseAddr(), false);
    State state = line ? line->getState() : I;

    if (mem_h_is_debug_event(event))
        event_debuginfo_.prefill(event->getID(), Command::GetXResp, "", addr, state);

    stat_event_state_[(int)Command::GetXResp][state]->addData(1);

    // Get matching request
    MemEvent * req = static_cast<MemEvent*>(mshr_->getFrontEvent(event->getBaseAddr()));
    bool local_prefetch = req->isPrefetch() && (req->getRqstr() == cachename_);
    req->setFlags(event->getMemFlags());

    std::vector<uint8_t> data;
    Addr offset = req->getAddr() - req->getBaseAddr();

    switch (state) {
        case IS:
        {
            line->setData(event->getPayload(), 0);

            if (event->getDirty())  {
                line->setState(M); // Sometimes get dirty data from a noninclusive cache
            } else {
                line->setState(protocol_state_);
            }

            if (mem_h_is_debug_addr(addr))
                printDataValue(addr, line->getData(), true);

            if (local_prefetch) {
                line->setPrefetch(true);
                if (mem_h_is_debug_event(event))
                    event_debuginfo_.action = "Done";
            } else {
                if (protocol_ && line->getState() != S && mshr_->getSize(addr) == 1) {
                    line->setOwner(req->getSrc());
                    uint64_t send_time = sendResponseUp(req, line->getData(), true, line->getTimestamp(), Command::GetXResp);
                    line->setTimestamp(send_time - 1);
                } else {
                    line->addSharer(req->getSrc());
                    uint64_t send_time = sendResponseUp(req, line->getData(), true, line->getTimestamp(), Command::GetSResp);
                    line->setTimestamp(send_time - 1);
                }
            }
            cleanUpAfterResponse(event, in_mshr);
            break;
        }
        case IM:
            line->setData(event->getPayload(), 0);
            if (mem_h_is_debug_addr(line->getAddr()))
                printDataValue(addr, line->getData(), true);
        case SM:
        {
            line->setState(M);
            line->setOwner(req->getSrc());
            if (line->isSharer(req->getSrc()))
                line->removeSharer(req->getSrc());

            uint64_t send_time = sendResponseUp(req, line->getData(), true, line->getTimestamp());
            line->setTimestamp(send_time-1);
            cleanUpAfterResponse(event, in_mshr);
            break;
        }
        case SM_Inv:
            if (mem_h_is_debug_event(event))
                event_debuginfo_.action = "Stall";
            line->setState(M_Inv);
            delete event;
            break;
        default:
            debug_->fatal(CALL_INFO,-1,"%s, Error: Received GetXResp in unhandled state '%s'. Event: %s. Time = %" PRIu64 "ns\n",
                    getName().c_str(), StateString[state], event->getVerboseString().c_str(), getCurrentSimTimeNano());
    }

    if (mem_h_is_debug_addr(addr) && line) {
        event_debuginfo_.newst = line->getState();
        event_debuginfo_.verboseline = line->getString();
    }
    return true;
}


bool MESIInclusive::handleFlushLineResp(MemEvent * event, bool in_mshr) {
    Addr addr = event->getBaseAddr();
    SharedCacheLine * line = cache_array_->lookup(addr, false);
    State state = line ? line->getState() : I;

    if (mem_h_is_debug_event(event))
        event_debuginfo_.prefill(event->getID(), Command::FlushLineResp, "", addr, state);

    stat_event_state_[(int)Command::FlushLineResp][state]->addData(1);

    MemEvent * req = static_cast<MemEvent*>(mshr_->getFrontEvent(addr));

    switch (state) {
        case I:
            break;
        case I_B:
            line->setState(I);
            break;
        case S_B:
            line->setState(S);
            break;
        default:
            debug_->fatal(CALL_INFO,-1,"%s, Error: Received FlushLineResp in unhandled state '%s'. Event: %s. Time = %" PRIu64 "ns\n",
                    getName().c_str(), StateString[state], event->getVerboseString().c_str(), getCurrentSimTimeNano());
    }

    sendResponseUp(req, nullptr, true, timestamp_, Command::FlushLineResp, event->success());

    if (mem_h_is_debug_addr(addr) && line) {
        event_debuginfo_.newst = line->getState();
        event_debuginfo_.verboseline = line->getString();
    }
    cleanUpAfterResponse(event, in_mshr);
    return true;
}


bool MESIInclusive::handleFlushAllResp(MemEvent * event, bool in_mshr) {
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


bool MESIInclusive::handleFetchResp(MemEvent * event, bool in_mshr) {
    Addr addr = event->getBaseAddr();
    SharedCacheLine * line = cache_array_->lookup(addr, false);
    State state = line->getState();

    if (mem_h_is_debug_event(event))
        event_debuginfo_.prefill(event->getID(), Command::FetchResp, "", addr, state);

    stat_event_state_[(int)Command::FetchResp][state]->addData(1);

    // Check acks needed
    mshr_->decrementAcksNeeded(addr);

    // Do invalidation & update data
    state = doEviction(event, line, state);
    responses_.find(addr)->second.erase(event->getSrc());
    if (responses_.find(addr)->second.empty()) responses_.erase(addr);

    if (state == M_Inv) {
        line->setState(M);
    } else {
        line->setState(E);
    }
    retry(addr);

    if (mem_h_is_debug_addr(addr)) {
        event_debuginfo_.action = "Retry";
        if (line) {
            event_debuginfo_.newst = line->getState();
            event_debuginfo_.verboseline = line->getString();
        }
    }

    delete event;
    return true;
}


bool MESIInclusive::handleFetchXResp(MemEvent * event, bool in_mshr) {
    Addr addr = event->getBaseAddr();
    SharedCacheLine * line = cache_array_->lookup(addr, false);
    State state = line->getState();

    if (mem_h_is_debug_addr(addr) && line) {
        event_debuginfo_.newst = line->getState();
        event_debuginfo_.verboseline = line->getString();
    }
    if (mem_h_is_debug_event(event))
        event_debuginfo_.prefill(event->getID(), Command::FetchXResp, "", addr, state);

    stat_event_state_[(int)Command::FetchXResp][state]->addData(1);

    mshr_->decrementAcksNeeded(addr);

    state = doEviction(event, line, state);
    responses_.find(addr)->second.erase(event->getSrc());
    if (responses_.find(addr)->second.empty()) responses_.erase(addr);
    line->addSharer(event->getSrc());

    if (state == M_InvX)
        line->setState(M);
    else
        line->setState(E);

    retry(addr);

    if (mem_h_is_debug_addr(addr)) {
        event_debuginfo_.action = "Retry";
        if (line) {
            event_debuginfo_.newst = line->getState();
            event_debuginfo_.verboseline = line->getString();
        }
    }

    delete event;
    return true;
}


bool MESIInclusive::handleAckFlush(MemEvent * event, bool in_mshr) {
    event_debuginfo_.prefill(event->getID(), Command::AckFlush, "", 0, State::NP);

    mshr_->decrementFlushCount();
    if (mshr_->getFlushCount() == 0) {
        retry_buffer_.push_back(mshr_->getFlush());
    }

    delete event;
    return true;
}

bool MESIInclusive::handleUnblockFlush(MemEvent * event, bool in_mshr) {
    event_debuginfo_.prefill(event->getID(), Command::UnblockFlush, "", 0, State::NP);

    if (flush_helper_) {
        broadcastMemEventToSources(Command::UnblockFlush, event, timestamp_ + 1);
    }
    delete event;

    return true;
}


bool MESIInclusive::handleAckInv(MemEvent * event, bool in_mshr) {
    Addr addr = event->getBaseAddr();
    SharedCacheLine * line = cache_array_->lookup(addr, false);
    State state = line->getState();

    if (mem_h_is_debug_event(event))
        event_debuginfo_.prefill(event->getID(), Command::AckInv, "", addr, state);

    stat_event_state_[(int)Command::AckInv][state]->addData(1);

    if (line->isSharer(event->getSrc()))
        line->removeSharer(event->getSrc());
    else
        line->removeOwner();

    responses_.find(addr)->second.erase(event->getSrc());
    if (responses_.find(addr)->second.empty()) responses_.erase(addr);

    bool done = mshr_->decrementAcksNeeded(addr);
    if (!done) {
        if (mem_h_is_debug_event(event))
            event_debuginfo_.action = "Stall";
        delete event;
        return true;
    }

    switch (state) {
        case S_Inv:
            line->setState(S);
            retry(addr);
            break;
        case E_Inv:
            line->setState(E);
            retry(addr);
            break;
        case M_Inv:
            line->setState(M);
            retry(addr);
            break;
        case SB_Inv:
            line->setState(S_B);
            retry(addr);
            break;
        case SM_Inv:
            line->setState(SM);
            if (!mshr_->getInProgress(addr))
                retry(addr);
            break;
        default:
            debug_->fatal(CALL_INFO,-1,"%s, Error: Received AckInv in unhandled state '%s'. Event: %s. Time = %" PRIu64 "ns\n",
                    getName().c_str(), StateString[state], event->getVerboseString().c_str(), getCurrentSimTimeNano());
    }

    if (mem_h_is_debug_addr(addr)) {
        event_debuginfo_.action = (done ? "Retry" : "DecAcks");
        if (line) {
            event_debuginfo_.newst = line->getState();
            event_debuginfo_.verboseline = line->getString();
        }
    }

    delete event;
    return true;
}


bool MESIInclusive::handleAckPut(MemEvent * event, bool in_mshr) {
    Addr addr = event->getBaseAddr();
    SharedCacheLine * line = cache_array_->lookup(addr, false);
    State state = line ? line->getState() : I;

    if (mem_h_is_debug_event(event)) {
        event_debuginfo_.prefill(event->getID(), Command::AckPut, "", addr, state);
        event_debuginfo_.action = "Done";
    }

    stat_event_state_[(int)Command::AckPut][state]->addData(1);

    cleanUpAfterResponse(event, in_mshr);
    return true;
}


/* We're using NULLCMD to signal an internally generated event - in this case an eviction */
bool MESIInclusive::handleNULLCMD(MemEvent* event, bool in_mshr) {
    Addr oldAddr = event->getAddr();
    Addr newAddr = event->getBaseAddr();

    SharedCacheLine * line = cache_array_->lookup(oldAddr, false);

    bool evicted = handleEviction(newAddr, line);

    if (mem_h_is_debug_addr(newAddr)) {
        event_debuginfo_.prefill(event->getID(), Command::NULLCMD, "", line->getAddr(), evict_debuginfo_.oldst);
        event_debuginfo_.newst = line->getState();
        event_debuginfo_.verboseline = line->getString();
    }
    if (evicted) {
        notifyListenerOfEvict(line->getAddr(), line_size_, event->getInstructionPointer());
        cache_array_->deallocate(line);

        if (oldAddr != newAddr) { /* Reallocating a line to a new address */
            retry_buffer_.push_back(mshr_->getFrontEvent(newAddr));
            mshr_->addPendingRetry(newAddr);
            if (mshr_->removeEvictPointer(oldAddr, newAddr))
                retry(oldAddr);
            if (mem_h_is_debug_addr(newAddr)) {
                event_debuginfo_.action = "Retry";
                std::stringstream reason;
                reason << "0x" << std::hex << newAddr;
                event_debuginfo_.reason = reason.str();
            }
        } else { /* Deallocating a line for a cache flush */
            mshr_->decrementFlushCount();
            if (mshr_->getFlushCount() == 0) {
                retry_buffer_.push_back(mshr_->getFlush());
            }
        }
    } else { // Check if we're waiting for a new address
        if (mem_h_is_debug_addr(newAddr)) {
            event_debuginfo_.action = "Stall";
            std::stringstream reason;
            reason << "0x" << std::hex << newAddr << ", evict failed";
            event_debuginfo_.reason = reason.str();
        }
        if (oldAddr != line->getAddr()) { // We're waiting for a new line now...
            if (mem_h_is_debug_addr(newAddr)) {
                std::stringstream reason;
                reason << "0x" << std::hex << newAddr << ", new targ";
                event_debuginfo_.reason = reason.str();
            }
            mshr_->insertEviction(line->getAddr(), newAddr);
            if (mshr_->removeEvictPointer(oldAddr, newAddr))
                retry(oldAddr);
        }
    }

    delete event;
    return true;
}

bool MESIInclusive::handleNACK(MemEvent * event, bool in_mshr) {
    MemEvent * nackedEvent = event->getNACKedEvent();
    Command cmd = nackedEvent->getCmd();
    Addr addr = nackedEvent->getBaseAddr();

    if (mem_h_is_debug_event(event) || mem_h_is_debug_event(nackedEvent)) {
        SharedCacheLine * line = cache_array_->lookup(addr, false);
        event_debuginfo_.prefill(event->getID(), Command::NACK, "", addr, line ? line->getState() : I);
    }

    delete event;

    switch (cmd) {
        /* These always need to get retried */
        case Command::GetS:
        case Command::GetX:
        case Command::GetSX:
        case Command::FlushLineInv:
        case Command::PutS:
        case Command::PutE:
        case Command::PutM:
        case Command::FlushAll:
            resendEvent(nackedEvent, false); // Resend towards memory
            break;
        /* These *probably* need to get retried but need to handle races with Invs */
        case Command::FlushLine:
            resendEvent(nackedEvent, false);
            break;
        /* These always need to get retried */
        case Command::ForwardFlush:
            resendEvent(nackedEvent, true);
            break;
        /* These get retried unless there's been a race with an eviction/writeback */
        case Command::FetchInv:
        case Command::FetchInvX:
        case Command::Inv:
        case Command::ForceInv:

            if (responses_.find(addr) != responses_.end()
                    && responses_.find(addr)->second.find(nackedEvent->getDst()) != responses_.find(addr)->second.end()
                    && responses_.find(addr)->second.find(nackedEvent->getDst())->second == nackedEvent->getID()) {
                resendEvent(nackedEvent, true); // Resend towards CPU
            } else {
                if (mem_h_is_debug_event(nackedEvent))
                    event_debuginfo_.action = "Drop";
                    //debug_->debug(_L5_, "\tDrop NACKed event: %s\n", nackedEvent->getVerboseString().c_str());
                delete nackedEvent;
            }
            break;
        default:
            debug_->fatal(CALL_INFO,-1,"%s, Error: Received NACK with unhandled command type. Event: %s. Time = %" PRIu64 "ns\n",
                    getName().c_str(), nackedEvent->getVerboseString().c_str(), getCurrentSimTimeNano());
    }
    return true;
}


/***********************************************************************************************************
 * MSHR and CacheArray management
 ***********************************************************************************************************/

MemEventStatus MESIInclusive::processCacheMiss(MemEvent * event, SharedCacheLine * line, bool in_mshr) {
    MemEventStatus status = in_mshr ? MemEventStatus::OK : allocateMSHR(event, false); // Miss means we need an MSHR entry
    if (in_mshr && mshr_->getFrontEvent(event->getBaseAddr()) != event) { // Sometimes happens if eviction and event both retry
        if (mem_h_is_debug_event(event))
            event_debuginfo_.action = "Stall";
        return MemEventStatus::Stall;
    }
    if (status == MemEventStatus::OK && !line) { // Need a cache line too
        line = allocateLine(event, line);
        status = line ? MemEventStatus::OK : MemEventStatus::Stall;
    }
    return status;
}


SharedCacheLine * MESIInclusive::allocateLine(MemEvent * event, SharedCacheLine * line) {
    bool evicted = handleEviction(event->getBaseAddr(), line);
    if (evicted) {
        notifyListenerOfEvict(line->getAddr(), line_size_, event->getInstructionPointer());
        cache_array_->replace(event->getBaseAddr(), line);
        if (mem_h_is_debug_event(event))
            printDebugAlloc(true, event->getBaseAddr(), "");
        return line;
    } else {
        mshr_->insertEviction(line->getAddr(), event->getBaseAddr());
        if (mem_h_is_debug_event(event)) {
            event_debuginfo_.action = "Stall";
            std::stringstream reason;
            reason << "evict 0x" << std::hex << line->getAddr();
            event_debuginfo_.reason = reason.str();
        }
        return nullptr;
    }
}


bool MESIInclusive::handleEviction(Addr addr, SharedCacheLine *& line) {
    if (!line)
        line = cache_array_->findReplacementCandidate(addr);
    State state = line->getState();

    if (mem_h_is_debug_addr(addr) || (line && mem_h_is_debug_addr(line->getAddr())))
        evict_debuginfo_.oldst = state;

    stat_evict_[state]->addData(1);

    bool evict = false;
    bool wbSent = false;
    switch (state) {
        case I:
            return true;
        case S:
            if (!mshr_->getPendingRetries(line->getAddr())) {
                if (invalidateAll(nullptr, line, false)) {
                    evict = false;
                    line->setState(S_Inv);
                    if (mem_h_is_debug_addr(line->getAddr()))
                        printDebugAlloc(false, line->getAddr(), "InProg, S_Inv");
                    break;
                } else if (!silent_evict_clean_) {
                    sendWriteback(Command::PutS, line, false);
                    wbSent = true;
                    if (mem_h_is_debug_addr(line->getAddr()))
                        printDebugAlloc(false, line->getAddr(), "Writeback");
                } else if (mem_h_is_debug_addr(line->getAddr()))
                    printDebugAlloc(false, line->getAddr(), "Drop");
                line->setState(I);
                evict = true;
                break;
            }
        case E:
            if (!mshr_->getPendingRetries(line->getAddr())) {
                if (invalidateAll(nullptr, line, false)) {
                    evict = false;
                    line->setState(E_Inv);
                    if (mem_h_is_debug_addr(line->getAddr()))
                        printDebugAlloc(false, line->getAddr(), "InProg, E_Inv");
                    break;
                } else if (!silent_evict_clean_) {
                    sendWriteback(Command::PutE, line, false);
                    wbSent = true;
                    if (mem_h_is_debug_addr(line->getAddr()))
                        printDebugAlloc(false, line->getAddr(), "Writeback");
                } else if (mem_h_is_debug_addr(line->getAddr()))
                    printDebugAlloc(false, line->getAddr(), "Drop");
                line->setState(I);
                evict = true;
                break;
            }
        case M:
            if (!mshr_->getPendingRetries(line->getAddr())) {
                if (invalidateAll(nullptr, line, false)) {
                    evict = false;
                    line->setState(M_Inv);
                    if (mem_h_is_debug_addr(line->getAddr()))
                        printDebugAlloc(false, line->getAddr(), "InProg, M_Inv");
                } else {
                    sendWriteback(Command::PutM, line, true);
                    wbSent = true;
                    line->setState(I);
                    evict = true;
                    if (mem_h_is_debug_addr(line->getAddr()))
                        printDebugAlloc(false, line->getAddr(), "Writeback");
                }
                break;
            }
        default:
            if (mem_h_is_debug_addr(line->getAddr())) {
                std::stringstream note;
                note << "InProg, " << StateString[state];
                printDebugAlloc(false, line->getAddr(), note.str());
            }
            return false;
    }

    if (wbSent && recv_writeback_ack_) {
        mshr_->insertWriteback(line->getAddr(), false);
    }

    recordPrefetchResult(line, stat_prefetch_evict_);
    return evict;
}


void MESIInclusive::cleanUpEvent(MemEvent* event, bool in_mshr) {
    Addr addr = event->getBaseAddr();
    Command cmd = event->getCmd();

    /* Remove from MSHR */
    if (in_mshr) {
        if (event->isPrefetch() && event->getRqstr() == cachename_) outstanding_prefetch_count_--;
        mshr_->removeFront(addr);
    }

    delete event;
}


void MESIInclusive::cleanUpAfterRequest(MemEvent* event, bool in_mshr) {
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


void MESIInclusive::cleanUpAfterResponse(MemEvent* event, bool in_mshr) {
    Addr addr = event->getBaseAddr();

    /* Clean up MSHR */
    MemEvent * req = nullptr;
    if (mshr_->getFrontType(addr) == MSHREntryType::Event) {
        req = static_cast<MemEvent*>(mshr_->getFrontEvent(addr));
    }
    mshr_->removeFront(addr);
    delete event;
    if (req) {
        if (req->isPrefetch() && req->getRqstr() == cachename_)
            outstanding_prefetch_count_--;
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


void MESIInclusive::retry(Addr addr) {
    if (mshr_->exists(addr)) {
        if (mshr_->getFrontType(addr) == MSHREntryType::Event) {
            //if (mem_h_is_debug_addr(addr))
            //    debug_->debug(_L5_, "    Retry: Waiting Event exists in MSHR and is ready to retry\n");
            retry_buffer_.push_back(mshr_->getFrontEvent(addr));
            mshr_->addPendingRetry(addr);
        } else if (!(mshr_->pendingWriteback(addr))) {
            //if (mem_h_is_debug_addr(addr))
            //    debug_->debug(_L5_, "    Retry: Waiting Evict in MSHR, retrying eviction\n");
            std::list<Addr>* evictPointers = mshr_->getEvictPointers(addr);
            for (std::list<Addr>::iterator it = evictPointers->begin(); it != evictPointers->end(); it++) {
                MemEvent * ev = new MemEvent(cachename_, addr, *it, Command::NULLCMD);
                retry_buffer_.push_back(ev);
            }
        }
    }
}


State MESIInclusive::doEviction(MemEvent * event, SharedCacheLine * line, State state) {
    State nState = state;
    recordPrefetchResult(line, stat_prefetch_evict_);

    if (event->getDirty()) {
        line->setData(event->getPayload(), 0);
        if (mem_h_is_debug_addr(event->getBaseAddr())) {
                printDataValue(event->getBaseAddr(), line->getData(), true);
        }

        switch (state) {
            case E:
                nState = M;
                break;
            case E_Inv:
                nState = M_Inv;
                break;
            case E_InvX:
                nState = M_InvX;
                break;
            default: // Already in a dirty state
                break;
        }
    }
    if (line->getOwner() == event->getSrc())
        line->removeOwner();
    else if (line->isSharer(event->getSrc()))
        line->removeSharer(event->getSrc());

    event->setEvict(false); // Avoid doing an eviction twice if the event gets replayed
    line->setState(nState);
    return nState;
}


/***********************************************************************************************************
 * Event creation and send
 ***********************************************************************************************************/

SimTime_t MESIInclusive::sendResponseUp(MemEvent * event, vector<uint8_t>* data, bool in_mshr, uint64_t time, Command cmd, bool success) {
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

    // Debugging
    if (mem_h_is_debug_event(responseEvent)) {
        event_debuginfo_.action = "Respond";
    }

    return delivery_time;
}


void MESIInclusive::sendResponseDown(MemEvent * event, SharedCacheLine * line, bool data, bool evict) {
    MemEvent * responseEvent = event->makeResponse();

    if (data) {
        responseEvent->setPayload(*line->getData());
        if (line->getState() == M)
            responseEvent->setDirty(true);
    }

    responseEvent->setEvict(evict);

    responseEvent->setSize(line_size_);

    uint64_t delivery_time = timestamp_ + (data ? access_latency_ : tag_latency_);
    forwardByDestination(responseEvent, delivery_time);

    if (mem_h_is_debug_event(responseEvent)) {
        event_debuginfo_.action = "Respond";
    }
}


void MESIInclusive::forwardFlush(MemEvent * event, SharedCacheLine * line, bool evict) {
    MemEvent * flush = new MemEvent(*event);

    uint64_t latency = tag_latency_;
    if (evict) {
        flush->setEvict(true);
        // TODO only send payload when needed
        flush->setPayload(*(line->getData()));
        flush->setDirty(line->getState() == M);
        latency = access_latency_;
    } else {
        flush->setPayload(0, nullptr);
    }
    bool payload = false;

    uint64_t base_time = timestamp_;
    if (line && line->getTimestamp() > base_time) base_time = line->getTimestamp();
    uint64_t delivery_time = base_time + latency;
    forwardByAddress(flush, delivery_time);
    if (line)
        line->setTimestamp(delivery_time-1);

    if (mem_h_is_debug_addr(event->getBaseAddr())) {
        event_debuginfo_.action = "Forward";
    //    debug_->debug(_L3_,"Forwarding Flush at cycle = %" PRIu64 ", Cmd = %s, Src = %s, %s\n", delivery_time, CommandString[(int)flush->getCmd()], flush->getSrc().c_str(), evict ? "with data" : "without data");
    }
}

/*
 *  Handles: sending writebacks
 *  Latency: cache access + tag to read data that is being written back and update coherence state
 */
void MESIInclusive::sendWriteback(Command cmd, SharedCacheLine* line, bool dirty) {
    MemEvent* writeback = new MemEvent(cachename_, line->getAddr(), line->getAddr(), cmd);
    writeback->setSize(line_size_);

    uint64_t latency = tag_latency_;

    /* Writeback data */
    if (dirty || writeback_clean_blocks_) {
        writeback->setPayload(*line->getData());
        writeback->setDirty(dirty);

        if (mem_h_is_debug_addr(line->getAddr())) {
            printDataValue(line->getAddr(), line->getData(), false);
        }

        latency = access_latency_;
    }

    writeback->setRqstr(cachename_);

    uint64_t base_time = (timestamp_ > line->getTimestamp()) ? timestamp_ : line->getTimestamp();
    uint64_t delivery_time = base_time + latency;
    forwardByAddress(writeback, delivery_time);

    line->setTimestamp(delivery_time-1);

   //f (mem_h_is_debug_addr(line->getAddr()))
        //debug_->debug(_L3_,"Sending Writeback at cycle = %" PRIu64 ", Cmd = %s. With%s data.\n", delivery_time, CommandString[(int)cmd], ((cmd == Command::PutM || writeback_clean_blocks_) ? "" : "out"));
}


void MESIInclusive::sendAckPut(MemEvent * event) {
    MemEvent * ack = event->makeResponse();
    ack->copyMetadata(event);
    ack->setSize(event->getSize());

    uint64_t delivery_time = timestamp_ + tag_latency_;
    forwardByDestination(ack, delivery_time);

    if (mem_h_is_debug_event(event))
        event_debuginfo_.action = "Ack";
        //debug_->debug(_L7_, "Sending AckPut at cycle %" PRIu64 "\n", delivery_time);
}


void MESIInclusive::downgradeOwner(MemEvent * event, SharedCacheLine* line, bool in_mshr) {
    Addr addr = event->getBaseAddr();
    MemEvent * fetch = new MemEvent(cachename_, addr, addr, Command::FetchInvX);
    fetch->copyMetadata(event);
    fetch->setDst(line->getOwner());
    fetch->setSize(line_size_);

    mshr_->incrementAcksNeeded(addr);

    if (responses_.find(addr) != responses_.end()) {
        responses_.find(addr)->second.insert(std::make_pair(line->getOwner(), fetch->getID())); // Record events we're waiting for to avoid trying to figure out what happened if we get a NACK
    } else {
        std::map<std::string,MemEvent::id_type> respid;
        respid.insert(std::make_pair(line->getOwner(), fetch->getID()));
        responses_.insert(std::make_pair(addr, respid));
    }

    uint64_t base_time = timestamp_ > line->getTimestamp() ? timestamp_ : line->getTimestamp();
    uint64_t delivery_time = (in_mshr) ? base_time + mshr_latency_ : base_time + tag_latency_;
    forwardByDestination(fetch, delivery_time);

    line->setTimestamp(delivery_time);


    if (mem_h_is_debug_addr(line->getAddr())) {
        event_debuginfo_.action = "Stall";
        event_debuginfo_.reason = "Dgr owner";
        //debug_->debug(_L7_, "Sending %s: Addr = 0x%" PRIx64 ", Dst = %s @ cycles = %" PRIu64 ".\n",
        //        CommandString[(int)cmd], line->getAddr(), line->getOwner().c_str(), delivery_time);
    }
}


bool MESIInclusive::invalidateExceptRequestor(MemEvent * event, SharedCacheLine * line, bool in_mshr) {
    uint64_t delivery_time = 0;
    std::string rqstr = event->getSrc();

    for (set<std::string>::iterator it = line->getSharers()->begin(); it != line->getSharers()->end(); it++) {
        if (*it == rqstr) continue;

        delivery_time =  invalidateSharer(*it, event, line, in_mshr);
    }

    if (delivery_time != 0) line->setTimestamp(delivery_time);

    return delivery_time != 0;
}


bool MESIInclusive::invalidateAll(MemEvent * event, SharedCacheLine * line, bool in_mshr, Command cmd) {
    uint64_t delivery_time = 0;
    if (invalidateOwner(event, line, in_mshr, (cmd == Command::NULLCMD ? Command::FetchInv : cmd))) {
        return true;
    } else {
        if (cmd == Command::NULLCMD)
            cmd = Command::Inv;
        for (std::set<std::string>::iterator it = line->getSharers()->begin(); it != line->getSharers()->end(); it++) {
            delivery_time = invalidateSharer(*it, event, line, in_mshr, cmd);
        }
        if (delivery_time != 0) {
            line->setTimestamp(delivery_time);
            return true;
        }
    }
    return false;
}

uint64_t MESIInclusive::invalidateSharer(std::string shr, MemEvent * event, SharedCacheLine * line, bool in_mshr, Command cmd) {
    if (line->isSharer(shr)) {
        Addr addr = line->getAddr();
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

        uint64_t base_time = timestamp_ > line->getTimestamp() ? timestamp_ : line->getTimestamp();
        uint64_t delivery_time = (in_mshr) ? base_time + mshr_latency_ : base_time + tag_latency_;
        forwardByDestination(inv, delivery_time);

        mshr_->incrementAcksNeeded(addr);

        if (mem_h_is_debug_addr(addr)) {
            event_debuginfo_.action = "Stall";
            event_debuginfo_.reason = "Inv sharer(s)";
        }
        return delivery_time;
    }
    return 0;
}


bool MESIInclusive::invalidateOwner(MemEvent * event, SharedCacheLine * line, bool in_mshr, Command cmd) {
    Addr addr = line->getAddr();
    if (line->getOwner() == "")
        return false;

    MemEvent * inv = new MemEvent(cachename_, addr, addr, cmd);
    if (event) {
        inv->copyMetadata(event);
    } else {
        inv->setRqstr(cachename_);
    }
    inv->setDst(line->getOwner());
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

    uint64_t base_time = timestamp_ > line->getTimestamp() ? timestamp_ : line->getTimestamp();
    uint64_t delivery_time = (in_mshr) ? base_time + mshr_latency_ : base_time + tag_latency_;
    forwardByDestination(inv, delivery_time);
    line->setTimestamp(delivery_time);

    if (mem_h_is_debug_addr(addr)) {
        event_debuginfo_.action = "Stall";
        event_debuginfo_.reason = "Inv owner";
    }
    return true;
}


/*----------------------------------------------------------------------------------------------------------------------
 *  Override message send functions with versions that record statistics & call parent class
 *---------------------------------------------------------------------------------------------------------------------*/

void MESIInclusive::forwardByAddress(MemEventBase* ev, Cycle_t timestamp) {
    stat_event_sent_[(int)ev->getCmd()]->addData(1);
    CoherenceController::forwardByAddress(ev, timestamp);
}

void MESIInclusive::forwardByDestination(MemEventBase* ev, Cycle_t timestamp) {
    stat_event_sent_[(int)ev->getCmd()]->addData(1);
    CoherenceController::forwardByDestination(ev, timestamp);
}

/***********************************************************************************************************
 * Statistics and listeners
 ***********************************************************************************************************/

void MESIInclusive::recordPrefetchResult(SharedCacheLine * line, Statistic<uint64_t> * stat) {
    if (line->getPrefetch()) {
        stat->addData(1);
        line->setPrefetch(false);
    }
}


void MESIInclusive::recordLatency(Command cmd, int type, uint64_t latency) {
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
 * Statistics and listeners
 ***********************************************************************************************************/


 void MESIInclusive::printLine(Addr addr) {
    if (!mem_h_is_debug_addr(addr))
        return;

    SharedCacheLine * line = cache_array_->lookup(addr, false);
    std::string state = (line == nullptr) ? "NP" : line->getString();
    debug_->debug(_L8_, "  Line 0x%" PRIx64 ": %s\n", addr, state.c_str());
}


void MESIInclusive::printStatus(Output &out) {
    cache_array_->printCacheArray(out);
}


/***********************************************************************************************************
 * Cache flush at simulation shutdown
 ***********************************************************************************************************/

 void MESIInclusive::beginCompleteStage() {
    shutdown_flush_counter_ = 0;
    flush_state_ = FlushState::Ready;
}


void MESIInclusive::processCompleteEvent(MemEventInit* event, MemLinkBase* highlink, MemLinkBase* lowlink) {
    if (event->getInitCmd() == MemEventInit::InitCommand::Flush) {
        MemEventUntimedFlush* flush = static_cast<MemEventUntimedFlush*>(event);

        if ( flush->request() ) {
            if ( flush_state_ == FlushState::Ready ) {
                flush_state_ = FlushState::Invalidate;
                std::set<MemLinkBase::EndpointInfo>* src = highlink->getSources();
                shutdown_flush_counter_ += src->size(); // In case distances are nonuniform and we start getting acks before request
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
        for (auto it : *cache_array_) {
            // Only flush dirty data, no need to fix coherence state
            switch (it->getState()) {
                case I:
                case S:
                case E:
                    break;
                case M:
                    {
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
    } else if (event->getCmd() == Command::Write ) {
        SharedCacheLine * line = cache_array_->lookup(event->getAddr(), false);
        line->setData(event->getPayload(), 0);
        line->setState(M); // Force a writeback of this data
    }
    delete event; // Nothing for now
}


/***********************************************************************************************************
 * Miscellaneous
 ***********************************************************************************************************/

MemEventInitCoherence * MESIInclusive::getInitCoherenceEvent() {
    return new MemEventInitCoherence(cachename_, Endpoint::Cache, true /* inclusive */, false /* sends WBAck */, false /* expects WBAck */, line_size_, true /* tracks block presence */);
}

std::set<Command> MESIInclusive::getValidReceiveEvents() {
    std::set<Command> cmds = { Command::GetS,
        Command::GetX,
        Command::GetSX,
        Command::Write,
        Command::FlushLine,
        Command::FlushLineInv,
        Command::FlushAll,
        Command::ForwardFlush,
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
        Command::FlushAllResp,
        Command::FetchResp,
        Command::FetchXResp,
        Command::AckInv,
        Command::AckPut,
        Command::AckFlush,
        Command::UnblockFlush,
        Command::NACK };
    return cmds;
}

void MESIInclusive::serialize_order(SST::Core::Serialization::serializer& ser) {
    CoherenceController::serialize_order(ser);

    SST_SER(cache_array_);
    SST_SER(protocol_state_);
    SST_SER(protocol_);
    SST_SER(responses_);
    SST_SER(flush_state_);
    SST_SER(shutdown_flush_counter_);
    SST_SER(stat_latency_GetS_);
    SST_SER(stat_latency_GetX_);
    SST_SER(stat_latency_GetSX_);
    SST_SER(stat_latency_FlushLine_);
    SST_SER(stat_latency_FlushLineInv_);
    SST_SER(stat_latency_FlushAll_);
    SST_SER(stat_hit_);
    SST_SER(stat_miss_);
    SST_SER(stat_hits_);
    SST_SER(stat_misses_);
}