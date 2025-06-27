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
#include "coherencemgr/MESI_Private_Noninclusive.h"

using namespace SST;
using namespace SST::MemHierarchy;

/* Debug macros included from util.h */

/*----------------------------------------------------------------------------------------------------------------------
 * MESI/MSI Non-Inclusive Coherence Controller for private cache
 *
 * This protocol does not support prefetchers because the cache can't determine whether an address would be a hit
 * in an upper level (closer to cpu) cache.
 *
 *---------------------------------------------------------------------------------------------------------------------*/

/***********************************************************************************************************
 * Constructor
 ***********************************************************************************************************/
MESIPrivNoninclusive::MESIPrivNoninclusive(SST::ComponentId_t id, Params& params, Params& ownerParams, bool prefetch) :
    CoherenceController(id, params, ownerParams, prefetch)
{
    params.insert(ownerParams);
    debug_->debug(_INFO_,"--------------------------- Initializing [MESI Controller] ... \n\n");

    protocol_ = params.find<bool>("protocol", 1);
    if (protocol_) {
        protocolState_ = E;
    } else {
        protocolState_ = S;
    }

    // Cache Array
    uint64_t lines = params.find<uint64_t>("lines");
    uint64_t assoc = params.find<uint64_t>("associativity");

    ReplacementPolicy * rmgr = createReplacementPolicy(lines, assoc, params, false);
    HashFunction * ht = createHashFunction(params);
    cacheArray_ = new CacheArray<PrivateCacheLine>(debug_, lines, assoc, lineSize_, rmgr, ht);
    cacheArray_->setBanked(params.find<uint64_t>("banks", 0));

    flush_state_ = FlushState::Ready;
    shutdown_flush_counter_ = 0;

    stat_evict[I] =      registerStatistic<uint64_t>("evict_I");
    stat_evict[S] =      registerStatistic<uint64_t>("evict_S");
    stat_evict[M] =      registerStatistic<uint64_t>("evict_M");
    stat_evict[SM] =     registerStatistic<uint64_t>("evict_SM");
    stat_evict[I_B] =     registerStatistic<uint64_t>("evict_IB");
    stat_evict[S_B] =     registerStatistic<uint64_t>("evict_SB");
    stat_evict[S_Inv] =   registerStatistic<uint64_t>("evict_SInv");
    stat_evict[M_Inv] =   registerStatistic<uint64_t>("evict_MInv");
    stat_evict[SM_Inv] =  registerStatistic<uint64_t>("evict_SMInv");
    stat_evict[M_InvX] =  registerStatistic<uint64_t>("evict_MInvX");

    stat_eventstate_[(int)Command::GetS][I] =    registerStatistic<uint64_t>("stateEvent_GetS_I");
    stat_eventstate_[(int)Command::GetS][S] =    registerStatistic<uint64_t>("stateEvent_GetS_S");
    stat_eventstate_[(int)Command::GetS][M] =    registerStatistic<uint64_t>("stateEvent_GetS_M");
    stat_eventstate_[(int)Command::GetX][I] =    registerStatistic<uint64_t>("stateEvent_GetX_I");
    stat_eventstate_[(int)Command::GetX][S] =    registerStatistic<uint64_t>("stateEvent_GetX_S");
    stat_eventstate_[(int)Command::GetX][M] =    registerStatistic<uint64_t>("stateEvent_GetX_M");
    stat_eventstate_[(int)Command::GetSX][I] =  registerStatistic<uint64_t>("stateEvent_GetSX_I");
    stat_eventstate_[(int)Command::GetSX][S] =  registerStatistic<uint64_t>("stateEvent_GetSX_S");
    stat_eventstate_[(int)Command::GetSX][M] =  registerStatistic<uint64_t>("stateEvent_GetSX_M");
    stat_eventstate_[(int)Command::GetSResp][I] =        registerStatistic<uint64_t>("stateEvent_GetSResp_I");
    stat_eventstate_[(int)Command::GetXResp][I] =        registerStatistic<uint64_t>("stateEvent_GetXResp_I");
    stat_eventstate_[(int)Command::GetXResp][SM] =       registerStatistic<uint64_t>("stateEvent_GetXResp_SM");
    stat_eventstate_[(int)Command::PutS][I] =        registerStatistic<uint64_t>("stateEvent_PutS_I");
    stat_eventstate_[(int)Command::PutS][S] =        registerStatistic<uint64_t>("stateEvent_PutS_S");
    stat_eventstate_[(int)Command::PutS][M] =        registerStatistic<uint64_t>("stateEvent_PutS_M");
    stat_eventstate_[(int)Command::PutS][M_Inv] =     registerStatistic<uint64_t>("stateEvent_PutS_MInv");
    stat_eventstate_[(int)Command::PutS][S_Inv] =     registerStatistic<uint64_t>("stateEvent_PutS_SInv");
    stat_eventstate_[(int)Command::PutM][I] =        registerStatistic<uint64_t>("stateEvent_PutM_I");
    stat_eventstate_[(int)Command::PutM][M] =        registerStatistic<uint64_t>("stateEvent_PutM_M");
    stat_eventstate_[(int)Command::PutM][M_Inv] =     registerStatistic<uint64_t>("stateEvent_PutM_MInv");
    stat_eventstate_[(int)Command::PutM][M_InvX] =    registerStatistic<uint64_t>("stateEvent_PutM_MInvX");
    stat_eventstate_[(int)Command::PutX][I] =        registerStatistic<uint64_t>("stateEvent_PutX_I");
    stat_eventstate_[(int)Command::PutX][M] =        registerStatistic<uint64_t>("stateEvent_PutX_M");
    stat_eventstate_[(int)Command::PutX][M_Inv] =     registerStatistic<uint64_t>("stateEvent_PutX_MInv");
    stat_eventstate_[(int)Command::PutX][M_InvX] =    registerStatistic<uint64_t>("stateEvent_PutX_MInvX");
    stat_eventstate_[(int)Command::Inv][I] =         registerStatistic<uint64_t>("stateEvent_Inv_I");
    stat_eventstate_[(int)Command::Inv][S] =         registerStatistic<uint64_t>("stateEvent_Inv_S");
    stat_eventstate_[(int)Command::Inv][SM] =        registerStatistic<uint64_t>("stateEvent_Inv_SM");
    stat_eventstate_[(int)Command::Inv][S_B] =        registerStatistic<uint64_t>("stateEvent_Inv_SB");
    stat_eventstate_[(int)Command::Inv][I_B] =        registerStatistic<uint64_t>("stateEvent_Inv_IB");
    stat_eventstate_[(int)Command::FetchInvX][I] =       registerStatistic<uint64_t>("stateEvent_FetchInvX_I");
    stat_eventstate_[(int)Command::FetchInvX][M] =       registerStatistic<uint64_t>("stateEvent_FetchInvX_M");
    stat_eventstate_[(int)Command::FetchInvX][I_B] =      registerStatistic<uint64_t>("stateEvent_FetchInvX_IB");
    stat_eventstate_[(int)Command::FetchInvX][S_B] =      registerStatistic<uint64_t>("stateEvent_FetchInvX_SB");
    stat_eventstate_[(int)Command::Fetch][I] =           registerStatistic<uint64_t>("stateEvent_Fetch_I");
    stat_eventstate_[(int)Command::Fetch][S] =           registerStatistic<uint64_t>("stateEvent_Fetch_S");
    stat_eventstate_[(int)Command::Fetch][SM] =          registerStatistic<uint64_t>("stateEvent_Fetch_SM");
    stat_eventstate_[(int)Command::Fetch][S_Inv] =        registerStatistic<uint64_t>("stateEvent_Fetch_SInv");
    stat_eventstate_[(int)Command::Fetch][I_B] =          registerStatistic<uint64_t>("stateEvent_Fetch_IB");
    stat_eventstate_[(int)Command::Fetch][S_B] =          registerStatistic<uint64_t>("stateEvent_Fetch_SB");
    stat_eventstate_[(int)Command::FetchInv][I] =        registerStatistic<uint64_t>("stateEvent_FetchInv_I");
    stat_eventstate_[(int)Command::FetchInv][S] =        registerStatistic<uint64_t>("stateEvent_FetchInv_S");
    stat_eventstate_[(int)Command::FetchInv][M] =        registerStatistic<uint64_t>("stateEvent_FetchInv_M");
    stat_eventstate_[(int)Command::FetchInv][SM] =       registerStatistic<uint64_t>("stateEvent_FetchInv_SM");
    stat_eventstate_[(int)Command::FetchInv][I_B] =      registerStatistic<uint64_t>("stateEvent_FetchInv_IB");
    stat_eventstate_[(int)Command::FetchInv][S_B] =      registerStatistic<uint64_t>("stateEvent_FetchInv_SB");
    stat_eventstate_[(int)Command::ForceInv][I] =        registerStatistic<uint64_t>("stateEvent_ForceInv_I");
    stat_eventstate_[(int)Command::ForceInv][S] =        registerStatistic<uint64_t>("stateEvent_ForceInv_S");
    stat_eventstate_[(int)Command::ForceInv][M] =        registerStatistic<uint64_t>("stateEvent_ForceInv_M");
    stat_eventstate_[(int)Command::ForceInv][SM] =       registerStatistic<uint64_t>("stateEvent_ForceInv_SM");
    stat_eventstate_[(int)Command::ForceInv][I_B] =      registerStatistic<uint64_t>("stateEvent_ForceInv_IB");
    stat_eventstate_[(int)Command::ForceInv][S_B] =      registerStatistic<uint64_t>("stateEvent_ForceInv_SB");
    stat_eventstate_[(int)Command::ForceInv][SM_Inv] =   registerStatistic<uint64_t>("stateEvent_ForceInv_SMInv");
    stat_eventstate_[(int)Command::FetchResp][I] =       registerStatistic<uint64_t>("stateEvent_FetchResp_I");
    stat_eventstate_[(int)Command::FetchResp][M_Inv] =   registerStatistic<uint64_t>("stateEvent_FetchResp_MInv");
    stat_eventstate_[(int)Command::FetchXResp][I] =      registerStatistic<uint64_t>("stateEvent_FetchXResp_I");
    stat_eventstate_[(int)Command::FetchXResp][M_InvX] = registerStatistic<uint64_t>("stateEvent_FetchXResp_MInvX");
    stat_eventstate_[(int)Command::AckInv][I] =          registerStatistic<uint64_t>("stateEvent_AckInv_I");
    stat_eventstate_[(int)Command::AckInv][M_Inv] =      registerStatistic<uint64_t>("stateEvent_AckInv_MInv");
    stat_eventstate_[(int)Command::AckInv][S_Inv] =      registerStatistic<uint64_t>("stateEvent_AckInv_SInv");
    stat_eventstate_[(int)Command::AckInv][SM_Inv] =     registerStatistic<uint64_t>("stateEvent_AckInv_SMInv");
    stat_eventstate_[(int)Command::AckInv][SB_Inv] =     registerStatistic<uint64_t>("stateEvent_AckInv_SBInv");
    stat_eventstate_[(int)Command::FlushLine][I] =       registerStatistic<uint64_t>("stateEvent_FlushLine_I");
    stat_eventstate_[(int)Command::FlushLine][S] =       registerStatistic<uint64_t>("stateEvent_FlushLine_S");
    stat_eventstate_[(int)Command::FlushLine][M] =       registerStatistic<uint64_t>("stateEvent_FlushLine_M");
    stat_eventstate_[(int)Command::FlushLineInv][I] =        registerStatistic<uint64_t>("stateEvent_FlushLineInv_I");
    stat_eventstate_[(int)Command::FlushLineInv][S] =        registerStatistic<uint64_t>("stateEvent_FlushLineInv_S");
    stat_eventstate_[(int)Command::FlushLineInv][M] =        registerStatistic<uint64_t>("stateEvent_FlushLineInv_M");
    stat_eventstate_[(int)Command::FlushLineResp][I] =       registerStatistic<uint64_t>("stateEvent_FlushLineResp_I");
    stat_eventstate_[(int)Command::FlushLineResp][I_B] =     registerStatistic<uint64_t>("stateEvent_FlushLineResp_IB");
    stat_eventstate_[(int)Command::FlushLineResp][S_B] =     registerStatistic<uint64_t>("stateEvent_FlushLineResp_SB");
    stat_eventSent[(int)Command::GetS]          = registerStatistic<uint64_t>("eventSent_GetS");
    stat_eventSent[(int)Command::GetX]          = registerStatistic<uint64_t>("eventSent_GetX");
    stat_eventSent[(int)Command::GetSX]         = registerStatistic<uint64_t>("eventSent_GetSX");
    stat_eventSent[(int)Command::Write]         = registerStatistic<uint64_t>("eventSent_Write");
    stat_eventSent[(int)Command::PutS]          = registerStatistic<uint64_t>("eventSent_PutS");
    stat_eventSent[(int)Command::PutM]          = registerStatistic<uint64_t>("eventSent_PutM");
    stat_eventSent[(int)Command::PutX]          = registerStatistic<uint64_t>("eventSent_PutX");
    stat_eventSent[(int)Command::FlushLine]     = registerStatistic<uint64_t>("eventSent_FlushLine");
    stat_eventSent[(int)Command::FlushLineInv]  = registerStatistic<uint64_t>("eventSent_FlushLineInv");
    stat_eventSent[(int)Command::FlushAll]      = registerStatistic<uint64_t>("eventSent_FlushAll");
    stat_eventSent[(int)Command::FlushAllResp]  = registerStatistic<uint64_t>("eventSent_FlushAllResp");
    stat_eventSent[(int)Command::ForwardFlush]  = registerStatistic<uint64_t>("eventSent_ForwardFlush");
    stat_eventSent[(int)Command::UnblockFlush]  = registerStatistic<uint64_t>("eventSent_UnblockFlush");
    stat_eventSent[(int)Command::AckFlush]      = registerStatistic<uint64_t>("eventSent_AckFlush");
    stat_eventSent[(int)Command::FetchResp]     = registerStatistic<uint64_t>("eventSent_FetchResp");
    stat_eventSent[(int)Command::FetchXResp]    = registerStatistic<uint64_t>("eventSent_FetchXResp");
    stat_eventSent[(int)Command::AckInv]        = registerStatistic<uint64_t>("eventSent_AckInv");
    stat_eventSent[(int)Command::GetSResp]      = registerStatistic<uint64_t>("eventSent_GetSResp");
    stat_eventSent[(int)Command::GetXResp]      = registerStatistic<uint64_t>("eventSent_GetXResp");
    stat_eventSent[(int)Command::WriteResp]     = registerStatistic<uint64_t>("eventSent_WriteResp");
    stat_eventSent[(int)Command::FlushLineResp] = registerStatistic<uint64_t>("eventSent_FlushLineResp");
    stat_eventSent[(int)Command::Fetch]         = registerStatistic<uint64_t>("eventSent_Fetch");
    stat_eventSent[(int)Command::FetchInv]      = registerStatistic<uint64_t>("eventSent_FetchInv");
    stat_eventSent[(int)Command::FetchInvX]     = registerStatistic<uint64_t>("eventSent_FetchInvX");
    stat_eventSent[(int)Command::ForceInv]      = registerStatistic<uint64_t>("eventSent_ForceInv");
    stat_eventSent[(int)Command::Inv]           = registerStatistic<uint64_t>("eventSent_Inv");
    stat_eventSent[(int)Command::NACK]          = registerStatistic<uint64_t>("eventSent_NACK");
    stat_eventSent[(int)Command::AckPut]        = registerStatistic<uint64_t>("eventSent_AckPut");
    stat_eventSent[(int)Command::Put]           = registerStatistic<uint64_t>("eventSent_Put");
    stat_eventSent[(int)Command::Get]           = registerStatistic<uint64_t>("eventSent_Get");
    stat_eventSent[(int)Command::AckMove]       = registerStatistic<uint64_t>("eventSent_AckMove");
    stat_eventSent[(int)Command::CustomReq]     = registerStatistic<uint64_t>("eventSent_CustomReq");
    stat_eventSent[(int)Command::CustomResp]    = registerStatistic<uint64_t>("eventSent_CustomResp");
    stat_eventSent[(int)Command::CustomAck]     = registerStatistic<uint64_t>("eventSent_CustomAck");
    stat_latencyGetS[LatType::HIT]      = registerStatistic<uint64_t>("latency_GetS_hit");
    stat_latencyGetS[LatType::MISS]     = registerStatistic<uint64_t>("latency_GetS_miss");
    stat_latencyGetS[LatType::INV]      = registerStatistic<uint64_t>("latency_GetS_inv");
    stat_latencyGetX[LatType::HIT]      = registerStatistic<uint64_t>("latency_GetX_hit");
    stat_latencyGetX[LatType::MISS]     = registerStatistic<uint64_t>("latency_GetX_miss");
    stat_latencyGetX[LatType::INV]      = registerStatistic<uint64_t>("latency_GetX_inv");
    stat_latencyGetX[LatType::UPGRADE]  = registerStatistic<uint64_t>("latency_GetX_upgrade");
    stat_latencyGetSX[LatType::HIT]     = registerStatistic<uint64_t>("latency_GetSX_hit");
    stat_latencyGetSX[LatType::MISS]    = registerStatistic<uint64_t>("latency_GetSX_miss");
    stat_latencyGetSX[LatType::INV]     = registerStatistic<uint64_t>("latency_GetSX_inv");
    stat_latencyGetSX[LatType::UPGRADE] = registerStatistic<uint64_t>("latency_GetSX_upgrade");
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

    /* Only for caches that expect writeback acks but we don't know yet so always enabled for now (can't register statistics later) */
    stat_eventstate_[(int)Command::AckPut][I] = registerStatistic<uint64_t>("stateEvent_AckPut_I");

    /* MESI-specific statistics (as opposed to MSI) */
    if (protocol_) {
        stat_evict[E] =         registerStatistic<uint64_t>("evict_E");
        stat_evict[E_Inv] =     registerStatistic<uint64_t>("evict_EInv");
        stat_evict[E_InvX] =    registerStatistic<uint64_t>("evict_EInvX");
        stat_eventstate_[(int)Command::GetS][E] =    registerStatistic<uint64_t>("stateEvent_GetS_E");
        stat_eventstate_[(int)Command::GetX][E] =    registerStatistic<uint64_t>("stateEvent_GetX_E");
        stat_eventstate_[(int)Command::GetSX][E] =  registerStatistic<uint64_t>("stateEvent_GetSX_E");
        stat_eventstate_[(int)Command::PutS][E] =        registerStatistic<uint64_t>("stateEvent_PutS_E");
        stat_eventstate_[(int)Command::PutS][E_Inv] =     registerStatistic<uint64_t>("stateEvent_PutS_EInv");
        stat_eventstate_[(int)Command::PutE][I] =        registerStatistic<uint64_t>("stateEvent_PutE_I");
        stat_eventstate_[(int)Command::PutE][E] =        registerStatistic<uint64_t>("stateEvent_PutE_E");
        stat_eventstate_[(int)Command::PutE][M] =        registerStatistic<uint64_t>("stateEvent_PutE_M");
        stat_eventstate_[(int)Command::PutE][M_Inv] =     registerStatistic<uint64_t>("stateEvent_PutE_MInv");
        stat_eventstate_[(int)Command::PutE][M_InvX] =    registerStatistic<uint64_t>("stateEvent_PutE_MInvX");
        stat_eventstate_[(int)Command::PutE][E_Inv] =     registerStatistic<uint64_t>("stateEvent_PutE_EInv");
        stat_eventstate_[(int)Command::PutE][E_InvX] =    registerStatistic<uint64_t>("stateEvent_PutE_EInvX");
        stat_eventstate_[(int)Command::PutM][E] =        registerStatistic<uint64_t>("stateEvent_PutM_E");
        stat_eventstate_[(int)Command::PutM][E_Inv] =     registerStatistic<uint64_t>("stateEvent_PutM_EInv");
        stat_eventstate_[(int)Command::PutM][E_InvX] =    registerStatistic<uint64_t>("stateEvent_PutM_EInvX");
        stat_eventstate_[(int)Command::PutX][E] =        registerStatistic<uint64_t>("stateEvent_PutX_E");
        stat_eventstate_[(int)Command::PutX][E_Inv] =     registerStatistic<uint64_t>("stateEvent_PutX_EInv");
        stat_eventstate_[(int)Command::PutX][E_InvX] =    registerStatistic<uint64_t>("stateEvent_PutX_EInvX");
        stat_eventstate_[(int)Command::FetchInvX][E] =       registerStatistic<uint64_t>("stateEvent_FetchInvX_E");
        stat_eventstate_[(int)Command::FetchInv][E] =        registerStatistic<uint64_t>("stateEvent_FetchInv_E");
        stat_eventstate_[(int)Command::ForceInv][E] =        registerStatistic<uint64_t>("stateEvent_ForceInv_E");
        stat_eventstate_[(int)Command::FetchResp][E_Inv] =    registerStatistic<uint64_t>("stateEvent_FetchResp_EInv");
        stat_eventstate_[(int)Command::FetchXResp][E_InvX] =  registerStatistic<uint64_t>("stateEvent_FetchXResp_EInvX");
        stat_eventstate_[(int)Command::AckInv][E_Inv] =       registerStatistic<uint64_t>("stateEvent_AckInv_EInv");
        stat_eventstate_[(int)Command::FlushLine][E] =       registerStatistic<uint64_t>("stateEvent_FlushLine_E");
        stat_eventstate_[(int)Command::FlushLineInv][E] =        registerStatistic<uint64_t>("stateEvent_FlushLineInv_E");
        stat_eventSent[(int)Command::PutE] = registerStatistic<uint64_t>("eventSent_PutE");
    }

    recvWritebackAck_ = true;
    sendWritebackAck_ = true;
}

bool MESIPrivNoninclusive::handleGetS(MemEvent* event, bool inMSHR) {
    Addr addr = event->getBaseAddr();
    PrivateCacheLine * line = cacheArray_->lookup(addr, true);
    State state = line ? line->getState() : I;
    uint64_t sendTime = 0;
    MemEventStatus status = MemEventStatus::OK;
    vector<uint8_t> data;
    Command respcmd;
    if (mem_h_is_debug_event(event))
        event_debuginfo_.prefill(event->getID(), Command::GetS, "", addr, state);

    if (inMSHR)
        mshr_->removePendingRetry(addr);

    switch (state) {
        case I:
            status = inMSHR ? MemEventStatus::OK : allocateMSHR(event, false);

            if (status == MemEventStatus::OK) {
                if (!mshr_->getProfiled(addr)) {
                    stat_eventstate_[(int)Command::GetS][I]->addData(1);
                    stat_miss[0][inMSHR]->addData(1);
                    stat_misses->addData(1);
                    notifyListenerOfAccess(event, NotifyAccessType::READ, NotifyResultType::MISS);
                    mshr_->setProfiled(addr);
                }
                recordLatencyType(event->getID(), LatType::MISS);
                sendTime = forwardMessage(event, lineSize_, 0, nullptr);
                mshr_->setInProgress(addr);

                if (mem_h_is_debug_event(event))
                    event_debuginfo_.reason = "miss";
            }
            break;
        case S:
            if (!inMSHR || !mshr_->getProfiled(addr)) {
                stat_eventstate_[(int)Command::GetS][S]->addData(1);
                stat_hit[0][inMSHR]->addData(1);
                stat_hits->addData(1);
                notifyListenerOfAccess(event, NotifyAccessType::READ, NotifyResultType::HIT);
            }
            line->setShared(true);
            sendTime = sendResponseUp(event, line->getData(), inMSHR, line->getTimestamp());
            recordLatencyType(event->getID(), LatType::HIT);
            line->setTimestamp(sendTime);
            if (mem_h_is_debug_event(event))
                event_debuginfo_.reason = "hit";
            cleanUpAfterRequest(event, inMSHR);
            break;
        case E:
        case M:
            if (!inMSHR || !mshr_->getProfiled(addr)) {
                stat_eventstate_[(int)Command::GetS][state]->addData(1);
                stat_hit[0][inMSHR]->addData(1);
                stat_hits->addData(1);
                notifyListenerOfAccess(event, NotifyAccessType::READ, NotifyResultType::HIT);
            }
            if (mem_h_is_debug_event(event))
                event_debuginfo_.reason = "hit";
            if (protocol_) { // Transfer ownership of dirty block
                line->setOwned(true);
                sendTime = sendExclusiveResponse(event, line->getData(), inMSHR, line->getTimestamp(), state == M);
            } else { // Will writeback dirty block if we evict
                line->setShared(true);
                sendTime = sendResponseUp(event, line->getData(), inMSHR, line->getTimestamp(), Command::GetSResp);
            }
            recordLatencyType(event->getID(), LatType::HIT);
            line->setTimestamp(sendTime);
            cleanUpAfterRequest(event, inMSHR);
            break;
        default:
            if (!inMSHR)
                status = allocateMSHR(event, false);
            else if (mem_h_is_debug_event(event))
                event_debuginfo_.action = "Stall";
            break;
    }

    if (status == MemEventStatus::Reject)
        sendNACK(event);

    if (mem_h_is_debug_addr(addr) && line) {
        event_debuginfo_.newst = line->getState();
        event_debuginfo_.verboseline = line->getString();
    }

    return true;
}


bool MESIPrivNoninclusive::handleGetX(MemEvent* event, bool inMSHR) {
    Addr addr = event->getBaseAddr();
    PrivateCacheLine * line = cacheArray_->lookup(addr, true);
    State state = line ? line->getState() : I;

    MemEventStatus status = MemEventStatus::OK;
    uint64_t sendTime = 0;

    if (mem_h_is_debug_event(event))
        event_debuginfo_.prefill(event->getID(), event->getCmd(), "", addr, state);

    if (inMSHR)
        mshr_->removePendingRetry(addr);

    if (state == S && lastLevel_) { // Special case, silent upgrade
        state = M;
        line->setState(M);
    }

    switch (state) {
        case I:
        case S:
            status = inMSHR ? MemEventStatus::OK : allocateMSHR(event, false);
            if (status == MemEventStatus::OK) {
                if (!mshr_->getProfiled(addr)) {
                    stat_eventstate_[(int)event->getCmd()][state]->addData(1);
                    stat_miss[(event->getCmd() == Command::GetX ? 1 : 2)][inMSHR]->addData(1);
                    stat_misses->addData(1);
                    notifyListenerOfAccess(event, NotifyAccessType::WRITE, NotifyResultType::MISS);
                    mshr_->setProfiled(addr);
                }
                sendTime = forwardMessage(event, lineSize_, 0, nullptr);
                mshr_->setInProgress(addr);
                if (state == S) {
                    recordLatencyType(event->getID(), LatType::UPGRADE);
                    line->setState(SM);
                    line->setTimestamp(sendTime);
                } else {
                    recordLatencyType(event->getID(), LatType::MISS);
                }
                if (mem_h_is_debug_event(event))
                    event_debuginfo_.reason = "miss";
            }
            break;
        case E:
            line->setState(M);
        case M:
            if (!inMSHR || !mshr_->getProfiled(addr)) {
                stat_eventstate_[(int)event->getCmd()][state]->addData(1);
                stat_hit[(event->getCmd() == Command::GetX ? 1 : 2)][inMSHR]->addData(1);
                stat_hits->addData(1);
                notifyListenerOfAccess(event, NotifyAccessType::WRITE, NotifyResultType::HIT);
            }
            line->setOwned(true);
            line->setShared(false);
            sendTime = sendExclusiveResponse(event, line->getData(), inMSHR, line->getTimestamp(), true);
            line->setTimestamp(sendTime);
            recordLatencyType(event->getID(), LatType::HIT);
            if (mem_h_is_debug_event(event))
                event_debuginfo_.reason = "hit";
            cleanUpAfterRequest(event, inMSHR);
            break;
        default:
            if (!inMSHR) {
                status = allocateMSHR(event, false);
            } else if (mem_h_is_debug_event(event))
                event_debuginfo_.action = "Stall";
    }

    if (status == MemEventStatus::Reject)
        sendNACK(event);

    if (mem_h_is_debug_addr(addr) && line) {
        event_debuginfo_.newst = line->getState();
        event_debuginfo_.verboseline = line->getString();
    }

    return true;
}


bool MESIPrivNoninclusive::handleGetSX(MemEvent* event, bool inMSHR) {
    return handleGetX(event, inMSHR);
}


bool MESIPrivNoninclusive::handleFlushLine(MemEvent * event, bool inMSHR) {
    Addr addr = event->getBaseAddr();
    PrivateCacheLine * line = cacheArray_->lookup(addr, false);
    State state = line ? line->getState() : I;

    if (mem_h_is_debug_event(event))
        event_debuginfo_.prefill(event->getID(), Command::FlushLine, "", addr, state);

    if (inMSHR)
        mshr_->removePendingRetry(addr);

    MemEventStatus status = inMSHR ? MemEventStatus::OK : allocateMSHR(event, false);

    recordLatencyType(event->getID(), LatType::HIT);

    uint64_t sendTime;

    switch (state) {
        case I:
            if (status == MemEventStatus::OK) {
                forwardFlush(event, event->getEvict(), &(event->getPayload()), event->getDirty(), 0);
                event->setEvict(false);
                mshr_->setInProgress(addr);
                if (!mshr_->getProfiled(addr)) {
                    stat_eventstate_[(int)Command::FlushLine][I]->addData(1);
                    mshr_->setProfiled(addr);
                }
            } else if (mshr_->getAcksNeeded(addr) != 0 && event->getEvict()) {
                mshr_->setData(addr, event->getPayload(), event->getDirty());
                event->setEvict(false);
                if ((static_cast<MemEvent*>(mshr_->getFrontEvent(addr)))->getCmd() == Command::FetchInvX) {
                    responses.erase(addr);
                    mshr_->decrementAcksNeeded(addr);
                    retry(addr);
                }
            }
            break;
        case S:
            if (status == MemEventStatus::OK) {
                sendTime = forwardFlush(event, false, nullptr, false, line->getTimestamp());
                line->setTimestamp(sendTime);
                line->setState(S_B);
                mshr_->setInProgress(addr);
                if (!mshr_->getProfiled(addr)) {
                    stat_eventstate_[(int)Command::FlushLine][S]->addData(1);
                    mshr_->setProfiled(addr);
                }
            }
            break;
        case E:
        case M:
            if (status == MemEventStatus::OK) {
                if (event->getEvict()) {
                    line->setOwned(false);
                    line->setShared(true);
                    if (event->getDirty()) {
                        line->setData(event->getPayload(), 0);
                        if (mem_h_is_debug_addr(addr))
                            printDataValue(line->getAddr(), line->getData(), true);
                    }
                    event->setEvict(false);
                }
                forwardFlush(event, true, line->getData(), (state == M || event->getDirty()), line->getTimestamp());
                line->setState(S_B);
                mshr_->setInProgress(addr);
                if (!mshr_->getProfiled(addr)) {
                    stat_eventstate_[(int)Command::FlushLine][state]->addData(1);
                    mshr_->setProfiled(addr);
                }
            }
            break;
        case E_Inv:
        case M_Inv:
            if (event->getEvict()) {
                line->setOwned(false);
                line->setShared(true);
                if (event->getDirty()) {
                    line->setData(event->getPayload(), 0);
                    if (mem_h_is_debug_addr(addr))
                        printDataValue(line->getAddr(), line->getData(), true);
                    line->setState(M_Inv);
                }
                event->setEvict(false);
            }
            break;
        case E_InvX:
        case M_InvX:
            state == E_InvX ? line->setState(E) : line->setState(M);
            line->setOwned(false);
            line->setShared(true);
            if (event->getDirty()) {
                line->setData(event->getPayload(), 0);
                if (mem_h_is_debug_addr(addr))
                    printDataValue(line->getAddr(), line->getData(), true);
                line->setState(M_Inv);
            }
            event->setEvict(false);
            mshr_->decrementAcksNeeded(addr);
            responses.erase(addr);
            retry(addr);
            break;
        default:
            break;
    }

    if (status == MemEventStatus::Reject)
        sendNACK(event);

    if (mem_h_is_debug_event(event) && line) {
        event_debuginfo_.newst = line->getState();
        event_debuginfo_.verboseline = line->getString();
    }

    return true;
}


bool MESIPrivNoninclusive::handleFlushLineInv(MemEvent * event, bool inMSHR) {
    Addr addr = event->getBaseAddr();
    PrivateCacheLine * line = cacheArray_->lookup(addr, false);
    State state = line ? line->getState() : I;

    if (mem_h_is_debug_event(event))
        event_debuginfo_.prefill(event->getID(), Command::FlushLineInv, "", addr, state);

    if (inMSHR)
        mshr_->removePendingRetry(addr);

    MemEventStatus status = inMSHR ? MemEventStatus::OK : allocateMSHR(event, false);

    recordLatencyType(event->getID(), LatType::HIT);

    uint64_t sendTime;

    switch (state) {
        case I:
            if (inMSHR && mshr_->getInProgress(addr))
                break; // Triggered an unneccessary retry
            if (status == MemEventStatus::OK) {
                forwardFlush(event, event->getEvict(), &(event->getPayload()), event->getDirty(), 0); // No need to evict since we didn't race
                mshr_->setInProgress(addr);
                if (!mshr_->getProfiled(addr)) {
                    stat_eventstate_[(int)Command::FlushLineInv][I]->addData(1);
                    mshr_->setProfiled(addr);
                }
            } else if (event->getEvict()) {
                MemEventBase * race;
                if (mshr_->getFrontEvent(addr)) { // There's an event with raced with
                    race = mshr_->getFrontEvent(addr);
                    mshr_->decrementAcksNeeded(addr);
                    responses.erase(addr);
                    retry(addr);
                    if (mem_h_is_debug_event(event))
                        event_debuginfo_.action = "Retry";
                } else if (status == MemEventStatus::Stall && mshr_->pendingWritebackIsDowngrade(addr) && mshr_->getEntryEvent(addr, 1)) { // There's an event we *will* race with
                    // In this case this cache issued a downgrade (PutX), an invalidation was received & stalled pending the AckPut and now we've got a flushlineinv
                    // Resolve race with the invalidation
                    race = mshr_->getEntryEvent(addr, 1);
                } else
                    break;

                // Copy data in and update state to resolve race with conflicting event
                mshr_->setData(addr, event->getPayload(), event->getDirty());
                if (race->getCmd() == Command::FetchInvX) {
                    event->setDirty(false);
                } else if (race->getCmd() != Command::Fetch) { // FetchInv, ForceInv, or Inv
                    event->setDirty(false);
                    event->setEvict(false);
                }
            }
            /* Note: If there's a WB-X followed by a FetchInv/Inv/etc. in the MSHR, the FetchInv will get replayed when the WB-X resolves
             * and will not forward since we copied the evict data into the MSHR.
             */
            break;
        case S:
            if (status == MemEventStatus::OK) {
                if (event->getEvict())
                    line->setShared(false);
                line->setState(I_B);
                forwardFlush(event, true, line->getData(), false, line->getTimestamp());
                mshr_->setInProgress(addr);
                if (!mshr_->getProfiled(addr)) {
                    stat_eventstate_[(int)Command::FlushLineInv][S]->addData(1);
                    mshr_->setProfiled(addr);
                }
            }
            break;
        case E:
        case M:
            if (status == MemEventStatus::OK) {
                if (event->getEvict()) {
                    line->setOwned(false);
                    line->setShared(false);
                    if (event->getDirty()) {
                        line->setData(event->getPayload(), 0);
                        line->setState(M);
                        if (mem_h_is_debug_addr(addr))
                            printDataValue(line->getAddr(), line->getData(), true);
                    }
                }
                forwardFlush(event, true, line->getData(), line->getState() == M, line->getTimestamp());
                line->setState(I_B);
                mshr_->setInProgress(addr);
                if (!mshr_->getProfiled(addr)) {
                    stat_eventstate_[(int)Command::FlushLineInv][state]->addData(1);
                    mshr_->setProfiled(addr);
                }
            }
            break;
        case S_Inv:
            line->setShared(false);
            line->setState(S);
            event->setEvict(false);
            mshr_->decrementAcksNeeded(addr);
            responses.erase(addr);
            retry(addr);
            if (mem_h_is_debug_event(event))
                event_debuginfo_.action = "Retry";
            break;
        case E_Inv:
        case E_InvX:
            line->setOwned(false);
            line->setShared(false);
            if (event->getDirty()) {
                line->setData(event->getPayload(), 0);
                line->setState(M);
                if (mem_h_is_debug_addr(addr))
                    printDataValue(line->getAddr(), line->getData(), true);
            } else {
                line->setState(E);
            }
            event->setEvict(false);
            mshr_->decrementAcksNeeded(addr);
            responses.erase(addr);
            retry(addr);
            if (mem_h_is_debug_event(event))
                event_debuginfo_.action = "Retry";
            break;
        case M_Inv:
        case M_InvX:
            line->setOwned(false);
            line->setShared(false);
            if (event->getDirty()) {
                line->setData(event->getPayload(), 0);
                if (mem_h_is_debug_addr(addr))
                    printDataValue(line->getAddr(), line->getData(), true);
            }
            event->setEvict(false);
            mshr_->decrementAcksNeeded(addr);
            responses.erase(addr);
            retry(addr);
            if (mem_h_is_debug_event(event))
                event_debuginfo_.action = "Retry";
            break;
        default:
            if (inMSHR && mem_h_is_debug_event(event))
                event_debuginfo_.action = "Stall";
            break;
    }

    if (status == MemEventStatus::Reject)
        sendNACK(event);

    if (mem_h_is_debug_event(event) && line) {
        event_debuginfo_.newst = line->getState();
        event_debuginfo_.verboseline = line->getString();
    }

    return true;
}


bool MESIPrivNoninclusive::handleFlushAll(MemEvent * event, bool inMSHR) {
    event_debuginfo_.prefill(event->getID(), Command::FlushAll, "", 0, State::NP);

    if (!flush_manager_) {
        if (!inMSHR) {
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
        forwardByDestination(flush, timestamp_ + mshrLatency_);
        event_debuginfo_.action = "Forward";
        return true;
    }

    if (!inMSHR) {
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
            /* Forward requests up, transition to FlushState::Forward */
            // Broadcast ForwardFlush to all sources
            //      Wait for Ack from all sources -> retry when count == 0
            int count = broadcastMemEventToSources(Command::ForwardFlush, event, timestamp_ + 1);
            mshr_->incrementFlushCount(count);
            flush_state_ = FlushState::Forward;
            event_debuginfo_.action = "Begin";
            break;
        }
        case FlushState::Forward:
        {
            if (mshr_->getSize() != mshr_->getFlushSize()) {
                mshr_->incrementFlushCount(mshr_->getSize() - mshr_->getFlushSize());
                event_debuginfo_.action = "Drain";
                flush_state_ = FlushState::Drain;
                break;
            } /* else fall-thru */
        }
        case FlushState::Drain:
        {
            /* Have received all acks, do local flush. No peers since this is a private cache. */
            bool eviction_needed = false;

            for (auto it : *cacheArray_) {
                if (it->getState() == I) continue;
                if (it->getState() == S || it->getState() == E || it->getState() == M) {
                        MemEvent * ev = new MemEvent(cachename_, it->getAddr(), it->getAddr(), Command::NULLCMD);
                        retryBuffer_.push_back(ev);
                        mshr_->incrementFlushCount();
                        eviction_needed = true;
                } else {
                    debug_->fatal(CALL_INFO, -1, "%s, Error: Attempting to flush a cache line that is in a transient state '%s'. Addr = 0x%" PRIx64 ". Event: %s. Time: %" PRIu64 "ns\n",
                            cachename_.c_str(), StateString[it->getState()], it->getAddr(), event->getVerboseString().c_str(), getCurrentSimTimeNano());
                }
            }
            if (eviction_needed) {
                event_debuginfo_.action = "Flush";
                flush_state_ = FlushState::Invalidate;
                break;
            } /* else fall-thru */
        }
        case FlushState::Invalidate:
            /* Have finished invalidating */
            // Unblock/respond to sources
            sendResponseUp(event, nullptr, true, timestamp_);
            broadcastMemEventToSources(Command::UnblockFlush, event, timestamp_ + 1);
            mshr_->removeFlush();
            delete event;
            if (mshr_->getFlush() != nullptr) {
                retryBuffer_.push_back(mshr_->getFlush());
            }
            flush_state_ = FlushState::Ready;
            break;
    }

    return true;
}


bool MESIPrivNoninclusive::handleForwardFlush(MemEvent * event, bool inMSHR) {
    event_debuginfo_.prefill(event->getID(), Command::ForwardFlush, "", 0, State::NP);

    if (!inMSHR) {
        MemEventStatus status = mshr_->insertFlush(event, true);
        if (status == MemEventStatus::Reject) { /* No room for flush in MSHR */
            sendNACK(event);
            return true;
        }
    }

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
            if (mshr_->getSize() != mshr_->getFlushSize()) {
                mshr_->incrementFlushCount(mshr_->getSize() - mshr_->getFlushSize());
                event_debuginfo_.action = "Drain";
                flush_state_ = FlushState::Drain;
                break;
            } /* else fall-thru */
        }
        case FlushState::Drain:
        {
            /* Have received all acks, do local flush. No peers since this is a private cache. */
            bool eviction_needed = false;
            for (auto it : *cacheArray_) {
                if (it->getState() == I) continue;
                if (it->getState() == S || it->getState() == E || it->getState() == M) {
                    MemEvent * ev = new MemEvent(cachename_, it->getAddr(), it->getAddr(), Command::NULLCMD);
                    retryBuffer_.push_back(ev);
                    eviction_needed = true;
                    mshr_->incrementFlushCount();
                } else {
                    debug_->fatal(CALL_INFO, -1, "%s, Error: Attempting to flush a cache line that is in a transient state '%s'. Addr = 0x%" PRIx64 ". Event: %s. Time: %" PRIu64 "ns\n",
                        cachename_.c_str(), StateString[it->getState()], it->getAddr(), event->getVerboseString().c_str(), getCurrentSimTimeNano());
                }
            }
            if (eviction_needed) {
                event_debuginfo_.action = "Flush";
                flush_state_ = FlushState::Invalidate;
                break;
            } /* else fall-thru */
        }
        case FlushState::Invalidate:
            /* All blocks written back - respond */
            sendResponseDown(event, 0, nullptr, false);
            mshr_->removeFlush();
            delete event;
            flush_state_ = FlushState::Ready;
            break;
    } /* End switch */

    return true;
}

bool MESIPrivNoninclusive::handlePutS(MemEvent * event, bool inMSHR) {
    Addr addr = event->getBaseAddr();
    PrivateCacheLine* line = cacheArray_->lookup(addr, false);
    State state = line ? line->getState() : I;
    MemEventStatus status = MemEventStatus::OK;

    if (mem_h_is_debug_event(event))
        event_debuginfo_.prefill(event->getID(), Command::PutS, "", addr, state);

    if (!inMSHR)
        stat_eventstate_[(int)Command::PutS][state]->addData(1);
    else
        mshr_->removePendingRetry(addr);

    switch (state) {
        case I:
            if (!inMSHR && mshr_->exists(addr)) { // Raced with something; Inv/Fetch or a local replacement
                if (mshr_->getFrontType(addr) == MSHREntryType::Writeback) {
                    // Clean data so OK to drop this request
                    sendWritebackAck(event);
                    delete event;
                } else {
                    mshr_->setData(addr, event->getPayload(), false);
                    responses.erase(addr);
                    mshr_->decrementAcksNeeded(addr);
                    if (mshr_->getFrontType(addr) == MSHREntryType::Event && mshr_->getFrontEvent(addr)->getCmd() == Command::Fetch) {
                        status = allocateLine(event, line, false);
                        if (status == MemEventStatus::OK) {
                            line->setState(S);
                            line->setData(event->getPayload(), 0);
                            if (mem_h_is_debug_addr(addr))
                                printDataValue(line->getAddr(), line->getData(), true);
                            mshr_->clearData(addr);
                            sendWritebackAck(event);
                            cleanUpAfterRequest(event, inMSHR);
                            break;
                        }
                    } else { // Inv of some sort
                        sendWritebackAck(event);
                        delete event;
                    }
                    retry(addr);
                }
            } else {
                status = allocateLine(event, line, inMSHR);
                if (status == MemEventStatus::OK) {
                    line->setState(S);
                    line->setData(event->getPayload(), 0);
                    if (mem_h_is_debug_addr(addr))
                        printDataValue(line->getAddr(), line->getData(), true);
                    if (mshr_->hasData(addr)) mshr_->clearData(addr);
                    sendWritebackAck(event);
                    cleanUpAfterRequest(event, inMSHR);
                }
            }
            break;
        case S:
        case E:
        case M:
            line->setShared(false);
            sendWritebackAck(event);
            cleanUpAfterRequest(event, inMSHR);
            break;
        case S_Inv:
            line->setShared(false);
            line->setState(S);
            responses.erase(addr);
            mshr_->decrementAcksNeeded(addr);
            sendWritebackAck(event);
            delete event;
            retry(addr);
            break;
        case E_Inv:
            line->setShared(false);
            line->setState(E);
            responses.erase(addr);
            mshr_->decrementAcksNeeded(addr);
            sendWritebackAck(event);
            delete event;
            retry(addr);
            break;
        case M_Inv:
            line->setShared(false);
            line->setState(M);
            responses.erase(addr);
            mshr_->decrementAcksNeeded(addr);
            sendWritebackAck(event);
            delete event;
            retry(addr);
            break;
        default:
            debug_->fatal(CALL_INFO,-1,"%s, Error: Received PutS in unhandled state '%s'. Event: %s. Time = %" PRIu64 "ns\n",
                    getName().c_str(), StateString[state], event->getVerboseString().c_str(), getCurrentSimTimeNano());
    }

    //printLine(addr);

    if (status == MemEventStatus::Reject)
        sendNACK(event);

    if (mem_h_is_debug_addr(addr) && line) {
        event_debuginfo_.newst = line->getState();
        event_debuginfo_.verboseline = line->getString();
    }

    return true;
}


bool MESIPrivNoninclusive::handlePutE(MemEvent * event, bool inMSHR) {
    Addr addr = event->getBaseAddr();
    PrivateCacheLine * line = cacheArray_->lookup(addr, false);
    State state = line ? line->getState() : I;
    MemEventStatus status = MemEventStatus::OK;

    if (mem_h_is_debug_event(event))
        event_debuginfo_.prefill(event->getID(), Command::PutE, "", addr, state);

    if (!inMSHR)
        stat_eventstate_[(int)Command::PutE][state]->addData(1);
    else
        mshr_->removePendingRetry(addr);

    switch (state) {
        case I:
            if (mshr_->getAcksNeeded(addr) != 0) { // Race
                if (mshr_->getFrontType(addr) == MSHREntryType::Event && mshr_->getFrontEvent(addr)->getCmd() == Command::FetchInvX) {
                    mshr_->decrementAcksNeeded(addr);
                    responses.erase(addr);
                    mshr_->setData(addr, event->getPayload(), false);
                    event->setCmd(Command::PutS);
                    event->setDirty(false);
                    retry(addr);
                    status = allocateMSHR(event, false, 1, false);
                } else {
                    mshr_->setData(addr, event->getPayload(), false);
                    mshr_->decrementAcksNeeded(addr);
                    responses.erase(addr);
                    sendWritebackAck(event);
                    delete event;
                    retry(addr);
                }
            } else {
                status = allocateLine(event, line, inMSHR);
                if (status == MemEventStatus::OK) {
                    event->getDirty() ? line->setState(M) : line->setState(E);
                    line->setData(event->getPayload(), 0);
                    if (mem_h_is_debug_addr(addr))
                        printDataValue(line->getAddr(), line->getData(), true);
                    sendWritebackAck(event);
                    if (mshr_->hasData(addr)) mshr_->clearData(addr);
                    cleanUpAfterRequest(event, inMSHR);
                }
            }
            break;
        case E:
        case M:
            line->setOwned(false);
            sendWritebackAck(event);
            cleanUpAfterRequest(event, inMSHR);
            break;
        case E_Inv:
        case E_InvX:
            line->setOwned(false);
            responses.erase(addr);
            line->setState(E);
            sendWritebackAck(event);
            delete event;
            retry(addr);
            break;
        case M_Inv:
        case M_InvX:
            line->setOwned(false);
            responses.erase(addr);
            line->setState(M);
            sendWritebackAck(event);
            delete event;
            retry(addr);
            break;
        default:
            debug_->fatal(CALL_INFO,-1,"%s, Error: Received PutE in unhandled state '%s'. Event: %s. Time = %" PRIu64 "ns\n",
                    getName().c_str(), StateString[state], event->getVerboseString().c_str(), getCurrentSimTimeNano());

    }

    if (status == MemEventStatus::Reject)
        sendNACK(event);

    if (mem_h_is_debug_event(event) && line) {
        event_debuginfo_.newst = line->getState();
        event_debuginfo_.verboseline = line->getString();
    }

    return true;
}


bool MESIPrivNoninclusive::handlePutM(MemEvent * event, bool inMSHR) {
    Addr addr = event->getBaseAddr();
    PrivateCacheLine * line = cacheArray_->lookup(addr, false);
    State state = line ? line->getState() : I;
    MemEventStatus status = MemEventStatus::OK;

    //printLine(addr);

    if (mem_h_is_debug_event(event))
        event_debuginfo_.prefill(event->getID(), Command::PutM, "", addr, state);

    if (!inMSHR)
        stat_eventstate_[(int)Command::PutM][state]->addData(1);
    else
        mshr_->removePendingRetry(addr);

    switch (state) {
        case I:
            if (mshr_->getAcksNeeded(addr) != 0) { // Race with eviction, FetchInvX, or FetchInv/Inv/ForceInv
                if (mshr_->getFrontType(addr) == MSHREntryType::Event && mshr_->getFrontEvent(addr)->getCmd() == Command::FetchInvX) {
                    mshr_->decrementAcksNeeded(addr);
                    responses.erase(addr);
                    mshr_->setData(addr, event->getPayload(), true);
                    event->setCmd(Command::PutS);
                    event->setDirty(false);
                    retry(addr);
                    status = allocateMSHR(event, false, 1);
                } else { // Eviction or invalidation -> we won't need a line
                    mshr_->setData(addr, event->getPayload(), true);
                    mshr_->decrementAcksNeeded(addr);
                    responses.erase(addr);
                    sendWritebackAck(event);
                    delete event;
                    retry(addr);
                }
            } else {
                status = allocateLine(event, line, inMSHR);
                if (status == MemEventStatus::OK) {
                    line->setState(M);
                    line->setData(event->getPayload(), 0);
                    if (mem_h_is_debug_addr(addr))
                        printDataValue(line->getAddr(), line->getData(), true);
                    if (mshr_->hasData(addr)) mshr_->clearData(addr);
                    sendWritebackAck(event);
                    cleanUpAfterRequest(event, inMSHR);
                }
            }
            break;
        case E:
        case M:
            line->setOwned(false);
            line->setState(M);
            line->setData(event->getPayload(), 0);
            if (mem_h_is_debug_addr(addr))
                printDataValue(line->getAddr(), line->getData(), true);
            sendWritebackAck(event);
            cleanUpAfterRequest(event, inMSHR);
            break;
        case E_Inv:
        case E_InvX:
        case M_Inv:
        case M_InvX:
            line->setOwned(false);
            line->setState(M);
            responses.erase(addr);
            sendWritebackAck(event);
            delete event;
            retry(addr);
            break;
        default:
            debug_->fatal(CALL_INFO,-1,"%s, Error: Received PutM in unhandled state '%s'. Event: %s. Time = %" PRIu64 "ns\n",
                    getName().c_str(), StateString[state], event->getVerboseString().c_str(), getCurrentSimTimeNano());
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


bool MESIPrivNoninclusive::handlePutX(MemEvent * event, bool inMSHR) {
    Addr addr = event->getBaseAddr();
    PrivateCacheLine * line = cacheArray_->lookup(addr, false);
    State state = line ? line->getState() : I;
    MemEventStatus status = MemEventStatus::OK;

//    printLine(addr);

    if (mem_h_is_debug_event(event))
        event_debuginfo_.prefill(event->getID(), Command::PutX, "", addr, state);

    if (!inMSHR)
        stat_eventstate_[(int)Command::PutX][state]->addData(1);
    else
        mshr_->removePendingRetry(addr);

    switch (state) {
        case I:
            if (mshr_->getAcksNeeded(addr)) {
                mshr_->setData(addr, event->getPayload(), event->getDirty());
                sendWritebackAck(event);
                delete event;

                if (mshr_->getFrontEvent(addr)->getCmd() == Command::FetchInvX) {
                    mshr_->decrementAcksNeeded(addr);
                    responses.erase(addr);
                    retry(addr);
                }
            } else {
                status = allocateLine(event, line, inMSHR);
                if (status == MemEventStatus::OK) {
                    event->getDirty() ? line->setState(M) : line->setState(E);
                    line->setData(event->getPayload(), 0);
                    if (mem_h_is_debug_addr(addr))
                        printDataValue(line->getAddr(), line->getData(), true);
                    sendWritebackAck(event);
                    if (mshr_->hasData(addr)) mshr_->clearData(addr);
                    cleanUpAfterRequest(event, inMSHR);
                }
            }
            break;
        case E:
        case M:
            line->setOwned(false);
            line->setShared(true);
            if (event->getDirty()) {
                line->setState(M);
                line->setData(event->getPayload(), 0);
                if (mem_h_is_debug_addr(addr))
                    printDataValue(line->getAddr(), line->getData(), true);
            }
            sendWritebackAck(event);
            cleanUpAfterRequest(event, inMSHR);
            break;
        case E_Inv:
        case M_Inv:
            line->setOwned(false);
            line->setShared(true);
            if (event->getDirty()) {
                line->setState(M_Inv);
                line->setData(event->getPayload(), 0);
                if (mem_h_is_debug_addr(addr))
                    printDataValue(line->getAddr(), line->getData(), true);
            }
            sendWritebackAck(event);
            delete event;
            break;
        case E_InvX:
        case M_InvX:
            mshr_->decrementAcksNeeded(addr);
            responses.erase(addr);
            line->setOwned(false);
            line->setShared(true);
            if (event->getDirty()) {
                line->setState(M);
                line->setData(event->getPayload(), 0);
                if (mem_h_is_debug_addr(addr))
                        printDataValue(line->getAddr(), line->getData(), true);
            } else {
                line->setState(E);
            }
            sendWritebackAck(event);
            delete event;
            retry(addr);
            break;
        default:
            debug_->fatal(CALL_INFO,-1,"%s, Error: Received PutX in unhandled state '%s'. Event: %s. Time = %" PRIu64 "ns\n",
                    getName().c_str(), StateString[state], event->getVerboseString().c_str(), getCurrentSimTimeNano());
    }

    if (status == MemEventStatus::Reject)
        sendNACK(event);

    if (mem_h_is_debug_addr(addr) && line) {
        event_debuginfo_.newst = line->getState();
        event_debuginfo_.verboseline = line->getString();
    }

    return true;
}


bool MESIPrivNoninclusive::handleFetch(MemEvent * event, bool inMSHR) {
    Addr addr = event->getBaseAddr();
    PrivateCacheLine * line = cacheArray_->lookup(addr, false);
    State state = line ? line->getState() : I;
    MemEventStatus status = MemEventStatus::OK;

    //printLine(addr);

    if (mem_h_is_debug_event(event))
        event_debuginfo_.prefill(event->getID(), Command::Fetch, "", addr, state);

    if (!inMSHR)
        stat_eventstate_[(int)Command::Fetch][state]->addData(1);
    else
        mshr_->removePendingRetry(addr);

    switch (state) {
        case I:
            if (mshr_->hasData(addr)) {
                sendResponseDown(event, lineSize_, &(mshr_->getData(addr)), mshr_->getDataDirty(addr));
                cleanUpAfterRequest(event, inMSHR);
            } else if (!inMSHR && mshr_->exists(addr)) {
                if (mshr_->getFrontType(addr) == MSHREntryType::Writeback || mshr_->getFrontEvent(addr)->getCmd() == Command::FlushLineInv) {  // Raced with eviction
                    if (mem_h_is_debug_event(event)) {
                        event_debuginfo_.action = "Drop";
                        event_debuginfo_.reason = "MSHR conflict";
                    }
                    delete event;
                } else if (mshr_->getFrontEvent(addr)->getCmd() == Command::PutS) { // Raced with replacement
                    MemEvent* put = static_cast<MemEvent*>(mshr_->getFrontEvent(addr));
                    sendResponseDown(event, lineSize_, &(put->getPayload()), false);
                    delete event;
                } else { // Raced with GetX or FlushLine
                    status = allocateMSHR(event, true, 0);
                    sendFwdRequest(event, Command::Fetch, upperCacheName_, lineSize_, 0, inMSHR);
                }
            } else {
                if (!inMSHR)
                    status = allocateMSHR(event, true, 0);
                if (status == MemEventStatus::OK)
                    sendFwdRequest(event, Command::Fetch, upperCacheName_, lineSize_, 0, inMSHR);
            }
            break;
        case S:
        case SM:
        case S_B:
        case S_Inv:
            sendResponseDown(event, lineSize_, line->getData(), false);
            cleanUpAfterRequest(event, inMSHR);
            break;
        case I_B:
            if (mem_h_is_debug_event(event)) {
                event_debuginfo_.action = "Drop";
                event_debuginfo_.reason = "MSHR conflict";
            }
            cleanUpAfterRequest(event, inMSHR);
            break;
        default:
            debug_->fatal(CALL_INFO,-1,"%s, Error: Received Fetch in unhandled state '%s'. Event: %s. Time = %" PRIu64 "ns\n",
                    getName().c_str(), StateString[state], event->getVerboseString().c_str(), getCurrentSimTimeNano());
    }

    if (status == MemEventStatus::Reject)
        sendNACK(event);

    if (mem_h_is_debug_addr(addr) && line) {
        event_debuginfo_.newst = line->getState();
        event_debuginfo_.verboseline = line->getString();
    }

    return true;
}


bool MESIPrivNoninclusive::handleInv(MemEvent * event, bool inMSHR) {
    Addr addr = event->getBaseAddr();
    PrivateCacheLine * line = cacheArray_->lookup(addr, false);
    State state = line ? line->getState() : I;
    MemEventStatus status = MemEventStatus::OK;

    //printLine(addr);

    if (mem_h_is_debug_event(event))
        event_debuginfo_.prefill(event->getID(), Command::Inv, "", addr, state);

    bool handle = false;
    State state1, state2;
    MemEventBase * req;

    if (!inMSHR)
        stat_eventstate_[(int)Command::Inv][state]->addData(1);
    else
        mshr_->removePendingRetry(addr);

    switch (state) {
        case I:
            // GetX     Forward Inv & insert ahead
            // FL       Forward Inv & insert ahead
            if (mshr_->exists(addr) && (mshr_->getFrontType(addr) == MSHREntryType::Writeback || mshr_->getFrontEvent(addr)->getCmd() == Command::FlushLineInv)) { // Already evicted block, drop the Inv
                if (mem_h_is_debug_event(event)) {
                    event_debuginfo_.action = "Drop";
                    event_debuginfo_.reason = "MSHR conflict";
                }
                delete event;
            } else if (mshr_->exists(addr) && mshr_->getFrontEvent(addr)->getCmd() == Command::PutS) {
                // Handle Inv
                sendResponseDown(event, lineSize_, nullptr, false);
                if (mshr_->hasData(addr)) mshr_->clearData(addr);
                // Drop PutS
                sendWritebackAck(static_cast<MemEvent*>(mshr_->getFrontEvent(addr)));
                req = mshr_->getFrontEvent(addr);
                mshr_->removeFront(addr);
                delete req;
                // Clean up
                cleanUpAfterRequest(event, inMSHR);
            } else if (mshr_->exists(addr) && mshr_->hasData(addr)) { // Race with Put (replay)
                // Handle Inv
                sendResponseDown(event, lineSize_, nullptr, false);
                if (mshr_->hasData(addr)) mshr_->clearData(addr);
                // Clean up
                cleanUpAfterRequest(event, inMSHR);
            } else {
                status = inMSHR ? MemEventStatus::OK : allocateMSHR(event, true, 0);
                if (status == MemEventStatus::OK) {
                    sendFwdRequest(event, Command::Inv, upperCacheName_, lineSize_, 0, inMSHR);
                }
            }
            break;
        case S:
            state1 = S_Inv;
            state2 = I;
            handle = true;
            break;
        case S_B:   // If sharers -> SB_Inv & stall, if no sharers -> I & respond (done)
            state1 = SB_Inv;
            state2 = I;
            handle = true;
            break;
        case SM:    // If requestor is sharer, send inv -> SM_Inv, & stall. If no sharers, -> I & respond(done)
            state1 = SM_Inv;
            state2 = I;
            handle = true;
            break;
        case I_B:
            if (mem_h_is_debug_event(event)) {
                event_debuginfo_.action = "Drop";
                event_debuginfo_.reason = "MSHR conflict";
            }
            line->setState(I);
            cleanUpEvent(event, inMSHR); // No replay since state doesn't change
            break;
        default:
            debug_->fatal(CALL_INFO,-1,"%s, Error: Received Inv in unhandled state '%s'. Event: %s. Time = %" PRIu64 "ns\n",
                    getName().c_str(), StateString[state], event->getVerboseString().c_str(), getCurrentSimTimeNano());
    }


    if (handle) {
        if (line->getShared()) {
            status = inMSHR ? MemEventStatus::OK : allocateMSHR(event, true, 0);
            if (status == MemEventStatus::OK) {
                uint64_t sendTime = sendFwdRequest(event, Command::Inv, upperCacheName_, lineSize_, line->getTimestamp(), inMSHR);
                line->setState(state1);
                line->setTimestamp(sendTime);
            }
        } else {
            sendResponseDown(event, lineSize_, line->getData(), false);
            line->setState(state2);
            if (mshr_->hasData(addr))
                mshr_->clearData(addr);
            cleanUpAfterRequest(event, inMSHR);
        }
    }

    if (status == MemEventStatus::Reject)
        sendNACK(event);

    if (mem_h_is_debug_addr(addr) && line) {
        event_debuginfo_.newst = line->getState();
        event_debuginfo_.verboseline = line->getString();
    }

    return true;
}


bool MESIPrivNoninclusive::handleForceInv(MemEvent * event, bool inMSHR) {
    Addr addr = event->getBaseAddr();
    PrivateCacheLine * line = cacheArray_->lookup(addr, false);
    State state = line ? line->getState() : I;
    MemEventStatus status = MemEventStatus::OK;

    if (mem_h_is_debug_event(event))
        event_debuginfo_.prefill(event->getID(), Command::ForceInv, "", addr, state);

    if (!inMSHR)
        stat_eventstate_[(int)Command::ForceInv][state]->addData(1);
    else
        mshr_->removePendingRetry(addr);

    bool handle = false;
    State state1, state2;
    MemEventBase * entry;
    switch (state) {
        case I:
            if (mshr_->pendingWritebackIsDowngrade(addr)) { // Waiting for AckPut to PutX, go ahead with the force inv
                entry = mshr_->getEntryEvent(addr, 1);
                if (entry) {
                    if (entry->getCmd() == Command::PutS) {
                        // Handle ForceInv
                        sendResponseDown(event, lineSize_, nullptr, false);
                        if (mshr_->hasData(addr)) mshr_->clearData(addr);
                        // Drop PutS
                        sendWritebackAck(static_cast<MemEvent*>(entry));
                        delete entry;
                        mshr_->removeEntry(addr, 1);
                        break;
                    } else if (entry->getCmd() == Command::FlushLineInv) {
                        // Handle ForceInv
                        sendResponseDown(event, lineSize_, nullptr, false);
                        if (mshr_->hasData(addr)) mshr_->clearData(addr);
                        // Drop evict part of Flush if needed
                        MemEvent* flush = static_cast<MemEvent*>(entry);
                        if (flush->getEvict())
                            flush->setEvict(false);
                        break;
                    }
                }
                status = allocateMSHR(event, true, 1); // Retry when we get the AckPut
            } else if (mshr_->pendingWriteback(addr) || (mshr_->exists(addr) && mshr_->getFrontEvent(addr)->getCmd() == Command::FlushLineInv)) {
                if (mshr_->hasData(addr)) mshr_->clearData(addr);
                if (mem_h_is_debug_event(event)) {
                    event_debuginfo_.action = "Drop";
                    event_debuginfo_.reason = "MSHR conflict";
                }
                delete event;
            } else if (mshr_->exists(addr) && mshr_->getFrontEvent(addr)->getCmd() == Command::PutX) { // Drop PutX, Ack it, forward request up
                MemEvent * put = static_cast<MemEvent*>(mshr_->swapFrontEvent(addr, event));
                sendWritebackAck(put);
                mshr_->setData(addr, put->getPayload(), put->getDirty());
                delete put;
                sendFwdRequest(event, Command::ForceInv, upperCacheName_, lineSize_, 0, inMSHR);
            } else if (mshr_->exists(addr) && (CommandWriteback[(int)mshr_->getFrontEvent(addr)->getCmd()])) {
                if (mshr_->hasData(addr)) mshr_->clearData(addr);
                sendWritebackAck(static_cast<MemEvent*>(mshr_->getFrontEvent(addr)));
                delete mshr_->getFrontEvent(addr);
                mshr_->removeFront(addr);
                sendResponseDown(event, lineSize_, nullptr, false);
            } else {
                status = inMSHR ? MemEventStatus::OK : allocateMSHR(event, true, 0);
                if (status == MemEventStatus::OK) {
                    sendFwdRequest(event, Command::ForceInv, upperCacheName_, lineSize_, 0, inMSHR);
                }
            }
            break;
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
        case SM: // ForceInv if un-inv'd sharer & SM_Inv, else I & ack
            state1 = SM_Inv;
            state2 = I;
            handle = true;
            break;
        case S_B: // if shared, forward & SB_Inv, else ack & I
            state1 = SB_Inv;
            state2 = I;
            handle = true;
            break;
        case I_B:
            line->setState(I);
            if (mem_h_is_debug_event(event)) {
                event_debuginfo_.action = "Drop";
                event_debuginfo_.reason = "MSHR conflict";
            }
            cleanUpEvent(event, inMSHR); // No replay since state doesn't change
            break;
        case SM_Inv: { // ForceInv if there's an un-inv'd sharer, else in mshr & stall
            std::string src = mshr_->getFrontEvent(addr)->getSrc();
            status = inMSHR ? MemEventStatus::OK : allocateMSHR(event, true, 0);
            if (status != MemEventStatus::Reject) {
                if (line->getShared()) {
                    uint64_t sendTime = sendFwdRequest(event, Command::ForceInv, src, lineSize_, line->getTimestamp(), inMSHR);
                    line->setTimestamp(sendTime);
                }
                status = MemEventStatus::Stall;
            }
            break;
            }
        default:
            debug_->fatal(CALL_INFO,-1,"%s, Error: Received ForceInv in unhandled state '%s'. Event: %s. Time = %" PRIu64 "ns\n",
                    getName().c_str(), StateString[state], event->getVerboseString().c_str(), getCurrentSimTimeNano());
    }

    if (handle) {
        if (line->getShared() || line->getOwned()) {
            if (!inMSHR)
                status = allocateMSHR(event, true, 0);
            if (status == MemEventStatus::OK) {
                uint64_t sendTime = sendFwdRequest(event, Command::ForceInv, upperCacheName_, lineSize_, 0, inMSHR);
                line->setTimestamp(sendTime);
                line->setState(state1);
                status = MemEventStatus::Stall;
            }
        } else {
            sendResponseDown(event, lineSize_, nullptr, false);
            line->setState(state2);
            cleanUpAfterRequest(event, inMSHR);
        }
    }

    if (status == MemEventStatus::Reject)
        sendNACK(event);

    if (mem_h_is_debug_addr(addr) && line) {
        event_debuginfo_.newst = line->getState();
        event_debuginfo_.verboseline = line->getString();
    }

    return true;
}


bool MESIPrivNoninclusive::handleFetchInv(MemEvent * event, bool inMSHR){
    Addr addr = event->getBaseAddr();
    PrivateCacheLine * line = cacheArray_->lookup(addr, false);
    State state = line ? line->getState() : I;
    MemEventStatus status = MemEventStatus::OK;

    //printLine(addr);

    if (mem_h_is_debug_event(event))
        event_debuginfo_.prefill(event->getID(), Command::FetchInv, "", addr, state);

    if (!inMSHR)
        stat_eventstate_[(int)Command::FetchInv][state]->addData(1);
    else
        mshr_->removePendingRetry(addr);

    State state1, state2;
    bool handle = false;
    MemEventBase* entry;
    switch (state) {
        case I:
            if (inMSHR && mshr_->hasData(addr)) { // Replay
                sendResponseDown(event, lineSize_, &(mshr_->getData(addr)), mshr_->getDataDirty(addr));
                mshr_->clearData(addr);
                cleanUpAfterRequest(event, inMSHR);
            } else if (mshr_->pendingWritebackIsDowngrade(addr)) { // Resolve races with pending evictions from upper level caches
                /* In this case we're waiting for an AckPut for a PutX so we've gone from M/E to S. Check if any
                 * queued events are evictions and if so, handle them. Otherwise stallthe FetchInv until the writeback ack returns.
                 * Note that the next level can NACK a PutX if it is noninclusive and in a stable state,
                 * but it won't NACK the PutX if it's in a transition state.
                 * So stalling the response won't cause deadlock.
                 */
                entry = mshr_->getEntryEvent(addr, 1);
                if (entry) {
                    if (entry->getCmd() == Command::PutS) {
                        // Return AckInv
                        sendResponseDown(event, lineSize_, &(static_cast<MemEvent*>(entry)->getPayload()), false);
                        delete event;
                        // Drop PutS
                        if (mshr_->hasData(addr)) mshr_->clearData(addr);
                        sendWritebackAck(static_cast<MemEvent*>(entry));
                        delete entry;
                        mshr_->removeEntry(addr, 1);
                        break;
                    } else if (entry->getCmd() == Command::FlushLineInv) {
                        // Handle FetchInv
                        sendResponseDown(event, lineSize_, &(static_cast<MemEvent*>(entry)->getPayload()), false);
                        if (mshr_->hasData(addr)) mshr_->clearData(addr);
                        // Drop evict part of Flush if needed
                        MemEvent* flush = static_cast<MemEvent*>(entry);
                        if (flush->getEvict())
                            flush->setEvict(false);
                        break;
                    }
                }
                // Stall the event until the writeback ack returns (don't actually have to but it makes the tracking simpler)
                // Since we profiled this event in I above, that means we're going to have slightly imprecise statistics (will eventually be correct) TODO fix this maybe?
                status = allocateMSHR(event, true, 1);
            } else if (mshr_->pendingWriteback(addr) || (mshr_->exists(addr) && mshr_->getFrontEvent(addr)->getCmd() == Command::FlushLineInv)) {
                if (mshr_->hasData(addr)) mshr_->clearData(addr);
                if (mem_h_is_debug_event(event)) {
                    event_debuginfo_.action = "Drop";
                    event_debuginfo_.reason = "Req conflict";
                }
                delete event;
            } else if (mshr_->exists(addr) && mshr_->getFrontEvent(addr)->getCmd() == Command::PutX) { // Drop PutX, Ack it, forward request up
                MemEvent * put = static_cast<MemEvent*>(mshr_->swapFrontEvent(addr, event));
                sendWritebackAck(put);
                mshr_->setData(addr, put->getPayload(), put->getDirty());
                delete put;
                sendFwdRequest(event, Command::FetchInv, upperCacheName_, lineSize_, 0, inMSHR);
            } else if (mshr_->exists(addr) && (CommandWriteback[(int)mshr_->getFrontEvent(addr)->getCmd()])) {
                MemEvent * put = static_cast<MemEvent*>(mshr_->getFrontEvent(addr));
                sendWritebackAck(put);
                sendResponseDown(event, put->getSize(), &(put->getPayload()), put->getDirty());
                mshr_->removeFront(addr);
                delete put;
                cleanUpAfterRequest(event, inMSHR);
            } else {
                status = inMSHR ? MemEventStatus::OK : allocateMSHR(event, true, 0);
                if (status == MemEventStatus::OK) {
                    sendFwdRequest(event, Command::FetchInv, upperCacheName_, lineSize_, 0, inMSHR);
                }
            }
            break;
        case I_B:
            line->setState(I);
            if (mem_h_is_debug_event(event)) {
                event_debuginfo_.action = "Drop";
                event_debuginfo_.reason = "MSHR conflict";
            }
            cleanUpEvent(event, inMSHR); // No replay since state doesn't change
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
            state2 = I;
            handle = true;
            break;
        case S_B:
            state1 = SB_Inv;
            state2 = I;
            handle = true;
            break;
        default:
            debug_->fatal(CALL_INFO,-1,"%s, Error: Received FetchInv in unhandled state '%s'. Event: %s. Time = %" PRIu64 "ns\n",
                    getName().c_str(), StateString[state], event->getVerboseString().c_str(), getCurrentSimTimeNano());
    }

    if (handle) {
        if (line->getShared() || line->getOwned()) {
            status = inMSHR ? MemEventStatus::OK : allocateMSHR(event, true, 0);
            if (status == MemEventStatus::OK) {
                uint64_t sendTime = sendFwdRequest(event, Command::FetchInv, upperCacheName_, lineSize_, 0, inMSHR);
                line->setTimestamp(sendTime);
                line->setState(state1);
            }
        } else {
            sendResponseDown(event, lineSize_, line->getData(), state == M);
            line->setState(state2);
            cleanUpAfterRequest(event, inMSHR);
        }
    }
    //printLine(addr);

    if (status == MemEventStatus::Reject)
        sendNACK(event);

    if (mem_h_is_debug_addr(addr) && line) {
        event_debuginfo_.newst = line->getState();
        event_debuginfo_.verboseline = line->getString();
    }

    return true;
}


bool MESIPrivNoninclusive::handleFetchInvX(MemEvent * event, bool inMSHR) {
    Addr addr = event->getBaseAddr();
    PrivateCacheLine * line = cacheArray_->lookup(addr, false);
    State state = line ? line->getState() : I;
    MemEventStatus status = MemEventStatus::OK;

    if (!inMSHR)
        stat_eventstate_[(int)Command::FetchInvX][state]->addData(1);
    else
        mshr_->removePendingRetry(addr);

    if (mem_h_is_debug_event(event))
        event_debuginfo_.prefill(event->getID(), Command::FetchInvX, "", addr, state);

    switch (state) {
        case I:
            if (inMSHR && mshr_->hasData(addr)) { // Replay
                sendResponseDown(event, lineSize_, &(mshr_->getData(addr)), mshr_->getDataDirty(addr));
                mshr_->clearData(addr);
                cleanUpAfterRequest(event, inMSHR);
                break;
            } else if (mshr_->pendingWriteback(addr)) {
                if (mem_h_is_debug_event(event)) {
                    event_debuginfo_.action = "Drop";
                    event_debuginfo_.reason = "MSHR conflict";
                }
                delete event;
                break;
            } else if (mshr_->exists(addr)) {
                if (mshr_->getFrontEvent(addr)->getCmd() == Command::FlushLine || mshr_->getFrontEvent(addr)->getCmd() == Command::FlushLineInv) {
                    if (mem_h_is_debug_event(event)) {
                        event_debuginfo_.action = "Drop";
                        event_debuginfo_.reason = "MSHR conflict";
                    }
                    delete event;
                    break;
                } else if (mshr_->getFrontEvent(addr)->getCmd() == Command::PutX) {
                    MemEvent * put = static_cast<MemEvent*>(mshr_->getFrontEvent(addr));
                    sendWritebackAck(put);
                    sendResponseDown(event, put->getSize(), &(put->getPayload()), put->getDirty());
                    delete put;
                    mshr_->removeFront(addr);
                    cleanUpAfterRequest(event, inMSHR);
                    break;
                } else if (mshr_->getFrontEvent(addr)->getCmd() == Command::PutE || mshr_->getFrontEvent(addr)->getCmd() == Command::PutM) {
                    MemEvent * put = static_cast<MemEvent*>(mshr_->getFrontEvent(addr));
                    sendResponseDown(event, put->getSize(), &(put->getPayload()), put->getDirty());
                    put->setCmd(Command::PutS); // Make this a PutS so we only record the block in shared later
                    put->setDirty(false);
                    delete event;
                    break;
                }
            }
            if (!inMSHR)
                status = allocateMSHR(event, true, 0);
            if (status == MemEventStatus::OK) {
                sendFwdRequest(event, Command::FetchInvX, upperCacheName_, lineSize_, 0, inMSHR);
            }
            break;
        case E:
        case M:
            if (line->getOwned()) {
                status = inMSHR ? MemEventStatus::OK : allocateMSHR(event, true, 0);
                if (status == MemEventStatus::OK) {
                    uint64_t sendTime = sendFwdRequest(event, Command::FetchInvX, upperCacheName_, lineSize_, line->getTimestamp(), inMSHR);
                    line->setTimestamp(sendTime);
                    mshr_->setInProgress(addr);
                    state == E ? line->setState(E_InvX) : line->setState(M_InvX);
                    status = MemEventStatus::Stall;
                }
                break;
            }
            sendResponseDown(event, lineSize_, line->getData(), state == M);
            line->setState(S);
            cleanUpAfterRequest(event, inMSHR);
            break;
        case S_B:
        case I_B:
            if (mem_h_is_debug_event(event)) {
                event_debuginfo_.action = "Drop";
                event_debuginfo_.reason = "MSHR conflict";
            }
            cleanUpEvent(event, inMSHR); // No replay since state doesn't change
            break;
        default:
            debug_->fatal(CALL_INFO,-1,"%s, Error: Received FetchInvX in unhandled state '%s'. Event: %s. Time = %" PRIu64 "ns\n",
                    getName().c_str(), StateString[state], event->getVerboseString().c_str(), getCurrentSimTimeNano());
    }

    //printLine(addr);

    if (status == MemEventStatus::Reject)
        sendNACK(event);

    if (mem_h_is_debug_addr(addr) && line) {
        event_debuginfo_.newst = line->getState();
        event_debuginfo_.verboseline = line->getString();
    }

    return true;
}

bool MESIPrivNoninclusive::handleGetSResp(MemEvent * event, bool inMSHR) {
    Addr addr = event->getBaseAddr();
    PrivateCacheLine * line = cacheArray_->lookup(addr, false);
    State state = line ? line->getState() : I;

    if (mem_h_is_debug_event(event))
        event_debuginfo_.prefill(event->getID(), Command::GetSResp, "", addr, state);

    stat_eventstate_[(int)Command::GetSResp][state]->addData(1);

    // Find matching request in MSHR
    MemEvent * req = static_cast<MemEvent*>(mshr_->getFrontEvent(addr));
    req->setFlags(event->getMemFlags());

    uint64_t sendTime = sendResponseUp(req, &(event->getPayload()), true, line ? line->getTimestamp() : 0);

    // Update line
    if (line) {
        line->setData(event->getPayload(), 0);
        line->setState(S);
        line->setShared(true);
        line->setTimestamp(sendTime-1);
        if (mem_h_is_debug_addr(addr))
            printDataValue(line->getAddr(), line->getData(), true);
    }

    cleanUpAfterResponse(event, inMSHR);

    if (mem_h_is_debug_addr(addr) && line) {
        event_debuginfo_.newst = line->getState();
        event_debuginfo_.verboseline = line->getString();
    }

    return true;
}


bool MESIPrivNoninclusive::handleGetXResp(MemEvent * event, bool inMSHR) {
    Addr addr = event->getBaseAddr();
    PrivateCacheLine * line = cacheArray_->lookup(addr, false);
    State state = line ? line->getState() : I;

    if (mem_h_is_debug_event(event))
        event_debuginfo_.prefill(event->getID(), Command::GetXResp, "", addr, state);

    // Get matching request
    MemEvent * req = static_cast<MemEvent*>(mshr_->getFrontEvent(addr));
    req->setFlags(event->getMemFlags());

    std::vector<uint8_t> data;
    Addr offset = req->getAddr() - req->getBaseAddr();

    switch (state) {
        case I:
        {
            sendExclusiveResponse(req, &(event->getPayload()), true, 0, event->getDirty());
            cleanUpAfterResponse(event, inMSHR);
            break;
        }
        case SM:
        {
            line->setState(M);
            line->setOwned(true);
            if (line->getShared())
                line->setShared(false);

            uint64_t sendTime = sendExclusiveResponse(req, line->getData(), true, line->getTimestamp(), event->getDirty());
            line->setTimestamp(sendTime-1);
            cleanUpAfterResponse(event, inMSHR);
            break;
        }
        default:
            debug_->fatal(CALL_INFO,-1,"%s, Error: Received GetXResp in unhandled state '%s'. Event: %s. Time = %" PRIu64 "ns\n",
                    getName().c_str(), StateString[state], event->getVerboseString().c_str(), getCurrentSimTimeNano());
    }

    stat_eventstate_[(int)Command::GetXResp][state]->addData(1);

    if (mem_h_is_debug_addr(addr) && line) {
        event_debuginfo_.newst = line->getState();
        event_debuginfo_.verboseline = line->getString();
    }

    return true;
}


bool MESIPrivNoninclusive::handleFlushLineResp(MemEvent * event, bool inMSHR) {
    Addr addr = event->getBaseAddr();
    PrivateCacheLine * line = cacheArray_->lookup(addr, false);
    State state = line ? line->getState() : I;
    //printLine(addr);

    if (mem_h_is_debug_event(event))
        event_debuginfo_.prefill(event->getID(), Command::FlushLineResp, "", addr, state);

    stat_eventstate_[(int)Command::FlushLineResp][state]->addData(1);

    MemEvent * req = static_cast<MemEvent*>(mshr_->getFrontEvent(addr));

    switch (state) {
        case I:
            break;
        case I_B:
            line->setState(I);
            cacheArray_->deallocate(line);
            break;
        case S_B:
            line->setState(S);
            break;
        default:
            debug_->fatal(CALL_INFO,-1,"%s, Error: Received FlushLineResp in unhandled state '%s'. Event: %s. Time = %" PRIu64 "ns\n",
                    getName().c_str(), StateString[state], event->getVerboseString().c_str(), getCurrentSimTimeNano());
    }

    sendResponseUp(req, nullptr, true, timestamp_, Command::FlushLineResp, event->success());

    if (mem_h_is_debug_event(event) && line) {
        event_debuginfo_.newst = line->getState();
        event_debuginfo_.verboseline = line->getString();
    }

    cleanUpAfterResponse(event, inMSHR);

    return true;
}


bool MESIPrivNoninclusive::handleFlushAllResp(MemEvent * event, bool inMSHR) {
    event_debuginfo_.prefill(event->getID(), Command::FlushAllResp, "", 0, State::NP);

    MemEvent* flush_request = static_cast<MemEvent*>(mshr_->getFlush());
    mshr_->removeFlush(); // Remove FlushAll

    event_debuginfo_.action = "Respond";

    sendResponseUp(flush_request, nullptr, true, timestamp_);

    delete flush_request;
    delete event;

    if (mshr_->getFlush() != nullptr) {
        retryBuffer_.push_back(mshr_->getFlush());
    }

    return true;
}

// Response to a Fetch or FetchInv
bool MESIPrivNoninclusive::handleFetchResp(MemEvent * event, bool inMSHR) {
    Addr addr = event->getBaseAddr();
    PrivateCacheLine * line = cacheArray_->lookup(addr, false);
    State state = line ? line->getState() : I;
    //printLine(addr);

    if (mem_h_is_debug_event(event))
        event_debuginfo_.prefill(event->getID(), Command::FetchResp, "", addr, state);

    stat_eventstate_[(int)Command::FetchResp][state]->addData(1);

    mshr_->decrementAcksNeeded(addr);
    responses.erase(addr);

    if (state == I) { // Fetch or FetchInv
        MemEvent * req = static_cast<MemEvent*>(mshr_->getFrontEvent(addr));
        sendResponseDown(req, event->getSize(), &(event->getPayload()), event->getDirty());
        cleanUpAfterResponse(event, inMSHR);
    } else {    // FetchInv only
        if (event->getDirty()) {
            line->setState(M);
            line->setData(event->getPayload(), 0);
            if (mem_h_is_debug_addr(addr))
                printDataValue(line->getAddr(), line->getData(), true);
        } else if (state == M_Inv) {
            line->setState(M);
        } else {
            line->setState(E);
        }
        if (line->getOwned())
            line->setOwned(false);
        else
            line->setShared(false);
        retry(addr);
        delete event;
    }

    if (mem_h_is_debug_addr(addr) && line) {
        event_debuginfo_.newst = line->getState();
        event_debuginfo_.verboseline = line->getString();
    }

    return true;
}


bool MESIPrivNoninclusive::handleFetchXResp(MemEvent * event, bool inMSHR) {
    Addr addr = event->getBaseAddr();
    PrivateCacheLine * line = cacheArray_->lookup(addr, false);
    State state = line ? line->getState() : I;

    if (mem_h_is_debug_event(event))
        event_debuginfo_.prefill(event->getID(), Command::FetchXResp, "", addr, state);

    stat_eventstate_[(int)Command::FetchXResp][state]->addData(1);

    mshr_->decrementAcksNeeded(addr);
    responses.erase(addr);

    if (state == I) {
        MemEvent * req = static_cast<MemEvent*>(mshr_->getFrontEvent(addr));
        sendResponseDown(req, event->getSize(), &(event->getPayload()), event->getDirty());
        cleanUpAfterResponse(event, inMSHR);
    } else {
        line->setOwned(false);
        line->setShared(true);
        if (event->getDirty()) {
            line->setState(M);
            line->setData(event->getPayload(), 0);
            if (mem_h_is_debug_addr(addr))
                printDataValue(line->getAddr(), line->getData(), true);
        } else if (state == M_InvX) {
            line->setState(M);
        } else {
            line->setState(E);
        }
        line->setOwned(false);
        line->setShared(true);
        retry(addr);
        delete event;
    }

    if (mem_h_is_debug_addr(addr) && line) {
        event_debuginfo_.newst = line->getState();
        event_debuginfo_.verboseline = line->getString();
    }

    return true;
}


bool MESIPrivNoninclusive::handleAckFlush(MemEvent* event, bool inMSHR) {
    event_debuginfo_.prefill(event->getID(), Command::AckFlush, "", 0, State::NP);

    mshr_->decrementFlushCount();
    if (mshr_->getFlushCount() == 0) {
        retryBuffer_.push_back(mshr_->getFlush());
    }

    delete event;
    return true;
}


bool MESIPrivNoninclusive::handleUnblockFlush(MemEvent* event, bool inMSHR) {
    event_debuginfo_.prefill(event->getID(), Command::UnblockFlush, "", 0, State::NP);

    broadcastMemEventToSources(Command::UnblockFlush, event, timestamp_ + 1);
    delete event;

    return true;
}


bool MESIPrivNoninclusive::handleAckInv(MemEvent * event, bool inMSHR) {
    Addr addr = event->getAddr();
    PrivateCacheLine * line = cacheArray_->lookup(addr, false);
    State state = line ? line->getState() : I;

    if (mem_h_is_debug_event(event))
        event_debuginfo_.prefill(event->getID(), Command::AckInv, "", addr, state);

    if (line) {
        line->setShared(false);
        line->setOwned(false);
    }
    responses.erase(addr);

    mshr_->decrementAcksNeeded(addr);

    stat_eventstate_[(int)Command::AckInv][state]->addData(1);

    switch (state) {
        case I:
            {
            MemEvent * req = static_cast<MemEvent*>(mshr_->getFrontEvent(addr));
            if (mshr_->hasData(addr) && req->getCmd() == Command::FetchInv)
                sendResponseDown(req, lineSize_, &(mshr_->getData(addr)), mshr_->getDataDirty(addr));
            else
                sendResponseDown(req, lineSize_, nullptr, false);
            cleanUpAfterResponse(event, inMSHR);
            return true;
            }
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

    delete event;

    if (mem_h_is_debug_addr(addr) && line) {
        event_debuginfo_.newst = line->getState();
        event_debuginfo_.verboseline = line->getString();
    }

    return true;
}


bool MESIPrivNoninclusive::handleAckPut(MemEvent * event, bool inMSHR) {
    stat_eventstate_[(int)Command::AckPut][I]->addData(1);

    if (mem_h_is_debug_event(event)) {
        event_debuginfo_.prefill(event->getID(), Command::AckPut, "", event->getBaseAddr(), I);
        event_debuginfo_.action = "Done";
    }
    cleanUpAfterResponse(event, inMSHR);
    return true;
}


/* We're using NULLCMD to signal an internally generated event - in this case an eviction */
bool MESIPrivNoninclusive::handleNULLCMD(MemEvent* event, bool inMSHR) {
    Addr oldAddr = event->getAddr();
    Addr newAddr = event->getBaseAddr();

    PrivateCacheLine * line = cacheArray_->lookup(oldAddr, false);

    if (mem_h_is_debug_addr(oldAddr))
        event_debuginfo_.prefill(event->getID(), Command::Evict, "", oldAddr, (line ? line->getState() : I));

    bool evicted = handleEviction(newAddr, line, event_debuginfo_);
    if (mem_h_is_debug_addr(oldAddr) || mem_h_is_debug_addr(newAddr) || mem_h_is_debug_addr(line->getAddr())) {
        event_debuginfo_.newst = line->getState();
        event_debuginfo_.verboseline = line->getString();
    }
    if (evicted) {
        notifyListenerOfEvict(line->getAddr(), lineSize_, event->getInstructionPointer());
        if (oldAddr != newAddr) {
            if (mshr_->exists(newAddr) && mshr_->getStalledForEvict(newAddr)) {
                debug_->debug(_L5_, "%s, Retry for 0x%" PRIx64 "\n", cachename_.c_str(), newAddr);
                retryBuffer_.push_back(mshr_->getFrontEvent(newAddr));
                mshr_->addPendingRetry(newAddr);
                mshr_->setStalledForEvict(newAddr, false);
                if (mem_h_is_debug_addr(newAddr)) {
                }
            }
            if (mshr_->removeEvictPointer(oldAddr, newAddr))
                retry(oldAddr);
        } else {
            cacheArray_->deallocate(line);
            mshr_->decrementFlushCount();
            if (mshr_->getFlushCount() == 0) {
                retryBuffer_.push_back(mshr_->getFlush());
            }
        }
    } else { // Check if we're waiting for a new address
        if (oldAddr != line ->getAddr()) { // We're waiting for a new line now...
            if (mem_h_is_debug_addr(oldAddr) || mem_h_is_debug_addr(line->getAddr()) || mem_h_is_debug_addr(newAddr)) {
                stringstream reason;
                reason << "New target, old was 0x" << std::hex << oldAddr;
                event_debuginfo_.reason = reason.str();
                event_debuginfo_.action = "Stall";
            }
            mshr_->insertEviction(line->getAddr(), newAddr);
            if (mshr_->removeEvictPointer(oldAddr, newAddr))
                retry(oldAddr);
        }
    }

    if (mem_h_is_debug_event(event) && line) {
        event_debuginfo_.newst = line->getState();
        event_debuginfo_.verboseline = line->getString();
    }

    delete event;
    return true;
}


bool MESIPrivNoninclusive::handleNACK(MemEvent* event, bool inMSHR) {
    MemEvent * nackedEvent = event->getNACKedEvent();
    Command cmd = nackedEvent->getCmd();
    Addr addr = nackedEvent->getBaseAddr();
    PrivateCacheLine * line = cacheArray_->lookup(event->getBaseAddr(), false);

    if (mem_h_is_debug_event(event) || mem_h_is_debug_event(nackedEvent)) {
        event_debuginfo_.prefill(event->getID(), Command::NACK, "", event->getBaseAddr(), (line ? line->getState() : I));
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
        case Command::PutX:
        case Command::FlushAll:
            resendEvent(nackedEvent, false); // Resend towards memory
            break;
        case Command::FetchInv:
        case Command::FetchInvX:
        case Command::Inv:
        case Command::ForceInv:
            /*if (mem_h_is_debug_addr(addr)) {
                if (responses.find(addr) != responses.end()) {
                    debug_->debug(_L3_, "\tResponse for 0x%" PRIx64 " is: %" PRIu64 "\n", addr, responses.find(addr)->second);
                } else {
                    debug_->debug(_L3_, "\tResponse for 0x%" PRIx64 " is: None\n", addr);
                }
            }*/
            if ((responses.find(addr) != responses.end()) && (responses.find(addr)->second == nackedEvent->getID())) {
                //if (mem_h_is_debug_addr(addr))
                //    debug_->debug(_L5_, "\tRetry NACKed event\n");
                resendEvent(nackedEvent, true); // Resend towards CPU
            } else {
                if (mem_h_is_debug_event(nackedEvent)) {
                    event_debuginfo_.action = "Drop";
                    stringstream reason;
                    reason << "already satisfied, event: " << nackedEvent->getBriefString();
                    event_debuginfo_.reason = reason.str();
                }
                   // debug_->debug(_L5_, "\tDrop NACKed event: %s\n", nackedEvent->getVerboseString().c_str());
                delete nackedEvent;
            }
            break;
        case Command::ForwardFlush:
                resendEvent(nackedEvent, true); // Resend towards CPU
                break;
        default:
            debug_->fatal(CALL_INFO,-1,"%s, Error: Received NACK with unhandled command type. Event: %s. NackedEvent: %s Time = %" PRIu64 "ns\n",
                    getName().c_str(), event->getVerboseString().c_str(), nackedEvent ? nackedEvent->getVerboseString().c_str() : "nullptr", getCurrentSimTimeNano());
    }
    return true;
}


/***********************************************************************************************************
 * MSHR & CacheArray management
 ***********************************************************************************************************/

/**
 * Allocate a new cache line
 */
MemEventStatus MESIPrivNoninclusive::allocateLine(MemEvent * event, PrivateCacheLine* &line, bool inMSHR) {

    evict_debuginfo_.prefill(event->getID(), Command::Evict, "", 0, I);

    bool evicted = handleEviction(event->getBaseAddr(), line, evict_debuginfo_);

    if (mem_h_is_debug_event(event) || mem_h_is_debug_addr(line->getAddr())) {
        evict_debuginfo_.newst = line->getState();
        evict_debuginfo_.verboseline = line->getString();
        event_debuginfo_.action = evict_debuginfo_.action;
        event_debuginfo_.reason = evict_debuginfo_.reason;
    }

    if (evicted) {
        notifyListenerOfEvict(line->getAddr(), lineSize_, event->getInstructionPointer());
        cacheArray_->replace(event->getBaseAddr(), line);
        if (mem_h_is_debug_addr(event->getBaseAddr()))
            printDebugAlloc(true, event->getBaseAddr(), "");
        return MemEventStatus::OK;
    } else {
        if (inMSHR || mshr_->insertEvent(event->getBaseAddr(), event, -1, false, true) != -1) {


            mshr_->insertEviction(line->getAddr(), event->getBaseAddr()); // Since we're private we never start an eviction (e.g., send invs) so it's safe to ignore an attempted eviction
            if (inMSHR) {
                mshr_->setStalledForEvict(event->getBaseAddr(), true);
            }
            if (mem_h_is_debug_event(event) || mem_h_is_debug_addr(line->getAddr())) {
                stringstream reason;
                reason << "New evict target 0x" << std::hex << line->getAddr();
                evict_debuginfo_.reason = reason.str();
                printDebugInfo(&evict_debuginfo_);
            }
            return MemEventStatus::Stall;
        }
        if (mem_h_is_debug_event(event) || mem_h_is_debug_addr(line->getAddr()))
            printDebugInfo(&evict_debuginfo_);
        return MemEventStatus::Reject;
    }
}


bool MESIPrivNoninclusive::handleEviction(Addr addr, PrivateCacheLine*& line, dbgin &diStruct) {
    if (!line)
        line = cacheArray_->findReplacementCandidate(addr);

    State state = line->getState();

    if (mem_h_is_debug_addr(addr) || mem_h_is_debug_addr(line->getAddr())) {
        diStruct.oldst = state;
        diStruct.addr = line->getAddr();
    }

    //if (mem_h_is_debug_addr(addr) || mem_h_is_debug_addr(line->getAddr()))
    //    debug_->debug(_L5_, "    Evicting line (0x%" PRIx64 ", %s)\n", line->getAddr(), StateString[state]);

    stat_evict[state]->addData(1);

    bool evict = false;
    bool wbSent = false;
    switch (state) {
        case I:
            if (mem_h_is_debug_addr(addr) || mem_h_is_debug_addr(line->getAddr())) {
                diStruct.action = "None";
                diStruct.reason = "already idle";
            }
            return true;
        case S:
            if (!mshr_->getPendingRetries(line->getAddr())) {
                if (!line->getShared() && !silentEvictClean_) {
                    uint64_t sendTime = sendWriteback(line->getAddr(), lineSize_, Command::PutS, line->getData(), false, line->getTimestamp());
                    line->setTimestamp(sendTime-1);
                    if (!lastLevel_) {
                        mshr_->insertWriteback(line->getAddr(), false);
                    }
                    if (mem_h_is_debug_addr(line->getAddr()))
                        printDebugAlloc(false, line->getAddr(), "Writeback");
                    if (mem_h_is_debug_addr(addr) || mem_h_is_debug_addr(line->getAddr())) {
                        diStruct.action = "Writeback";
                    }
                } else if (mem_h_is_debug_addr(addr) || mem_h_is_debug_addr(line->getAddr())) {
                    diStruct.action = "Drop";
                    if (mem_h_is_debug_addr(line->getAddr()))
                        printDebugAlloc(false, line->getAddr(), "Drop");
                }
                line->setState(I);
                return true;
            }
        case E:
            if (!mshr_->getPendingRetries(line->getAddr())) {
                if (line->getShared()) {
                    uint64_t sendTime = sendWriteback(line->getAddr(), lineSize_, Command::PutX, line->getData(), false, line->getTimestamp());
                    line->setTimestamp(sendTime-1);
                    if (!lastLevel_) {
                        mshr_->insertWriteback(line->getAddr(), true);
                    }
                    if (mem_h_is_debug_addr(addr) || mem_h_is_debug_addr(line->getAddr()))
                        diStruct.action = "Writeback";
                    if (mem_h_is_debug_addr(line->getAddr()))
                        printDebugAlloc(false, line->getAddr(), "Writeback");
                } else if (!line->getOwned() && !silentEvictClean_) {
                    uint64_t sendTime = sendWriteback(line->getAddr(), lineSize_, Command::PutE, line->getData(), false, line->getTimestamp());
                    line->setTimestamp(sendTime-1);
                    if (!lastLevel_) {
                        mshr_->insertWriteback(line->getAddr(), false);
                    }
                    if (mem_h_is_debug_addr(line->getAddr()))
                        printDebugAlloc(false, line->getAddr(), "Writeback");
                    if (mem_h_is_debug_addr(addr) || mem_h_is_debug_addr(line->getAddr()))
                        diStruct.action = "Writeback";
                } else if (mem_h_is_debug_addr(addr) || mem_h_is_debug_addr(line->getAddr())) {
                    if (mem_h_is_debug_addr(line->getAddr()))
                        printDebugAlloc(false, line->getAddr(), "Drop");
                    diStruct.action = "Drop";
                }
                line->setState(I);
                return true;
            }
        case M:
            if (!mshr_->getPendingRetries(line->getAddr())) {
                if (line->getShared()) {
                    Command cmd = lastLevel_ ? Command::PutM : Command::PutX;
                    uint64_t sendTime = sendWriteback(line->getAddr(), lineSize_, cmd, line->getData(), true, line->getTimestamp());
                    line->setTimestamp(sendTime-1);
                    if (!lastLevel_) {
                        mshr_->insertWriteback(line->getAddr(), true);
                    }
                    if (mem_h_is_debug_addr(addr) || mem_h_is_debug_addr(line->getAddr()))
                        diStruct.action = "Writeback";
                    if (mem_h_is_debug_addr(line->getAddr()))
                        printDebugAlloc(false, line->getAddr(), "Writeback");
                } else if (!line->getOwned()) {
                    uint64_t sendTime = sendWriteback(line->getAddr(), lineSize_, Command::PutM, line->getData(), true, line->getTimestamp());
                    line->setTimestamp(sendTime-1);
                    if (!lastLevel_) {
                        mshr_->insertWriteback(line->getAddr(), false);
                    }
                    if (mem_h_is_debug_addr(addr) || mem_h_is_debug_addr(line->getAddr())) {
                        diStruct.action = "Writeback";
                    }
                    if (mem_h_is_debug_addr(line->getAddr()))
                        printDebugAlloc(false, line->getAddr(), "Writeback");
                } else if (mem_h_is_debug_addr(addr) || mem_h_is_debug_addr(line->getAddr())) {
                    diStruct.action = "Drop";

                    if (mem_h_is_debug_addr(line->getAddr()))
                        printDebugAlloc(false, line->getAddr(), "Drop");
                }
                line->setState(I);
                return true;
            }
        case S_B:
        case S_Inv:
            if (!mshr_->getPendingRetries(line->getAddr())) {
                if (line->getShared()) { // This cache is not responsible for the block, drop it
                    line->setState(I);
                    if (mem_h_is_debug_addr(addr) || mem_h_is_debug_addr(line->getAddr())) {
                        diStruct.action = "Drop";
                    }
                    if (mem_h_is_debug_addr(line->getAddr()))
                        printDebugAlloc(false, line->getAddr(), "Drop");
                } else if (mem_h_is_debug_addr(addr) || mem_h_is_debug_addr(line->getAddr())) {
                    diStruct.action = "Stall";
                }
                return false;
            }
        case M_Inv:
        case E_Inv:
        case M_InvX:
        case E_InvX:
            if (!mshr_->getPendingRetries(line->getAddr())) {
                if (line->getOwned()) { // This cache is not responsible for the block, drop it
                    line->setState(I);
                    if (mem_h_is_debug_addr(addr) || mem_h_is_debug_addr(line->getAddr())) {
                        diStruct.action = "Drop";
                    }
                    if (mem_h_is_debug_addr(line->getAddr()))
                        printDebugAlloc(false, line->getAddr(), "Drop");
                    return true;
                } else if (mem_h_is_debug_addr(addr) || mem_h_is_debug_addr(line->getAddr()))
                    diStruct.action = "Stall";
                return false;
            }
        default:
            if (mem_h_is_debug_addr(addr) || mem_h_is_debug_addr(line->getAddr()))
                diStruct.action = "Stall";
            return false;
    }

    return false;
}


void MESIPrivNoninclusive::cleanUpEvent(MemEvent* event, bool inMSHR) {
    Addr addr = event->getBaseAddr();
    Command cmd = event->getCmd();

    /* Remove from MSHR */
    if (inMSHR) {
        mshr_->removeFront(addr);

        if (flush_state_ == FlushState::Drain) {
            mshr_->decrementFlushCount();
            // Retry flush if done draining
            if (mshr_->getFlushCount() == 0) {
                retryBuffer_.push_back(mshr_->getFlush());
            }
        }
    }

    delete event;
}


void MESIPrivNoninclusive::cleanUpAfterRequest(MemEvent* event, bool inMSHR) {
    Addr addr = event->getBaseAddr();
    Command cmd = event->getCmd();

    cleanUpEvent(event, inMSHR);

    /* Replay any waiting events */
    if (mshr_->exists(addr)) {
        if (mshr_->getFrontType(addr) == MSHREntryType::Event) {
            if (!mshr_->getInProgress(addr) && !mshr_->getStalledForEvict(addr) && mshr_->getAcksNeeded(addr) == 0) {
                retryBuffer_.push_back(mshr_->getFrontEvent(addr));
                mshr_->addPendingRetry(addr);
            }
        } else { // Pointer -> either we're waiting for a writeback ACK or another address is waiting for this one
            if (mshr_->getFrontType(addr) == MSHREntryType::Evict && mshr_->getAcksNeeded(addr) == 0) {
                std::list<Addr>* evictPointers = mshr_->getEvictPointers(addr);
                for (std::list<Addr>::iterator it = evictPointers->begin(); it != evictPointers->end(); it++) {
                    MemEvent * ev = new MemEvent(cachename_, addr, *it, Command::NULLCMD);
                    retryBuffer_.push_back(ev);
                }
            }
        }
    }
}


void MESIPrivNoninclusive::cleanUpAfterResponse(MemEvent* event, bool inMSHR) {
    Addr addr = event->getBaseAddr();

    /* Clean up matching request from MSHR MSHR */
    MemEvent * req = nullptr;
    if (mshr_->getFrontType(addr) == MSHREntryType::Event) {
        req = static_cast<MemEvent*>(mshr_->getFrontEvent(addr));
    }
    mshr_->removeFront(addr);

    // Clean up request and response
    delete event;
    if (req)
        delete req;

    if (mshr_->exists(addr)) {
        if (mshr_->getFrontType(addr) == MSHREntryType::Event) {
            if (!mshr_->getInProgress(addr) && mshr_->getAcksNeeded(addr) == 0) {
                retryBuffer_.push_back(mshr_->getFrontEvent(addr));
                mshr_->addPendingRetry(addr);
            }
        } else {
            if (mshr_->getAcksNeeded(addr) == 0) {
                std::list<Addr>* evictPointers = mshr_->getEvictPointers(addr);
                for (std::list<Addr>::iterator it = evictPointers->begin(); it != evictPointers->end(); it++) {
                    MemEvent * ev = new MemEvent(cachename_, addr, *it, Command::NULLCMD);
                    retryBuffer_.push_back(ev);
                }
            }
        }
    }
}


void MESIPrivNoninclusive::retry(Addr addr) {
    if (mshr_->exists(addr)) {
        if (mshr_->getFrontType(addr) == MSHREntryType::Event) {
            retryBuffer_.push_back(mshr_->getFrontEvent(addr));
            mshr_->addPendingRetry(addr);
        } else if (!(mshr_->pendingWriteback(addr))) {
            std::list<Addr>* evictPointers = mshr_->getEvictPointers(addr);
            for (std::list<Addr>::iterator it = evictPointers->begin(); it != evictPointers->end(); it++) {
                MemEvent * ev = new MemEvent(cachename_, addr, *it, Command::NULLCMD);
                retryBuffer_.push_back(ev);
            }
        }
    }
}


/***********************************************************************************************************
 * Protocol helper functions
 ***********************************************************************************************************/

uint64_t MESIPrivNoninclusive::sendExclusiveResponse(MemEvent * event, vector<uint8_t>* data, bool inMSHR, uint64_t time, bool dirty) {
    MemEvent * responseEvent = event->makeResponse();
    responseEvent->setCmd(Command::GetXResp);

    if (data) {
        responseEvent->setPayload(*data);
        responseEvent->setSize(data->size()); // Return size that was written
        if (mem_h_is_debug_event(event)) {
            printDataValue(event->getAddr(), data, false);
        }
        responseEvent->setDirty(dirty);
    }

    if (time < timestamp_) time = timestamp_;
    uint64_t deliveryTime = time + (inMSHR ? mshrLatency_ : accessLatency_);
    forwardByDestination(responseEvent, deliveryTime);

    // Debugging
    if (mem_h_is_debug_event(responseEvent))
        event_debuginfo_.action = "Respond";
//    if (mem_h_is_debug_event(responseEvent)) {
//        debug_->debug(_L3_, "Sending response at cycle %" PRIu64 ". Current time = %" PRIu64 ". Response = (%s)\n",
//                deliveryTime, timestamp_, responseEvent->getBriefString().c_str());
//    }

    return deliveryTime;
}

uint64_t MESIPrivNoninclusive::sendResponseUp(MemEvent * event, vector<uint8_t> * data, bool inMSHR, uint64_t time, Command cmd, bool success) {
    MemEvent * responseEvent = event->makeResponse();
    if (cmd != Command::NULLCMD)
        responseEvent->setCmd(cmd);

    if (data) {
        responseEvent->setPayload(*data);
        responseEvent->setSize(data->size()); // Return size that was written
        if (mem_h_is_debug_event(event)) {
            printDataValue(event->getAddr(), data, false);
        }
    }

    if (!success)
        responseEvent->setFail();

    // Compute latency, accounting for serialization of requests to the address
    if (time < timestamp_) time = timestamp_;
    uint64_t deliveryTime = time + (inMSHR ? mshrLatency_ : accessLatency_);
    forwardByDestination(responseEvent, deliveryTime);

    // Debugging
    if (mem_h_is_debug_event(responseEvent)) {
        event_debuginfo_.action = "Respond";
//        debug_->debug(_L3_, "Sending response at cycle %" PRIu64 ". Current time = %" PRIu64 ". Response = (%s)\n",
//                deliveryTime, timestamp_, responseEvent->getBriefString().c_str());
    }

    return deliveryTime;
}

void MESIPrivNoninclusive::sendResponseDown(MemEvent * event, uint32_t size, vector<uint8_t>* data, bool dirty) {
    MemEvent * responseEvent = event->makeResponse();

    if (data) {
        responseEvent->setPayload(*data);
        responseEvent->setDirty(dirty);
    }

    responseEvent->setSize(size);

    uint64_t deliveryTime = timestamp_ + (data ? accessLatency_ : tagLatency_);
    forwardByDestination(responseEvent, deliveryTime);

    if (mem_h_is_debug_event(responseEvent)) {
        event_debuginfo_.action = "Respond";
//        debug_->debug(_L3_, "Sending response at cycle %" PRIu64 ". Current time = %" PRIu64 ". Response = (%s)\n",
//                deliverTime, timestamp_, responseEvent->getBriefString().c_str());
    }
}


uint64_t MESIPrivNoninclusive::forwardFlush(MemEvent * event, bool evict, std::vector<uint8_t>* data, bool dirty, uint64_t time) {
    MemEvent * flush = new MemEvent(*event);

    uint64_t latency = tagLatency_;
    if (evict) {
        flush->setEvict(true);
        // TODO only send payload when needed
        flush->setPayload(*data);
        flush->setDirty(dirty);
        latency = accessLatency_;
    } else {
        flush->setPayload(0, nullptr);
    }
    bool payload = false;

    uint64_t baseTime = (time > timestamp_) ? time : timestamp_;
    uint64_t sendTime = baseTime + latency;
    forwardByAddress(flush, sendTime);

    if (mem_h_is_debug_addr(event->getBaseAddr())) {
        event_debuginfo_.action = "Forward";
        //debug_->debug(_L3_,"Forwarding Flush at cycle = %" PRIu64 ", Cmd = %s, Src = %s, %s\n", sendTime, CommandString[(int)flush->getCmd()], flush->getSrc().c_str(), evict ? "with data" : "without data");
    }
    return sendTime;
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

uint64_t MESIPrivNoninclusive::sendWriteback(Addr addr, uint32_t size, Command cmd, std::vector<uint8_t>* data, bool dirty, uint64_t startTime) {
    MemEvent* writeback = new MemEvent(cachename_, addr, addr, cmd);
    writeback->setSize(size);

    uint64_t latency = tagLatency_;

    /* Writeback data */
    if (dirty || writebackCleanBlocks_) {
        writeback->setPayload(*data);
        writeback->setDirty(dirty);

        if (mem_h_is_debug_addr(addr)) {
            printDataValue(addr, data, false);
        }

        latency = accessLatency_;
    }

    writeback->setRqstr(cachename_);

    uint64_t sendTime = timestamp_ > startTime ? timestamp_ : startTime;
    sendTime += latency;
    forwardByAddress(writeback, sendTime);

    return sendTime;
    //if (mem_h_is_debug_addr(addr))
        //debug_->debug(_L3_,"Sending Writeback at cycle = %" PRIu64 ", Cmd = %s. With%s data.\n", sendTime, CommandString[(int)cmd], ((cmd == Command::PutM || writebackCleanBlocks_) ? "" : "out"));
}


uint64_t MESIPrivNoninclusive::sendFwdRequest(MemEvent * event, Command cmd, std::string dst, uint32_t size, uint64_t startTime, bool inMSHR) {
    Addr addr = event->getBaseAddr();
    MemEvent * req = new MemEvent(cachename_, addr, addr, cmd);
    req->copyMetadata(event);
    req->setDst(dst);
    req->setSize(size);

    mshr_->incrementAcksNeeded(addr);

    responses.insert(std::make_pair(addr, req->getID()));  // Record events we're waiting for to avoid trying to figure out what happened if we get a NACK

    uint64_t baseTime = timestamp_ > startTime ? timestamp_ : startTime;
    uint64_t deliveryTime = (inMSHR) ? baseTime + mshrLatency_ : baseTime + tagLatency_;
    forwardByDestination(req, deliveryTime);
    if (mem_h_is_debug_addr(addr)) {
        event_debuginfo_.action = "Forward";
//        debug_->debug(_L7_, "Sending %s: Addr = 0x%" PRIx64 ", Dst = %s @ cycles = %" PRIu64 ".\n",
//                CommandString[(int)cmd], addr, dst.c_str(), deliveryTime);
    }
    return deliveryTime;
}

void MESIPrivNoninclusive::sendWritebackAck(MemEvent * event) {
    MemEvent * ack = event->makeResponse();

    uint64_t deliveryTime = timestamp_ + tagLatency_;
    forwardByDestination(ack, deliveryTime);

    if (mem_h_is_debug_event(event))
        event_debuginfo_.action = "Ack";
        //debug_->debug(_L7_, "Sending AckPut at cycle %" PRIu64 "\n", deliveryTime);
}

/*----------------------------------------------------------------------------------------------------------------------
 *  Override message send functions with versions that record statistics & call parent class
 *---------------------------------------------------------------------------------------------------------------------*/
void MESIPrivNoninclusive::forwardByAddress(MemEventBase* ev, Cycle_t timestamp) {
    stat_eventSent[(int)ev->getCmd()]->addData(1);
    CoherenceController::forwardByAddress(ev, timestamp);
}

void MESIPrivNoninclusive::forwardByDestination(MemEventBase* ev, Cycle_t timestamp) {
    stat_eventSent[(int)ev->getCmd()]->addData(1);
    CoherenceController::forwardByDestination(ev, timestamp);
}

/********************
 * Helper functions
 ********************/

MemEventInitCoherence* MESIPrivNoninclusive::getInitCoherenceEvent() {
    // Source, Endpoint type, inclusive, sends WB Acks, line size, tracks block presence
    return new MemEventInitCoherence(cachename_, Endpoint::Cache, false, true, true, lineSize_, true);
}

void MESIPrivNoninclusive::hasUpperLevelCacheName(std::string cachename) {
    upperCacheName_ = cachename;
}

void MESIPrivNoninclusive::printLine(Addr addr) {
    if (!mem_h_is_debug_addr(addr)) return;
    PrivateCacheLine * line = cacheArray_->lookup(addr, false);
    std::string state = (line == nullptr) ? "NP" : line->getString();
    debug_->debug(_L8_, "  Line 0x%" PRIx64 ": %s\n", addr, state.c_str());
}


void MESIPrivNoninclusive::recordLatency(Command cmd, int type, uint64_t latency) {
    if (type == -1)
        return;

    switch (cmd) {
        case Command::GetS:
            stat_latencyGetS[type]->addData(latency);
            break;
        case Command::GetX:
            stat_latencyGetX[type]->addData(latency);
            break;
        case Command::GetSX:
            stat_latencyGetSX[type]->addData(latency);
            break;
        case Command::FlushLine:
            stat_latencyFlushLine->addData(latency);
            break;
        case Command::FlushLineInv:
            stat_latencyFlushLineInv->addData(latency);
            break;
        default:
            break;
    }
}

/***********************************************************************************************************
 * Cache flush at simulation shutdown
 ***********************************************************************************************************/
void MESIPrivNoninclusive::beginCompleteStage() {
    flush_state_ = FlushState::Ready;
    shutdown_flush_counter_ = 0;
}

void MESIPrivNoninclusive::processCompleteEvent(MemEventInit* event, MemLinkBase* highlink, MemLinkBase* lowlink) {
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
        for (auto it : *cacheArray_) {
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
        delete event;
    } else if (event->getCmd() == Command::Write ) {
        event->setSrc(getName());
        lowlink->sendUntimedData(event, false, true);

        // Invalidate local line so we don't flush it
        PrivateCacheLine * line = cacheArray_->lookup(event->getAddr(), false);
        if (line) line->setState(I);
    } else {
        delete event; // Nothing for now
    }
}

std::set<Command> MESIPrivNoninclusive::getValidReceiveEvents() {
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
        Command::AckFlush,
        Command::UnblockFlush,
        Command::AckInv,
        Command::AckPut,
        Command::Write,
        Command::WriteResp,
        Command::NACK };
    return cmds;
}