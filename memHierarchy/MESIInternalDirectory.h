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


#ifndef MESIINTERNALDIRCONTROLLER_H
#define MESIINTERNALDIRCONTROLLER_H

#include <iostream>
#include "coherenceControllers.h"


namespace SST { namespace MemHierarchy {

class MESIInternalDirectory : public CoherencyController {
public:
    /** Constructor for MESIInternalDirectory. */
    MESIInternalDirectory(const Cache* directory, string ownerName, Output* dbg, vector<Link*>* parentLinks, vector<Link*>* childLinks, CacheListener* listener, 
            unsigned int lineSize, uint64 accessLatency, uint64 tagLatency, uint64 mshrLatency, bool LLC, bool LL, MSHR * mshr, bool protocol,
            bool wbClean, MemNIC* bottomNetworkLink, MemNIC* topNetworkLink, bool groupStats, vector<int> statGroupIds, bool debugAll, Addr debugAddr) :
                 CoherencyController(directory, dbg, ownerName, lineSize, accessLatency, tagLatency, mshrLatency, LLC, LL, parentLinks, childLinks, 
                         bottomNetworkLink, topNetworkLink, listener, mshr, wbClean, groupStats, statGroupIds, debugAll, debugAddr) {
        d_->debug(_INFO_,"--------------------------- Initializing [MESI + Directory Controller] ... \n\n");
        protocol_           = protocol;

        evictionRequiredInv_ = 0;
    }

    ~MESIInternalDirectory() {}
    
    
    /** Init funciton */
    void init(const char* name){}
 
/*----------------------------------------------------------------------------------------------------------------------
 *  Public functions form external interface to the coherence controller
 *---------------------------------------------------------------------------------------------------------------------*/  

/* Event handlers */
    /** Send cache line data to the lower level caches */
    CacheAction handleEviction(CacheLine* replacementLine, uint32_t groupId, string origRqstr, bool fromDataCache);

    /** Process cache request:  GetX, GetS, GetSEx */
    CacheAction handleRequest(MemEvent* event, CacheLine* dirLine, bool replay);
    
    /** Process replacement request - PutS, PutE, PutM. May also resolve an outstanding/racing request event */
    CacheAction handleReplacement(MemEvent* event, CacheLine* dirLine, MemEvent * reqEvent, bool replay);
    
    /** Process invalidation requests - Inv, FetchInv, FetchInvX */
    CacheAction handleInvalidationRequest(MemEvent *event, CacheLine* dirLine, bool replay);

    /** Process responses - GetSResp, GetXResp, FetchResp */
    CacheAction handleResponse(MemEvent* responseEvent, CacheLine* dirLine, MemEvent* origRequest);

/* Miscellaneous */
    
    /** Determine in advance if a request will miss (and what kind of miss). Used for stats */
    int isCoherenceMiss(MemEvent* event, CacheLine* dirLine);

    /** Print statistics at the end of simulation */
    void printStats(int statsFile, vector<int> statGroupIds, map<int, CtrlStats> ctrlStats, uint64_t updgradeLatency, uint64_t lat_GetS_IS, uint64_t lat_GetS_M, uint64_t lat_GetX_IM, uint64_t lat_GetX_SM, uint64_t lat_GetX_M, uint64_t lat_GetSEx_IM, uint64_t lat_GetSEx_SM, uint64_t lat_GetSEx_M);
    
private:
/* Private data members */
    bool                protocol_;  // True for MESI, false for MSI

    uint64_t            evictionRequiredInv_;
    
/* Private event handlers */
    /** Handle GetX request. Request upgrade if needed */
    CacheAction handleGetXRequest(MemEvent* event, CacheLine* dirLine, bool replay);

    /** Handle GetS request. Request block if needed */
    CacheAction handleGetSRequest(MemEvent* event, CacheLine* dirLine, bool replay);
    
    /** Handle PutS request. Possibly complete a waiting request if it raced with the PutS */
    CacheAction handlePutSRequest(MemEvent* event, CacheLine * dirLine, MemEvent * origReq);
    
    /** Handle PutM request. Possibly complete a waiting request if it raced with the PutM */
    CacheAction handlePutMRequest(MemEvent* event, CacheLine * dirLine, MemEvent * origReq);

    /** Handle Inv */
    CacheAction handleInv(MemEvent * event, CacheLine * dirLine, bool replay, MemEvent * collisionEvent);
    
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
    void sendAckInv(Addr baseAddr, string origRqstr);

    /** Fetch data from owner and invalidate their copy of the line */
    void sendFetchInv(CacheLine * dirLine, string rqstr, bool replay);
    
    /** Fetch data from owner and downgrade owner to sharer */
    void sendFetchInvX(CacheLine * dirLine, string rqstr, bool replay);

    /** Fetch data from sharer */
    void sendFetch(CacheLine * dirLine, string rqstr, bool replay);

    /** Invalidate all sharers of a block. Used for invalidations and evictions */
    void invalidateAllSharers(CacheLine * dirLine, string rqstr, bool replay);
    
    /** Invalidate all sharers of a block and fetch block from one of them. Used for invalidations and evictions */
    void invalidateAllSharersAndFetch(CacheLine * dirLine, string rqstr, bool replay);
    
    /** Invalidate all sharers of a block except the requestor (rqstr). If requestor is not a sharer, may fetch data from a sharer. Used for upgrade requests. */
    bool invalidateSharersExceptRequestor(CacheLine * dirLine, string rqstr, string origRqstr, bool replay, bool checkFetch);


/* Miscellaneous */
   
    void printData(vector<uint8_t> * data, bool set);


/* Stat counting & callbacks to prefetcher */
    void inc_GETXMissSM(MemEvent* event) {
        if (!event->statsUpdated()) {
            if (event->getCmd() == GetX)   statGetXMissSM->addData(1);
            else statGetSExMissSM->addData(1);
            stats_[0].GETXMissSM_++;
            if (groupStats_) stats_[getGroupId()].GETXMissSM_++;
        }

        if (!(event->isPrefetch())) {
		CacheListenerNotification notify(event->getBaseAddr(),
			event->getVirtualAddress(), event->getInstructionPointer(),
			event->getSize(), WRITE, MISS);

        	listener_->notifyAccess(notify);
	}

        event->setStatsUpdated(true);
    }


    void inc_GETXMissIM(MemEvent* event) {
        if (!event->statsUpdated()) {
            if (event->getCmd() == GetX)   statGetXMissIM->addData(1);
            else statGetSExMissIM->addData(1);
            stats_[0].GETXMissIM_++;
            if (groupStats_) stats_[getGroupId()].GETXMissIM_++;
        }

        if (!(event->isPrefetch())) {
		// probably should be 'if (prefetching is on and not a prefetch miss)'
		CacheListenerNotification notify(event->getBaseAddr(),
                        event->getVirtualAddress(), event->getInstructionPointer(),
                        event->getSize(), WRITE, MISS);

                listener_->notifyAccess(notify);
	}

        event->setStatsUpdated(true);
    }


    void inc_GETSHit(MemEvent* event) {
        if (!event->statsUpdated()) {
            if (!event->inMSHR()) {
                stats_[0].GETSHit_++;
                if (groupStats_) stats_[getGroupId()].GETSHit_++;
            }
        }

        if (!(event->isPrefetch())) {
		CacheListenerNotification notify(event->getBaseAddr(),
                        event->getVirtualAddress(), event->getInstructionPointer(),
                        event->getSize(), READ, HIT);

                listener_->notifyAccess(notify);
	}

        event->setStatsUpdated(true);
    }


    void inc_GETXHit(MemEvent* event) {
        if (!event->statsUpdated()) {
            if (!event->inMSHR()) {
                stats_[0].GETXHit_++;
                if (groupStats_) stats_[getGroupId()].GETXHit_++;
            }
        }

        if (!(event->isPrefetch())) {
		CacheListenerNotification notify(event->getBaseAddr(),
                        event->getVirtualAddress(), event->getInstructionPointer(),
                        event->getSize(), WRITE, HIT);

                listener_->notifyAccess(notify);
	}

        event->setStatsUpdated(true);
    }


    void inc_GETSMissIS(MemEvent* event) {
        if (!event->statsUpdated()) {
            statGetSMissIS->addData(1);
            stats_[0].GETSMissIS_++;
            if (groupStats_) stats_[getGroupId()].GETSMissIS_++;
        }

        if (!(event->isPrefetch())) {
		CacheListenerNotification notify(event->getBaseAddr(),
                        event->getVirtualAddress(), event->getInstructionPointer(),
                        event->getSize(), READ, MISS);

                listener_->notifyAccess(notify);
	}

        event->setStatsUpdated(true);
    }

    void inc_GetSExReqsReceived(bool replay) {
        if (!replay) stats_[0].GetSExReqsReceived_++;
        if (groupStats_) stats_[getGroupId()].GetSExReqsReceived_++;
    }


    void inc_PUTSReqsReceived() {
        stats_[0].PUTSReqsReceived_++;
        if (groupStats_) stats_[getGroupId()].PUTSReqsReceived_++;
    }

    void inc_PUTMReqsReceived() {
        stats_[0].PUTMReqsReceived_++;
        if (groupStats_) stats_[getGroupId()].PUTMReqsReceived_++;
    }

    void inc_PUTEReqsReceived() {
        stats_[0].PUTEReqsReceived_++;
        if (groupStats_) stats_[getGroupId()].PUTEReqsReceived_++;
    }

    void inc_InvalidatePUTSReqSent() {
        stats_[0].InvalidatePUTSReqSent_++;
        if (groupStats_) stats_[getGroupId()].InvalidatePUTSReqSent_++;
    }

    void inc_EvictionPUTSReqSent() {
        stats_[0].EvictionPUTSReqSent_++;
        if (groupStats_) stats_[getGroupId()].EvictionPUTSReqSent_++;
    }

    void inc_EvictionPUTMReqSent() { 
        stats_[0].EvictionPUTMReqSent_++;
        if (groupStats_) stats_[getGroupId()].EvictionPUTMReqSent_++;
    }


    void inc_EvictionPUTEReqSent() {
        stats_[0].EvictionPUTEReqSent_++;
        if (groupStats_) stats_[getGroupId()].EvictionPUTEReqSent_++;
    }

    void inc_FetchInvReqSent() {
        stats_[0].FetchInvReqSent_++;
        if (groupStats_) stats_[getGroupId()].FetchInvReqSent_++;
    }


    void inc_FetchInvXReqSent() {
        stats_[0].FetchInvXReqSent_++;
        if (groupStats_) stats_[getGroupId()].FetchInvXReqSent_++;
    }

};


}}


#endif

