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
#include "coherencemgr/Incoherent.h"

using namespace SST;
using namespace SST::MemHierarchy;

/* Debug macros included from util.h */

/*----------------------------------------------------------------------------------------------------------------------
 * Incoherent Controller Implementation
 * Non-Inclusive -> does not allocate on Get* requests except for prefetches
 * No writebacks except dirty data
 * I = not present in the cache, E = present & clean, M = present & dirty
 *---------------------------------------------------------------------------------------------------------------------*/

/***********************************************************************************************************
 * Constructor
 ***********************************************************************************************************/
Incoherent::Incoherent(SST::ComponentId_t id, Params& params, Params& owner_params, bool prefetch) :
    CoherenceController(id, params, owner_params, prefetch)
{
    params.insert(owner_params);

    // Cache Array
    uint64_t lines = params.find<uint64_t>("lines");
    uint64_t assoc = params.find<uint64_t>("associativity");
    ReplacementPolicy * rmgr = createReplacementPolicy(lines, assoc, params, true);
    HashFunction * ht = createHashFunction(params);

    cache_array_ = new CacheArray<PrivateCacheLine>(debug_, lines, assoc, line_size_, rmgr, ht);
    cache_array_->setBanked(params.find<uint64_t>("banks", 0));

    stat_event_state_[(int)Command::GetS][I] = registerStatistic<uint64_t>("stateEvent_GetS_I");
    stat_event_state_[(int)Command::GetS][E] = registerStatistic<uint64_t>("stateEvent_GetS_E");
    stat_event_state_[(int)Command::GetS][M] = registerStatistic<uint64_t>("stateEvent_GetS_M");
    stat_event_state_[(int)Command::GetX][I] = registerStatistic<uint64_t>("stateEvent_GetX_I");
    stat_event_state_[(int)Command::GetX][E] = registerStatistic<uint64_t>("stateEvent_GetX_E");
    stat_event_state_[(int)Command::GetX][M] = registerStatistic<uint64_t>("stateEvent_GetX_M");
    stat_event_state_[(int)Command::GetSX][I] = registerStatistic<uint64_t>("stateEvent_GetSX_I");
    stat_event_state_[(int)Command::GetSX][E] = registerStatistic<uint64_t>("stateEvent_GetSX_E");
    stat_event_state_[(int)Command::GetSX][M] = registerStatistic<uint64_t>("stateEvent_GetSX_M");
    stat_event_state_[(int)Command::GetSResp][I] = registerStatistic<uint64_t>("stateEvent_GetSResp_I");
    stat_event_state_[(int)Command::GetSResp][IS] = registerStatistic<uint64_t>("stateEvent_GetSResp_IS");
    stat_event_state_[(int)Command::GetXResp][I] = registerStatistic<uint64_t>("stateEvent_GetXResp_I");
    stat_event_state_[(int)Command::GetXResp][IS] = registerStatistic<uint64_t>("stateEvent_GetXResp_IS");
    stat_event_state_[(int)Command::GetXResp][IM] = registerStatistic<uint64_t>("stateEvent_GetXResp_IM");
    stat_event_state_[(int)Command::PutE][I] = registerStatistic<uint64_t>("stateEvent_PutE_I");
    stat_event_state_[(int)Command::PutE][E] = registerStatistic<uint64_t>("stateEvent_PutE_E");
    stat_event_state_[(int)Command::PutE][M] = registerStatistic<uint64_t>("stateEvent_PutE_M");
    stat_event_state_[(int)Command::PutE][IS] = registerStatistic<uint64_t>("stateEvent_PutE_IS");
    stat_event_state_[(int)Command::PutE][IM] = registerStatistic<uint64_t>("stateEvent_PutE_IM");
    stat_event_state_[(int)Command::PutE][I_B] = registerStatistic<uint64_t>("stateEvent_PutE_IB");
    stat_event_state_[(int)Command::PutE][S_B] = registerStatistic<uint64_t>("stateEvent_PutE_SB");
    stat_event_state_[(int)Command::PutM][I] = registerStatistic<uint64_t>("stateEvent_PutM_I");
    stat_event_state_[(int)Command::PutM][E] = registerStatistic<uint64_t>("stateEvent_PutM_E");
    stat_event_state_[(int)Command::PutM][M] = registerStatistic<uint64_t>("stateEvent_PutM_M");
    stat_event_state_[(int)Command::PutM][IS] = registerStatistic<uint64_t>("stateEvent_PutM_IS");
    stat_event_state_[(int)Command::PutM][IM] = registerStatistic<uint64_t>("stateEvent_PutM_IM");
    stat_event_state_[(int)Command::PutM][I_B] = registerStatistic<uint64_t>("stateEvent_PutM_IB");
    stat_event_state_[(int)Command::PutM][S_B] = registerStatistic<uint64_t>("stateEvent_PutM_SB");
    stat_event_state_[(int)Command::FlushLine][I] = registerStatistic<uint64_t>("stateEvent_FlushLine_I");
    stat_event_state_[(int)Command::FlushLine][E] = registerStatistic<uint64_t>("stateEvent_FlushLine_E");
    stat_event_state_[(int)Command::FlushLine][M] = registerStatistic<uint64_t>("stateEvent_FlushLine_M");
    stat_event_state_[(int)Command::FlushLine][IS] = registerStatistic<uint64_t>("stateEvent_FlushLine_IS");
    stat_event_state_[(int)Command::FlushLine][IM] = registerStatistic<uint64_t>("stateEvent_FlushLine_IM");
    stat_event_state_[(int)Command::FlushLine][I_B] = registerStatistic<uint64_t>("stateEvent_FlushLine_IB");
    stat_event_state_[(int)Command::FlushLine][S_B] = registerStatistic<uint64_t>("stateEvent_FlushLine_SB");
    stat_event_state_[(int)Command::FlushLineInv][I] = registerStatistic<uint64_t>("stateEvent_FlushLineInv_I");
    stat_event_state_[(int)Command::FlushLineInv][E] = registerStatistic<uint64_t>("stateEvent_FlushLineInv_E");
    stat_event_state_[(int)Command::FlushLineInv][M] = registerStatistic<uint64_t>("stateEvent_FlushLineInv_M");
    stat_event_state_[(int)Command::FlushLineInv][IS] = registerStatistic<uint64_t>("stateEvent_FlushLineInv_IS");
    stat_event_state_[(int)Command::FlushLineInv][IM] = registerStatistic<uint64_t>("stateEvent_FlushLineInv_IM");
    stat_event_state_[(int)Command::FlushLineInv][I_B] = registerStatistic<uint64_t>("stateEvent_FlushLineInv_IB");
    stat_event_state_[(int)Command::FlushLineInv][S_B] = registerStatistic<uint64_t>("stateEvent_FlushLineInv_SB");
    stat_event_state_[(int)Command::FlushLineResp][I] = registerStatistic<uint64_t>("stateEvent_FlushLineResp_I");
    stat_event_state_[(int)Command::FlushLineResp][I_B] = registerStatistic<uint64_t>("stateEvent_FlushLineResp_IB");
    stat_event_state_[(int)Command::FlushLineResp][S_B] = registerStatistic<uint64_t>("stateEvent_FlushLineResp_SB");
    stat_event_sent_[(int)Command::GetS]             = registerStatistic<uint64_t>("eventSent_GetS");
    stat_event_sent_[(int)Command::GetX]             = registerStatistic<uint64_t>("eventSent_GetX");
    stat_event_sent_[(int)Command::GetSX]           = registerStatistic<uint64_t>("eventSent_GetSX");
    stat_event_sent_[(int)Command::PutE]             = registerStatistic<uint64_t>("eventSent_PutE");
    stat_event_sent_[(int)Command::PutM]             = registerStatistic<uint64_t>("eventSent_PutM");
    stat_event_sent_[(int)Command::FlushLine]        = registerStatistic<uint64_t>("eventSent_FlushLine");
    stat_event_sent_[(int)Command::FlushLineInv]     = registerStatistic<uint64_t>("eventSent_FlushLineInv");
    stat_event_sent_[(int)Command::NACK]           = registerStatistic<uint64_t>("eventSent_NACK");
    stat_event_sent_[(int)Command::GetSResp]         = registerStatistic<uint64_t>("eventSent_GetSResp");
    stat_event_sent_[(int)Command::GetXResp]         = registerStatistic<uint64_t>("eventSent_GetXResp");
    stat_event_sent_[(int)Command::FlushLineResp]    = registerStatistic<uint64_t>("eventSent_FlushLineResp");
    stat_event_sent_[(int)Command::Put]           = registerStatistic<uint64_t>("eventSent_Put");
    stat_event_sent_[(int)Command::Get]           = registerStatistic<uint64_t>("eventSent_Get");
    stat_event_sent_[(int)Command::AckMove]       = registerStatistic<uint64_t>("eventSent_AckMove");
    stat_event_sent_[(int)Command::CustomReq]     = registerStatistic<uint64_t>("eventSent_CustomReq");
    stat_event_sent_[(int)Command::CustomResp]    = registerStatistic<uint64_t>("eventSent_CustomResp");
    stat_event_sent_[(int)Command::CustomAck]     = registerStatistic<uint64_t>("eventSent_CustomAck");
    stat_latency_GetS_[LatType::HIT] = registerStatistic<uint64_t>("latency_GetS_hit");
    stat_latency_GetS_[LatType::MISS] = registerStatistic<uint64_t>("latency_GetS_miss");
    stat_latency_GetX_[LatType::HIT] = registerStatistic<uint64_t>("latency_GetX_hit");
    stat_latency_GetX_[LatType::MISS] = registerStatistic<uint64_t>("latency_GetX_miss");
    stat_latency_GetSX_[LatType::HIT] = registerStatistic<uint64_t>("latency_GetSX_hit");
    stat_latency_GetSX_[LatType::MISS] = registerStatistic<uint64_t>("latency_GetSX_miss");
    stat_latency_FlushLine_ = registerStatistic<uint64_t>("latency_FlushLine");
    stat_latency_FlushLineInv_ = registerStatistic<uint64_t>("latency_FlushLineInv");
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
    stat_evict_[I] = registerStatistic<uint64_t>("evict_I");
    stat_evict_[E] = registerStatistic<uint64_t>("evict_E");
    stat_evict_[M] = registerStatistic<uint64_t>("evict_M");
    stat_evict_[IS] = registerStatistic<uint64_t>("evict_IS");
    stat_evict_[IM] = registerStatistic<uint64_t>("evict_IM");
    stat_evict_[I_B] = registerStatistic<uint64_t>("evict_IB");
    stat_evict_[S_B] = registerStatistic<uint64_t>("evict_SB");

    if (prefetch) {
        stat_prefetch_evict_ = registerStatistic<uint64_t>("prefetch_evict");
        stat_prefetch_hit_ = registerStatistic<uint64_t>("prefetch_useful");
        stat_prefetch_redundant_ = registerStatistic<uint64_t>("prefetch_redundant");
    }
}



/***********************************************************************************************************
 * Event handlers
 ***********************************************************************************************************/

bool Incoherent::handleGetS(MemEvent * event, bool in_mshr) {
    Addr addr = event->getBaseAddr();
    PrivateCacheLine * line = cache_array_->lookup(addr, true);
    bool local_prefetch = event->isPrefetch() && (event->getRqstr() == cachename_);
    State state = line ? line->getState() : I;
    uint64_t send_time = 0;
    MemEventStatus status = MemEventStatus::OK;

    if (mem_h_is_debug_event(event))
        event_debuginfo_.prefill(event->getID(), Command::GetS, (local_prefetch ? "-pref" : ""), addr, state);

    switch (state) {
        case I:
            if (local_prefetch)
                status = processCacheMiss(event, line, in_mshr);
            else
                status = in_mshr ? MemEventStatus::OK : allocateMSHR(event, false);
            if (status == MemEventStatus::OK) {
                if (!mshr_->getProfiled(addr)) {
                    stat_event_state_[(int)Command::GetS][I]->addData(1);
                    notifyListenerOfAccess(event, NotifyAccessType::READ, NotifyResultType::MISS);
                    mshr_->setProfiled(addr);
                    stat_misses_->addData(1);
                    stat_miss_[0][(int)in_mshr]->addData(1);
                }
                recordLatencyType(event->getID(), LatType::MISS);
                send_time = forwardMessage(event, event->getSize(), 0, nullptr);
                mshr_->setInProgress(addr);
                if (local_prefetch) { // Only case where we allocate a line
                    line->setState(IS);
                    line->setTimestamp(send_time);
                }
                if (mem_h_is_debug_event(event))
                    event_debuginfo_.reason = "miss";
            }
            break;
        case E:
        case M:
            if (!in_mshr || mshr_->getProfiled(addr)) {
                notifyListenerOfAccess(event, NotifyAccessType::READ, NotifyResultType::HIT);
                stat_event_state_[(int)Command::GetS][state]->addData(1);
                stat_hit_[0][(int)in_mshr]->addData(1);
                stat_hits_->addData(1);
            }
            if (local_prefetch) {
                stat_prefetch_redundant_->addData(1);
                recordPrefetchLatency(event->getID(), LatType::HIT);
                return DONE;
            }
            recordPrefetchResult(line, stat_prefetch_hit_);
            recordLatencyType(event->getID(), LatType::HIT);

            send_time = sendResponseUp(event, line->getData(), in_mshr, line->getTimestamp());
            line->setTimestamp(send_time);
            if (mem_h_is_debug_event(event))
                event_debuginfo_.reason = "hit";
            cleanUpAfterRequest(event, in_mshr);
            break;
        default:
            if (!in_mshr)
                status = allocateMSHR(event, false);
            else if (mem_h_is_debug_event(event))
                event_debuginfo_.action = "Stall";
            break;
    }

    if (status == MemEventStatus::Reject) {
        if (local_prefetch) return false;
        sendNACK(event);
    }

    if (mem_h_is_debug_addr(addr) && line) {
        event_debuginfo_.new_state = line->getState();
        event_debuginfo_.verbose_line = line->getString();
    }

    return true;
}


bool Incoherent::handleGetX(MemEvent * event, bool in_mshr) {
    Addr addr = event->getBaseAddr();
    PrivateCacheLine * line = cache_array_->lookup(addr, true);
    State state = line ? line->getState() : I;
    uint64_t send_time = 0;
    MemEventStatus status = MemEventStatus::OK;

    if (mem_h_is_debug_event(event))
        event_debuginfo_.prefill(event->getID(), event->getCmd(), "", addr, state);

    switch (state) {
        case I:
            status = in_mshr ? MemEventStatus::OK : allocateMSHR(event, false);
            if (status == MemEventStatus::OK) {
                if (!mshr_->getProfiled(addr)) {
                    stat_event_state_[(int)event->getCmd()][I]->addData(1);
                    notifyListenerOfAccess(event, NotifyAccessType::WRITE, NotifyResultType::MISS);
                    mshr_->setProfiled(addr);
                    if (event->getCmd() == Command::GetX)
                        stat_miss_[1][(int)in_mshr]->addData(1);
                    else
                        stat_miss_[2][(int)in_mshr]->addData(1);
                    stat_misses_->addData(1);
                }
                recordLatencyType(event->getID(), LatType::MISS);
                forwardMessage(event, event->getSize(), 0, nullptr);
                mshr_->setInProgress(addr);
                if (mem_h_is_debug_event(event))
                    event_debuginfo_.reason = "miss";
            }
            break;
        case E:
        case M:
            if (!in_mshr || !mshr_->getProfiled(addr)) {
                notifyListenerOfAccess(event, NotifyAccessType::WRITE, NotifyResultType::HIT);
                stat_event_state_[(int)event->getCmd()][I]->addData(1);
                if (event->getCmd() == Command::GetX)
                    stat_hit_[1][(int)in_mshr]->addData(1);
                else
                    stat_hit_[2][(int)in_mshr]->addData(1);
                stat_hits_->addData(1);
            }
            recordPrefetchResult(line, stat_prefetch_hit_);
            send_time = sendResponseUp(event, line->getData(), in_mshr, line->getTimestamp());
            line->setTimestamp(send_time);
            recordLatencyType(event->getID(), LatType::HIT);

            if (mem_h_is_debug_event(event))
                event_debuginfo_.reason = "hit";
            cleanUpAfterRequest(event, in_mshr);
            break;
        default:
            if (!in_mshr)
                status = allocateMSHR(event, false);
            else if (mem_h_is_debug_event(event))
                event_debuginfo_.action = "Stall";
            break;
    }

    if (status == MemEventStatus::Reject)
        sendNACK(event);

    if (mem_h_is_debug_addr(addr) && line) {
        event_debuginfo_.new_state = line->getState();
        event_debuginfo_.verbose_line = line->getString();
    }

    return true;
}


bool Incoherent::handleGetSX(MemEvent * event, bool in_mshr) {
    return handleGetX(event, in_mshr);
}


bool Incoherent::handleFlushLine(MemEvent * event, bool in_mshr) {
    Addr addr = event->getBaseAddr();
    PrivateCacheLine * line = cache_array_->lookup(addr, false);
    State state = line ? line->getState() : I;

    if (mem_h_is_debug_event(event))
        event_debuginfo_.prefill(event->getID(), Command::FlushLine, "", addr, state);

    MemEventStatus status = in_mshr ? MemEventStatus::OK : allocateMSHR(event, false);
    if (!in_mshr)
        stat_event_state_[(int)Command::FlushLine][state]->addData(1);

    recordLatencyType(event->getID(), LatType::HIT);

    if (event->getEvict()) {
        doEvict(event, line);
        state = line->getState();
    }

    switch (state) {
        case I:
            if (status == MemEventStatus::OK) {
                forwardFlush(event, event->getEvict(), &(event->getPayload()), event->getDirty(), 0);
                mshr_->setInProgress(addr);
            }
            break;
        case E:
        case M:
            if (status == MemEventStatus::OK) {
                forwardFlush(event, state == M, line->getData(), state == M, 0);
                line->setState(S_B);
                mshr_->setInProgress(addr);
            }
            break;
        default:
            break;
    }

    if (status == MemEventStatus::Reject)
        sendNACK(event);

    if (mem_h_is_debug_addr(addr) && line) {
        event_debuginfo_.new_state = line->getState();
        event_debuginfo_.verbose_line = line->getString();
    }

    return true;
}


bool Incoherent::handleFlushLineInv(MemEvent * event, bool in_mshr) {
    Addr addr = event->getBaseAddr();
    PrivateCacheLine * line = cache_array_->lookup(addr, false);
    State state = line ? line->getState() : I;

    if (mem_h_is_debug_event(event))
        event_debuginfo_.prefill(event->getID(), Command::FlushLine, "", addr, state);

    MemEventStatus status = in_mshr ? MemEventStatus::OK : allocateMSHR(event, false);
    if (!in_mshr)
        stat_event_state_[(int)Command::FlushLineInv][state]->addData(1);

    recordLatencyType(event->getID(), LatType::HIT);

    if (event->getEvict()) {
        doEvict(event, line);
        state = line->getState();
    }

    switch (state) {
        case I:
            if (status == MemEventStatus::OK) {
                forwardFlush(event, event->getEvict(), &(event->getPayload()), event->getDirty(), 0);
                mshr_->setInProgress(addr);
            }
            break;
        case E:
        case M:
            if (status == MemEventStatus::OK) {
                recordPrefetchResult(line, stat_prefetch_evict_);
                forwardFlush(event, true, line->getData(), state == M, line->getTimestamp());
                line->setState(I_B);
                mshr_->setInProgress(addr);
            }
            break;
        default:
                break;
            }


    if (status == MemEventStatus::Reject)
        sendNACK(event);

    if (mem_h_is_debug_addr(addr) && line) {
        event_debuginfo_.new_state = line->getState();
        event_debuginfo_.verbose_line = line->getString();
    }

    return true;
}


bool Incoherent::handlePutE(MemEvent * event, bool in_mshr) {
    Addr addr = event->getBaseAddr();
    PrivateCacheLine * line = cache_array_->lookup(addr, true);
    State state = line ? line->getState() : I;
    MemEventStatus status = MemEventStatus::OK;

    if (!in_mshr)
        stat_event_state_[(int)Command::PutE][state]->addData(1);

    switch (state) {
        case I:
            status = allocateLine(event, line, in_mshr);
            if (status == MemEventStatus::OK) {
                line->setData(event->getPayload(), 0);
                line->setState(E);
                if (send_writeback_ack_)
                    sendWritebackAck(event);
                cleanUpAfterRequest(event, in_mshr);
            }
            break;
        case E:
        case M:
            if (send_writeback_ack_)
                sendWritebackAck(event);
            cleanUpAfterRequest(event, in_mshr);
            break;
        default:
            if (!in_mshr)
                status = allocateMSHR(event, false);
    }

    if (status == MemEventStatus::Reject)
        sendNACK(event);

    if (mem_h_is_debug_addr(addr) && line) {
        event_debuginfo_.new_state = line->getState();
        event_debuginfo_.verbose_line = line->getString();
    }

    return true;
}


bool Incoherent::handlePutM(MemEvent * event, bool in_mshr) {
    Addr addr = event->getBaseAddr();
    PrivateCacheLine * line = cache_array_->lookup(addr, true);
    State state = line ? line->getState() : I;
    MemEventStatus status = MemEventStatus::OK;

    if (!in_mshr)
        stat_event_state_[(int)Command::PutM][state]->addData(1);

    switch (state) {
        case I:
            status = allocateLine(event, line, in_mshr);
            if (status == MemEventStatus::OK) {
                line->setData(event->getPayload(), 0);
                line->setState(M);
                if (send_writeback_ack_)
                    sendWritebackAck(event);
                cleanUpAfterRequest(event, in_mshr);
            }
            break;
        case E:
            line->setState(M);
        case M:
            line->setData(event->getPayload(), 0);
            if (send_writeback_ack_)
                sendWritebackAck(event);
            cleanUpAfterRequest(event, in_mshr);
            break;
        default:
            if (!in_mshr)
                status = allocateMSHR(event, false);
    }

    if (status == MemEventStatus::Reject)
        sendNACK(event);

    if (mem_h_is_debug_addr(addr) && line) {
        event_debuginfo_.new_state = line->getState();
        event_debuginfo_.verbose_line = line->getString();
    }

    return true;
}


bool Incoherent::handleGetSResp(MemEvent * event, bool in_mshr) {
    Addr addr = event->getBaseAddr();
    PrivateCacheLine * line = cache_array_->lookup(addr, false);
    State state = line ? line->getState() : I;

    if (mem_h_is_debug_event(event))
        event_debuginfo_.prefill(event->getID(), Command::GetSResp, "", addr, state);

    stat_event_state_[(int)Command::GetSResp][state]->addData(1);

    MemEvent * req = static_cast<MemEvent*>(mshr_->getFrontEvent(addr));
    req->setFlags(event->getMemFlags());

    sendResponseUp(req, &event->getPayload(), true, 0);

    if (line) {
        line->setState(E);
        line->setData(event->getPayload(), 0);
        // Has to be a local prefetch
        line->setPrefetch(true);
        recordPrefetchLatency(req->getID(), LatType::MISS);
    }

    cleanUpAfterResponse(event);

    if (mem_h_is_debug_addr(addr) && line) {
        event_debuginfo_.new_state = line->getState();
        event_debuginfo_.verbose_line = line->getString();
    }

    return true;
}


bool Incoherent::handleGetXResp(MemEvent * event, bool in_mshr) {
    Addr addr = event->getBaseAddr();
    PrivateCacheLine * line = cache_array_->lookup(addr, false);
    State state = line ? line->getState() : I;

    if (mem_h_is_debug_event(event))
        event_debuginfo_.prefill(event->getID(), Command::GetXResp, "", addr, state);

    stat_event_state_[(int)Command::GetXResp][state]->addData(1);

    MemEvent * req = static_cast<MemEvent*>(mshr_->getFrontEvent(addr));
    req->setFlags(event->getMemFlags());

    sendResponseUp(req, &event->getPayload(), true, 0);

    cleanUpAfterResponse(event);

    return true;
}


bool Incoherent::handleFlushLineResp(MemEvent * event, bool in_mshr) {
    Addr addr = event->getBaseAddr();
    PrivateCacheLine * line = cache_array_->lookup(addr, false);
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
            cache_array_->deallocate(line);
            break;
        case S_B:
            line->setState(E);
            break;
        default:
            debug_->fatal(CALL_INFO, -1, "%s, Error: Received FlushLineResp in unhandled state '%s'. Event: %s. Time = %" PRIu64 "ns\n",
                    getName().c_str(), StateString[state], event->getVerboseString().c_str(), getCurrentSimTimeNano());
    }

    sendResponseUp(req, nullptr, true, timestamp_, Command::FlushLineResp, event->success());

    if (mem_h_is_debug_addr(addr) && line) {
        event_debuginfo_.new_state = line->getState();
        event_debuginfo_.verbose_line = line->getString();
    }

    cleanUpAfterResponse(event);

    return true;
}


/* We're using NULLCMD to signal an internally generated event - in this case an eviction */
bool Incoherent::handleNULLCMD(MemEvent* event, bool in_mshr) {
    Addr oldAddr = event->getAddr();
    Addr newAddr = event->getBaseAddr();

    PrivateCacheLine * line = cache_array_->lookup(oldAddr, false);
    printLine(event->getBaseAddr());
    bool evicted = handleEviction(newAddr, line, event_debuginfo_);
    if (evicted) {
        notifyListenerOfEvict(line->getAddr(), line_size_, event->getInstructionPointer());
        retry_buffer_.push_back(mshr_->getFrontEvent(newAddr));
        if (mshr_->removeEvictPointer(oldAddr, newAddr))
            retry(oldAddr);
    } else { // Could be stalling for a new address or locked line
        if (oldAddr != line ->getAddr()) { // We're waiting for a new line now...
            if (mem_h_is_debug_addr(oldAddr) || mem_h_is_debug_addr(line->getAddr()) || mem_h_is_debug_addr(newAddr))
                debug_->debug(_L8_, "\tAddr 0x%" PRIx64 " now waiting for 0x%" PRIx64 " instead of 0x%" PRIx64 "\n",
                        oldAddr, line->getAddr(), newAddr);
            mshr_->insertEviction(line->getAddr(), newAddr);
            if (mshr_->removeEvictPointer(oldAddr, newAddr))
                retry(oldAddr);
        }
    }

    printLine(event->getBaseAddr());
    delete event;
    return true;
}


bool Incoherent::handleNACK(MemEvent* event, bool in_mshr) {
    MemEvent* nackedEvent = event->getNACKedEvent();
    Addr addr = nackedEvent->getBaseAddr();
    PrivateCacheLine * line = cache_array_->lookup(addr, false);
    State state = line ? line->getState() : I;

    if (mem_h_is_debug_event(event) || mem_h_is_debug_event(nackedEvent))
        event_debuginfo_.prefill(event->getID(), Command::NACK, "", addr, state);

    delete event;

    resendEvent(nackedEvent, false); // Re-send since we don't have races that make events redundant

    return true;
}


/***********************************************************************************************************
 * MSHR & CacheArray management
 ***********************************************************************************************************/

MemEventStatus Incoherent::processCacheMiss(MemEvent * event, PrivateCacheLine* &line, bool in_mshr) {
    MemEventStatus status = in_mshr ? MemEventStatus::OK : allocateMSHR(event, false);
    if (in_mshr && mshr_->getFrontEvent(event->getBaseAddr()) != event) {
        if (mem_h_is_debug_event(event))
            event_debuginfo_.action = "Stall";
        return MemEventStatus::Stall;
    }

    if (status == MemEventStatus::OK && !line) {
        status = allocateLine(event, line, in_mshr);
    }
    return status;
}

MemEventStatus Incoherent::allocateLine(MemEvent * event, PrivateCacheLine* &line, bool in_mshr) {
    evict_debuginfo_.prefill(event->getID(), Command::Evict, "", 0, I);

    bool evicted = handleEviction(event->getBaseAddr(), line, evict_debuginfo_);

    if (mem_h_is_debug_event(event) || mem_h_is_debug_addr(line->getAddr())) {
        evict_debuginfo_.new_state = line->getState();
        evict_debuginfo_.verbose_line = line->getString();
        evict_debuginfo_.action = event_debuginfo_.action;
        evict_debuginfo_.reason = event_debuginfo_.reason;
    }

    if (evicted) {
        notifyListenerOfEvict(line->getAddr(), line_size_, event->getInstructionPointer());
        cache_array_->replace(event->getBaseAddr(), line);
        if (mem_h_is_debug_event(event))
            printDebugAlloc(true, event->getBaseAddr(), "");
        return MemEventStatus::OK;
    } else {
        if (in_mshr || mshr_->insertEvent(event->getBaseAddr(), event, -1, false, true) != -1) {
            mshr_->insertEviction(line->getAddr(), event->getBaseAddr());
            if (in_mshr)
                mshr_->setStalledForEvict(event->getBaseAddr(), true);
            if (mem_h_is_debug_event(event)) {
                event_debuginfo_.action = "Stall";
                std::stringstream reason;
                reason << "evict 0x" << std::hex << line->getAddr();
                event_debuginfo_.reason = reason.str();
            }
            return MemEventStatus::Stall;
        }
        if (mem_h_is_debug_event(event) || mem_h_is_debug_addr(line->getAddr()))
            printDebugInfo(&evict_debuginfo_);
        return MemEventStatus::Reject;
    }
}


bool Incoherent::handleEviction(Addr addr, PrivateCacheLine* &line, dbgin &debug_info) {
    if (!line)
        line = cache_array_->findReplacementCandidate(addr);

    State state = line->getState();

    if (mem_h_is_debug_addr(addr) || mem_h_is_debug_addr(line->getAddr())) {
        debug_info.old_state = state;
        debug_info.addr = line->getAddr();
    }

    stat_evict_[state]->addData(1);

    switch (state) {
        case I:
            if (mem_h_is_debug_addr(addr) || mem_h_is_debug_addr(line->getAddr())) {
                debug_info.action = "None";
                debug_info.reason = "already idle";
            }
            return true;
        case E:
            if (!silent_evict_clean_) {
                sendWriteback(Command::PutE, line, false);
                if (mem_h_is_debug_addr(line->getAddr()))
                    printDebugAlloc(false, line->getAddr(), "Writeback");
            } else if (mem_h_is_debug_addr(line->getAddr()))
                printDebugAlloc(false, line->getAddr(), "Drop");
            line->setState(I);
            break;
        case M:
            sendWriteback(Command::PutM, line, true);
            line->setState(I);
            if (mem_h_is_debug_addr(line->getAddr()))
                printDebugAlloc(false, line->getAddr(), "Writeback");
            break;
        default:
            if (mem_h_is_debug_addr(line->getAddr())) {
                std::stringstream note;
                note << "InProg, " << StateString[state];
                printDebugAlloc(false, line->getAddr(), note.str());
            }
            return false;
    }

    recordPrefetchResult(line, stat_prefetch_evict_);
    return true;
}


void Incoherent::cleanUpAfterRequest(MemEvent * event, bool in_mshr) {
    Addr addr = event->getBaseAddr();

    if (in_mshr) {
        if (event->isPrefetch() && event->getRqstr() == cachename_) outstanding_prefetch_count_--;
        mshr_->removeFront(addr);
    }

    delete event;

    /* Replay any waiting events */
    retry(addr);
}


void Incoherent::cleanUpAfterResponse(MemEvent * event) {
    Addr addr = event->getBaseAddr();

    /* Clean up MSHR */
    MemEvent * req = nullptr;
    if (mshr_->getFrontType(addr) == MSHREntryType::Event)
        req = static_cast<MemEvent*>(mshr_->getFrontEvent(addr));

    mshr_->removeFront(addr);
    delete event;

    if (req) {
        if (req->isPrefetch() && req->getRqstr() == cachename_) outstanding_prefetch_count_--;
        delete req;
    }
    retry(addr);
}


void Incoherent::retry(Addr addr) {
    if (mshr_->exists(addr)) {
        if (mshr_->getFrontType(addr) == MSHREntryType::Event) {
            if (!mshr_->getInProgress(addr))
                retry_buffer_.push_back(mshr_->getFrontEvent(addr));
        } else { // Pointer -> another request is waiting to evict this address
            std::list<Addr>* evictPointers = mshr_->getEvictPointers(addr);
            for (std::list<Addr>::iterator it = evictPointers->begin(); it != evictPointers->end(); it++) {
                MemEvent * ev = new MemEvent(cachename_, addr, *it, Command::NULLCMD);
                retry_buffer_.push_back(ev);
            }
        }
    }
}


void Incoherent::doEvict(MemEvent * event, PrivateCacheLine * line) {
    State state = line ? line->getState() : I;

    if (state == E || state == M) {
        if (event->getDirty()) {
            line->setState(M);
            line->setData(event->getPayload(), 0);
        }

        event->setEvict(false);
    }
}

/***********************************************************************************************************
 * Event creation and send
 ***********************************************************************************************************/

SimTime_t Incoherent::sendResponseUp(MemEvent * event, vector<uint8_t> * data, bool in_mshr, SimTime_t time, Command cmd, bool success) {
    MemEvent * responseEvent = event->makeResponse();
    if (cmd != Command::NULLCMD)
        responseEvent->setCmd(cmd);

    if (data) {
        responseEvent->setPayload(*data);
        responseEvent->setSize(data->size());
    }

    if (!success)
        responseEvent->setFail();

    if (time < timestamp_) time = timestamp_;
    SimTime_t delivery_time = time + (in_mshr ? mshr_latency_ : access_latency_);
    forwardByDestination(responseEvent, delivery_time);

    if (mem_h_is_debug_event(event))
        event_debuginfo_.action = "Respond";

    return delivery_time;
}


void Incoherent::sendWriteback(Command cmd, PrivateCacheLine * line, bool dirty) {
    MemEvent * writeback = new MemEvent(cachename_, line->getAddr(), line->getAddr(), cmd);
    writeback->setSize(line_size_);

    uint64_t latency = tag_latency_;

    if (dirty) {
        writeback->setPayload(*(line->getData()));
        writeback->setDirty(dirty);

        latency = access_latency_;
    }

    writeback->setRqstr(cachename_);

    uint64_t time = (timestamp_ > line->getTimestamp()) ? timestamp_ : line->getTimestamp();
    time += latency;
    forwardByAddress(writeback, time);
    line->setTimestamp(time-1);
}


void Incoherent::forwardFlush(MemEvent * event, bool evict, std::vector<uint8_t>* data, bool dirty, uint64_t time) {
    MemEvent * flush = new MemEvent(*event);

    uint64_t latency = tag_latency_;
    if (evict) {
        flush->setEvict(true);
        flush->setPayload(*data);
        flush->setDirty(dirty);
        latency = access_latency_;
    } else {
        flush->setPayload(0, nullptr);
    }

    uint64_t send_time = timestamp_ + latency;
    forwardByAddress(flush, send_time);

    if (mem_h_is_debug_addr(event->getBaseAddr()))
        event_debuginfo_.action = "Forward";
}


void Incoherent::sendWritebackAck(MemEvent * event) {
    MemEvent * ack = event->makeResponse();

    uint64_t time = timestamp_ + tag_latency_;
    forwardByDestination(ack, time);

    if (mem_h_is_debug_event(event))
        event_debuginfo_.action = "Ack";
}

/*----------------------------------------------------------------------------------------------------------------------
 *  Override message send functions with versions that record statistics & call parent class
 *---------------------------------------------------------------------------------------------------------------------*/
void Incoherent::forwardByAddress(MemEventBase* ev, Cycle_t timestamp) {
    stat_event_sent_[(int)ev->getCmd()]->addData(1);
    CoherenceController::forwardByAddress(ev, timestamp);
}

void Incoherent::forwardByDestination(MemEventBase* ev, Cycle_t timestamp) {
    stat_event_sent_[(int)ev->getCmd()]->addData(1);
    CoherenceController::forwardByDestination(ev, timestamp);
}


/*************************
 * Helper functions
 *************************/

MemEventInitCoherence * Incoherent::getInitCoherenceEvent() {
    return new MemEventInitCoherence(cachename_, Endpoint::Cache, false, false, false, line_size_, true);
}

void Incoherent::recordPrefetchResult(PrivateCacheLine * line, Statistic<uint64_t>* stat) {
    if (line->getPrefetch()) {
        stat->addData(1);
        line->setPrefetch(false);
    }
}

void Incoherent::recordLatency(Command cmd, int type, uint64_t latency) {
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

void Incoherent::printLine(Addr addr) { }

std::set<Command> Incoherent::getValidReceiveEvents()  {
    std::set<Command> cmds = { Command::GetS,
        Command::GetX,
        Command::GetSX,
        Command::FlushLine,
        Command::FlushLineInv,
        Command::GetSResp,
        Command::GetXResp,
        Command::FlushLineResp,
        Command::PutE,
        Command::PutM,
        Command::NULLCMD,
        Command::NACK };
    return cmds;
}

void Incoherent::serialize_order(SST::Core::Serialization::serializer& ser) {
    CoherenceController::serialize_order(ser);

    SST_SER(cache_array_);
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