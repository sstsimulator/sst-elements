// Copyright 2009-2014 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2014, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

/*
 * File:   MESIBottomCoherenceController.h
 * Author: Caesar De la Paz III
 * Email:  caesar.sst@gmail.com
 */

#ifndef MESIBOTTOMCOHERENCECONTROLLERS_H
#define MESIBOTTOMCOHERENCECONTROLLERS_H

#include <iostream>
#include "coherenceControllers.h"


namespace SST { namespace MemHierarchy {

class MESIBottomCC : public CoherencyController{
public:
    /** Constructor for MESIBottomCC. */
    MESIBottomCC(const Cache* _cache, string _ownerName, Output* _dbg,
                 vector<Link*>* _parentLinks, CacheListener* _listener, unsigned int _lineSize,
                 uint64 _accessLatency, uint64 _mshrLatency, bool _L1, MemNIC* _directoryLink, bool _groupStats, vector<int> _statGroupIds) :
                 CoherencyController(_cache, _dbg, _lineSize, _accessLatency, _mshrLatency), lowNetPorts_(_parentLinks),
                 listener_(_listener), ownerName_(_ownerName) {
        d_->debug(_INFO_,"--------------------------- Initializing [BottomCC] ... \n\n");
        L1_             = _L1;
        directoryLink_  = _directoryLink;
        groupStats_     = _groupStats;
        statGroupIds_   = _statGroupIds;
        
        if(groupStats_){
            for(unsigned int i = 0; i < statGroupIds_.size(); i++)
                stats_[statGroupIds_[i]].initialize();
        }
    }
    
    
    /** Init funciton */
    void init(const char* name){}
    
    /** Send cache line data to the lower level caches */
    virtual void handleEviction(CacheLine* _wbCacheLine, uint32_t _groupId);

    /** Process new cache request:  GetX, GetS, GetSEx, PutM, PutS, PutX */
    virtual void handleRequest(MemEvent* event, CacheLine* cacheLine, Command cmd);
    
    /** Process Inv/InvX request.  -----Arguably the most important function within MemHierarchy.-----
       Most errors come from deadlocks due to invalidation/evicitons/upgrades all happening simultaneously
       Special care needs to be taken when a invalidate is received. */
    virtual void handleInvalidate(MemEvent *event, CacheLine* cacheLine, Command cmd);

    /** Process GetSResp/GetXResp.  Update the cache line with the 
        provided/given state and payload */
    virtual void handleResponse(MemEvent* _responseEvent, CacheLine* cacheLine, MemEvent* _origRequest);
    
    /** Handle request from Directory controller to invalidate, and 'fetch' or provide 
        the up-to-date cache line to the directory controller. */
    virtual void handleFetchInvalidate(MemEvent* _event, CacheLine* _cacheLine, int _parentId, bool _mshrHit);

    /** Function serves three purposes:  1) check if we need to to upgrade the cache line
        and 2) change to transition state if upgrade needed, and 3) forward request to
        lower level cache */
    bool isUpgradeToModifiedNeeded(MemEvent* _event, CacheLine* _cacheLine);

    /** Determine if can process an invalidate request.  Prevents possible deadlocks */
    bool canInvalidateRequestProceed(MemEvent* _event, CacheLine* _cacheLine, bool _sendAcks);

    /** Handle GetX request.  Cache line is already in the correct state
        and/or has been upgraded */
    void handleGetXRequest(MemEvent* _event, CacheLine* cacheLine);

    /** Handle GetS request.  Cache line is already in the correct state
        and/or has been upgraded */
    void handleGetSRequest(MemEvent* event, CacheLine* cacheLine);
    
    /** Handle PutM request.  Write data to cache line.  Update E->M state if necessary */
    void handlePutMRequest(MemEvent* event, CacheLine* cacheLine);
    
    /** Handle PutM request.  Same as handlePutMRequest, just updates different stats */
    void handlePutXRequest(MemEvent* _event, CacheLine* _cacheLine);
    
    /** Does nothing except for update stats */
    void handlePutERequest(CacheLine* cacheLine);
    
    /** Handle Inv request.  Cache line is already in the correct state
        and/or has been upgraded */
    void processInvRequest(MemEvent* event, CacheLine* cacheLine);
    
    /** Handle InvX request.  Cache line is already in the correct state
        and/or has been upgraded */
    void processInvXRequest(MemEvent* event, CacheLine* cacheLine);
    
    /** Write data to cache line */
    void updateCacheLineRxWriteback(MemEvent* _event, CacheLine* _cacheLine);
    
    /** Wrapper for the other 'forwardMessage' function with a different signature */
    void forwardMessage(MemEvent* _event, CacheLine* cacheLine, vector<uint8_t>* _data);
    
    /** Cache line needs to be upgraded in order to proceed with the request.  
        Forward request to lower level caches */
    void forwardMessage(MemEvent* _event, Addr _baseAddr, unsigned int _lineSize, vector<uint8_t>* _data);

    /** Send Event.  This version simply forwards/sends the event provided */
    void sendEvent(MemEvent* _event);
    
    /** Send response memEvent to lower level caches */
    void sendResponse(MemEvent* _event, CacheLine* _cacheLine, int _parentId, bool _mshrHit);

    /** Send writeback request to lower level caches */
    void sendWriteback(Command cmd, CacheLine* cacheLine);

    /** Print statistics at the end of simulation */
    void printStats(int _statsFile, vector<int> _statGroupIds, map<int, CtrlStats> _ctrlStats, uint64_t _updgradeLatency);
    
    /** Sets the name of the next level cache */
    void setNextLevelCache(string _nlc){ nextLevelCacheName_ = _nlc; }

    /** Create MemEvent and send NACK cmd to HgLvl caches */
    void sendNACK(MemEvent*);
    
    /* Send andy outgoing messages directed to lower level caches or 
       directory controller (if one exists) */
    void sendOutgoingCommands(){
        timestamp_++;
        
        while(!outgoingEventQueue_.empty() && outgoingEventQueue_.front().deliveryTime <= timestamp_) {
            MemEvent *outgoingEvent = outgoingEventQueue_.front().event;
            if(directoryLink_) {
                outgoingEvent->setDst(directoryLink_->findTargetDirectory(outgoingEvent->getBaseAddr()));
                directoryLink_->send(outgoingEvent);
            } else {
                lowNetPorts_->at(0)->send(outgoingEvent);
            }
            outgoingEventQueue_.pop_front();
        }
        
    }

private:
    vector<Link*>*      lowNetPorts_;

    CacheListener*      listener_;
    map<uint32_t,Stats> stats_;
    vector<int>         statGroupIds_;
    
    string              ownerName_;
    string              nextLevelCacheName_;

    uint32_t            groupId_;
    uint32_t            groupId_timestamp_;
    
    void setGroupId(uint32_t _groupId){
        groupId_timestamp_ = timestamp_;
        groupId_ = _groupId;
    }
    
    uint32_t getGroupId(){
        assert(timestamp_ == groupId_timestamp_);
        assert(groupId_ != 0);
        return groupId_;
    }

    //TODO move all this functions to its own "stats" class

   void inc_GETXMissSM(Addr _addr, bool _prefetchRequest){
        if(!_prefetchRequest){
            stats_[0].GETXMissSM_++;
            if(groupStats_) stats_[getGroupId()].GETXMissSM_++;
            listener_->notifyAccess(CacheListener::WRITE, CacheListener::MISS, _addr);
        }
    }


    void inc_GETXMissIM(Addr _addr, bool _prefetchRequest){
        if(!_prefetchRequest){
            stats_[0].GETXMissIM_++;
            if(groupStats_) stats_[getGroupId()].GETXMissIM_++;
            listener_->notifyAccess(CacheListener::WRITE, CacheListener::MISS, _addr);
        }
    }


    void inc_GETSHit(Addr _addr, bool _prefetchRequest){
        if(!_prefetchRequest){
            stats_[0].GETSHit_++;
            if(groupStats_) stats_[getGroupId()].GETSHit_++;
            listener_->notifyAccess(CacheListener::READ, CacheListener::HIT, _addr);
        }
    }


    void inc_GETXHit(Addr _addr, bool _prefetchRequest){
        if(!_prefetchRequest){
            stats_[0].GETXHit_++;
            if(groupStats_) stats_[getGroupId()].GETXHit_++;
            listener_->notifyAccess(CacheListener::WRITE, CacheListener::HIT, _addr);
        }
    }


    void inc_GETSMissIS(Addr _addr, bool _prefetchRequest){
        if(!_prefetchRequest){
            stats_[0].GETSMissIS_++;
            if(groupStats_) stats_[getGroupId()].GETSMissIS_++;
            listener_->notifyAccess(CacheListener::READ, CacheListener::MISS, _addr);
        }
    }

    void inc_GetSExReqsReceived(){
        stats_[0].GetSExReqsReceived_++;
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

    void inc_PUTXReqsReceived(){
        stats_[0].PUTXReqsReceived_++;
        if(groupStats_) stats_[getGroupId()].PUTXReqsReceived_++;
    }

    void inc_PUTEReqsReceived(){
        stats_[0].PUTEReqsReceived_++;
        if(groupStats_) stats_[getGroupId()].PUTEReqsReceived_++;
    }

    void inc_InvalidatePUTMReqSent(){
        stats_[0].InvalidatePUTMReqSent_++;
        if(groupStats_) stats_[getGroupId()].InvalidatePUTMReqSent_++;
    }

    void inc_InvalidatePUTEReqSent(){
        stats_[0].InvalidatePUTEReqSent_++;
        if(groupStats_) stats_[getGroupId()].InvalidatePUTEReqSent_++;
    }

    void inc_InvalidatePUTSReqSent(){
        stats_[0].InvalidatePUTSReqSent_++;
        if(groupStats_) stats_[getGroupId()].InvalidatePUTSReqSent_++;
    }

    void inc_InvalidatePUTXReqSent(){
        stats_[0].InvalidatePUTXReqSent_++;
        if(groupStats_) stats_[getGroupId()].InvalidatePUTXReqSent_++;
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

    void inc_NACKsSent(){
        stats_[0].NACKsSent_++;
        if(groupStats_) stats_[getGroupId()].NACKsSent_++;
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

