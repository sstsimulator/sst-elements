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

#ifndef INCOHERENTCONTROLLERS_H
#define INCOHERENTCONTROLLERS_H

#include <iostream>
#include "coherenceControllers.h"


namespace SST { namespace MemHierarchy {

class IncoherentController : public CoherencyController{
public:
    /** Constructor for IncoherentController. */
    IncoherentController(const Cache* cache, string ownerName, Output* dbg, vector<Link*>* parentLinks, vector<Link*>* childLinks, CacheListener* listener, 
            unsigned int lineSize, uint64 accessLatency, uint64 tagLatency, uint64 mshrLatency, bool LLC, bool LL, MSHR * mshr, bool inclusive, bool wbClean,
            MemNIC* bottomNetworkLink, MemNIC* topNetworkLink, bool groupStats, vector<int> statGroupIds, bool debugAll, Addr debugAddr) :
                 CoherencyController(cache, dbg, ownerName, lineSize, accessLatency, tagLatency, mshrLatency, LLC, LL, parentLinks, childLinks, 
                         bottomNetworkLink, topNetworkLink, listener, mshr, wbClean, groupStats, statGroupIds, debugAll, debugAddr) {
        d_->debug(_INFO_,"--------------------------- Initializing [Incoherent Controller] ... \n\n");
        inclusive_           = inclusive;

    }

    ~IncoherentController() {}
    
    /** Init function */
    void init(const char* name) {}
    
 
/*----------------------------------------------------------------------------------------------------------------------
 *  Public functions form external interface to the coherence controller
 *---------------------------------------------------------------------------------------------------------------------*/    

/* Event handlers */
    /* Public event handlers called by cache controller */
    /** Send cache line data to the lower level caches */
    CacheAction handleEviction(CacheLine* _wbCacheLine, uint32_t groupId, string _origRqstr, bool ignoredParameter=false);

    /** Process cache request:  GetX, GetS, GetSEx */
    CacheAction handleRequest(MemEvent* event, CacheLine* cacheLine, bool replay);
    
    /** Process replacement request - PutS, PutE, PutM */
    CacheAction handleReplacement(MemEvent* event, CacheLine* cacheLine, MemEvent * reqEvent, bool replay);
    
    /** Process invalidation requests - Inv, FetchInv, FetchInvX */
    CacheAction handleInvalidationRequest(MemEvent *event, CacheLine* cacheLine, bool replay);

    /** Process responses - GetSResp, GetXResp, FetchResp */
    CacheAction handleResponse(MemEvent* _responseEvent, CacheLine* cacheLine, MemEvent* _origRequest);

/* Miscellaneous */

    /** Determine in advance if a request will miss (and what kind of miss). Used for stats */
    int isCoherenceMiss(MemEvent* event, CacheLine* cacheLine);
   
    /** Print statistics at the end of simulation */
    void printStats(int statsFile, vector<int> statGroupIds, map<int, CtrlStats> _ctrlStats, uint64_t _updgradeLatency, uint64_t lat_GetS_IS, uint64_t lat_GetS_M, uint64_t lat_GetX_IM, uint64_t lat_GetX_SM, uint64_t lat_GetX_M, uint64_t lat_GetSEx_IM, uint64_t lat_GetSEx_SM, uint64_t lat_GetSEx_M);
    
private:
/* Private data members */
    bool                inclusive_;
    
/* Private event handlers */
    /** Handle GetX request. Request upgrade if needed */
    CacheAction handleGetXRequest(MemEvent* event, CacheLine* _cacheLine, bool replay);

    /** Handle GetS request. Request block if needed */
    CacheAction handleGetSRequest(MemEvent* event, CacheLine* cacheLine, bool replay);
    
    /** Handle PutM request. Write data to cache line.  Update E->M state if necessary */
    void handlePutMRequest(MemEvent* event, CacheLine* cacheLine);
    
    /** Process GetSResp/GetXResp.  Update the cache line */
    CacheAction handleDataResponse(MemEvent* responseEvent, CacheLine * cacheLine, MemEvent * reqEvent);
    
/* Private methods for sending events */
    /** Send writeback request to lower level caches */
    void sendWriteback(Command cmd, CacheLine* cacheLine, string origRqstr);
    
/* Helper methods */
   
    void printData(vector<uint8_t> * data, bool set);

/* Stat counting & callbacks to prefetcher */
    void inc_GETXMissIM(MemEvent* event){
        if(!event->statsUpdated()){
            if (event->getCmd() == GetX)   statGetXMissIM->addData(1);
            else statGetSExMissIM->addData(1);
            stats_[0].GETXMissIM_++;
            if(groupStats_) stats_[getGroupId()].GETXMissIM_++;
        }

        if(!(event->isPrefetch())) {
		// probably should be 'if (prefetching is on and not a prefetch miss)'
		CacheListenerNotification notify(event->getBaseAddr(), event->getVirtualAddress(), 
                        event->getInstructionPointer(), event->getSize(), WRITE, MISS);
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
		CacheListenerNotification notify(event->getBaseAddr(), event->getVirtualAddress(), 
                        event->getInstructionPointer(), event->getSize(), READ, HIT);
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
		CacheListenerNotification notify(event->getBaseAddr(), event->getVirtualAddress(), 
                        event->getInstructionPointer(), event->getSize(), WRITE, HIT);
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
		CacheListenerNotification notify(event->getBaseAddr(), event->getVirtualAddress(), 
                        event->getInstructionPointer(), event->getSize(), READ, MISS);
                listener_->notifyAccess(notify);
	}
        event->setStatsUpdated(true);
    }

    void inc_GetSExReqsReceived(bool replay){
        if(!replay) stats_[0].GetSExReqsReceived_++;
        if(groupStats_) stats_[getGroupId()].GetSExReqsReceived_++;
    }


    void inc_PUTMReqsReceived(){
        stats_[0].PUTMReqsReceived_++;
        if(groupStats_) stats_[getGroupId()].PUTMReqsReceived_++;
    }

    void inc_PUTEReqsReceived(){
        stats_[0].PUTEReqsReceived_++;
        if(groupStats_) stats_[getGroupId()].PUTEReqsReceived_++;
    }

    void inc_EvictionPUTMReqSent(){
        stats_[0].EvictionPUTMReqSent_++;
        if(groupStats_) stats_[getGroupId()].EvictionPUTMReqSent_++;
    }

    void inc_EvictionPUTEReqSent(){
        stats_[0].EvictionPUTEReqSent_++;
        if(groupStats_) stats_[getGroupId()].EvictionPUTEReqSent_++;
    }
};


}}


#endif

