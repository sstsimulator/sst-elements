// Copyright 2009-2015 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2015, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#include <sst_config.h>
#include <vector>
#include "IncoherentController.h"
using namespace SST;
using namespace SST::MemHierarchy;

/*----------------------------------------------------------------------------------------------------------------------
 * Incoherent Controller Implementation
 * Non-Inclusive caches do not allocate on Get* requests except for prefetches
 * Inclusive caches do
 * No writebacks except dirty data
 * I = not present in the cache, E = present & clean, M = present & dirty
 *---------------------------------------------------------------------------------------------------------------------*/

/*----------------------------------------------------------------------------------------------------------------------
 *  External interface functions for routing events from cache controller to appropriate coherence handlers
 *---------------------------------------------------------------------------------------------------------------------*/

/**
 *  Handle eviction. Stall if eviction candidate is in transition.
 */
CacheAction IncoherentController::handleEviction(CacheLine* wbCacheLine, uint32_t groupId, string origRqstr, bool ignoredParam) {
    State state = wbCacheLine->getState();
    setGroupId(groupId);
    recordEvictionState(state);
    
    switch (state) {
        case I:
            return DONE;
        case E:
            if (writebackCleanBlocks_) {
                sendWriteback(PutE, wbCacheLine, origRqstr);
                inc_EvictionPUTEReqSent();
            }
            wbCacheLine->setState(I);
            return DONE;
        case M:
            sendWriteback(PutM, wbCacheLine, origRqstr);
            inc_EvictionPUTMReqSent();
            wbCacheLine->setState(I);
            return DONE;
        case IS:
            return STALL;
        default:
	    d_->fatal(CALL_INFO,-1,"%s, Error: State is invalid during eviction: %s. Addr = 0x%" PRIx64 ". Time = %" PRIu64 "ns\n", 
                    name_.c_str(), StateString[state], wbCacheLine->getBaseAddr(), ((Component *)owner_)->getCurrentSimTimeNano());
    }
    return STALL; // Eliminate compiler warning
}


/**
 *  Handle a Get* request
 *  Obtain block if a cache miss
 *  Obtain needed coherence permission from lower level cache/memory if coherence miss
 */
CacheAction IncoherentController::handleRequest(MemEvent* event, CacheLine* cacheLine, bool replay) {
    setGroupId(event->getGroupId());
    if (DEBUG_ALL || DEBUG_ADDR == cacheLine->getBaseAddr())   d_->debug(_L6_,"State = %s\n", StateString[cacheLine->getState()]);

    Command cmd = event->getCmd();

    switch(cmd) {
        case GetS:
            return handleGetSRequest(event, cacheLine, replay);
        case GetX:
        case GetSEx:
            return handleGetXRequest(event, cacheLine, replay);
        default:
	    d_->fatal(CALL_INFO,-1,"%s, Error: Received an unrecognized request: %s. Addr = 0x%" PRIx64 ", Src = %s. Time = %" PRIu64 "ns\n", 
                    name_.c_str(), CommandString[cmd], event->getBaseAddr(), event->getSrc().c_str(), ((Component *)owner_)->getCurrentSimTimeNano());
    }
    return STALL;    // Eliminate compiler warning
}


/**
 *  Handle replacement. 
 */
CacheAction IncoherentController::handleReplacement(MemEvent* event, CacheLine* cacheLine, MemEvent * reqEvent, bool replay) {
    setGroupId(event->getGroupId());
    
    // May need to update state since we just allocated
    if (reqEvent != NULL && reqEvent->getCmd() == GetS && cacheLine->getState() == I) cacheLine->setState(IS);
    if (reqEvent != NULL && reqEvent->getCmd() == GetX && cacheLine->getState() == I) cacheLine->setState(IM);

    if (cacheLine != NULL && (DEBUG_ALL || DEBUG_ADDR == event->getBaseAddr()))   d_->debug(_L6_,"State = %s\n", StateString[cacheLine->getState()]);
    

    Command cmd = event->getCmd();
    CacheAction action = DONE;

    switch(cmd) {
        case PutM:
            inc_PUTMReqsReceived();
            action = handlePutMRequest(event, cacheLine);
            break;
        case PutE:
            inc_PUTEReqsReceived();
            action = handlePutMRequest(event, cacheLine);
            break;
        default:
	    d_->fatal(CALL_INFO,-1,"%s, Error: Received an unrecognized request: %s. Addr = 0x%" PRIx64 ", Src = %s. Time = %" PRIu64 "ns\n", 
                    name_.c_str(), CommandString[cmd], event->getBaseAddr(), event->getSrc().c_str(), ((Component *)owner_)->getCurrentSimTimeNano());
    }
    return action;
}


/**
 *  Handle invalidation request.
 *  Invalidations do not exist for incoherent caches but function must be implemented.
 */
CacheAction IncoherentController::handleInvalidationRequest(MemEvent * event, CacheLine * cacheLine, bool replay) {
    d_->fatal(CALL_INFO, -1, "%s, Error: Received an invalidation request: %s, but incoherent protocol does not support invalidations. Addr = 0x%" PRIx64 ", Src = %s. Time = %" PRIu64 "ns\n", 
            name_.c_str(), CommandString[event->getCmd()], event->getBaseAddr(), event->getSrc().c_str(), ((Component *)owner_)->getCurrentSimTimeNano());
    
    return STALL; // eliminate compiler warning
}


/**
 *  Handle data responses.
 */
CacheAction IncoherentController::handleResponse(MemEvent * respEvent, CacheLine * cacheLine, MemEvent * reqEvent) {
    Command cmd = respEvent->getCmd();
    switch (cmd) {
        case GetSResp:
        case GetXResp:
            return handleDataResponse(respEvent, cacheLine, reqEvent);
        default:
            d_->fatal(CALL_INFO, -1, "%s, Error: Received unrecognized response: %s. Addr = 0x%" PRIx64 ", Src = %s. Time = %" PRIu64 "ns\n",
                    name_.c_str(), CommandString[cmd], respEvent->getBaseAddr(), respEvent->getSrc().c_str(), ((Component*)owner_)->getCurrentSimTimeNano());
    }
    return DONE;
}


/*
 *  Return type of miss for profiling incoming events
 *  0:  Hit
 *  1:  NP/I
 *  2:  Wrong state (e.g., S but GetX request) --> N/A for incoherent
 *  3:  Right state but owners/sharers need to be invalidated or line is in transition --> N/A for incoherent
 */
int IncoherentController::isCoherenceMiss(MemEvent* event, CacheLine* cacheLine) {
    State state = cacheLine->getState();
    if (state == I) return 1;
    return 0;
}


/*----------------------------------------------------------------------------------------------------------------------
 *  Internal event handlers
 *---------------------------------------------------------------------------------------------------------------------*/


/**
 *  Handle GetS requests
 *  Return hit if block is present, otherwise forward request to lower level cache
 */
CacheAction IncoherentController::handleGetSRequest(MemEvent* event, CacheLine* cacheLine, bool replay) {
    State state = cacheLine->getState();
    vector<uint8_t>* data = cacheLine->getData();
    if (DEBUG_ALL || DEBUG_ADDR == event->getBaseAddr()) printData(cacheLine->getData(), false);
    
    bool shouldRespond = !(event->isPrefetch() && (event->getRqstr() == ((Component*)owner_)->getName()));
    stateStats_[event->getCmd()][state]++;
    recordStateEventCount(event->getCmd(), state);

    switch (state) {
        case I:
            forwardMessage(event, cacheLine->getBaseAddr(), cacheLine->getSize(), NULL);
            inc_GETSMissIS(event);
            cacheLine->setState(IS);
            d_->debug(_L6_,"Forwarding GetS, new state IS\n");
            return STALL;
        case E:
        case M:
            inc_GETSHit(event);
            if (!shouldRespond) return DONE;
            sendResponseUp(event, E, data, replay);
            return DONE;
        default:
            d_->fatal(CALL_INFO,-1,"%s, Error: Handling a GetS request but coherence state is not valid and stable. Addr = 0x%" PRIx64 ", Cmd = %s, Src = %s, State = %s. Time = %" PRIu64 "ns\n",
                    ((Component *)owner_)->getName().c_str(), event->getBaseAddr(), CommandString[event->getCmd()], event->getSrc().c_str(), 
                    StateString[state], ((Component *)owner_)->getCurrentSimTimeNano());
    }
    return STALL;    // eliminate compiler warning
}


/**
 *  Handle GetX request
 *  Return hit if block is present, otherwise forward request to lower level cache
 */
CacheAction IncoherentController::handleGetXRequest(MemEvent* event, CacheLine* cacheLine, bool replay) {
    State state = cacheLine->getState();
    Command cmd = event->getCmd();
    stateStats_[cmd][state]++;
    recordStateEventCount(event->getCmd(), state);
    
    switch (state) {
        case E:
        case M:
            if (cmd == GetSEx) inc_GetSExReqsReceived(replay);
            inc_GETXHit(event);
            sendResponseUp(event, M, cacheLine->getData(), replay);
            if (DEBUG_ALL || DEBUG_ADDR == event->getBaseAddr()) printData(cacheLine->getData(), false);
            return DONE;
        default:
            d_->fatal(CALL_INFO, -1, "%s, Error: Received %s int unhandled state %s. Addr = 0x%" PRIx64 ", Src = %s. Time = %" PRIu64 "ns\n",
                    name_.c_str(), CommandString[cmd], StateString[state], event->getBaseAddr(), event->getSrc().c_str(), ((Component*)owner_)->getCurrentSimTimeNano());
    }
    return STALL; // Eliminate compiler warning
}


/**
 *  Handle PutM
 *  Incoherent caches only replace dirty data
 */
CacheAction IncoherentController::handlePutMRequest(MemEvent* event, CacheLine* cacheLine) {
    State state = cacheLine->getState();

    stateStats_[event->getCmd()][state]++;
    recordStateEventCount(event->getCmd(), state);
    
    switch (state) {
        case I:
            cacheLine->setData(event->getPayload(), event);
            if (event->getDirty()) cacheLine->setState(M);
            else cacheLine->setState(E);
            break;
        case IS: // Occurs if we issued a request for a block that was cached by an upper level cache
        case IM:
            if (event->getDirty()) return BLOCK;
            else return DONE;
        case E:
            if (event->getDirty()) cacheLine->setState(M);
        case M:
            if (event->getDirty()) {
                cacheLine->setData(event->getPayload(), event);
                if (DEBUG_ALL || DEBUG_ADDR == event->getBaseAddr()) printData(cacheLine->getData(), true);
            }
            break;
        default:
	    d_->fatal(CALL_INFO, -1, "%s, Error: Updating data but cache is not in E or M state. Addr = 0x%" PRIx64 ", Cmd = %s, Src = %s, State = %s. Time = %" PRIu64 "ns\n", 
                    name_.c_str(), event->getBaseAddr(), CommandString[event->getCmd()], event->getSrc().c_str(), StateString[state], ((Component *)owner_)->getCurrentSimTimeNano());
    }
    return DONE;
}


/**
 * Handles data responses (GetSResp, GetXResp)
 */
CacheAction IncoherentController::handleDataResponse(MemEvent* responseEvent, CacheLine* cacheLine, MemEvent* origRequest){
    
    if (!inclusive_ && (cacheLine == NULL || cacheLine->getState() == I)) {
        sendResponseUp(origRequest, responseEvent->getGrantedState(), &responseEvent->getPayload(), true);
        return DONE;
    }

    cacheLine->setData(responseEvent->getPayload(), responseEvent);
    if (DEBUG_ALL || DEBUG_ADDR == responseEvent->getBaseAddr()) printData(cacheLine->getData(), true);

    State state = cacheLine->getState();
    stateStats_[responseEvent->getCmd()][state]++;
    recordStateEventCount(responseEvent->getCmd(), state);
    
    bool shouldRespond = !(origRequest->isPrefetch() && (origRequest->getRqstr() == ((Component*)owner_)->getName()));
    
    switch (state) {
        case IS:
            cacheLine->setState(E);
            inc_GETSHit(origRequest);
            if (!shouldRespond) return DONE;
            sendResponseUp(origRequest, cacheLine->getState(), cacheLine->getData(), true);
            if (DEBUG_ALL || DEBUG_ADDR == responseEvent->getBaseAddr()) printData(cacheLine->getData(), false);
            return DONE;
        case IM:
            cacheLine->setState(M); 
            sendResponseUp(origRequest, M, cacheLine->getData(), true);
            if (DEBUG_ALL || DEBUG_ADDR == responseEvent->getBaseAddr()) printData(cacheLine->getData(), false);
            return DONE;
        default:
            d_->fatal(CALL_INFO, -1, "%s, Error: Response received but state is not handled. Addr = 0x%" PRIx64 ", Cmd = %s, Src = %s, State = %s. Time = %" PRIu64 "ns\n",
                    name_.c_str(), responseEvent->getBaseAddr(), CommandString[responseEvent->getCmd()], 
                    responseEvent->getSrc().c_str(), StateString[state], ((Component *)owner_)->getCurrentSimTimeNano());
    }
    return DONE; // Eliminate compiler warning
}


/*----------------------------------------------------------------------------------------------------------------------
 *  Functions for sending events. Some of these are part of the external interface 
 *---------------------------------------------------------------------------------------------------------------------*/


/**
 *  Send writeback to lower level cache
 *  Latency: cache access + tag to read data that is being written back and update coherence state
 */
void IncoherentController::sendWriteback(Command cmd, CacheLine* cacheLine, string origRqstr){
    MemEvent* newCommandEvent = new MemEvent((SST::Component*)owner_, cacheLine->getBaseAddr(), cacheLine->getBaseAddr(), cmd);
    newCommandEvent->setDst(getDestination(cacheLine->getBaseAddr()));
    if (cmd == PutM || writebackCleanBlocks_) {
        newCommandEvent->setSize(cacheLine->getSize());
        newCommandEvent->setPayload(*cacheLine->getData());
        if (DEBUG_ALL || DEBUG_ADDR == cacheLine->getBaseAddr()) printData(cacheLine->getData(), false);
    }
    newCommandEvent->setRqstr(origRqstr);
    if (cacheLine->getState() == M) newCommandEvent->setDirty(true);
    
    uint64 deliveryTime = timestamp_ + accessLatency_;
    Response resp = {newCommandEvent, deliveryTime, false};
    addToOutgoingQueue(resp);
    
    if (DEBUG_ALL || DEBUG_ADDR == cacheLine->getBaseAddr()) d_->debug(_L3_,"Sending Writeback at cycle = %" PRIu64 ", Cmd = %s\n", deliveryTime, CommandString[cmd]);
}


/*----------------------------------------------------------------------------------------------------------------------
 *  Miscellaneous helper functions
 *---------------------------------------------------------------------------------------------------------------------*/


/*
 *  Print data values for debugging
 */
void IncoherentController::printData(vector<uint8_t> * data, bool set) {
    /*if (set)    printf("Setting data (%zu): 0x", data->size());
    else        printf("Getting data (%zu): 0x", data->size());
    
    for (unsigned int i = 0; i < data->size(); i++) {
        printf("%02x", data->at(i));
    }
    printf("\n");
    */
}

/**
 *  Print stats. If stat API stats and these disagree, stat API is probably correct!
 */
void IncoherentController::printStats(int stats, vector<int> groupIds, map<int, CtrlStats> ctrlStats, uint64_t upgradeLatency, 
        uint64_t lat_GetS_IS, uint64_t lat_GetS_M, uint64_t lat_GetX_IM, uint64_t lat_GetX_SM,
        uint64_t lat_GetX_M, uint64_t lat_GetSEx_IM, uint64_t lat_GetSEx_SM, uint64_t lat_GetSEx_M){
    Output* dbg = new Output();
    dbg->init("", 0, 0, (Output::output_location_t)stats);
    dbg->output(CALL_INFO,"\n------------------------------------------------------------------------\n");
    dbg->output(CALL_INFO,"--- Cache Stats\n");
    dbg->output(CALL_INFO,"--- Name: %s\n", name_.c_str());
    dbg->output(CALL_INFO,"--- Overall Statistics\n");
    dbg->output(CALL_INFO,"------------------------------------------------------------------------\n");

    for(unsigned int i = 0; i < groupIds.size(); i++){
        uint64_t totalMisses =  ctrlStats[groupIds[i]].newReqGetSMisses_ + ctrlStats[groupIds[i]].newReqGetXMisses_ + ctrlStats[groupIds[i]].newReqGetSExMisses_ +
                                ctrlStats[groupIds[i]].blockedReqGetSMisses_ + ctrlStats[groupIds[i]].blockedReqGetXMisses_ + ctrlStats[groupIds[i]].blockedReqGetSExMisses_;
        uint64_t totalHits =    ctrlStats[groupIds[i]].newReqGetSHits_ + ctrlStats[groupIds[i]].newReqGetXHits_ + ctrlStats[groupIds[i]].newReqGetSExHits_ +
                                ctrlStats[groupIds[i]].blockedReqGetSHits_ + ctrlStats[groupIds[i]].blockedReqGetXHits_ + ctrlStats[groupIds[i]].blockedReqGetSExHits_;

        uint64_t totalRequests = totalHits + totalMisses;
        double hitRatio = ((double)totalHits / ( totalHits + totalMisses)) * 100;
        
        if(i != 0){
            dbg->output(CALL_INFO,"------------------------------------------------------------------------\n");
            dbg->output(CALL_INFO,"--- Cache Stats\n");
            dbg->output(CALL_INFO,"--- Name: %s\n", name_.c_str());
            dbg->output(CALL_INFO,"--- Group Statistics, Group ID = %i\n", groupIds[i]);
            dbg->output(CALL_INFO,"------------------------------------------------------------------------\n");
        }
        dbg->output(CALL_INFO,"- Total data requests:                           %" PRIu64 "\n", totalRequests);
        dbg->output(CALL_INFO,"     GetS:                                       %" PRIu64 "\n", 
                ctrlStats[groupIds[i]].newReqGetSHits_ + ctrlStats[groupIds[i]].newReqGetSMisses_ + 
                ctrlStats[groupIds[i]].blockedReqGetSHits_ + ctrlStats[groupIds[i]].blockedReqGetSMisses_);                                  
        dbg->output(CALL_INFO,"     GetX:                                       %" PRIu64 "\n", 
                ctrlStats[groupIds[i]].newReqGetXHits_ + ctrlStats[groupIds[i]].newReqGetXMisses_ + 
                ctrlStats[groupIds[i]].blockedReqGetXHits_ + ctrlStats[groupIds[i]].blockedReqGetXMisses_);                                  
        dbg->output(CALL_INFO,"     GetSEx:                                     %" PRIu64 "\n", 
                ctrlStats[groupIds[i]].newReqGetSExHits_ + ctrlStats[groupIds[i]].newReqGetSExMisses_ + 
                ctrlStats[groupIds[i]].blockedReqGetSExHits_ + ctrlStats[groupIds[i]].blockedReqGetSExMisses_);                                  
        dbg->output(CALL_INFO,"- Total misses:                                  %" PRIu64 "\n", totalMisses);
        // Report misses at the time a request was handled -> "blocked" indicates request was blocked by another pending request before being handled
        dbg->output(CALL_INFO,"     GetS, miss on arrival:                      %" PRIu64 "\n", ctrlStats[groupIds[i]].newReqGetSMisses_);
        dbg->output(CALL_INFO,"     GetS, miss after being blocked:             %" PRIu64 "\n", ctrlStats[groupIds[i]].blockedReqGetSMisses_);
        dbg->output(CALL_INFO,"     GetX, miss on arrival:                      %" PRIu64 "\n", ctrlStats[groupIds[i]].newReqGetXMisses_);
        dbg->output(CALL_INFO,"     GetX, miss after being blocked:             %" PRIu64 "\n", ctrlStats[groupIds[i]].blockedReqGetXMisses_);
        dbg->output(CALL_INFO,"     GetSEx, miss on arrival:                    %" PRIu64 "\n", ctrlStats[groupIds[i]].newReqGetSExMisses_);
        dbg->output(CALL_INFO,"     GetSEx, miss after being blocked:           %" PRIu64 "\n", ctrlStats[groupIds[i]].blockedReqGetSExMisses_);
        dbg->output(CALL_INFO,"- Total hits:                                    %" PRIu64 "\n", totalHits);
        dbg->output(CALL_INFO,"     GetS, hit on arrival:                       %" PRIu64 "\n", ctrlStats[groupIds[i]].newReqGetSHits_);
        dbg->output(CALL_INFO,"     GetS, hit after being blocked:              %" PRIu64 "\n", ctrlStats[groupIds[i]].blockedReqGetSHits_);
        dbg->output(CALL_INFO,"     GetX, hit on arrival:                       %" PRIu64 "\n", ctrlStats[groupIds[i]].newReqGetXHits_);
        dbg->output(CALL_INFO,"     GetX, hit after being blocked:              %" PRIu64 "\n", ctrlStats[groupIds[i]].blockedReqGetXHits_);
        dbg->output(CALL_INFO,"     GetSEx, hit on arrival:                     %" PRIu64 "\n", ctrlStats[groupIds[i]].newReqGetSExHits_);
        dbg->output(CALL_INFO,"     GetSEx, hit after being blocked:            %" PRIu64 "\n", ctrlStats[groupIds[i]].blockedReqGetSExHits_);
        dbg->output(CALL_INFO,"- Hit ratio:                                     %.3f%%\n", hitRatio);
        dbg->output(CALL_INFO,"- Miss ratio:                                    %.3f%%\n", 100 - hitRatio);
        dbg->output(CALL_INFO,"------------ Coherence transitions for misses -------------\n");
        dbg->output(CALL_INFO,"- GetS   I->S:                                   %" PRIu64 "\n", ctrlStats[groupIds[i]].GetS_IS);
        dbg->output(CALL_INFO,"- GetS   M(present at another cache):            %" PRIu64 "\n", ctrlStats[groupIds[i]].GetS_M);
        dbg->output(CALL_INFO,"- GetX   I->M:                                   %" PRIu64 "\n", ctrlStats[groupIds[i]].GetX_IM);
        dbg->output(CALL_INFO,"- GetX   M(present at another cache):            %" PRIu64 "\n", ctrlStats[groupIds[i]].GetX_M);
        dbg->output(CALL_INFO,"- GetSEx I->M:                                   %" PRIu64 "\n", ctrlStats[groupIds[i]].GetSE_IM);
        dbg->output(CALL_INFO,"- GetSEx M(present at another cache):            %" PRIu64 "\n", ctrlStats[groupIds[i]].GetSE_M);
        dbg->output(CALL_INFO,"------------ Replacements and evictions -------------------\n");
        dbg->output(CALL_INFO,"- PutM received:                                 %" PRIu64 "\n", stats_[groupIds[i]].PUTMReqsReceived_);
        dbg->output(CALL_INFO,"- PUTM sent due to [inv, evictions]:             [%" PRIu64 ", %" PRIu64 "]\n", stats_[groupIds[i]].InvalidatePUTMReqSent_, stats_[groupIds[i]].EvictionPUTSReqSent_);
        dbg->output(CALL_INFO,"- PUTE sent due to [inv, evictions]:             [%" PRIu64 ", %" PRIu64 "]\n", stats_[groupIds[i]].InvalidatePUTEReqSent_, stats_[groupIds[i]].EvictionPUTMReqSent_);
        dbg->output(CALL_INFO,"------------ Other stats ----------------------------------\n");
        dbg->output(CALL_INFO,"- Requests received (incl coherence traffic):    %" PRIu64 "\n", ctrlStats[groupIds[i]].TotalRequestsReceived_);
        dbg->output(CALL_INFO,"- Requests handled by MSHR (MSHR hits):          %" PRIu64 "\n", ctrlStats[groupIds[i]].TotalMSHRHits_);
        dbg->output(CALL_INFO,"- NACKs sent (MSHR Full, Down):                  %" PRIu64 "\n", stats_[groupIds[i]].NACKsSentDown_);
        dbg->output(CALL_INFO,"- NACKs sent (MSHR Full, Up):                    %" PRIu64 "\n", stats_[groupIds[i]].NACKsSentUp_);
        dbg->output(CALL_INFO,"------------ Latency stats --------------------------------\n");
        dbg->output(CALL_INFO,"- Avg Miss Latency (cyc):                        %" PRIu64 "\n", upgradeLatency);
        if (ctrlStats[groupIds[0]].GetS_IS > 0) 
            dbg->output(CALL_INFO,"- Latency GetS   I->S                            %" PRIu64 "\n", (lat_GetS_IS / ctrlStats[groupIds[0]].GetS_IS));
        if (ctrlStats[groupIds[0]].GetS_M > 0) 
            dbg->output(CALL_INFO,"- Latency GetS   M                               %" PRIu64 "\n", (lat_GetS_M / ctrlStats[groupIds[0]].GetS_M));
        if (ctrlStats[groupIds[0]].GetX_IM > 0)
            dbg->output(CALL_INFO,"- Latency GetX   I->M                            %" PRIu64 "\n", (lat_GetX_IM / ctrlStats[groupIds[0]].GetX_IM));
        if (ctrlStats[groupIds[0]].GetX_M > 0) 
            dbg->output(CALL_INFO,"- Latency GetX   M                               %" PRIu64 "\n", (lat_GetX_M / ctrlStats[groupIds[0]].GetX_M));
        if (ctrlStats[groupIds[0]].GetSE_IM > 0)
            dbg->output(CALL_INFO,"- Latency GetSEx I->M                            %" PRIu64 "\n", (lat_GetSEx_IM / ctrlStats[groupIds[0]].GetSE_IM));
        if (ctrlStats[groupIds[0]].GetSE_M > 0)
            dbg->output(CALL_INFO,"- Latency GetSEx M                               %" PRIu64 "\n", (lat_GetSEx_M / ctrlStats[groupIds[0]].GetSE_M));
    }
    dbg->output(CALL_INFO,"------------ State and event stats ---------------------------\n");
    for (int i = 0; i < LAST_CMD; i++) {
        for (int j = 0; j < LAST_CMD; j++) {
            if (stateStats_[i][j] == 0) continue;
            dbg->output(CALL_INFO,"%s, %s:        %" PRIu64 "\n", CommandString[i], StateString[j], stateStats_[i][j]);
        }
    }    
}

