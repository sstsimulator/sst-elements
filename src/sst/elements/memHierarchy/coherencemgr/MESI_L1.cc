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
#include "coherencemgr/MESI_L1.h"

using namespace SST;
using namespace SST::MemHierarchy;

/* Debug macros included from util.h */

/*----------------------------------------------------------------------------------------------------------------------
 * L1 Coherence Controller
 *---------------------------------------------------------------------------------------------------------------------*/

 /***********************************************************************************************************
 * Constructor
 ***********************************************************************************************************/
MESIL1::MESIL1(ComponentId_t id, Params& params, Params& owner_params, bool prefetch) :
    CoherenceController(id, params, owner_params, prefetch)
{
    params.insert(owner_params);

    snoop_l1_invs_ = params.find<bool>("snoop_l1_invalidations", false);
    bool MESI = params.find<bool>("protocol", true);
    llsc_block_cycles_ = params.find<Cycle_t>("llsc_block_cycles", 0);

    std::string frequency = params.find<std::string>("cache_frequency", "");

    llsc_timeout_ = configureSelfLink("llscTimeoutLink", frequency, new Event::Handler2<MESIL1, &MESIL1::handleLoadLinkExpiration>(this));

    // Coherence protocol transition states
    if (MESI) {
        protocol_read_state_ = E;
        protocol_exclusive_state_ = E;
    } else {
        protocol_read_state_ = S; // State to transition to when a GetXResp/clean is received in response to a read (GetS)
        protocol_exclusive_state_ = M; // State to transition to on a Read-exclusive/read-for-ownership
    }

    is_flushing_ = false;
    flush_drain_ = false;
    flush_complete_timestamp_ = 0;

    // Cache Array
    uint64_t lines = params.find<uint64_t>("lines", 0);
    uint64_t assoc = params.find<uint64_t>("associativity", 0);
    ReplacementPolicy * rmgr = createReplacementPolicy(lines, assoc, params, true);
    HashFunction * ht = createHashFunction(params);

    cache_array_ = new CacheArray<L1CacheLine>(debug_, lines, assoc, line_size_, rmgr, ht);
    cache_array_->setBanked(params.find<uint64_t>("banks", 0));

    // Register statistics
    stat_event_state_[(int)Command::GetS][I] =      registerStatistic<uint64_t>("stateEvent_GetS_I");
    stat_event_state_[(int)Command::GetS][S] =      registerStatistic<uint64_t>("stateEvent_GetS_S");
    stat_event_state_[(int)Command::GetS][M] =      registerStatistic<uint64_t>("stateEvent_GetS_M");
    stat_event_state_[(int)Command::GetX][I] =      registerStatistic<uint64_t>("stateEvent_GetX_I");
    stat_event_state_[(int)Command::GetX][S] =      registerStatistic<uint64_t>("stateEvent_GetX_S");
    stat_event_state_[(int)Command::GetX][M] =      registerStatistic<uint64_t>("stateEvent_GetX_M");
    stat_event_state_[(int)Command::GetSX][I] =     registerStatistic<uint64_t>("stateEvent_GetSX_I");
    stat_event_state_[(int)Command::GetSX][S] =     registerStatistic<uint64_t>("stateEvent_GetSX_S");
    stat_event_state_[(int)Command::GetSX][M] =     registerStatistic<uint64_t>("stateEvent_GetSX_M");
    stat_event_state_[(int)Command::GetSResp][IS] = registerStatistic<uint64_t>("stateEvent_GetSResp_IS");
    stat_event_state_[(int)Command::GetXResp][IS] = registerStatistic<uint64_t>("stateEvent_GetXResp_IS");
    stat_event_state_[(int)Command::GetXResp][IM] = registerStatistic<uint64_t>("stateEvent_GetXResp_IM");
    stat_event_state_[(int)Command::GetXResp][SM] = registerStatistic<uint64_t>("stateEvent_GetXResp_SM");
    stat_event_state_[(int)Command::Inv][I] =       registerStatistic<uint64_t>("stateEvent_Inv_I");
    stat_event_state_[(int)Command::Inv][S] =       registerStatistic<uint64_t>("stateEvent_Inv_S");
    stat_event_state_[(int)Command::Inv][IS] =      registerStatistic<uint64_t>("stateEvent_Inv_IS");
    stat_event_state_[(int)Command::Inv][IM] =      registerStatistic<uint64_t>("stateEvent_Inv_IM");
    stat_event_state_[(int)Command::Inv][SM] =      registerStatistic<uint64_t>("stateEvent_Inv_SM");
    stat_event_state_[(int)Command::Inv][S_B] =       registerStatistic<uint64_t>("stateEvent_Inv_SB");
    stat_event_state_[(int)Command::Inv][I_B] =       registerStatistic<uint64_t>("stateEvent_Inv_IB");
    stat_event_state_[(int)Command::FetchInvX][I] =   registerStatistic<uint64_t>("stateEvent_FetchInvX_I");
    stat_event_state_[(int)Command::FetchInvX][M] =   registerStatistic<uint64_t>("stateEvent_FetchInvX_M");
    stat_event_state_[(int)Command::FetchInvX][IS] =  registerStatistic<uint64_t>("stateEvent_FetchInvX_IS");
    stat_event_state_[(int)Command::FetchInvX][IM] =  registerStatistic<uint64_t>("stateEvent_FetchInvX_IM");
    stat_event_state_[(int)Command::FetchInvX][S_B] = registerStatistic<uint64_t>("stateEvent_FetchInvX_SB");
    stat_event_state_[(int)Command::FetchInvX][I_B] = registerStatistic<uint64_t>("stateEvent_FetchInvX_IB");
    stat_event_state_[(int)Command::Fetch][I] =       registerStatistic<uint64_t>("stateEvent_Fetch_I");
    stat_event_state_[(int)Command::Fetch][S] =       registerStatistic<uint64_t>("stateEvent_Fetch_S");
    stat_event_state_[(int)Command::Fetch][IS] =      registerStatistic<uint64_t>("stateEvent_Fetch_IS");
    stat_event_state_[(int)Command::Fetch][IM] =      registerStatistic<uint64_t>("stateEvent_Fetch_IM");
    stat_event_state_[(int)Command::Fetch][SM] =      registerStatistic<uint64_t>("stateEvent_Fetch_SM");
    stat_event_state_[(int)Command::Fetch][I_B] =     registerStatistic<uint64_t>("stateEvent_Fetch_IB");
    stat_event_state_[(int)Command::Fetch][S_B] =     registerStatistic<uint64_t>("stateEvent_Fetch_SB");
    stat_event_state_[(int)Command::FetchInv][I] =    registerStatistic<uint64_t>("stateEvent_FetchInv_I");
    stat_event_state_[(int)Command::FetchInv][S] =    registerStatistic<uint64_t>("stateEvent_FetchInv_S");
    stat_event_state_[(int)Command::FetchInv][M] =    registerStatistic<uint64_t>("stateEvent_FetchInv_M");
    stat_event_state_[(int)Command::FetchInv][IS] =   registerStatistic<uint64_t>("stateEvent_FetchInv_IS");
    stat_event_state_[(int)Command::FetchInv][IM] =   registerStatistic<uint64_t>("stateEvent_FetchInv_IM");
    stat_event_state_[(int)Command::FetchInv][SM] =   registerStatistic<uint64_t>("stateEvent_FetchInv_SM");
    stat_event_state_[(int)Command::FetchInv][S_B] =  registerStatistic<uint64_t>("stateEvent_FetchInv_SB");
    stat_event_state_[(int)Command::FetchInv][I_B] =  registerStatistic<uint64_t>("stateEvent_FetchInv_IB");
    stat_event_state_[(int)Command::ForceInv][I] =    registerStatistic<uint64_t>("stateEvent_ForceInv_I");
    stat_event_state_[(int)Command::ForceInv][S] =    registerStatistic<uint64_t>("stateEvent_ForceInv_S");
    stat_event_state_[(int)Command::ForceInv][M] =    registerStatistic<uint64_t>("stateEvent_ForceInv_M");
    stat_event_state_[(int)Command::ForceInv][IS] =   registerStatistic<uint64_t>("stateEvent_ForceInv_IS");
    stat_event_state_[(int)Command::ForceInv][IM] =   registerStatistic<uint64_t>("stateEvent_ForceInv_IM");
    stat_event_state_[(int)Command::ForceInv][SM] =   registerStatistic<uint64_t>("stateEvent_ForceInv_SM");
    stat_event_state_[(int)Command::ForceInv][S_B] =  registerStatistic<uint64_t>("stateEvent_ForceInv_SB");
    stat_event_state_[(int)Command::ForceInv][I_B] =  registerStatistic<uint64_t>("stateEvent_ForceInv_IB");
    stat_event_state_[(int)Command::FlushLine][I] =   registerStatistic<uint64_t>("stateEvent_FlushLine_I");
    stat_event_state_[(int)Command::FlushLine][S] =   registerStatistic<uint64_t>("stateEvent_FlushLine_S");
    stat_event_state_[(int)Command::FlushLine][M] =   registerStatistic<uint64_t>("stateEvent_FlushLine_M");
    stat_event_state_[(int)Command::FlushLineInv][I] =    registerStatistic<uint64_t>("stateEvent_FlushLineInv_I");
    stat_event_state_[(int)Command::FlushLineInv][S] =    registerStatistic<uint64_t>("stateEvent_FlushLineInv_S");
    stat_event_state_[(int)Command::FlushLineInv][M] =    registerStatistic<uint64_t>("stateEvent_FlushLineInv_M");
    stat_event_state_[(int)Command::FlushLineResp][I] =   registerStatistic<uint64_t>("stateEvent_FlushLineResp_I");
    stat_event_state_[(int)Command::FlushLineResp][I_B] = registerStatistic<uint64_t>("stateEvent_FlushLineResp_IB");
    stat_event_state_[(int)Command::FlushLineResp][S_B] = registerStatistic<uint64_t>("stateEvent_FlushLineResp_SB");
    stat_event_sent_[(int)Command::GetS] =            registerStatistic<uint64_t>("eventSent_GetS");
    stat_event_sent_[(int)Command::GetX] =            registerStatistic<uint64_t>("eventSent_GetX");
    stat_event_sent_[(int)Command::GetSX] =           registerStatistic<uint64_t>("eventSent_GetSX");
    stat_event_sent_[(int)Command::Write] =           registerStatistic<uint64_t>("eventSent_Write");
    stat_event_sent_[(int)Command::PutM] =            registerStatistic<uint64_t>("eventSent_PutM");
    stat_event_sent_[(int)Command::NACK] =            registerStatistic<uint64_t>("eventSent_NACK");
    stat_event_sent_[(int)Command::FlushLine] =       registerStatistic<uint64_t>("eventSent_FlushLine");
    stat_event_sent_[(int)Command::FlushLineInv] =    registerStatistic<uint64_t>("eventSent_FlushLineInv");
    stat_event_sent_[(int)Command::FlushAll] =         registerStatistic<uint64_t>("eventSent_FlushAll");
    stat_event_sent_[(int)Command::FetchResp] =       registerStatistic<uint64_t>("eventSent_FetchResp");
    stat_event_sent_[(int)Command::FetchXResp] =      registerStatistic<uint64_t>("eventSent_FetchXResp");
    stat_event_sent_[(int)Command::AckInv] =          registerStatistic<uint64_t>("eventSent_AckInv");
    stat_event_sent_[(int)Command::AckFlush] =        registerStatistic<uint64_t>("eventSent_AckFlush");
    stat_event_sent_[(int)Command::GetSResp] =        registerStatistic<uint64_t>("eventSent_GetSResp");
    stat_event_sent_[(int)Command::GetXResp] =        registerStatistic<uint64_t>("eventSent_GetXResp");
    stat_event_sent_[(int)Command::WriteResp] =       registerStatistic<uint64_t>("eventSent_WriteResp");
    stat_event_sent_[(int)Command::FlushLineResp] =   registerStatistic<uint64_t>("eventSent_FlushLineResp");
    stat_event_sent_[(int)Command::FlushAllResp] =    registerStatistic<uint64_t>("eventSent_FlushAllResp");
    stat_event_sent_[(int)Command::Put]           = registerStatistic<uint64_t>("eventSent_Put");
    stat_event_sent_[(int)Command::Get]           = registerStatistic<uint64_t>("eventSent_Get");
    stat_event_sent_[(int)Command::AckMove]       = registerStatistic<uint64_t>("eventSent_AckMove");
    stat_event_sent_[(int)Command::CustomReq]     = registerStatistic<uint64_t>("eventSent_CustomReq");
    stat_event_sent_[(int)Command::CustomResp]    = registerStatistic<uint64_t>("eventSent_CustomResp");
    stat_event_sent_[(int)Command::CustomAck]     = registerStatistic<uint64_t>("eventSent_CustomAck");
    stat_event_stalled_for_lock                = registerStatistic<uint64_t>("EventStalledForLockedCacheline");
    stat_evict_[I]                           = registerStatistic<uint64_t>("evict_I");
    stat_evict_[S]                           = registerStatistic<uint64_t>("evict_S");
    stat_evict_[M]                           = registerStatistic<uint64_t>("evict_M");
    stat_evict_[IS]                          = registerStatistic<uint64_t>("evict_IS");
    stat_evict_[IM]                          = registerStatistic<uint64_t>("evict_IM");
    stat_evict_[SM]                          = registerStatistic<uint64_t>("evict_SM");
    stat_evict_[I_B]                         = registerStatistic<uint64_t>("evict_SB");
    stat_evict_[S_B]                         = registerStatistic<uint64_t>("evict_SB");
    stat_latency_GetS_[LatType::HIT]          = registerStatistic<uint64_t>("latency_GetS_hit");
    stat_latency_GetS_[LatType::MISS]         = registerStatistic<uint64_t>("latency_GetS_miss");
    stat_latency_GetX_[LatType::HIT]          = registerStatistic<uint64_t>("latency_GetX_hit");
    stat_latency_GetX_[LatType::MISS]         = registerStatistic<uint64_t>("latency_GetX_miss");
    stat_latency_GetX_[LatType::UPGRADE]      = registerStatistic<uint64_t>("latency_GetX_upgrade");
    stat_latency_GetSX_[LatType::HIT]         = registerStatistic<uint64_t>("latency_GetSX_hit");
    stat_latency_GetSX_[LatType::MISS]        = registerStatistic<uint64_t>("latency_GetSX_miss");
    stat_latency_GetSX_[LatType::UPGRADE]     = registerStatistic<uint64_t>("latency_GetSX_upgrade");
    stat_latency_FlushLine_[LatType::HIT]     = registerStatistic<uint64_t>("latency_FlushLine");
    stat_latency_FlushLine_[LatType::MISS]    = registerStatistic<uint64_t>("latency_FlushLine_fail");
    stat_latency_FlushLineInv_[LatType::HIT]  = registerStatistic<uint64_t>("latency_FlushLineInv");
    stat_latency_FlushLineInv_[LatType::MISS] = registerStatistic<uint64_t>("latency_FlushLineInv_fail");
    stat_latency_FlushAll_                    = registerStatistic<uint64_t>("latency_FlushAll");
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

    /* Only for caches that expect writeback acks but don't know yet and can't register statistics later. Always enabled for now. */
    stat_event_state_[(int)Command::AckPut][I] = registerStatistic<uint64_t>("stateEvent_AckPut_I");

    /* Only for caches that don't silently drop clean blocks but don't know yet and can't register statistics later. Always enabled for now. */
    stat_event_sent_[(int)Command::PutS] = registerStatistic<uint64_t>("eventSent_PutS");
    stat_event_sent_[(int)Command::PutE] = registerStatistic<uint64_t>("eventSent_PutE");

    // Only for caches that forward invs to the processor
    if (snoop_l1_invs_) {
        stat_event_sent_[(int)Command::Inv] = registerStatistic<uint64_t>("eventSent_Inv");
    }

    /* Prefetch statistics */
    if (prefetch) {
        stat_prefetch_evict_ = registerStatistic<uint64_t>("prefetch_evict");
        stat_prefetch_inv_ = registerStatistic<uint64_t>("prefetch_inv");
        stat_prefetch_hit_ = registerStatistic<uint64_t>("prefetch_useful");
        stat_prefetch_upgrade_miss_ = registerStatistic<uint64_t>("prefetch_coherence_miss");
        stat_prefetch_redundant_ = registerStatistic<uint64_t>("prefetch_redundant");
    }

    /* MESI-specific statistics (as opposed to MSI) */
    if (MESI) {
        stat_event_state_[(int)Command::GetS][E]          = registerStatistic<uint64_t>("stateEvent_GetS_E");
        stat_event_state_[(int)Command::GetX][E]          = registerStatistic<uint64_t>("stateEvent_GetX_E");
        stat_event_state_[(int)Command::GetSX][E]         = registerStatistic<uint64_t>("stateEvent_GetSX_E");
        stat_event_state_[(int)Command::FlushLine][E]     = registerStatistic<uint64_t>("stateEvent_FlushLine_E");
        stat_event_state_[(int)Command::FlushLineInv][E]  = registerStatistic<uint64_t>("stateEvent_FlushLineInv_E");
        stat_event_state_[(int)Command::FetchInv][E]      = registerStatistic<uint64_t>("stateEvent_FetchInv_E");
        stat_event_state_[(int)Command::ForceInv][E]      = registerStatistic<uint64_t>("stateEvent_ForceInv_E");
        stat_event_state_[(int)Command::FetchInvX][E]     = registerStatistic<uint64_t>("stateEvent_FetchInvX_E");
        stat_evict_[E]                                   = registerStatistic<uint64_t>("evict_E");
    }
}


/***********************************************************************************************************
 * Event handlers
 ***********************************************************************************************************/

/*
 * Handle GetS (load/read) request
 * GetS may be the start of an LLSC
 */
bool MESIL1::handleGetS(MemEvent * event, bool in_mshr) {
    Addr addr = event->getBaseAddr();
    L1CacheLine * line = cache_array_->lookup(addr, true);
    bool local_prefetch = event->isPrefetch() && (event->getRqstr() == cachename_);
    State state = line ?  line->getState() : I;
    uint64_t send_time = 0;
    MemEventStatus status = MemEventStatus::OK;
    vector<uint8_t> data;

    if (in_mshr)
        mshr_->removePendingRetry(addr);

    if (mem_h_is_debug_addr(addr)) {
        event_debuginfo_.prefill(event->getID(), event->getThreadID(), Command::GetS, (local_prefetch ? "-pref" : "" ), addr, state);
        event_debuginfo_.reason = "hit";
    }

    if (is_flushing_ && !in_mshr) {
        event_debuginfo_.action = "Reject";
        event_debuginfo_.reason = "Cache flush in progress";
        return false;
    }

    switch (state) {
        case I: /* Miss */
            status = processCacheMiss(event, line, in_mshr); // Attempt to allocate an MSHR entry and/or line
            if (status == MemEventStatus::OK) {
                line = cache_array_->lookup(addr, false);
                //eventProfileAndNotify(event, I, NotifyAccessType::READ, NotifyResultType::MISS, true, LatType::MISS);
                if (!mshr_->getProfiled(addr)) {
                    recordLatencyType(event->getID(), LatType::MISS);
                    stat_event_state_[(int)Command::GetS][I]->addData(1);
                    stat_miss_[0][in_mshr]->addData(1);
                    stat_misses_->addData(1);
                    notifyListenerOfAccess(event, NotifyAccessType::READ, NotifyResultType::MISS);
                    mshr_->setProfiled(addr);
                }
                send_time = forwardMessage(event, line_size_, 0, nullptr);
                line->setState(IS);
                line->setTimestamp(send_time);
                if (mem_h_is_debug_addr(addr))
                    event_debuginfo_.reason = "miss";
                mshr_->setInProgress(addr);
            } else {
                recordMiss(event->getID());
            }
            break;
        case S: /* Hit */
        case E:
        case M:
            if (!in_mshr || !mshr_->getProfiled(addr)) {
                recordLatencyType(event->getID(), LatType::HIT);
                stat_event_state_[(int)Command::GetS][state]->addData(1);
                stat_hit_[0][in_mshr]->addData(1);
                stat_hits_->addData(1);
                notifyListenerOfAccess(event, NotifyAccessType::READ, NotifyResultType::HIT);
            }

            if (local_prefetch) {
                stat_prefetch_redundant_->addData(1); // Unneccessary prefetch
                recordPrefetchLatency(event->getID(), LatType::HIT);
                cleanUpAfterRequest(event, in_mshr);
                break;
            }

            recordPrefetchResult(line, stat_prefetch_hit_);

            if (event->isLoadLink()) {
                line->atomicStart(timestamp_ + llsc_block_cycles_, event->getThreadID());
            }
            data.assign(line->getData()->begin() + (event->getAddr() - event->getBaseAddr()), line->getData()->begin() + (event->getAddr() - event->getBaseAddr() + event->getSize()));
            send_time = sendResponseUp(event, &data, in_mshr, line->getTimestamp());
            line->setTimestamp(send_time - 1);
            cleanUpAfterRequest(event, in_mshr);
            break;
        default:
            if (!in_mshr) {
                status = allocateMSHR(event, false);
            } else if (mem_h_is_debug_event(event)) {
                event_debuginfo_.action = "Stall";
            }
            break;
    }

    if (mem_h_is_debug_addr(addr) && line) {
        event_debuginfo_.new_state = line->getState();
        event_debuginfo_.verbose_line = line->getString();
    }

    return (status == MemEventStatus::Reject) ? false : true;
}


/*
 * Handle cacheable Write requests
 * May also be a store-conditional or write-unlock
 */
bool MESIL1::handleWrite(MemEvent* event, bool in_mshr) {
    return handleGetX(event, in_mshr);
}

/*
 * Handle cacheable GetX/Write requests
 * May also be store-conditional or write-unlock
 */
bool MESIL1::handleGetX(MemEvent* event, bool in_mshr) {
    Addr addr = event->getBaseAddr();
    L1CacheLine* line = cache_array_->lookup(addr, true);
    State state = line ? line->getState() : I;

    if (mem_h_is_debug_addr(addr))
        event_debuginfo_.prefill(event->getID(), event->getThreadID(), Command::GetX, (event->isStoreConditional() ? "-SC" : ""), addr, state);

    if (in_mshr)
        mshr_->removePendingRetry(addr);

    // Any event other than an unlock or store-conditional needs to stall for a cache flush
    if (is_flushing_ && !in_mshr && !event->isStoreConditional() && !event->queryFlag(MemEvent::F_LOCKED)) {
        event_debuginfo_.action = "Reject";
        event_debuginfo_.reason = "Cache flush in progress";
        return false;
    }

    /* Special case - if this is the last coherence level (e.g., just mem below),
     * can upgrade without forwarding request */
    if (state == S && last_level_) {
        state = M;
        line->setState(M);
    }

    MemEventStatus status = MemEventStatus::OK;
    bool success = true;
    uint64_t send_time = 0;

    switch (state) {
        case I:
            if (event->isStoreConditional()) { /* Check if we can process the event, if so, it failed */
                status = checkMSHRCollision(event, in_mshr);
                if (status == MemEventStatus::OK) {
                    send_time = sendResponseUp(event, nullptr, in_mshr, 0, false);
                    if (mem_h_is_debug_addr(addr))
                        event_debuginfo_.reason = "hit/fail";
                    cleanUpAfterRequest(event, in_mshr);
                    break;
                }
                break;
            }

            status = processCacheMiss(event, line, in_mshr);

            if (status == MemEventStatus::OK) {
                line = cache_array_->lookup(addr, false);
                if (!mshr_->getProfiled(addr)) {
                    notifyListenerOfAccess(event, NotifyAccessType::WRITE, NotifyResultType::MISS);
                    stat_event_state_[(int)Command::GetX][I]->addData(1);
                    stat_miss_[1][in_mshr]->addData(1);
                    stat_misses_->addData(1);
                    recordLatencyType(event->getID(), LatType::MISS);
                    mshr_->setProfiled(addr);
                }

                send_time = forwardMessage(event, line_size_, 0, nullptr, Command::GetX);
                line->setState(IM);
                line->setTimestamp(send_time);
                mshr_->setInProgress(addr);
                if (mem_h_is_debug_addr(addr))
                    event_debuginfo_.reason = "miss";
            } else {
                recordMiss(event->getID());
            }
            break;
        case S:
            if (event->isStoreConditional()) { /* Definitely failed */
                send_time = sendResponseUp(event, nullptr, in_mshr, line->getTimestamp(), false);
                line->setTimestamp(send_time-1);
                if (mem_h_is_debug_addr(addr))
                    event_debuginfo_.reason = "hit/fail";
                cleanUpAfterRequest(event, in_mshr);
                break;
            }

            status = processCacheMiss(event, line, in_mshr); // Just acquire an MSHR entry
            if (status == MemEventStatus::OK) {
                if (!mshr_->getProfiled(addr)) {
                    notifyListenerOfAccess(event, NotifyAccessType::WRITE, NotifyResultType::MISS);
                    recordLatencyType(event->getID(), LatType::UPGRADE);
                    stat_event_state_[(int)Command::GetX][S]->addData(1);
                    stat_miss_[1][in_mshr]->addData(1);
                    stat_misses_->addData(1);
                    mshr_->setProfiled(addr);
                }
                recordPrefetchResult(line, stat_prefetch_upgrade_miss_);

                send_time = forwardMessage(event, line_size_, 0, nullptr, Command::GetX);
                line->setState(SM);
                line->setTimestamp(send_time);
                mshr_->setInProgress(addr);
                if (mem_h_is_debug_addr(addr))
                    event_debuginfo_.reason = "miss";
            }
            break;
        case E:
            line->setState(M);
        case M:
            recordPrefetchResult(line, stat_prefetch_hit_);
            if (!in_mshr || !mshr_->getProfiled(addr)) {
                notifyListenerOfAccess(event, NotifyAccessType::WRITE, NotifyResultType::HIT);
                recordLatencyType(event->getID(), LatType::HIT);
                stat_event_state_[(int)Command::GetX][state]->addData(1);
                stat_hit_[1][in_mshr]->addData(1);
                stat_hits_->addData(1);
            }

            if (!event->isStoreConditional() || line->isAtomic(event->getThreadID())) { // Don't write on a non-atomic SC
                line->setData(event->getPayload(), event->getAddr() - event->getBaseAddr());
                line->atomicEnd();
                if (mem_h_is_debug_addr(addr))
                    printDataValue(addr, line->getData(), true);
            } else {
                success = false;
            }
            if (event->queryFlag(MemEvent::F_LOCKED)) {
                line->decLock();
            }

            send_time = sendResponseUp(event, nullptr, in_mshr, line->getTimestamp(), success);
            line->setTimestamp(send_time-1);
            if (mem_h_is_debug_addr(addr)) {
                if (success) event_debuginfo_.reason = "hit";
                else event_debuginfo_.reason = "hit/fail";
            }
            cleanUpAfterRequest(event, in_mshr);
            break;
        default:
            if (!in_mshr) {
                status = allocateMSHR(event, false);
            } else if (mem_h_is_debug_addr(addr)) {
                event_debuginfo_.action = "Stall";
            }
            break;
    }

    if (mem_h_is_debug_addr(addr) && line) {
        event_debuginfo_.new_state = line->getState();
        event_debuginfo_.verbose_line = line->getString();
    }

    return (status == MemEventStatus::Reject) ? false: true;
}


/*
 * Handle GetSX (read-exclusive) request
 * GetSX acquires a line in exclusive or modified state
 *  F_LOCKED: Lock line until future GetX arrives. Line can preemptively be put into M (dirty) state
 *  F_LLSC: Flag line as atomic and watch for accesses. Line should be put in E state if possible, otherwise M.
 */
bool MESIL1::handleGetSX(MemEvent* event, bool in_mshr) {
    Addr addr = event->getBaseAddr();
    L1CacheLine * line = cache_array_->lookup(addr, true);
    State state = line ? line->getState() : I;

    if (in_mshr)
        mshr_->removePendingRetry(addr);

    if (mem_h_is_debug_addr(addr))
        event_debuginfo_.prefill(event->getID(), event->getThreadID(), Command::GetSX, (event->isLoadLink() ? "-LL" : ""), addr, state);

    if (is_flushing_ && !in_mshr) {
        event_debuginfo_.action = "Reject";
        event_debuginfo_.reason = "Cache flush in progress";
        return false;
    }

    /* Special case - if this is the last coherence level (e.g., just mem below),
     * can upgrade without forwarding request */
    if (state == S && last_level_) {
        state = protocol_exclusive_state_;
        line->setState(protocol_exclusive_state_);
    }

    MemEventStatus status = MemEventStatus::OK;
    uint64_t send_time = 0;
    vector<uint8_t> data;

    switch (state) {
        case I:
            status = processCacheMiss(event, line, in_mshr);

            if (status == MemEventStatus::OK) {
                line = cache_array_->lookup(addr, false);
                if (!mshr_->getProfiled(addr)) {
                    notifyListenerOfAccess(event, NotifyAccessType::READ, NotifyResultType::MISS);
                    stat_event_state_[(int)Command::GetSX][I]->addData(1);
                    stat_miss_[2][in_mshr]->addData(1);
                    stat_misses_->addData(1);
                    recordLatencyType(event->getID(), LatType::MISS);
                    mshr_->setProfiled(addr);
                }

                send_time = forwardMessage(event, line_size_, 0, nullptr);
                line->setState(IM);
                line->setTimestamp(send_time);
                mshr_->setInProgress(addr);
                if (mem_h_is_debug_addr(addr))
                    event_debuginfo_.reason = "miss";
            } else {
                recordMiss(event->getID());
            }
            break;
        case S:
            status = processCacheMiss(event, line, in_mshr); // Just acquire an MSHR entry
            if (status == MemEventStatus::OK) {
                if (!mshr_->getProfiled(addr)) {
                    notifyListenerOfAccess(event, NotifyAccessType::WRITE, NotifyResultType::MISS);
                    recordLatencyType(event->getID(), LatType::UPGRADE);
                    stat_event_state_[(int)Command::GetSX][S]->addData(1);
                    stat_miss_[2][in_mshr]->addData(1);
                    stat_misses_->addData(1);
                    mshr_->setProfiled(addr);
                }
                recordPrefetchResult(line, stat_prefetch_upgrade_miss_);

                send_time = forwardMessage(event, line_size_, 0, nullptr);
                line->setState(SM);
                line->setTimestamp(send_time);
                mshr_->setInProgress(addr);
                if (mem_h_is_debug_addr(addr))
                    event_debuginfo_.reason = "miss";
            }
            break;
        case E:
        case M:
            recordPrefetchResult(line, stat_prefetch_hit_);
            if (!in_mshr || !mshr_->getProfiled(addr)) {
                notifyListenerOfAccess(event, NotifyAccessType::READ, NotifyResultType::HIT);
                recordLatencyType(event->getID(), LatType::HIT);
                stat_event_state_[(int)Command::GetSX][state]->addData(1);
                stat_hit_[2][in_mshr]->addData(1);
                stat_hits_->addData(1);
            }
            if (event->isLoadLink()) {
                line->atomicStart(timestamp_ + llsc_block_cycles_, event->getThreadID());
            }
            else
                line->incLock();
            data.assign(line->getData()->begin() + (event->getAddr() - event->getBaseAddr()), line->getData()->begin() + (event->getAddr() - event->getBaseAddr() + event->getSize()));
            send_time = sendResponseUp(event, &data, in_mshr, line->getTimestamp());
            line->setTimestamp(send_time - 1);
            cleanUpAfterRequest(event, in_mshr);
            if (mem_h_is_debug_addr(addr))
                event_debuginfo_.reason = "hit";
            break;
        default:
            if (!in_mshr) {
                status = allocateMSHR(event, false);
            } else if (mem_h_is_debug_addr(addr)) {
                event_debuginfo_.action = "Stall";
            }
            break;
    }

    if (mem_h_is_debug_addr(addr) && line) {
        event_debuginfo_.new_state = line->getState();
        event_debuginfo_.verbose_line = line->getString();
    }

    return (status == MemEventStatus::Reject) ? false: true;
}

bool MESIL1::handleFlushLine(MemEvent* event, bool in_mshr) {
    Addr addr = event->getBaseAddr();
    L1CacheLine* line = cache_array_->lookup(addr, false);
    State state = line ? line->getState() : I;

    if (mem_h_is_debug_addr(addr))
        event_debuginfo_.prefill(event->getID(), event->getThreadID(), Command::FlushLine, "", addr, state);

    if (is_flushing_ && !in_mshr) {
        event_debuginfo_.action = "Reject";
        event_debuginfo_.reason = "Cache flush in progress";
        return false;
    }

    if (!in_mshr && mshr_->exists(addr)) {
        return (allocateMSHR(event, false) == MemEventStatus::Reject) ? false : true;
    } else if (in_mshr) {
        mshr_->removePendingRetry(addr);
    }

    /* If we're here, state is stable */

    /* Flush fails if line is locked */
    if (state != I && line->isLocked(timestamp_)) {
        if (!in_mshr || !mshr_->getProfiled(addr)) {
            stat_event_state_[(int)Command::FlushLine][state]->addData(1);
            recordLatencyType(event->getID(), LatType::MISS);
        }
        sendResponseUp(event, nullptr, in_mshr, line->getTimestamp(), false);
        cleanUpAfterRequest(event, in_mshr);

        if (mem_h_is_debug_addr(addr)) {
            if (line)
                event_debuginfo_.verbose_line = line->getString();
            event_debuginfo_.reason = "fail, line locked";
        }

        return true;
    }

    /* If we're here, we need an MSHR entry if we don't already have one */
    if (!in_mshr && (allocateMSHR(event, false) == MemEventStatus::Reject))
        return false;

    if (!mshr_->getProfiled(addr)) {
        stat_event_state_[(int)Command::FlushLine][state]->addData(1);
        recordLatencyType(event->getID(), LatType::HIT);
        mshr_->setProfiled(addr);
    }

    mshr_->setInProgress(addr);

    bool downgrade = (state == E || state == M);
    forwardFlush(event, line, downgrade);

    if (line && state != I)
        line->setState(S_B);

    if (mem_h_is_debug_addr(addr) && line) {
        event_debuginfo_.new_state = line->getState();
        event_debuginfo_.verbose_line = line->getString();
    }

    return true;
}

bool MESIL1::handleFlushLineInv(MemEvent* event, bool in_mshr) {
    Addr addr = event->getBaseAddr();
    L1CacheLine* line = cache_array_->lookup(addr, false);
    State state = line ? line->getState() : I;

    if (mem_h_is_debug_addr(addr))
        event_debuginfo_.prefill(event->getID(), event->getThreadID(), Command::FlushLineInv, "", addr, state);

    if (is_flushing_ && !in_mshr) {
        event_debuginfo_.action = "Reject";
        event_debuginfo_.reason = "Cache flush in progress";
        return false;
    }

    if (!in_mshr && mshr_->exists(addr)) {
        return (allocateMSHR(event, false) != MemEventStatus::Reject);
    } else if (in_mshr) {
        mshr_->removePendingRetry(addr);

        if (!mshr_->getFrontEvent(addr) || mshr_->getFrontEvent(addr) != event) {
            if (mem_h_is_debug_event(event))
                event_debuginfo_.action = "Stall";
            return true;
        }
    }
    /* Flush fails if line is locked */
    if (state != I && line->isLocked(timestamp_)) {
        if (!in_mshr || !mshr_->getProfiled(addr)) {
            stat_event_state_[(int)Command::FlushLineInv][state]->addData(1);
            recordLatencyType(event->getID(), LatType::MISS);
        }
        sendResponseUp(event, nullptr, in_mshr, line->getTimestamp(), false);
        cleanUpAfterRequest(event, in_mshr);

        if (mem_h_is_debug_addr(addr)) {
            if (line)
                event_debuginfo_.verbose_line = line->getString();
            event_debuginfo_.reason = "fail, line locked";
        }
        return true;
    }

    /* If we're here, we need an MSHR entry if we don't already have one */
    if (!in_mshr && (allocateMSHR(event, false) == MemEventStatus::Reject)) {
        return false;
    }

    mshr_->setInProgress(addr);
    if (!mshr_->getProfiled(addr)) {
        stat_event_state_[(int)Command::FlushLineInv][state]->addData(1);
        if (line)
            recordPrefetchResult(line, stat_prefetch_evict_);
        mshr_->setProfiled(addr);
    }

    forwardFlush(event, line, state != I);
    if (state != I)
        line->setState(I_B);

    recordLatencyType(event->getID(), LatType::HIT);

    if (mem_h_is_debug_addr(addr) && line) {
        event_debuginfo_.new_state = line->getState();
        event_debuginfo_.verbose_line = line->getString();
    }
    return true;
}


bool MESIL1::handleFlushAll(MemEvent* event, bool in_mshr) {
    event_debuginfo_.prefill(event->getID(), Command::FlushAll, "", 0, State::NP);

    // A core shouldn't send another FlushAll while one is outstanding but just in case
    if (!in_mshr) {
        if (is_flushing_) {
            event_debuginfo_.action = "Reject";
            event_debuginfo_.reason = "Flush in progress";
            return false;
        }

        if (mshr_->insertFlush(event, false) == MemEventStatus::Reject) {
            if (mem_h_is_debug_event(event)) {
                event_debuginfo_.action = "Reject";
                event_debuginfo_.reason = "MSHR full";
            }
            return false;
        }

        is_flushing_ = true;
        if (!flush_manager_) {
            // Forward flush to flush manager
            MemEvent* flush = new MemEvent(*event); // Copy event for forwarding
            flush->setDst(flush_dest_);
            forwardByDestination(flush, timestamp_ + mshr_latency_); // Time to insert event in MSHR
            event_debuginfo_.action = "forward";
            return true;
        }
    }

    if (!flush_manager_) {
        debug_->fatal(CALL_INFO, -1, "%s, ERROR: Trying to retry a flushall but not the flush manager...\n", getName().c_str());
    }

    if (mshr_->getFlushSize() != mshr_->getSize()) { /* Wait for MSHR to drain */
        event_debuginfo_.action = "Drain MSHR";
        flush_drain_ = true;
        return true;
    }

    flush_drain_ = false;

    bool success = true;
    bool eviction_needed = false;
    for (auto it : *cache_array_) {
        if (it->isLocked(timestamp_)) {
            success = false; // No good, should not have issued a FlushAll between a lock/unlock!
            eviction_needed = false;
            break;
        }

        switch (it->getState()) {
            case I:
                break;
            case S:
            case E:
            case M:
                {
                MemEvent * ev = new MemEvent(cachename_, it->getAddr(), it->getAddr(), Command::NULLCMD);
                retry_buffer_.push_back(ev);
                eviction_needed = true;
                mshr_->incrementFlushCount();
                break;
                }
        default:
            debug_->fatal(CALL_INFO, -1, "%s, Error: Attempting to flush a cache line that is in a transient state '%s'. Addr = 0x%" PRIx64 ". Event: %s. Time: %" PRIu64 "ns\n",
                cachename_.c_str(), StateString[it->getState()], it->getAddr(), event->getVerboseString().c_str(), getCurrentSimTimeNano());
            break;
        }
    }

    if (!eviction_needed) {
        is_flushing_ = false;
        sendResponseUp(event, nullptr, true, timestamp_, success);
        cleanUpAfterFlush(event); // Remove FlushAll
        event_debuginfo_.action = "Respond";
    } else {
        event_debuginfo_.action = "Flush";
    }

    return true;
}


bool MESIL1::handleForwardFlush(MemEvent* event, bool in_mshr) {
    event_debuginfo_.prefill(event->getID(), Command::ForwardFlush, "", 0, State::NP);
    MemEventStatus status = MemEventStatus::OK;

    if (!in_mshr) {
        status = mshr_->insertFlush(event, true);
        if (status == MemEventStatus::Reject) {
            if (mem_h_is_debug_event(event)) {
                event_debuginfo_.action = "Reject";
                event_debuginfo_.reason = "MSHR full";
            }
            return false; /* No room for flush in MSHR */
        } else if (status == MemEventStatus::Stall) {
            event_debuginfo_.action = "Stall";
            event_debuginfo_.reason = "Flush in progress";
            return true;
        }
    }
    is_flushing_ = true;

    if (mshr_->getFlushSize() != mshr_->getSize()) { /* Wait for MSHR to drain */
        event_debuginfo_.action = "Drain MSHR";
        flush_drain_ = true;
        return true;
    }

    flush_drain_ = false;

    bool success = true;
    bool eviction_needed = false;
    for (auto it : *cache_array_) {
        if (it->isLocked(timestamp_)) {
            // Retry in a few cycles
            success = false;
            continue;
        }

        switch (it->getState()) {
            case I:
                break;
            case S:
            case E:
            case M:
                {
                MemEvent * ev = new MemEvent(cachename_, it->getAddr(), it->getAddr(), Command::NULLCMD);
                retry_buffer_.push_back(ev);
                eviction_needed = true;
                mshr_->incrementFlushCount();
                break;
                }
            default:
                debug_->fatal(CALL_INFO, -1, "%s, Error: Attempting to flush a cache line that is in a transient state '%s'. Addr = 0x%" PRIx64 ". Event: %s, Time: %" PRIu64 "ns\n",
                    cachename_.c_str(), StateString[it->getState()], it->getAddr(), event->getVerboseString().c_str(), getCurrentSimTimeNano());
                break;
        }
    }

    if (eviction_needed) {
        // if unsuccessful that is OK, we'll retry when the evictions complete
        event_debuginfo_.action = "Flush";
    } else if (!success) { // No choice but to keep retrying until the unlock occurs
        retry_buffer_.push_back(event);
        event_debuginfo_.action = "Retry";
    } else {
        MemEvent * response_event = event->makeResponse();
        uint64_t delivery_time = std::max(timestamp_ + tag_latency_, flush_complete_timestamp_ + 1);
        forwardByDestination(response_event, delivery_time);
        event_debuginfo_.action = "Respond";

        mshr_->removeFlush(); // Remove ForwardFlush
        delete event;

        MemEventBase* next_flush = mshr_->getFlush();
        if (next_flush != nullptr && next_flush->getCmd() == Command::ForwardFlush) {
            retry_buffer_.push_back(next_flush);
        }
    }

    return true;
}


bool MESIL1::handleUnblockFlush(MemEvent* event, bool in_mshr) {
    event_debuginfo_.prefill(event->getID(), Command::UnblockFlush, "", 0, State::NP);

    if (mshr_->getFlush() == nullptr)
        is_flushing_ = false;
    delete event;

    return true;
}


bool MESIL1::handleFetch(MemEvent* event, bool in_mshr) {
    Addr addr = event->getBaseAddr();
    L1CacheLine* line = cache_array_->lookup(addr, false);
    State state = line ? line->getState() : I;

    snoopInvalidation(event, line); // Let core know that line is being accessed elsewhere (for ARM/gem5)

    if (mem_h_is_debug_event(event))
        event_debuginfo_.prefill(event->getID(), Command::Fetch, "", event->getBaseAddr(), state);

    if (in_mshr)
        mshr_->removePendingRetry(addr);

    switch (state) {
        case S:
        case SM:
        case S_B:
            sendResponseDown(event, line, true);
            break;
        case I:
        case IS:
        case IM:
        case I_B:
            /* In these cases, an eviction raced with this request; ignore and wait for a response to our request */
            if (mem_h_is_debug_event(event))
                event_debuginfo_.action = "Ignore";
            break;
        default:
            debug_->fatal(CALL_INFO,-1,"%s, Error: Received Fetch in unhandled state '%s'. Event: %s. Time = %" PRIu64 "ns\n",
                    cachename_.c_str(), StateString[state], event->getVerboseString().c_str(), getCurrentSimTimeNano());
    }

    stat_event_state_[(int)Command::Fetch][state]->addData(1);

    delete event;
    return true;
}

bool MESIL1::handleInv(MemEvent* event, bool in_mshr) {
    Addr addr = event->getBaseAddr();
    L1CacheLine* line = cache_array_->lookup(addr, false);
    State state = line ? line->getState() : I;

    if (mem_h_is_debug_event(event))
        event_debuginfo_.prefill(event->getID(), Command::Inv, "", event->getBaseAddr(), state);

    if (in_mshr)
        mshr_->removePendingRetry(addr);

    snoopInvalidation(event, line);

    /* Note - not possible to receive an inv when the line is locked (locked implies state = E or M) */

    stat_event_state_[(int)Command::Inv][state]->addData(1);
    if (line)
        recordPrefetchResult(line, stat_prefetch_inv_);

    switch (state) {
        case S:
        case S_B: // Inv raced with our FlushLine; ordering Inv before FlushLine
            line->atomicEnd();
            sendResponseDown(event, line, false);
            line->setState(I);
            break;
        case SM: // Inv raced with our upgrade; ordering Inv before upgrade
            line->atomicEnd();
            sendResponseDown(event, line, false);
            line->setState(IM);
            break;
        case I_B:
            line->setState(I);
        case IS:
        case IM:
        case I:
            // Raced with Put* or FlushLineInv; will be resolved by arrival of our request
            if (mem_h_is_debug_event(event))
                event_debuginfo_.action = "Ignore";
            break;
        default:
            debug_->fatal(CALL_INFO, -1, "%s, Error: Received Inv in unhandled state '%s'. Event: %s, Time = %" PRIu64 "ns\n",
                    getName().c_str(), StateString[state], event->getVerboseString().c_str(), getCurrentSimTimeNano());
    }

    if (mem_h_is_debug_event(event) && line) {
        event_debuginfo_.new_state = line->getState();
        event_debuginfo_.verbose_line = line->getString();
    }

    delete event;
    return true;
}

bool MESIL1::handleForceInv(MemEvent* event, bool in_mshr) {
    Addr addr = event->getBaseAddr();
    L1CacheLine* line = cache_array_->lookup(addr, false);
    State state = line ? line->getState() : I;

    if (in_mshr)
        mshr_->removePendingRetry(addr);

    snoopInvalidation(event, line);

    if (mem_h_is_debug_event(event))
        event_debuginfo_.prefill(event->getID(), Command::ForceInv, "", event->getBaseAddr(), state);

    switch (state) {
        case S:
        case E:
        case M:
        case S_B:
            if (line->isLocked(timestamp_)) {
                if (!in_mshr && allocateMSHR(event, true, 0) == MemEventStatus::Reject)
                    return false;
                else {
                    stat_event_stalled_for_lock->addData(1);
                    // If lock is on a timer, schedule a retry
                    // Otherwise, the unlock event will retry this one
                    uint64_t wakeupTime = line->getLLSCTime();
                    if (wakeupTime > timestamp_) {
                        llsc_timeout_->send(wakeupTime - timestamp_, new LoadLinkWakeup(addr, event->getID()));
                    } else {
                        retry_buffer_.push_back(mshr_->getFrontEvent(addr));
                        mshr_->addPendingRetry(addr);
                    }
                    if (mem_h_is_debug_event(event)) {
                        event_debuginfo_.action = "Stall";
                        event_debuginfo_.reason = "line locked";
                    }
                    return true;
                }
            }
            line->atomicEnd();
            sendResponseDown(event, line, false);
            line->setState(I);
            break;
        case I_B:
            line->setState(I);
        case I:
        case IS:
        case IM:
            if (mem_h_is_debug_event(event))
                event_debuginfo_.action = "Ignore";
            break;
        case SM:
            line->atomicEnd();
            sendResponseDown(event, line, false);
            line->setState(IM);
            break;
        default:
            debug_->fatal(CALL_INFO, -1, "%s, Error: Received ForceInv in unhandled state '%s'. Event: %s. Tme = %" PRIu64 "ns\n",
                    getName().c_str(), StateString[state], event->getVerboseString().c_str(), getCurrentSimTimeNano());
    }

    stat_event_state_[(int)Command::ForceInv][state]->addData(1);
    if (line) {
        recordPrefetchResult(line, stat_prefetch_inv_);

        if (mem_h_is_debug_event(event)) {
            event_debuginfo_.new_state = line->getState();
            event_debuginfo_.verbose_line = line->getString();
        }
    }

    return true;
}

bool MESIL1::handleFetchInv(MemEvent* event, bool in_mshr) {
    Addr addr = event->getBaseAddr();
    L1CacheLine* line = cache_array_->lookup(addr, false);
    State state = line ? line->getState() : I;

    if (in_mshr)
        mshr_->removePendingRetry(addr);

    snoopInvalidation(event, line);

    if (mem_h_is_debug_event(event))
        event_debuginfo_.prefill(event->getID(), Command::FetchInv, "", event->getBaseAddr(), state);

    switch (state) {
        case I:
        case IS:
        case IM:
            if (mem_h_is_debug_event(event))
                event_debuginfo_.action = "Ignore";
            stat_event_state_[(int)Command::FetchInv][state]->addData(1);
            delete event;
            return true;
        case E:
        case M:
            if (line->isLocked(timestamp_)) {
                if (!in_mshr && (allocateMSHR(event, true, 0) == MemEventStatus::Reject))
                    return false; /* Unable to allocate MSHR, must NACK */
                else {
                    stat_event_stalled_for_lock->addData(1);
                    // If lock is on a timer, schedule a retry
                    // Otherwise, the unlock event will retry this one
                    uint64_t wakeupTime = line->getLLSCTime();
                    if (wakeupTime > timestamp_) {
                        llsc_timeout_->send(wakeupTime - timestamp_, new LoadLinkWakeup(addr, event->getID()));
                    } else {
                        retry_buffer_.push_back(mshr_->getFrontEvent(addr));
                        mshr_->addPendingRetry(addr);
                    }
                    if (mem_h_is_debug_event(event)) {
                        event_debuginfo_.action = "Stall";
                        event_debuginfo_.reason = "line locked";
                    }
                    return true;
                }
            }
        case S:
        case S_B:
            line->atomicEnd();
            sendResponseDown(event, line, true);
            line->setState(I);
            break;
        case I_B:
            line->setState(I);
            if (mem_h_is_debug_event(event))
                event_debuginfo_.action = "Ignore";
            break;
        case SM:
            line->atomicEnd();
            sendResponseDown(event, line, true);
            line->setState(IM);
            break;
        default:
            debug_->fatal(CALL_INFO, -1, "%s, Error: Received FetchInv in unhandled state '%s'. Event: %s. Time = %" PRIu64 "ns\n",
                    getName().c_str(), StateString[state], event->getVerboseString().c_str(), getCurrentSimTimeNano());
    }

    stat_event_state_[(int)Command::FetchInv][state]->addData(1);

    if (line) {
        recordPrefetchResult(line, stat_prefetch_inv_);

        if (mem_h_is_debug_event(event)) {
            event_debuginfo_.new_state = line->getState();
            event_debuginfo_.verbose_line = line->getString();
        }
    }

    if (in_mshr)
        cleanUpAfterRequest(event, in_mshr);
    else
        delete event;
    return true;
}

bool MESIL1::handleFetchInvX(MemEvent* event, bool in_mshr) {
    Addr addr = event->getBaseAddr();
    L1CacheLine* line = cache_array_->lookup(addr, false);
    State state = line ? line->getState() : I;

    if (in_mshr)
        mshr_->removePendingRetry(addr);

    snoopInvalidation(event, line);

    if (mem_h_is_debug_event(event))
        event_debuginfo_.prefill(event->getID(), Command::FetchInvX, "", event->getBaseAddr(), state);

    switch (state) {
        case E:
        case M:
            if (line->isLocked(timestamp_)) {
                if (!in_mshr && (allocateMSHR(event, true, 0) == MemEventStatus::Reject)) {
                    return false;
                } else {
                    // If lock is on a timer, schedule a retry
                    // Otherwise, the unlock event will retry this one
                    uint64_t wakeupTime = line->getLLSCTime();
                    if (wakeupTime > timestamp_) {
                        llsc_timeout_->send(wakeupTime - timestamp_, new LoadLinkWakeup(addr, event->getID()));
                    } else {
                        retry_buffer_.push_back(mshr_->getFrontEvent(addr));
                        mshr_->addPendingRetry(addr);
                    }
                    stat_event_stalled_for_lock->addData(1);
                    if (mem_h_is_debug_event(event)) {
                        event_debuginfo_.action = "Stall";
                        event_debuginfo_.reason = "line locked";
                    }
                    return true;
                }
            }
            line->atomicEnd();
            sendResponseDown(event, line, true);
            line->setState(S);
            break;
        case I:
        case IS:
        case IM:
        case I_B:
        case S_B:
            if (mem_h_is_debug_event(event))
                event_debuginfo_.action = "Ignore";
            break;
        default:
            debug_->fatal(CALL_INFO, -1, "%s, Error: Received FetchInvX in unhandled state '%s'. Event: %s. Time = %" PRIu64 "ns\n",
                    getName().c_str(), StateString[state], event->getVerboseString().c_str(), getCurrentSimTimeNano());
    }

    stat_event_state_[(int)Command::FetchInvX][state]->addData(1);

    if (mem_h_is_debug_addr(event->getBaseAddr()) && line) {
        event_debuginfo_.new_state = line->getState();
        event_debuginfo_.verbose_line = line->getString();
    }
    if (in_mshr)
        cleanUpAfterRequest(event, in_mshr);
    else
        delete event;
    return true;
}

bool MESIL1::handleGetSResp(MemEvent* event, bool in_mshr) {
    Addr addr = event->getBaseAddr();
    L1CacheLine * line = cache_array_->lookup(addr, false);
    State state = line ? line->getState() : I;

    stat_event_state_[(int)Command::GetSResp][state]->addData(1);

    MemEvent * request = static_cast<MemEvent*>(mshr_->getFrontEvent(addr));
    bool local_prefetch = request->isPrefetch() && (request->getRqstr() == cachename_);

    if (mem_h_is_debug_addr(addr))
        event_debuginfo_.prefill(event->getID(), request->getThreadID(), Command::GetSResp, (local_prefetch ? "-pref" : ""), addr, state);

    request->setMemFlags(event->getMemFlags()); // Copy MemFlags through

    // Update line
    line->setData(event->getPayload(), 0);
    line->setState(S);
    if (mem_h_is_debug_addr(addr))
        printDataValue(addr, line->getData(), false);

    if (request->isLoadLink()) {
        line->atomicStart(timestamp_ + llsc_block_cycles_, request->getThreadID());
    }

    if (mem_h_is_debug_addr(addr))
        printDataValue(addr, line->getData(), true);

    if (local_prefetch) {
        line->setPrefetch(true);
        recordPrefetchLatency(request->getID(), LatType::MISS);
        if (mem_h_is_debug_addr(addr))
            event_debuginfo_.action = "Done";
    } else {
        request->setMemFlags(event->getMemFlags());
        Addr offset = request->getAddr() - addr;
        vector<uint8_t> data(line->getData()->begin() + offset, line->getData()->begin() + offset + request->getSize());
        uint64_t send_time = sendResponseUp(request, &data, true, line->getTimestamp());
        line->setTimestamp(send_time-1);
    }

    if (mem_h_is_debug_addr(addr)) {
        event_debuginfo_.new_state = line->getState();
        event_debuginfo_.verbose_line = line->getString();
    }

    cleanUpAfterResponse(event, in_mshr);
    return true;
}


bool MESIL1::handleGetXResp(MemEvent* event, bool in_mshr) {
    Addr addr = event->getBaseAddr();
    L1CacheLine * line = cache_array_->lookup(addr, false);
    State state = line ? line->getState() : I;

    stat_event_state_[(int)Command::GetXResp][state]->addData(1);

    MemEvent * request = static_cast<MemEvent*>(mshr_->getFrontEvent(addr));
    bool local_prefetch = request->isPrefetch() && (request->getRqstr() == cachename_);

    if (mem_h_is_debug_addr(addr)) {
        std::string mod = local_prefetch ? "-pref" : (request->isLoadLink() ? "-LL" : (request->isStoreConditional() ? "-SC" : ""));
        event_debuginfo_.prefill(event->getID(), request->getThreadID(), Command::GetXResp, mod, addr, state);
    }
    request->setMemFlags(event->getMemFlags()); // Copy MemFlags through

    std::vector<uint8_t> data;
    Addr offset = request->getAddr() - addr;
    bool success = true;

    switch (state) {
        case IS:
            {
                line->setData(event->getPayload(), 0);
                if (mem_h_is_debug_addr(addr))
                    printDataValue(addr, line->getData(), true);

                if (event->getDirty()) {
                    line->setState(M); // Sometimes get dirty data from a noninclusive cache
                } else {
                    line->setState(protocol_read_state_); // E (MESI) or S (MSI)
                }

                if (local_prefetch) {
                    line->setPrefetch(true);
                    recordPrefetchLatency(request->getID(), LatType::MISS);
                } else {
                    data.assign(line->getData()->begin() + offset, line->getData()->begin() + offset + request->getSize());
                    uint64_t send_time = sendResponseUp(request, &data, true, line->getTimestamp());
                    line->setTimestamp(send_time - 1);
                }
                break;
            }
        case IM:
            line->setData(event->getPayload(), 0);
            if (mem_h_is_debug_addr(addr))
                printDataValue(addr, line->getData(), true);
        case SM:
            {
                line->setState(M);

                if (request->getCmd() == Command::Write || request->getCmd() == Command::GetX) {
                    if (!request->isStoreConditional() || line->isAtomic(request->getThreadID())) { // Normal or successful store-conditional
                        line->setData(request->getPayload(), offset);

                        if (mem_h_is_debug_addr(addr))
                            printDataValue(addr, line->getData(), true);
                        line->atomicEnd(); // Any write causes a future SC to fail
                    } else {
                        success = false;
                    }

                    if (request->queryFlag(MemEventBase::F_LOCKED)) {
                        line->decLock();
                    }
                } else if (request->isLoadLink()) {
                    if (!event->getDirty()) {
                        line->setState(protocol_exclusive_state_);
                    }
                    line->atomicStart(timestamp_ + llsc_block_cycles_, request->getThreadID());
                } else { // ReadLock
                    line->incLock();
                }
                data.assign(line->getData()->begin() + offset, line->getData()->begin() + offset + request->getSize());
                uint64_t send_time = sendResponseUp(request, &data, true, line->getTimestamp(), success);
                line->setTimestamp(send_time-1);
                break;
            }
        default:
            debug_->fatal(CALL_INFO, -1, "%s, Error: Received GetXResp in unhandled state '%s'. Event: %s. Time = %" PRIu64 "ns\n",
                    getName().c_str(), StateString[state], event->getVerboseString().c_str(), getCurrentSimTimeNano());
    }

    cleanUpAfterResponse(event, in_mshr);

    if (mem_h_is_debug_addr(addr)) {
        event_debuginfo_.new_state = line->getState();
        event_debuginfo_.verbose_line = line->getString();
        if (!success)
            event_debuginfo_.reason = "hit/fail";
    }

    return true;
}


bool MESIL1::handleFlushLineResp(MemEvent * event, bool in_mshr) {
    Addr addr = event->getBaseAddr();
    L1CacheLine * line = cache_array_->lookup(addr, false);
    State state = line ? line->getState() : I;

    stat_event_state_[(int)Command::FlushLineResp][state]->addData(1);

    MemEvent * request = static_cast<MemEvent*>(mshr_->getFrontEvent(addr));

    if (mem_h_is_debug_addr(addr))
        event_debuginfo_.prefill(event->getID(), request->getThreadID(), Command::FlushLineResp, "", addr, state);

    switch (state) {
        case I:
            break;
        case I_B:
            line->setState(I);
            line->atomicEnd();
            break;
        case S_B:
            line->setState(S);
            break;
        default:
            debug_->fatal(CALL_INFO, -1, "%s, Error: Received FlushLineResp in unhandled state '%s'. Event: %s. Time = %" PRIu64 "ns\n",
                    getName().c_str(), StateString[state], event->getVerboseString().c_str(), getCurrentSimTimeNano());
    }

    sendResponseUp(request, nullptr, timestamp_, event->success());

    if (mem_h_is_debug_event(event)) {
        event_debuginfo_.action = "Respond";
        if (line) {
            event_debuginfo_.verbose_line = line->getString();
            event_debuginfo_.new_state = line->getState();
        }
    }

    cleanUpAfterResponse(event, in_mshr);
    return true;
}


bool MESIL1::handleFlushAllResp(MemEvent * event, bool in_mshr) {
    MemEvent* flush_request = static_cast<MemEvent*>(mshr_->getFlush());
    mshr_->removeFlush(); // Remove FlushAll

    event_debuginfo_.prefill(event->getID(), flush_request->getThreadID(), Command::FlushAllResp, "", 0, State::NP);
    event_debuginfo_.action = "Respond";

    sendResponseUp(flush_request, nullptr, true, timestamp_);

    delete flush_request;
    delete event;

    if (mshr_->getFlush() != nullptr) {
        retry_buffer_.push_back(mshr_->getFlush());
    }

    return true;
}


bool MESIL1::handleAckPut(MemEvent * event, bool in_mshr) {
    Addr addr = event->getBaseAddr();
    L1CacheLine * line = cache_array_->lookup(addr, false);
    State state = line ? line->getState() : I;

    stat_event_state_[(int)Command::AckPut][state]->addData(1);

    if (mem_h_is_debug_addr(addr)) {
        event_debuginfo_.prefill(event->getID(), Command::AckPut, "", addr, state);
    }

    cleanUpAfterResponse(event, in_mshr);
    return true;
}


/* We're using NULLCMD to signal an internally generated event - in this case an eviction */
bool MESIL1::handleNULLCMD(MemEvent * event, bool in_mshr) {
    Addr oldAddr = event->getAddr();
    Addr newAddr = event->getBaseAddr();

    L1CacheLine * line = cache_array_->lookup(oldAddr, false);

    bool evicted = handleEviction(newAddr, line, oldAddr == newAddr);

    if (mem_h_is_debug_addr(newAddr)) {
        event_debuginfo_.prefill(event->getID(), Command::NULLCMD, "", line->getAddr(), evict_debuginfo_.old_state);
        event_debuginfo_.new_state = line->getState();
        event_debuginfo_.verbose_line = line->getString();
    }

    if (evicted) {
        notifyListenerOfEvict(line->getAddr(), line_size_, event->getInstructionPointer(), event->getID());
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
    } else { // Could be stalling for a new address or locked line
        if (mem_h_is_debug_addr(newAddr)) {
            event_debuginfo_.action = "Stall";
            std::stringstream reason;
            reason << "0x" << std::hex << newAddr << ", evict failed";
                event_debuginfo_.reason = reason.str();
        }
        if (oldAddr != line ->getAddr()) { // We're waiting for a new line now...
            if (mem_h_is_debug_addr(line->getAddr()) || mem_h_is_debug_event(event)) {
                event_debuginfo_.action = "Stall";
                std::stringstream st;
                st << "0x" << std::hex << newAddr << ", new targ";
                event_debuginfo_.reason = st.str();
            }
            mshr_->insertEviction(line->getAddr(), newAddr);
            if (mshr_->removeEvictPointer(oldAddr, newAddr))
                retry(oldAddr);
        }
    }

    delete event;
    return true;
}


/* Handle a NACKed event */
bool MESIL1::handleNACK(MemEvent* event, bool in_mshr) {
    MemEvent* nackedEvent = event->getNACKedEvent();
    L1CacheLine* line = cache_array_->lookup(event->getBaseAddr(), false);
    State state = line ? line->getState() : I;

    if (mem_h_is_debug_addr(event->getBaseAddr()))
        event_debuginfo_.prefill(event->getID(), Command::NACK, "", event->getBaseAddr(), state);

    delete event;
    resendEvent(nackedEvent, false); // An L1 always resends NACKed events and they are always sent down

    return true;
}


/***********************************************************************************************************
 * MSHR and CacheArray management
 ***********************************************************************************************************/

/*
 * Handle a cache miss
 */
MemEventStatus MESIL1::processCacheMiss(MemEvent* event, L1CacheLine * line, bool in_mshr) {
    MemEventStatus status = MemEventStatus::OK;
    // Allocate an MSHR entry
    if (in_mshr) {
        status = MemEventStatus::Stall;
        if (mshr_->getFrontEvent(event->getBaseAddr())) {
            MemEvent* front = static_cast<MemEvent*>(mshr_->getFrontEvent(event->getBaseAddr()));
            status = (front == event) ? MemEventStatus::OK : MemEventStatus::Stall;
        }
    } else {
        status = allocateMSHR(event, false);
    }

    // Allocate a cache line
    if (status == MemEventStatus::OK && !line) {
        line = allocateLine(event, line);
        status = line ? MemEventStatus::OK : MemEventStatus::Stall;
    }
    return status;
}

/*
 * Check for an MSHR collision.
 * If one exists, put event into MSHR
 * Otherwise do nothing
 */
MemEventStatus MESIL1::checkMSHRCollision(MemEvent* event, bool in_mshr) {
    MemEventStatus status = MemEventStatus::OK;
    if (in_mshr) {
        status = MemEventStatus::Stall;
        if (mshr_->getFrontEvent(event->getBaseAddr())) {
            MemEvent* front = static_cast<MemEvent*>(mshr_->getFrontEvent(event->getBaseAddr()));
            status = (front == event) ? MemEventStatus::OK : MemEventStatus::Stall;
        }
    } else {
        int ins = mshr_->insertEventIfConflict(event->getBaseAddr(), event);
        if (ins == -1)
            status = MemEventStatus::Reject;
        else if (ins != 0)
            status = MemEventStatus::Stall;
    }
    return status;
}

/*
 * Allocate a new cache line
 */
L1CacheLine* MESIL1::allocateLine(MemEvent* event, L1CacheLine* line) {
    bool evicted = handleEviction(event->getBaseAddr(), line, false);

    if (evicted) {
        notifyListenerOfEvict(line->getAddr(), line_size_, event->getInstructionPointer(), event->getID());
        cache_array_->replace(event->getBaseAddr(), line);
        if (mem_h_is_debug_addr(event->getBaseAddr()))
            printDebugAlloc(true, event->getBaseAddr(), "");
        return line;
    } else {
        if (mem_h_is_debug_event(event)) {
            event_debuginfo_.action = "Stall";
            std::stringstream st;
            st << "evict 0x" << std::hex << line->getAddr() << ", ";
            event_debuginfo_.reason = st.str();
        }
        mshr_->insertEviction(line->getAddr(), event->getBaseAddr());
        return nullptr;
    }
}

/*
 * Evict a cacheline
 * Return whether successful and return line pointer via input parameters
 */
bool MESIL1::handleEviction(Addr addr, L1CacheLine*& line, bool flush) {
    if (!line) {
        line = cache_array_->findReplacementCandidate(addr);
    }
    State state = line->getState();

    if (mem_h_is_debug_addr(addr))
        evict_debuginfo_.old_state = line->getState();

    /* L1s can have locked cache lines which prevents replacement */
    if (line->isLocked(timestamp_)) {
        stat_event_stalled_for_lock->addData(1);
        if (mem_h_is_debug_addr(line->getAddr()))
            printDebugAlloc(false, line->getAddr(), "InProg, line locked");
        return false;
    }

    stat_evict_[state]->addData(1);

    switch (state) {
        case I:
            return true;
        case S:
            if (!mshr_->getPendingRetries(line->getAddr())) {
                if (!silent_evict_clean_) {
                    sendWriteback(Command::PutS, line, false, flush);
                    if (recv_writeback_ack_) {
                        mshr_->insertWriteback(line->getAddr(), false);
                    }
                    if (mem_h_is_debug_addr(line->getAddr()))
                        printDebugAlloc(false, line->getAddr(), "Writeback");
                } else if (mem_h_is_debug_addr(line->getAddr())) {
                        printDebugAlloc(false, line->getAddr(), "Drop");
                }
                line->setState(I);
                break;
            }
        case E:
            if (!mshr_->getPendingRetries(line->getAddr())) {
                if (!silent_evict_clean_) {
                    sendWriteback(Command::PutE, line, false, flush);
                    if (recv_writeback_ack_) {
                        mshr_->insertWriteback(line->getAddr(), false);
                    }
                    if (mem_h_is_debug_addr(line->getAddr()))
                        printDebugAlloc(false, line->getAddr(), "Writeback");
                } else if (mem_h_is_debug_addr(line->getAddr())) {
                        printDebugAlloc(false, line->getAddr(), "Drop");
                }
                line->setState(I);
                break;
            }
        case M:
            if (!mshr_->getPendingRetries(line->getAddr())) {
                sendWriteback(Command::PutM, line, true, flush);
                if (recv_writeback_ack_)
                    mshr_->insertWriteback(line->getAddr(), false);
                if (mem_h_is_debug_addr(line->getAddr()))
                    printDebugAlloc(false, line->getAddr(), "Writeback");
                line->setState(I);
                break;
            }
        default:
            if (mem_h_is_debug_addr(line->getAddr())) {
                stringstream note;
                if (mshr_->getPendingRetries(addr) != 0) {
                    note << "PendRetry, " << StateString[state];
                } else {
                    note << "InProg, " << StateString[state];
                }
                printDebugAlloc(false, line->getAddr(), note.str());
            }
            return false;
    }

    line->atomicEnd();
    recordPrefetchResult(line, stat_prefetch_evict_);
    return true;
}


/* Clean up MSHR state, events, etc. after a request. Also trigger any retries */
void MESIL1::cleanUpAfterRequest(MemEvent * event, bool in_mshr) {
    Addr addr = event->getBaseAddr();
    Command cmd = event->getCmd();

    /* Remove from MSHR */
    if (in_mshr) {
        if (event->isPrefetch() && event->getRqstr() == cachename_) outstanding_prefetch_count_--;
        mshr_->removeFront(addr);
    }

    delete event;

    /* Replay any waiting events */
    if (mshr_->exists(addr)) {
        if (mshr_->getFrontType(addr) == MSHREntryType::Event) {
            if (!mshr_->getInProgress(addr) && mshr_->getPendingRetries(addr) == 0) {
                retry_buffer_.push_back(mshr_->getFrontEvent(addr));
                mshr_->addPendingRetry(addr);
            }
        } else { // Pointer -> either we're waiting for a writeback ACK or another address is waiting to evict this one
            if (mshr_->getFrontType(addr) == MSHREntryType::Evict) {
                std::list<Addr>* evictPointers = mshr_->getEvictPointers(addr);
                for (std::list<Addr>::iterator it = evictPointers->begin(); it != evictPointers->end(); it++) {
                    MemEvent * ev = new MemEvent(cachename_, addr, *it, Command::NULLCMD);
                    retry_buffer_.push_back(ev);
                }
            }
        }
    } else if (is_flushing_ && flush_drain_ &&  mshr_->getSize() == mshr_->getFlushSize()) {
        retry_buffer_.push_back(mshr_->getFlush());
    }
}


/* Clean up MSHR state, events, etc. after a response. Also trigger any retries */
void MESIL1::cleanUpAfterResponse(MemEvent* event, bool in_mshr) {
    Addr addr = event->getBaseAddr();

    /* Clean up MSHR */
    MemEvent * request = nullptr;
    if (mshr_->getFrontType(addr) == MSHREntryType::Event)
        request = static_cast<MemEvent*>(mshr_->getFrontEvent(addr));
    mshr_->removeFront(addr); // delete request after this since debug might print the event it's removing
    delete event;
    if (request) {
        if (request->isPrefetch() && request->getRqstr() == cachename_) outstanding_prefetch_count_--;
        delete request;
    }

    /* Replay any waiting events */
    if (mshr_->exists(addr)) {
        if (mshr_->getFrontType(addr) == MSHREntryType::Event) {
            if (!mshr_->getInProgress(addr)) {
                retry_buffer_.push_back(mshr_->getFrontEvent(addr));
                mshr_->addPendingRetry(addr);
            }
        } else { // Pointer to an eviction
            std::list<Addr>* evictPointers = mshr_->getEvictPointers(addr);
            for (std::list<Addr>::iterator it = evictPointers->begin(); it != evictPointers->end(); it++) {
                MemEvent * ev = new MemEvent(cachename_, addr, *it, Command::NULLCMD);
                retry_buffer_.push_back(ev);
            }
        }
    } else if (is_flushing_ && flush_drain_ && mshr_->getSize() == mshr_->getFlushSize()) {
        retry_buffer_.push_back(mshr_->getFlush());
    }
}


/* Clean up MSHR state after a FlushAll or ForwardFlush completes */
void MESIL1::cleanUpAfterFlush(MemEvent* request, MemEvent* response, bool in_mshr) {
    if (in_mshr) mshr_->removeFlush();

    delete request;
    if (response) delete response;

    if (mshr_->getFlush() != nullptr) {
        retry_buffer_.push_back(mshr_->getFlush());
    } else {
        is_flushing_ = false;
    }
}


/* Retry the next event at address 'addr' */
void MESIL1::retry(Addr addr) {
    if (mshr_->exists(addr)) {
        if (mshr_->getFrontType(addr) == MSHREntryType::Event) {
            retry_buffer_.push_back(mshr_->getFrontEvent(addr));
            mshr_->addPendingRetry(addr);
        } else if (!(mshr_->pendingWriteback(addr))) {
            std::list<Addr>* evictPointers = mshr_->getEvictPointers(addr);
            for (std::list<Addr>::iterator it = evictPointers->begin(); it != evictPointers->end(); it++) {
                MemEvent * ev = new MemEvent(cachename_, addr, *it, Command::NULLCMD);
                retry_buffer_.push_back(ev);
            }
        }
    }
}

void MESIL1::handleLoadLinkExpiration(SST::Event* ev) {
    LoadLinkWakeup* ll = static_cast<LoadLinkWakeup*>(ev);

    if (mshr_->exists(ll->addr_) && (mshr_->getFrontType(ll->addr_) == MSHREntryType::Event) &&
            mshr_->getFrontEvent(ll->addr_)->getID() == ll->id_ && !mshr_->getInProgress(ll->addr_) &&
            (mshr_->getPendingRetries(ll->addr_) == 0)) {

        retry_buffer_.push_back(mshr_->getFrontEvent(ll->addr_));
        mshr_->addPendingRetry(ll->addr_);
        reenable_clock_();
    }

    fflush(stdout);
    delete ev;
}

/***********************************************************************************************************
 * Event creation and send
 ***********************************************************************************************************/

/*
 * Send a response event up towards core
 * Args:
 *  event: The request event that triggered this response
 *  data:  The data to be returned. **L1's return only the desired word so this should be ONLY the requested bytes
 *  in_mshr: Whether 'event' is in the MSHR (and required an MSHR access to look it up)
 *  time: The earliest time at which the requested cacheline can be accessed
 *  success: for requests that can fail (e.g., LLSC), whether the request was successful or not (default false)
 *
 *  Return: time that the requested cacheline can again be accessed
 */
uint64_t MESIL1::sendResponseUp(MemEvent* event, vector<uint8_t>* data, bool in_mshr, uint64_t time, bool success) {
    Command cmd = event->getCmd();
    MemEvent * response_event = event->makeResponse();

    uint64_t latency = in_mshr ? mshr_latency_ : tag_latency_;
    if (data) {
        response_event->setPayload(*data);
        response_event->setSize(data->size());
        if (mem_h_is_debug_event(event)) {
            printDataValue(event->getAddr(), data, false);
        }
        latency = access_latency_;
    }

    if (!success)
        response_event->setFail();

    // Compute latency, accounting for serialization of requests to the address
    uint64_t delivery_time = latency;
    if (time < timestamp_)
        delivery_time += timestamp_;
    else delivery_time += time;
    forwardByDestination(response_event, delivery_time);

    // Debugging
    if (mem_h_is_debug_event(response_event))
        event_debuginfo_.action = "Respond";

    return delivery_time;
}


/*
 * Send a response to a lower cache/directory/memory/etc.
 * E.g., AckInv, FetchResp, FetchXResp, etc.
 */
void MESIL1::sendResponseDown(MemEvent * event, L1CacheLine * line, bool data) {
    MemEvent * response_event = event->makeResponse();

    if (data) {
        response_event->setPayload(*line->getData());
        if (line->getState() == M)
            response_event->setDirty(true);
    }

    response_event->setSize(line_size_);

    uint64_t delivery_time = timestamp_ + (data ? access_latency_ : tag_latency_);
    forwardByDestination(response_event, delivery_time);

    if (mem_h_is_debug_event(response_event)) {
        event_debuginfo_.action = "Respond";
    }
}


/* Forward a flush to the next cache/directory/memory */
void MESIL1::forwardFlush(MemEvent* event, L1CacheLine* line, bool evict) {
    MemEvent* flush = new MemEvent(*event);

    uint64_t latency = tag_latency_; // Check coherence state/hitVmiss
    if (evict) {
        flush->setEvict(true);
        flush->setPayload(*(line->getData()));
        flush->setDirty(line->getState() == M);
        latency = access_latency_; // Time to check coherence & access data (in parallel)
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

    if (mem_h_is_debug_addr(event->getBaseAddr()))
        event_debuginfo_.action = "forward";
}


/*
 * Send a writeback
 * Latency: cache access + tag to read data that is being written back and update coherence state
 */
void MESIL1::sendWriteback(Command cmd, L1CacheLine * line, bool dirty, bool flush) {
    MemEvent* writeback = new MemEvent(cachename_, line->getAddr(), line->getAddr(), cmd);
    writeback->setSize(line_size_);

    uint64_t latency = tag_latency_;

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

    // If a full cache flush, we need to order flush w.r.t. *all* evictions
    if (flush && delivery_time > flush_complete_timestamp_) {
        flush_complete_timestamp_ = delivery_time;
    }


}


/* Send notification to the core that a line we have might have been lost */
void MESIL1::snoopInvalidation(MemEvent * event, L1CacheLine * line) {
    if (snoop_l1_invs_ && line) {
        for (auto it = system_cpu_names_.begin(); it != system_cpu_names_.end(); it++) {
            MemEvent * snoop = new MemEvent(cachename_, event->getAddr(), event->getBaseAddr(), Command::Inv);
            uint64_t base_time = timestamp_ > line->getTimestamp() ? timestamp_ : line->getTimestamp();
            uint64_t delivery_time = base_time + tag_latency_;
            snoop->setDst(*it);
            forwardByDestination(snoop, delivery_time);
        }
    }
}


/*----------------------------------------------------------------------------------------------------------------------
 *  Override message send functions with versions that record statistics & call parent class
 *---------------------------------------------------------------------------------------------------------------------*/
void MESIL1::forwardByAddress(MemEventBase* ev, Cycle_t ts) {
    stat_event_sent_[(int)ev->getCmd()]->addData(1);
    CoherenceController::forwardByAddress(ev, ts);
}

void MESIL1::forwardByDestination(MemEventBase* ev, Cycle_t ts) {
    stat_event_sent_[(int)ev->getCmd()]->addData(1);
    CoherenceController::forwardByDestination(ev, ts);
}



/***********************************************************************************************************
 * Statistics and listeners
 ***********************************************************************************************************/

/* Record result of a prefetch. Important: assumes line is not null */
void MESIL1::recordPrefetchResult(L1CacheLine* line, Statistic<uint64_t>* stat) {
    if (line->getPrefetch()) {
        stat->addData(1);
        line->setPrefetch(false);
    }
}


void MESIL1::recordLatency(Command cmd, int type, uint64_t latency) {
    if (type == -1)
        return; // Never set a hit/miss status
    switch (cmd) {
        case Command::GetS:
            stat_latency_GetS_[type]->addData(latency);
            break;
        case Command::Write:
        case Command::GetX:
            stat_latency_GetX_[type]->addData(latency);
            break;
        case Command::GetSX:
            stat_latency_GetSX_[type]->addData(latency);
            break;
        case Command::FlushLine:
            stat_latency_FlushLine_[type]->addData(latency);
            break;
        case Command::FlushLineInv:
            stat_latency_FlushLineInv_[type]->addData(latency);
            break;
        default:
            break;
    }
}


void MESIL1::eventProfileAndNotify(MemEvent * event, State state, NotifyAccessType type, NotifyResultType result, bool in_mshr) {
    if (!in_mshr || !mshr_->getProfiled(event->getBaseAddr())) {
        stat_event_state_[(int)event->getCmd()][state]->addData(1); // Profile event receive
        notifyListenerOfAccess(event, type, result);
        if (in_mshr)
            mshr_->setProfiled(event->getBaseAddr());
    }
}


/***********************************************************************************************************
 * Cache flush at simulation shutdown
 ***********************************************************************************************************/

void MESIL1::beginCompleteStage() {
    is_flushing_ = false;
}

void MESIL1::processCompleteEvent(MemEventInit* event, MemLinkBase* highlink, MemLinkBase* lowlink) {
    if (event->getInitCmd() == MemEventInit::InitCommand::Flush) {
        if (is_flushing_) { // Already in progress, ignore
            delete event;
            return;
        }

        is_flushing_ = true;
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

        if (!flush_manager_) { /* There's another cache/dir level below us; let it know we're done */
            MemEventUntimedFlush* response = new MemEventUntimedFlush(getName(), false);
            lowlink->sendUntimedData(response, true);
        }
    }
    delete event; // Nothing for now
}


/***********************************************************************************************************
 * Miscellaneous
 ***********************************************************************************************************/
MemEventInitCoherence* MESIL1::getInitCoherenceEvent() {
    return new MemEventInitCoherence(cachename_, Endpoint::Cache, true, false, false, line_size_, true);
}

std::set<Command> MESIL1::getValidReceiveEvents() {
    std::set<Command> cmds;
    cmds.insert(Command::GetS);
    cmds.insert(Command::GetX);
    cmds.insert(Command::Write);
    cmds.insert(Command::GetSX);
    cmds.insert(Command::FlushLine);
    cmds.insert(Command::FlushLineInv);
    cmds.insert(Command::FlushAll);
    cmds.insert(Command::ForwardFlush);
    cmds.insert(Command::Inv);
    cmds.insert(Command::ForceInv);
    cmds.insert(Command::Fetch);
    cmds.insert(Command::FetchInv);
    cmds.insert(Command::FetchInvX);
    cmds.insert(Command::NULLCMD);
    cmds.insert(Command::GetSResp);
    cmds.insert(Command::GetXResp);
    cmds.insert(Command::FlushLineResp);
    cmds.insert(Command::FlushAllResp);
    cmds.insert(Command::UnblockFlush);
    cmds.insert(Command::AckPut);
    cmds.insert(Command::NACK );

    return cmds;
}

void MESIL1::setSliceAware(uint64_t interleave_size, uint64_t interleave_step) {
    cache_array_->setSliceAware(interleave_size, interleave_step);
}


Addr MESIL1::getBank(Addr addr) {
    return cache_array_->getBank(addr);
}


void MESIL1::printLine(Addr addr) { }


void MESIL1::printStatus(Output &out) {
    cache_array_->printCacheArray(out);
}


void MESIL1::serialize_order(SST::Core::Serialization::serializer& ser) {
    CoherenceController::serialize_order(ser);

    SST_SER(is_flushing_);
    SST_SER(flush_drain_);
    SST_SER(flush_complete_timestamp_);
    SST_SER(snoop_l1_invs_);
    SST_SER(protocol_exclusive_state_);
    SST_SER(protocol_read_state_);
    SST_SER(llsc_block_cycles_);
    SST_SER(llsc_timeout_);
    SST_SER(cache_array_);
    SST_SER(stat_event_stalled_for_lock);
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