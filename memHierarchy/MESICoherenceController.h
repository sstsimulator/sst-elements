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

#ifndef MESICOHERENCECONTROLLER_H
#define MESICOHERENCECONTROLLER_H

#include <iostream>
#include "coherenceControllers.h"


namespace SST { namespace MemHierarchy {

class MESIController : public CoherencyController{
public:
    /** Constructor for MESIController. Note that MESIController handles both MESI & MSI protocols */
    MESIController(const Cache* cache, string ownerName, Output* dbg, vector<Link*>* parentLinks, vector<Link*>* childLinks, CacheListener* listener, 
            unsigned int lineSize, uint64 accessLatency, uint64 tagLatency, uint64 mshrLatency, bool LLC, bool LL, MSHR * mshr, bool protocol, bool inclusive, bool wbClean,
            MemNIC* bottomNetworkLink, MemNIC* topNetworkLink, bool groupStats, vector<int> statGroupIds, bool debugAll, Addr debugAddr) :
                 CoherencyController(cache, dbg, ownerName, lineSize, accessLatency, tagLatency, mshrLatency, LLC, LL, parentLinks, childLinks, bottomNetworkLink, topNetworkLink, 
                         listener, mshr, wbClean, groupStats, statGroupIds, debugAll, debugAddr) {
        d_->debug(_INFO_,"--------------------------- Initializing [MESI Controller] ... \n\n");
        protocol_           = protocol;         // 1 for MESI, 0 for MSI
        inclusive_          = inclusive;
        
        evictionRequiredInv_ = 0;
    }

    ~MESIController() {}
    
    /** Init funciton */
    void init(const char* name){}


/*----------------------------------------------------------------------------------------------------------------------
 *  Public functions form external interface to the coherence controller
 *---------------------------------------------------------------------------------------------------------------------*/  

/* Event handlers */
    /** Send cacheline data to the lower level caches */
    CacheAction handleEviction(CacheLine* wbCacheLine, uint32_t groupId, string origRqstr, bool ignoredParam=false);

    /** Process cache request:  GetX, GetS, GetSEx */
    CacheAction handleRequest(MemEvent* event, CacheLine* cacheLine, bool replay);
    
    /** Process replacement request - PutS, PutE, PutM */
    CacheAction handleReplacement(MemEvent* event, CacheLine* cacheLine, MemEvent * reqEvent, bool replay);
    
    /** Process invalidation requests - Inv, FetchInv, FetchInvX */
    CacheAction handleInvalidationRequest(MemEvent *event, CacheLine* cacheLine, bool replay);

    /** Process responses - GetSResp, GetXResp, FetchResp */
    CacheAction handleResponse(MemEvent* responseEvent, CacheLine* cacheLine, MemEvent* origRequest);

/* Message send */
    /** Forward a message up, used for non-inclusive caches */
    void forwardMessageUp(MemEvent * event);

/* Miscellaneous */
    /** Determine in advance if a request will miss (and what kind of miss). Used for stats */
    int isCoherenceMiss(MemEvent* event, CacheLine* cacheLine);
   
    /** Print statistics at the end of simulation */
    void printStats(int statsFile, vector<int> statGroupIds, map<int, CtrlStats> ctrlStats, uint64_t updgradeLatency, uint64_t lat_GetS_IS, uint64_t lat_GetS_M, uint64_t lat_GetX_IM, uint64_t lat_GetX_SM, uint64_t lat_GetX_M, uint64_t lat_GetSEx_IM, uint64_t lat_GetSEx_SM, uint64_t lat_GetSEx_M);
    void printStatsForMacSim(int _statsFile, vector<int> statGroupIds, map<int, CtrlStats> _ctrlStats, uint64_t _updgradeLatency, uint64_t lat_GetS_IS, uint64_t lat_GetS_M, uint64_t lat_GetX_IM, uint64_t lat_GetX_SM, uint64_t lat_GetX_M, uint64_t lat_GetSEx_IM, uint64_t lat_GetSEx_SM, uint64_t lat_GetSEx_M);
    
private:
/* Private data members */
    bool                protocol_;  // True for MESI, false for MSI
    bool                inclusive_;

    uint64_t            evictionRequiredInv_;
    
/* Private event handlers */
    /** Handle GetX request. Request upgrade if needed */
    CacheAction handleGetXRequest(MemEvent* event, CacheLine* cacheLine, bool replay);

    /** Handle GetS request. Request block if needed */
    CacheAction handleGetSRequest(MemEvent* event, CacheLine* cacheLine, bool replay);
    
    /** Handle PutM request. Write data to cache line.  Update E->M state if necessary */
    CacheAction handlePutMRequest(MemEvent* event, CacheLine* cacheLine, MemEvent * reqEvent);
    
    /** Handle PutS requesst. Update sharer state */ 
    CacheAction handlePutSRequest(MemEvent* event, CacheLine* cacheLine, MemEvent * reqEvent);
    
    /** Handle Inv */
    CacheAction handleInv(MemEvent * event, CacheLine * cacheLine, bool replay);
    
    /** Handle Fetch */
    CacheAction handleFetch(MemEvent * event, CacheLine * cacheLine, bool replay);
    
    /** Handle FetchInv */
    CacheAction handleFetchInv(MemEvent * event, CacheLine * cacheLine, bool replay);
    
    /** Handle FetchInvX */
    CacheAction handleFetchInvX(MemEvent * event, CacheLine * cacheLine, bool replay);

    /** Process GetSResp/GetXResp.  Update the cache line */
    CacheAction handleDataResponse(MemEvent* responseEvent, CacheLine * cacheLine, MemEvent * reqEvent);
    
    /** Handle FetchResp */
    CacheAction handleFetchResp(MemEvent * responseEvent, CacheLine* cacheLine, MemEvent * reqEvent);
    
    /** Handle Ack */
    CacheAction handleAckInv(MemEvent * responseEvent, CacheLine* cacheLine, MemEvent * reqEvent);

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
    void sendAckInv(Addr baseAddr, string origRqstr);

    /** Fetch data from owner and invalidate their copy of the line */
    void sendFetchInv(CacheLine * cacheLine, string rqstr, bool replay);
    
    /** Fetch data from owner and downgrade owner to sharer */
    void sendFetchInvX(CacheLine * cacheLine, string rqstr, bool replay);

    /** Invalidate all sharers of a block. Used for invalidations and evictions */
    void invalidateAllSharers(CacheLine * cacheLine, string rqstr, bool replay);
    
    /** Invalidate all sharers of a block except the requestor (rqstr). Used for upgrade requests. */
    bool invalidateSharersExceptRequestor(CacheLine * cacheLine, string rqstr, string origRqstr, bool replay);


/* Helper methods */
   
    void printData(vector<uint8_t> * data, bool set);

/* Stat counting & callbacks to prefetcher */
    void inc_GETXMissSM(MemEvent* event){
        if(!event->statsUpdated()){
            if (event->getCmd() == GetX)   statGetXMissSM->addData(1);
            else statGetSExMissSM->addData(1);
            stats_[0].GETXMissSM_++;
            if(groupStats_) stats_[getGroupId()].GETXMissSM_++;
        }

        if(!(event->isPrefetch())) {
		CacheListenerNotification notify(event->getBaseAddr(),
			event->getVirtualAddress(), event->getInstructionPointer(),
			event->getSize(), WRITE, MISS);

        	listener_->notifyAccess(notify);
	}

        event->setStatsUpdated(true);
    }


    void inc_GETXMissIM(MemEvent* event){
        if(!event->statsUpdated()){
            if (event->getCmd() == GetX)   statGetXMissIM->addData(1);
            else statGetSExMissIM->addData(1);
            stats_[0].GETXMissIM_++;
            if(groupStats_) stats_[getGroupId()].GETXMissIM_++;
        }

        if(!(event->isPrefetch())) {
		CacheListenerNotification notify(event->getBaseAddr(),
                        event->getVirtualAddress(), event->getInstructionPointer(),
                        event->getSize(), WRITE, MISS);

                listener_->notifyAccess(notify);
	}

        event->setStatsUpdated(true);
    }


    void inc_GETSHit(MemEvent* event){
        if(!event->statsUpdated()){
            if(!event->inMSHR()){
                stats_[0].GETSHit_++;
                if(groupStats_) stats_[getGroupId()].GETSHit_++;
            }
        }

        if(!(event->isPrefetch())) {
		CacheListenerNotification notify(event->getBaseAddr(),
                        event->getVirtualAddress(), event->getInstructionPointer(),
                        event->getSize(), READ, HIT);

                listener_->notifyAccess(notify);
	}

        event->setStatsUpdated(true);
    }


    void inc_GETXHit(MemEvent* event){
        if(!event->statsUpdated()){
            if(!event->inMSHR()){
                stats_[0].GETXHit_++;
                if(groupStats_) stats_[getGroupId()].GETXHit_++;
            }
        }

        if(!(event->isPrefetch())) {
		CacheListenerNotification notify(event->getBaseAddr(),
                        event->getVirtualAddress(), event->getInstructionPointer(),
                        event->getSize(), WRITE, HIT);

                listener_->notifyAccess(notify);
	}

        event->setStatsUpdated(true);
    }


    void inc_GETSMissIS(MemEvent* event){
        if(!event->statsUpdated()){
            statGetSMissIS->addData(1);
            stats_[0].GETSMissIS_++;
            if(groupStats_) stats_[getGroupId()].GETSMissIS_++;
        }

        if(!(event->isPrefetch())) {
		CacheListenerNotification notify(event->getBaseAddr(),
                        event->getVirtualAddress(), event->getInstructionPointer(),
                        event->getSize(), READ, MISS);

                listener_->notifyAccess(notify);
	}

        event->setStatsUpdated(true);
    }

    void inc_GetSExReqsReceived(bool replay){
        if(!replay) stats_[0].GetSExReqsReceived_++;
        if(groupStats_) stats_[getGroupId()].GetSExReqsReceived_++;
    }


    void inc_PUTSReqsReceived(){
        stats_[0].PUTSReqsReceived_++;
        if(groupStats_) stats_[getGroupId()].PUTSReqsReceived_++;
    }

    void inc_PUTMReqsReceived(){
        stats_[0].PUTMReqsReceived_++;
        if(groupStats_) stats_[getGroupId()].PUTMReqsReceived_++;
    }

    void inc_PUTEReqsReceived(){
        stats_[0].PUTEReqsReceived_++;
        if(groupStats_) stats_[getGroupId()].PUTEReqsReceived_++;
    }

    void inc_InvalidatePUTSReqSent(){
        stats_[0].InvalidatePUTSReqSent_++;
        if(groupStats_) stats_[getGroupId()].InvalidatePUTSReqSent_++;
    }

     void inc_EvictionPUTSReqSent(){
        stats_[0].EvictionPUTSReqSent_++;
        if(groupStats_) stats_[getGroupId()].EvictionPUTSReqSent_++;
    }

    void inc_EvictionPUTMReqSent(){
        stats_[0].EvictionPUTMReqSent_++;
        if(groupStats_) stats_[getGroupId()].EvictionPUTMReqSent_++;
    }

    void inc_EvictionPUTEReqSent(){
        stats_[0].EvictionPUTEReqSent_++;
        if(groupStats_) stats_[getGroupId()].EvictionPUTEReqSent_++;
    }

    void inc_FetchInvReqSent(){
        stats_[0].FetchInvReqSent_++;
        if(groupStats_) stats_[getGroupId()].FetchInvReqSent_++;
    }

    void inc_FetchInvXReqSent(){
        stats_[0].FetchInvXReqSent_++;
        if(groupStats_) stats_[getGroupId()].FetchInvXReqSent_++;
    }

};


}}


#endif

