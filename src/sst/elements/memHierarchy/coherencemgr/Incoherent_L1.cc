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
#include "coherencemgr/Incoherent_L1.h"

using namespace SST;
using namespace SST::MemHierarchy;

/* Debug macros included from util.h */

/*----------------------------------------------------------------------------------------------------------------------
 * L1 Incoherent Controller
 * -> Essentially coherent for a single thread/single core so will write back dirty blocks but assumes no sharing
 * States:
 * I: not present
 * E: present & clean
 * M: present & dirty
 *---------------------------------------------------------------------------------------------------------------------*/

/***********************************************************************************************************
 * Constructor
 ***********************************************************************************************************/

IncoherentL1::IncoherentL1(ComponentId_t id, Params& params, Params& owner_params, bool prefetch) :
    CoherenceController(id, params, owner_params, prefetch)
{
    params.insert(owner_params);

    // Cache Array
    uint64_t lines = params.find<uint64_t>("lines");
    uint64_t assoc = params.find<uint64_t>("associativity");
    ReplacementPolicy * rmgr = createReplacementPolicy(lines, assoc, params, true);
    HashFunction * ht = createHashFunction(params);

    cache_array_ = new CacheArray<L1CacheLine>(debug_, lines, assoc, line_size_, rmgr, ht);
    cache_array_->setBanked(params.find<uint64_t>("banks", 0));

    llsc_block_cycles_ = params.find<Cycle_t>("llsc_block_cycles", 0);

    stat_event_state_[(int)Command::GetS][I] =            registerStatistic<uint64_t>("stateEvent_GetS_I");
    stat_event_state_[(int)Command::GetS][E] =            registerStatistic<uint64_t>("stateEvent_GetS_E");
    stat_event_state_[(int)Command::GetS][M] =            registerStatistic<uint64_t>("stateEvent_GetS_M");
    stat_event_state_[(int)Command::GetX][I] =            registerStatistic<uint64_t>("stateEvent_GetX_I");
    stat_event_state_[(int)Command::GetX][E] =            registerStatistic<uint64_t>("stateEvent_GetX_E");
    stat_event_state_[(int)Command::GetX][M] =            registerStatistic<uint64_t>("stateEvent_GetX_M");
    stat_event_state_[(int)Command::GetSX][I] =           registerStatistic<uint64_t>("stateEvent_GetSX_I");
    stat_event_state_[(int)Command::GetSX][E] =           registerStatistic<uint64_t>("stateEvent_GetSX_E");
    stat_event_state_[(int)Command::GetSX][M] =           registerStatistic<uint64_t>("stateEvent_GetSX_M");
    stat_event_state_[(int)Command::GetSResp][IM] =       registerStatistic<uint64_t>("stateEvent_GetSResp_IM");
    stat_event_state_[(int)Command::GetXResp][IM] =       registerStatistic<uint64_t>("stateEvent_GetXResp_IM");
    stat_event_state_[(int)Command::FlushLine][I] =       registerStatistic<uint64_t>("stateEvent_FlushLine_I");
    stat_event_state_[(int)Command::FlushLine][E] =       registerStatistic<uint64_t>("stateEvent_FlushLine_E");
    stat_event_state_[(int)Command::FlushLine][M] =       registerStatistic<uint64_t>("stateEvent_FlushLine_M");
    stat_event_state_[(int)Command::FlushLine][IS] =      registerStatistic<uint64_t>("stateEvent_FlushLine_IS");
    stat_event_state_[(int)Command::FlushLine][IM] =      registerStatistic<uint64_t>("stateEvent_FlushLine_IM");
    stat_event_state_[(int)Command::FlushLine][I_B] =     registerStatistic<uint64_t>("stateEvent_FlushLine_IB");
    stat_event_state_[(int)Command::FlushLine][S_B] =     registerStatistic<uint64_t>("stateEvent_FlushLine_SB");
    stat_event_state_[(int)Command::FlushLineInv][I] =    registerStatistic<uint64_t>("stateEvent_FlushLineInv_I");
    stat_event_state_[(int)Command::FlushLineInv][E] =    registerStatistic<uint64_t>("stateEvent_FlushLineInv_E");
    stat_event_state_[(int)Command::FlushLineInv][M] =    registerStatistic<uint64_t>("stateEvent_FlushLineInv_M");
    stat_event_state_[(int)Command::FlushLineInv][IS] =   registerStatistic<uint64_t>("stateEvent_FlushLineInv_IS");
    stat_event_state_[(int)Command::FlushLineInv][IM] =   registerStatistic<uint64_t>("stateEvent_FlushLineInv_IM");
    stat_event_state_[(int)Command::FlushLineInv][I_B] =  registerStatistic<uint64_t>("stateEvent_FlushLineInv_IB");
    stat_event_state_[(int)Command::FlushLineInv][S_B] =  registerStatistic<uint64_t>("stateEvent_FlushLineInv_SB");
    stat_event_state_[(int)Command::FlushLineResp][I] =   registerStatistic<uint64_t>("stateEvent_FlushLineResp_I");
    stat_event_state_[(int)Command::FlushLineResp][I_B] = registerStatistic<uint64_t>("stateEvent_FlushLineResp_IB");
    stat_event_state_[(int)Command::FlushLineResp][S_B] = registerStatistic<uint64_t>("stateEvent_FlushLineResp_SB");
    stat_event_sent_[(int)Command::GetS] =                registerStatistic<uint64_t>("eventSent_GetS");
    stat_event_sent_[(int)Command::GetX] =                registerStatistic<uint64_t>("eventSent_GetX");
    stat_event_sent_[(int)Command::GetSX] =               registerStatistic<uint64_t>("eventSent_GetSX");
    stat_event_sent_[(int)Command::Write] =               registerStatistic<uint64_t>("eventSent_Write");
    stat_event_sent_[(int)Command::PutM] =                registerStatistic<uint64_t>("eventSent_PutM");
    stat_event_sent_[(int)Command::NACK] =                registerStatistic<uint64_t>("eventSent_NACK");
    stat_event_sent_[(int)Command::FlushLine] =           registerStatistic<uint64_t>("eventSent_FlushLine");
    stat_event_sent_[(int)Command::FlushLineInv] =        registerStatistic<uint64_t>("eventSent_FlushLineInv");
    stat_event_sent_[(int)Command::GetSResp] =            registerStatistic<uint64_t>("eventSent_GetSResp");
    stat_event_sent_[(int)Command::GetXResp] =            registerStatistic<uint64_t>("eventSent_GetXResp");
    stat_event_sent_[(int)Command::WriteResp] =           registerStatistic<uint64_t>("eventSent_WriteResp");
    stat_event_sent_[(int)Command::FlushLineResp] =       registerStatistic<uint64_t>("eventSent_FlushLineResp");
    stat_event_sent_[(int)Command::Put] =                 registerStatistic<uint64_t>("eventSent_Put");
    stat_event_sent_[(int)Command::Get] =                 registerStatistic<uint64_t>("eventSent_Get");
    stat_event_sent_[(int)Command::AckMove] =             registerStatistic<uint64_t>("eventSent_AckMove");
    stat_event_sent_[(int)Command::CustomReq] =           registerStatistic<uint64_t>("eventSent_CustomReq");
    stat_event_sent_[(int)Command::CustomResp] =          registerStatistic<uint64_t>("eventSent_CustomResp");
    stat_event_sent_[(int)Command::CustomAck] =           registerStatistic<uint64_t>("eventSent_CustomAck");
    stat_latency_GetS_[LatType::HIT]  =                   registerStatistic<uint64_t>("latency_GetS_hit");
    stat_latency_GetS_[LatType::MISS] =                   registerStatistic<uint64_t>("latency_GetS_miss");
    stat_latency_GetX_[LatType::HIT]  =                   registerStatistic<uint64_t>("latency_GetX_hit");
    stat_latency_GetX_[LatType::MISS] =                   registerStatistic<uint64_t>("latency_GetX_miss");
    stat_latency_GetSX_[LatType::HIT]  =                  registerStatistic<uint64_t>("latency_GetSX_hit");
    stat_latency_GetSX_[LatType::MISS] =                  registerStatistic<uint64_t>("latency_GetSX_miss");
    stat_latency_FlushLine_[LatType::HIT] =               registerStatistic<uint64_t>("latency_FlushLine");
    stat_latency_FlushLine_[LatType::MISS] =              registerStatistic<uint64_t>("latency_FlushLine_fail");
    stat_latency_FlushLineInv_[LatType::HIT] =            registerStatistic<uint64_t>("latency_FlushLineInv");
    stat_latency_FlushLineInv_[LatType::MISS] =           registerStatistic<uint64_t>("latency_FlushLineInv_fail");

    stat_hit_[0][0] =  registerStatistic<uint64_t>("GetSHit_Arrival");
    stat_hit_[1][0] =  registerStatistic<uint64_t>("GetXHit_Arrival");
    stat_hit_[2][0] =  registerStatistic<uint64_t>("GetSXHit_Arrival");
    stat_hit_[0][1] =  registerStatistic<uint64_t>("GetSHit_Blocked");
    stat_hit_[1][1] =  registerStatistic<uint64_t>("GetXHit_Blocked");
    stat_hit_[2][1] =  registerStatistic<uint64_t>("GetSXHit_Blocked");
    stat_miss_[0][0] = registerStatistic<uint64_t>("GetSMiss_Arrival");
    stat_miss_[1][0] = registerStatistic<uint64_t>("GetXMiss_Arrival");
    stat_miss_[2][0] = registerStatistic<uint64_t>("GetSXMiss_Arrival");
    stat_miss_[0][1] = registerStatistic<uint64_t>("GetSMiss_Blocked");
    stat_miss_[1][1] = registerStatistic<uint64_t>("GetXMiss_Blocked");
    stat_miss_[2][1] = registerStatistic<uint64_t>("GetSXMiss_Blocked");
    stat_hits_ =       registerStatistic<uint64_t>("CacheHits");
    stat_misses_ =     registerStatistic<uint64_t>("CacheMisses");
    stat_evict_[I] =   registerStatistic<uint64_t>("evict_I");
    stat_evict_[E] =   registerStatistic<uint64_t>("evict_E");
    stat_evict_[M] =   registerStatistic<uint64_t>("evict_M");
    stat_evict_[IM] =  registerStatistic<uint64_t>("evict_IM");
    stat_evict_[I_B] = registerStatistic<uint64_t>("evict_IB");
    stat_evict_[S_B] = registerStatistic<uint64_t>("evict_SB");

    /* Only for caches that write back clean blocks (i.e., lower cache is non-inclusive and may need the data) but don't know yet and can't register statistics later. Always enabled for now. */
    stat_event_sent_[(int)Command::PutE] = registerStatistic<uint64_t>("eventSent_PutE");

    /* Prefetch statistics */
    if (prefetch) {
        stat_prefetch_evict_ =     registerStatistic<uint64_t>("prefetch_evict");
        stat_prefetch_hit_ =       registerStatistic<uint64_t>("prefetch_useful");
        stat_prefetch_redundant_ = registerStatistic<uint64_t>("prefetch_redundant");
    }
}


/***********************************************************************************************************
 * Event handlers
 ***********************************************************************************************************/

/* Handle GetS (load/read) requests */
bool IncoherentL1::handleGetS(MemEvent* event, bool in_mshr){
    Addr addr = event->getBaseAddr();
    L1CacheLine * line = cache_array_->lookup(addr, true);
    bool local_prefetch = event->isPrefetch() && (event->getRqstr() == cachename_);
    State state = line ? line->getState() : I;
    uint64_t send_time = 0;
    MemEventStatus status = MemEventStatus::OK;
    vector<uint8_t> data;

    if (mem_h_is_debug_addr(addr)) {
        event_debuginfo_.prefill(event->getID(), Command::GetS, (local_prefetch ? "-pref" : ""), addr, state);
        event_debuginfo_.reason = "hit";
    }

    switch (state) {
        case I:
            status = processCacheMiss(event, line, in_mshr); // Attempt to allocate a line if needed

            if (status == MemEventStatus::OK) { // Both MSHR insert & cache line allocation succeeded
                line = cache_array_->lookup(addr, false);
                if (!mshr_->getProfiled(addr)) {
                    recordLatencyType(event->getID(), LatType::MISS);
                    stat_event_state_[(int)Command::GetS][I]->addData(1);
                    stat_miss_[0][in_mshr]->addData(1);
                    stat_misses_->addData(1);
                    notifyListenerOfAccess(event, NotifyAccessType::READ, NotifyResultType::MISS);
                    mshr_->setProfiled(addr);
                }
                send_time = forwardMessage(event, line_size_, 0, nullptr);
                line->setState(IM);
                line->setTimestamp(send_time);
                if (mem_h_is_debug_addr(addr))
                    event_debuginfo_.reason = "miss";
                mshr_->setInProgress(addr);
            } else {
                recordMiss(event->getID());
            }
            break;
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
                recordPrefetchResult(line, stat_prefetch_redundant_);
                cleanUpAfterRequest(event, in_mshr);
                break;
            }

            recordPrefetchResult(line, stat_prefetch_hit_);
            recordLatencyType(event->getID(), LatType::HIT);

            if (event->isLoadLink())
                line->atomicStart(timestamp_ + llsc_block_cycles_, event->getThreadID());

            data.assign(line->getData()->begin() + (event->getAddr() - event->getBaseAddr()), line->getData()->begin() + (event->getAddr() - event->getBaseAddr() + event->getSize()));
            send_time = sendResponseUp(event, &data, in_mshr, line->getTimestamp());
            line->setTimestamp(send_time-1);
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
        event_debuginfo_.newst = line->getState();
        event_debuginfo_.verboseline = line->getString();
    }

    return ((status == MemEventStatus::Reject) ? false : true);
}

/* Handle cacheable GetX requests */
bool IncoherentL1::handleWrite(MemEvent* event, bool in_mshr) {
    return handleGetX(event, in_mshr);
}

/* Handle GetX (store/write) requests
 * GetX may also be a store-conditional or write-unlock
 */
bool IncoherentL1::handleGetX(MemEvent* event, bool in_mshr) {
    Addr addr = event->getBaseAddr();
    L1CacheLine * line = cache_array_->lookup(addr, true);
    State state = line ? line->getState() : I;

    if (mem_h_is_debug_addr(addr))
        event_debuginfo_.prefill(event->getID(), Command::GetX, (event->isStoreConditional() ? "-SC" : ""), addr, state);

    MemEventStatus status = MemEventStatus::OK;
    bool success = true;
    uint64_t send_time = 0;

    if (in_mshr)
        mshr_->removePendingRetry(addr);

    switch (state) {
        case I:
            status = processCacheMiss(event, line, in_mshr); // Attempt to allocate a line if needed
            if (status == MemEventStatus::OK) { // Both MSHR insert & cache line allocation succeeded
                line = cache_array_->lookup(addr, false);

                // Profile
               if (!mshr_->getProfiled(addr)) {
                    notifyListenerOfAccess(event, NotifyAccessType::WRITE, NotifyResultType::MISS);
                    stat_event_state_[(int)Command::GetX][I]->addData(1);
                    stat_miss_[1][in_mshr]->addData(1);
                    stat_misses_->addData(1);
                    recordLatencyType(event->getID(), LatType::MISS);
                    mshr_->setProfiled(addr);
                }

                // Handle
                send_time = forwardMessage(event, line_size_, 0, nullptr, Command::GetX);
                line->setState(IM);
                line->setTimestamp(send_time);
                if (mem_h_is_debug_addr(addr))
                    event_debuginfo_.reason = "miss";
                mshr_->setInProgress(addr);
            } else {
                recordMiss(event->getID());
            }
            break;
        case E:
            line->setState(M);
        case M:
            // Profile
            recordPrefetchResult(line, stat_prefetch_hit_);
            if (!in_mshr || !mshr_->getProfiled(addr)) {
                notifyListenerOfAccess(event, NotifyAccessType::WRITE, NotifyResultType::HIT);
                recordLatencyType(event->getID(), LatType::HIT);
                stat_event_state_[(int)Command::GetX][state]->addData(1);
                stat_hit_[1][in_mshr]->addData(1);
                stat_hits_->addData(1);
            }

            // Handle
            if (!event->isStoreConditional() || line->isAtomic(event->getThreadID())) { /* Don't write on a non-atomic SC */
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
            if (mem_h_is_debug_addr(addr))
                event_debuginfo_.reason = "hit";
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
        event_debuginfo_.newst = line->getState();
        event_debuginfo_.verboseline = line->getString();
    }

    return (status == MemEventStatus::Reject) ? false : true;
}


bool IncoherentL1::handleGetSX(MemEvent* event, bool in_mshr) {
    Addr addr = event->getBaseAddr();
    L1CacheLine * line = cache_array_->lookup(addr, true);
    State state = line ? line->getState() : I;

    MemEventStatus status = MemEventStatus::OK;
    uint64_t send_time = 0;
    std::vector<uint8_t> data;

    if (mem_h_is_debug_addr(addr))
        event_debuginfo_.prefill(event->getID(), Command::GetSX, (event->isLoadLink() ? "-LL" : ""), addr, state);

    switch (state) {
        case I:
            status = processCacheMiss(event, line, in_mshr); // Attempt to allocate a line if needed
            if (status == MemEventStatus::OK) { // Both MSHR insert & cache line allocation succeeded
                line = cache_array_->lookup(addr, false);
                // Profile
                if (!mshr_->getProfiled(addr)) {
                    notifyListenerOfAccess(event, NotifyAccessType::READ, NotifyResultType::MISS);
                    stat_event_state_[(int)Command::GetSX][I]->addData(1);
                    stat_miss_[2][in_mshr]->addData(1);
                    stat_misses_->addData(1);
                    recordLatencyType(event->getID(), LatType::MISS);
                    mshr_->setProfiled(addr);
                }
                // Handle
                send_time = forwardMessage(event, line_size_, 0, nullptr);
                line->setState(IM);
                line->setTimestamp(send_time);
                if (mem_h_is_debug_addr(addr))
                    event_debuginfo_.reason = "miss";
                mshr_->setInProgress(addr);
            } else {
                recordMiss(event->getID());
            }
            break;
        case E:
            line->setState(M);
        case M:
            // Profile
            recordPrefetchResult(line, stat_prefetch_hit_);
            if (!in_mshr || !mshr_->getProfiled(addr)) {
                notifyListenerOfAccess(event, NotifyAccessType::READ, NotifyResultType::HIT);
                recordLatencyType(event->getID(), LatType::HIT);
                stat_event_state_[(int)Command::GetSX][state]->addData(1);
                stat_hit_[2][in_mshr]->addData(1);
                stat_hits_->addData(1);
            }
            // Handle
            line->incLock();
            std::copy(line->getData()->begin() + (event->getAddr() - event->getBaseAddr()), line->getData()->begin() + (event->getAddr() - event->getBaseAddr())  + event->getSize(), data.begin());
            send_time = sendResponseUp(event, &data, in_mshr, line->getTimestamp());
            line->setTimestamp(send_time-1);
            if (mem_h_is_debug_addr(addr))
                    event_debuginfo_.reason = "hit";
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
        event_debuginfo_.newst = line->getState();
        event_debuginfo_.verboseline = line->getString();
    }

    return (status == MemEventStatus::Reject) ? false : true;
}

/* Flush a dirty line from the cache but retain a clean copy */
bool IncoherentL1::handleFlushLine(MemEvent* event, bool in_mshr) {
    Addr addr = event->getBaseAddr();
    L1CacheLine * line = cache_array_->lookup(addr, false);
    State state = line ? line->getState() : I;

    if (mem_h_is_debug_addr(addr))
        event_debuginfo_.prefill(event->getID(), Command::FlushLine, "", addr, state);

    if (!in_mshr && mshr_->exists(addr)) {
        return (allocateMSHR(event, false) == MemEventStatus::Reject) ? false : true;
    }

    // At this point, state must be stable

    /* Flush fails if line is locked */
    if (state != I && line->isLocked(timestamp_)) {
        if (!in_mshr || !mshr_->getProfiled(addr)) {
            stat_event_state_[(int)Command::FlushLine][state]->addData(1);
        }
        sendResponseUp(event, nullptr, in_mshr, line->getTimestamp(), false);
        recordLatencyType(event->getID(), LatType::MISS);
        cleanUpAfterRequest(event, in_mshr);

        if (mem_h_is_debug_addr(addr)) {
            if (line)
                event_debuginfo_.verboseline = line->getString();
            event_debuginfo_.reason = "fail, line locked";
        }

        return true;
    }

    // At this point, we need an MSHR entry
    if (!in_mshr && (allocateMSHR(event, false) == MemEventStatus::Reject))
        return false;

    if (!mshr_->getProfiled(addr)) {
        stat_event_state_[(int)Command::FlushLine][state]->addData(1);
        mshr_->setProfiled(addr);
    }

    recordLatencyType(event->getID(), LatType::HIT);
    mshr_->setInProgress(addr);

    forwardFlush(event, line, state == M);

    if (line && state != I)
        line->setState(E_B);

    if (mem_h_is_debug_addr(addr) && line) {
        event_debuginfo_.newst = line->getState();
        event_debuginfo_.verboseline = line->getString();
    }

    return true;
}


/* Flush a line from cache & invalidate it */
bool IncoherentL1::handleFlushLineInv(MemEvent* event, bool in_mshr) {
    Addr addr = event->getBaseAddr();
    L1CacheLine * line = cache_array_->lookup(addr, false);
    State state = line ? line->getState() : I;

    if (mem_h_is_debug_addr(addr))
        event_debuginfo_.prefill(event->getID(), Command::FlushLineInv, "", addr, state);

    if (!in_mshr && mshr_->exists(addr)) {
        return (allocateMSHR(event, false) == MemEventStatus::Reject) ? false : true;
    }

    /* Flush fails if line is locked */
    if (state != I && line->isLocked(timestamp_)) {
        stat_event_state_[(int)Command::FlushLineInv][state]->addData(1);
        sendResponseUp(event, nullptr, in_mshr, line->getTimestamp(), false);
        recordLatencyType(event->getID(), LatType::MISS);
        cleanUpAfterRequest(event, in_mshr);
        if (mem_h_is_debug_addr(addr)) {
            if (line)
                event_debuginfo_.verboseline = line->getString();
            event_debuginfo_.reason = "fail, line locked";
        }
        return true;
    }

    // At this point we need an MSHR entry
    if (!in_mshr && (allocateMSHR(event, false) == MemEventStatus::Reject))
        return false;

    mshr_->setInProgress(addr);
    recordLatencyType(event->getID(), LatType::HIT);
    if (!mshr_->getProfiled(addr)) {
        stat_event_state_[(int)Command::FlushLineInv][state]->addData(1);
        if (line)
            recordPrefetchResult(line, stat_prefetch_evict_);
        mshr_->setProfiled(addr);
    }

    forwardFlush(event, line, state != I);
    if (state != I)
        line->setState(I_B);

    if (mem_h_is_debug_addr(addr) && line) {
        event_debuginfo_.newst = line->getState();
        event_debuginfo_.verboseline = line->getString();
    }

    return true;
}


bool IncoherentL1::handleGetSResp(MemEvent * event, bool in_mshr) {
    Addr addr = event->getBaseAddr();
    L1CacheLine * line = cache_array_->lookup(addr, false);
    State state = line ? line->getState() : I;

    stat_event_state_[(int)(event->getCmd())][state]->addData(1);

    MemEvent * request = static_cast<MemEvent*>(mshr_->getFrontEvent(addr));
    bool local_prefetch = request->isPrefetch() && (request->getRqstr() == cachename_);

   if (mem_h_is_debug_addr(addr))
        event_debuginfo_.prefill(event->getID(), Command::GetSResp, (local_prefetch ? "-pref" : ""), addr, state);

    // Update line
    line->setData(event->getPayload(), 0);
    line->setState(E);
    if (mem_h_is_debug_addr(addr))
        printDataValue(addr, line->getData(), false);

    if (request->isLoadLink())
        line->atomicStart(timestamp_ + llsc_block_cycles_, request->getThreadID());

    if (mem_h_is_debug_addr(addr))
        printDataValue(addr, line->getData(), true);

    // Notify processor or set prefetch so we can track prefetch results
    if (local_prefetch) {
        line->setPrefetch(true);
        recordPrefetchLatency(request->getID(), LatType::MISS);
        if (mem_h_is_debug_addr(addr))
            event_debuginfo_.action = "Done";
    } else {
        request->setMemFlags(event->getMemFlags());
        Addr offset = request->getAddr() - request->getBaseAddr();
        vector<uint8_t> data(line->getData()->begin() + offset, line->getData()->begin() + offset + request->getSize());
        uint64_t send_time = sendResponseUp(request, &data, true, line->getTimestamp());
        line->setTimestamp(send_time-1);
    }

    if (mem_h_is_debug_addr(addr)) {
        event_debuginfo_.newst = line->getState();
        event_debuginfo_.verboseline = line->getString();
    }

    cleanUpAfterResponse(event, in_mshr);
    return true;
}


bool IncoherentL1::handleGetXResp(MemEvent * event, bool in_mshr) {
    Addr addr = event->getBaseAddr();
    L1CacheLine * line = cache_array_->lookup(addr, false);
    State state = line ? line->getState() : I;

    MemEvent * request = static_cast<MemEvent*>(mshr_->getFrontEvent(event->getBaseAddr()));
    if (request->getCmd() == Command::GetS) {
        return handleGetSResp(event, in_mshr);
    }

    if (mem_h_is_debug_addr(addr))
        event_debuginfo_.prefill(event->getID(), Command::GetXResp, "", addr, state);

    request->setMemFlags(event->getMemFlags());

    // Set line data
    line->setData(event->getPayload(), 0);
    if (mem_h_is_debug_addr(line->getAddr()))
        printDataValue(line->getAddr(), line->getData(), true);


    line->setState(M);

    /* Execute write */
    Addr offset = request->getAddr() - request->getBaseAddr();
    std::vector<uint8_t> data;
    bool success = true;
    if (request->getCmd() == Command::GetX || request->getCmd() == Command::Write) {
        if (!request->isStoreConditional() || line->isAtomic(request->getThreadID())) {
            line->setData(request->getPayload(), offset);
            if (mem_h_is_debug_addr(line->getAddr()))
                printDataValue(line->getAddr(), line->getData(), true);
            line->atomicEnd();
        } else {
            success = false;
        }

        if (request->queryFlag(MemEventBase::F_LOCKED)) {
            line->decLock();
        }

    } else { // Read-lock
        line->incLock();
    }

    // Return response
    data.assign(line->getData()->begin() + offset, line->getData()->begin() + offset + request->getSize());
    uint64_t send_time = sendResponseUp(request, &data, true, line->getTimestamp(), success);
    line->setTimestamp(send_time-1);

    stat_event_state_[(int)Command::GetXResp][state]->addData(1);
    if (mem_h_is_debug_addr(addr)) {
        event_debuginfo_.newst = line->getState();
        event_debuginfo_.verboseline = line->getString();
    }
    cleanUpAfterResponse(event, in_mshr);
    return true;
}


bool IncoherentL1::handleFlushLineResp(MemEvent * event, bool in_mshr) {
    Addr addr = event->getBaseAddr();
    L1CacheLine * line = cache_array_->lookup(addr, false);
    State state = line ? line->getState() : I;

    stat_event_state_[(int)Command::FlushLineResp][state]->addData(1);

    if (mem_h_is_debug_addr(addr))
        event_debuginfo_.prefill(event->getID(), Command::FlushLineResp, "", addr, state);

    MemEvent * request = static_cast<MemEvent*>(mshr_->getFrontEvent(addr));

    switch (state) {
        case I:
            break;
        case I_B:
            line->setState(I);
            line->atomicEnd();
            break;
        case E_B:
            line->setState(E);
            break;
        default:
            debug_->fatal(CALL_INFO,-1,"%s, Error: Received FlushLineResp in unhandled state '%s'. Event: %s. Time = %" PRIu64 "ns\n",
                    getName().c_str(), StateString[state], event->getVerboseString().c_str(), getCurrentSimTimeNano());
    }

    sendResponseUp(request, nullptr, timestamp_, event->success());

    if (mem_h_is_debug_event(event)) {
        event_debuginfo_.action = "Respond";
        if (line) {
            event_debuginfo_.verboseline = line->getString();
            event_debuginfo_.newst = line->getState();
        }
    }

    cleanUpAfterResponse(event, in_mshr);
    return true;
}

/* We're using NULLCMD to signal an internally generated event - in this case an eviction */
bool IncoherentL1::handleNULLCMD(MemEvent* event, bool in_mshr) {
    Addr oldAddr = event->getAddr();
    Addr newAddr = event->getBaseAddr();

    L1CacheLine * line = cache_array_->lookup(oldAddr, false);
    bool evicted = handleEviction(newAddr, line);

    if (mem_h_is_debug_addr(newAddr)) {
        event_debuginfo_.prefill(event->getID(), Command::NULLCMD, "", line->getAddr(), evict_debuginfo_.oldst);
        event_debuginfo_.newst = line->getState();
        event_debuginfo_.verboseline = line->getString();
    }

    if (evicted) {
        notifyListenerOfEvict(line->getAddr(), line_size_, event->getInstructionPointer());
        retry_buffer_.push_back(mshr_->getFrontEvent(newAddr));
        if (mshr_->removeEvictPointer(oldAddr, newAddr))
            retry(oldAddr);
        if (mem_h_is_debug_addr(newAddr)) {
            event_debuginfo_.action = "Retry";
            std::stringstream reason;
            reason << "0x" << std::hex << newAddr;
            event_debuginfo_.reason = reason.str();
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

bool IncoherentL1::handleNACK(MemEvent* event, bool in_mshr) {
    MemEvent* nackedEvent = event->getNACKedEvent();
    L1CacheLine * line = cache_array_->lookup(event->getBaseAddr(), false);
    State state = line ? line->getState() : I;

    if (mem_h_is_debug_addr(event->getBaseAddr()))
        event_debuginfo_.prefill(event->getID(), Command::NACK, "", event->getBaseAddr(), state);

    delete event;
    resendEvent(nackedEvent, false); // resend this down (since we're an L1)

    return true;
}

/***********************************************************************************************************
 * MSHR & CacheArray management
 ***********************************************************************************************************/

/**
 * Handle a cache miss
 */
MemEventStatus IncoherentL1::processCacheMiss(MemEvent * event, L1CacheLine * line, bool in_mshr) {
    MemEventStatus status = in_mshr ? MemEventStatus::OK : allocateMSHR(event, false); // Miss means we need an MSHR entry
    if (status == MemEventStatus::OK && !line) { // Need a cache line too
        line = allocateLine(event, line);
        status = line ? MemEventStatus::OK : MemEventStatus::Stall;
    }
    return status;
}


/**
 * Allocate a new MSHR entry
 */
MemEventStatus IncoherentL1::allocateMSHR(MemEvent * event, bool fwdReq, int pos) {
    // Screen prefetches first to ensure limits are not exceeeded:
    //      - Maximum number of outstanding prefetches
    //      - MSHR too full to accept prefetches
    if (event->isPrefetch() && event->getRqstr() == cachename_) {
        if (drop_prefetch_level_ <= mshr_->getSize()) {
            event_debuginfo_.action = "Reject";
            event_debuginfo_.reason = "Prefetch drop level";
            return MemEventStatus::Reject;
        }
        if (max_outstanding_prefetch_ <= outstanding_prefetch_count_) {
            event_debuginfo_.action = "Reject";
            event_debuginfo_.reason = "Max outstanding prefetches";
            return MemEventStatus::Reject;
        }
    }

    int insert_pos = mshr_->insertEvent(event->getBaseAddr(), event, pos, fwdReq, false);
    if (insert_pos == -1)
        return MemEventStatus::Reject; // MSHR is full
    else if (insert_pos != 0) {
        if (event->isPrefetch()) outstanding_prefetch_count_++;
        return MemEventStatus::Stall;
    }

    if (event->isPrefetch()) outstanding_prefetch_count_++;

    return MemEventStatus::OK;
}


/**
 * Allocate a new cache line
 */
L1CacheLine* IncoherentL1::allocateLine(MemEvent * event, L1CacheLine * line) {
    bool evicted = handleEviction(event->getBaseAddr(), line);
    if (evicted) {
        if (mem_h_is_debug_event(event) || mem_h_is_debug_addr(event->getBaseAddr()))
            printDebugAlloc(true, event->getBaseAddr(), "");
        notifyListenerOfEvict(line->getAddr(), line_size_, event->getInstructionPointer());
        cache_array_->replace(event->getBaseAddr(), line);
        return line;
    } else {
        if (mem_h_is_debug_event(event) || mem_h_is_debug_addr(line->getAddr())) {
            event_debuginfo_.action = "Stall";
            std::stringstream st;
            st << "evict 0x" << std::hex << line->getAddr() << ", ";
            event_debuginfo_.reason = st.str();
        }
        mshr_->insertEviction(line->getAddr(), event->getBaseAddr());
        return nullptr;
    }
}

bool IncoherentL1::handleEviction(Addr addr, L1CacheLine*& line) {
    if (!line)
        line = cache_array_->findReplacementCandidate(addr);
    State state = line->getState();

    if (mem_h_is_debug_addr(addr))
        evict_debuginfo_.oldst = line->getState();

    /* L1s can have locked cache lines */
    if (line->isLocked(timestamp_)) {
        if (mem_h_is_debug_addr(line->getAddr()))
            printDebugAlloc(false, line->getAddr(), "InProg, line locked");
        return false;
    }

    stat_evict_[state]->addData(1);

    switch (state) {
        case I:
            if (mem_h_is_debug_addr(line->getAddr()))
                printDebugAlloc(false, line->getAddr(), "Drop");
            return true;
        case E:
            if (mem_h_is_debug_addr(line->getAddr()))
                printDebugAlloc(false, line->getAddr(), "Drop");
            line->setState(I);
            break;
        case M:
            if (mem_h_is_debug_addr(line->getAddr()))
                printDebugAlloc(false, line->getAddr(), "Writeback");
            sendWriteback(Command::PutM, line, true);
            cache_array_->deallocate(line);
            line->setState(I);
            break;
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


void IncoherentL1::cleanUpAfterRequest(MemEvent* event, bool in_mshr) {
    Addr addr = event->getBaseAddr();
    Command cmd = event->getCmd();

    /* Remove from MSHR */
    if (in_mshr) {
        mshr_->removeFront(addr);
    }

    delete event;

    /* Replay any waiting events */
    if (mshr_->exists(addr)) {
        if (mshr_->getFrontType(addr) == MSHREntryType::Event) {
            if (!mshr_->getInProgress(addr)) {
                retry_buffer_.push_back(mshr_->getFrontEvent(addr));
            }
        } else { // Pointer -> either we're waiting for a writeback ACK or another address is waiting for this one
            if (mshr_->getFrontType(addr) == MSHREntryType::Evict) {
                std::list<Addr>* evictPointers = mshr_->getEvictPointers(addr);
                for (std::list<Addr>::iterator it = evictPointers->begin(); it != evictPointers->end(); it++) {
                    MemEvent * ev = new MemEvent(cachename_, addr, *it, Command::NULLCMD, getCurrentSimTimeNano());
                    retry_buffer_.push_back(ev);
                }
            }
        }
    }
}

void IncoherentL1::cleanUpAfterResponse(MemEvent* event, bool in_mshr) {
    Addr addr = event->getBaseAddr();

    /* Clean up MSHR */
    MemEvent * request = nullptr;
    if (mshr_->getFrontType(addr) == MSHREntryType::Event)
        request = static_cast<MemEvent*>(mshr_->getFrontEvent(addr));
    mshr_->removeFront(addr); // delete request after this since debug might print the event it's removing
    delete event;
    if (request)
        delete request;

    if (mshr_->exists(addr)) {
        if (mshr_->getFrontType(addr) == MSHREntryType::Event) {
            if (!mshr_->getInProgress(addr)) {
                retry_buffer_.push_back(mshr_->getFrontEvent(addr));
            }
        } else {
            std::list<Addr>* evictPointers = mshr_->getEvictPointers(addr);
            for (std::list<Addr>::iterator it = evictPointers->begin(); it != evictPointers->end(); it++) {
                MemEvent * ev = new MemEvent(cachename_, addr, *it, Command::NULLCMD, getCurrentSimTimeNano());
                retry_buffer_.push_back(ev);
            }
        }
    }
}

void IncoherentL1::retry(Addr addr) {
    if (mshr_->exists(addr)) {
        if (mshr_->getFrontType(addr) == MSHREntryType::Event) {
            retry_buffer_.push_back(mshr_->getFrontEvent(addr));
        } else if (!(mshr_->pendingWriteback(addr))) {
            std::list<Addr>* evictPointers = mshr_->getEvictPointers(addr);
            for (std::list<Addr>::iterator it = evictPointers->begin(); it != evictPointers->end(); it++) {
                MemEvent * ev = new MemEvent(cachename_, addr, *it, Command::NULLCMD, getCurrentSimTimeNano());
                retry_buffer_.push_back(ev);
            }
        }
    }
}


/***********************************************************************************************************
 * Protocol helper functions
 ***********************************************************************************************************/

uint64_t IncoherentL1::sendResponseUp(MemEvent * event, vector<uint8_t> * data, bool in_mshr, uint64_t time, bool success) {
    Command cmd = event->getCmd();
    MemEvent * responseEvent = event->makeResponse();

    /* Only return the desired word */
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
    uint64_t delivery_time = time + (in_mshr ? mshr_latency_ : access_latency_);
    forwardByDestination(responseEvent, delivery_time);

    // Debugging
    if (mem_h_is_debug_event(responseEvent))
        event_debuginfo_.action = "Respond";

    return delivery_time;
}

void IncoherentL1::sendResponseDown(MemEvent * event, L1CacheLine * line, bool data) {
    MemEvent * responseEvent = event->makeResponse();

    if (data) {
        responseEvent->setPayload(*line->getData());
        if (line->getState() == M)
            responseEvent->setDirty(true);
    }

    responseEvent->setSize(line_size_);

    uint64_t delivery_time = timestamp_ + (data ? access_latency_ : tag_latency_);
    forwardByDestination(responseEvent, delivery_time);

    if (mem_h_is_debug_event(responseEvent)) {
        event_debuginfo_.action = "Respond";
    }
}


void IncoherentL1::forwardFlush(MemEvent * event, L1CacheLine * line, bool evict) {
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
    if (mem_h_is_debug_addr(event->getBaseAddr()))
        event_debuginfo_.action = "forward";
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
void IncoherentL1::sendWriteback(Command cmd, L1CacheLine* line, bool dirty) {
    MemEvent* writeback = new MemEvent(cachename_, line->getAddr(), line->getAddr(), cmd, getCurrentSimTimeNano());
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

    if (mem_h_is_debug_addr(line->getAddr()))
        debug_->debug(_L3_,"Sending Writeback at cycle = %" PRIu64 ", Cmd = %s. With%s data.\n", delivery_time, CommandString[(int)cmd], ((cmd == Command::PutM || writeback_clean_blocks_) ? "" : "out"));
}

/*----------------------------------------------------------------------------------------------------------------------
 *  Override message send functions with versions that record statistics & call parent class
 *---------------------------------------------------------------------------------------------------------------------*/
void IncoherentL1::forwardByAddress(MemEventBase* ev, Cycle_t timestamp) {
    stat_event_sent_[(int)ev->getCmd()]->addData(1);
    CoherenceController::forwardByAddress(ev, timestamp);
}

void IncoherentL1::forwardByDestination(MemEventBase* ev, Cycle_t timestamp) {
    stat_event_sent_[(int)ev->getCmd()]->addData(1);
    CoherenceController::forwardByDestination(ev, timestamp);
}

/********************
 * Helper functions
 ********************/

MemEventInitCoherence* IncoherentL1::getInitCoherenceEvent() {
    // Source, Endpoint type, inclusive, sends WB Acks, line size, tracks block presence
    return new MemEventInitCoherence(cachename_, Endpoint::Cache, true, false, false, line_size_, true);
}

void IncoherentL1::printLine(Addr addr) {
    /*if (!mem_h_is_debug_addr(addr)) return;
    L1CacheLine * line = cache_array_->lookup(addr, false);
    std::string state = (line == nullptr) ? "NP" : line->getString();
    debug_->debug(_L8_, "  Line 0x%" PRIx64 ": %s\n", addr, state.c_str());*/
}

/* Record the result of a prefetch. important: assumes line is not null */
void IncoherentL1::recordPrefetchResult(L1CacheLine * line, Statistic<uint64_t> * stat) {
    if (line->getPrefetch()) {
        stat->addData(1);
        line->setPrefetch(false);
    }
}

void IncoherentL1::recordLatency(Command cmd, int type, uint64_t latency) {
    if (type == -1)
        return;

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


std::set<Command> IncoherentL1::getValidReceiveEvents() {
    std::set<Command> cmds = { Command::GetS,
        Command::GetX,
        Command::GetSX,
        Command::Write,
        Command::FlushLine,
        Command::FlushLineInv,
        Command::NULLCMD,
        Command::GetSResp,
        Command::GetXResp,
        Command::WriteResp,
        Command::FlushLineResp,
        Command::NACK };
    return cmds;
}


void IncoherentL1::serialize_order(SST::Core::Serialization::serializer& ser) {
    CoherenceController::serialize_order(ser);

    SST_SER(cache_array_);
    SST_SER(llsc_block_cycles_);
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

