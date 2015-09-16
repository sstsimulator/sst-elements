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

#ifndef L1INCOHERENTCONTROLLER_H
#define L1INCOHERENTCONTROLLER_H

#include <iostream>
#include "coherenceControllers.h"


namespace SST { namespace MemHierarchy {

class L1IncoherentController : public CoherencyController {
public:
    /** Constructor for L1IncoherentController */
    L1IncoherentController(const Cache* _cache, string _ownerName, Output* _dbg, vector<Link*>* parentLinks, vector<Link*>* childLinks, CacheListener* listener, 
            unsigned int _lineSize, uint64 _accessLatency, uint64 _tagLatency, uint64 _mshrLatency, bool _LLC, bool _LL, MSHR * mshr, bool wbClean,
            MemNIC* bottomNetworkLink, MemNIC* topNetworkLink, bool groupStats, vector<int> statGroupIds, bool debugAll, Addr debugAddr) :
                 CoherencyController(_cache, _dbg, _ownerName, _lineSize, _accessLatency, _tagLatency, _mshrLatency, _LLC, _LL, parentLinks, childLinks, 
                         bottomNetworkLink, topNetworkLink, listener, mshr, wbClean, groupStats, statGroupIds, debugAll, debugAddr) {
        d_->debug(_INFO_,"--------------------------- Initializing [L1Controller] ... \n\n");
        
    }

    ~L1IncoherentController() {}
    
    
    /** Init funciton */
    void init(const char* name){}
    
    /** Used to determine in advance if an event will be a miss (and which kind of miss)
     * Used for statistics only
     */
    int isCoherenceMiss(MemEvent* event, CacheLine* cacheLine);
    
    /* Event handlers called by cache controller */
    /** Send cache line data to the lower level caches */
    CacheAction handleEviction(CacheLine* wbCacheLine, uint32_t groupId, string origRqstr, bool ignoredParam=false);

    /** Process new cache request:  GetX, GetS, GetSEx */
    CacheAction handleRequest(MemEvent* event, CacheLine* cacheLine, bool replay);
   
    /** Process replacement - implemented for compatibility with CoherencyController but L1s do not receive replacements */
    CacheAction handleReplacement(MemEvent* event, CacheLine* cacheLine, MemEvent * reqEvent, bool replay);
    
    /** Process Inv */
    CacheAction handleInvalidationRequest(MemEvent *event, CacheLine* cacheLine, bool replay);

    /** Process responses */
    CacheAction handleResponse(MemEvent* responseEvent, CacheLine* cacheLine, MemEvent* origRequest);

    /* Methods for sending events, called by cache controller */
    /** Send response up (to processor) */
    void sendResponseUp(MemEvent * event, State grantedState, vector<uint8_t>* data, bool replay, bool atomic = false);

    void printData(vector<uint8_t> * data, bool set);

    /** Print statistics at the end of simulation */
    void printStats(int _statsFile, vector<int> statGroupIds, map<int, CtrlStats> _ctrlStats, uint64_t _updgradeLatency, uint64_t lat_GetS_IS, uint64_t lat_GetS_M, uint64_t lat_GetX_IM, uint64_t lat_GetX_SM, uint64_t lat_GetX_M, uint64_t lat_GetSEx_IM, uint64_t lat_GetSEx_SM, uint64_t lat_GetSEx_M);
    
private:
    /* Private event handlers */
    /** Handle GetX request. Request upgrade if needed */
    CacheAction handleGetXRequest(MemEvent* event, CacheLine* cacheLine, bool replay);
    
    /** Handle GetS request. Request block if needed */
    CacheAction handleGetSRequest(MemEvent* event, CacheLine* cacheLine, bool replay);
    
    /** Handle data response - GetSResp or GetXResp */
    void handleDataResponse(MemEvent* responseEvent, CacheLine * cacheLine, MemEvent * reqEvent);

    
    /* Methods for sending events */
    /** Send writeback request to lower level caches */
    void sendWriteback(Command cmd, CacheLine* cacheLine, string origRqstr);

    /* Helper methods */

    /* Stat counting and callbacks to prefetcher */
    void inc_GETXMissIM(MemEvent* event){
        if(!event->statsUpdated()){
            if (event->getCmd() == GetX)   statGetXMissIM->addData(1);
            else statGetSExMissIM->addData(1);
            stats_[0].GETXMissIM_++;
            if(groupStats_) stats_[getGroupId()].GETXMissIM_++;
        }

        if(!(event->isPrefetch())) {
		// probably should be 'if (prefetching is on and not a prefetch miss)'
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

