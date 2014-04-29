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

namespace SST { namespace MemHierarchy {

class MESIBottomCC : public CoherencyController{
public:
    /** Constructor for MESIBottomCC. */
    MESIBottomCC(const SST::MemHierarchy::Cache* _cache, string _ownerName, Output* _dbg,
                 vector<Link*>* _parentLinks, CacheListener* _listener, unsigned int _lineSize,
                 uint64 _accessLatency, uint64 _mshrLatency, bool _L1, MemNIC* _directoryLink) :
                 CoherencyController(_cache, _dbg, _lineSize), lowNetPorts_(_parentLinks),
                 listener_(_listener), ownerName_(_ownerName) {
        d_->debug(_INFO_,"--------------------------- Initializing [BottomCC] ... \n\n");
        GETSMissIS_ = GETXMissSM_ = GETXMissIM_ = GETSHit_ = GETXHit_ = 0;
        PUTSReqsReceived_ = PUTEReqsReceived_ = PUTMReqsReceived_ = PUTXReqsReceived_ = 0;
        EvictionPUTSReqSent_ = EvictionPUTMReqSent_ = EvictionPUTEReqSent_ = 0;
        InvalidatePUTMReqSent_ = InvalidatePUTEReqSent_ = InvalidatePUTXReqSent_ = 0;
        GetSExReqsReceived_ = GetSReqsReceived_ = GetXReqsReceived_ = 0;
        FetchInvalidateReqSent_ = FetchInvalidateXReqSent_ = 0;

        L1_             = _L1;
        accessLatency_  = _accessLatency;
        mshrLatency_    = _mshrLatency;
        directoryLink_  = _directoryLink;
    }
    /** Init funciton */
    void init(const char* name){}
    
    /** Send cache line data to the lower level caches */
    virtual void handleEviction(CacheLine* _wbCacheLine);

    /** Process new cache request:  GetX, GetS, GetSEx, PutM, PutS, PutX */
    virtual void handleRequest(MemEvent* event, CacheLine* cacheLine, Command cmd);
    
    /** Process Inv/InvX request.  -----Arguably the most important function within MemHierarchy.-----
       Most errors come from deadlocks due to invalidation/evicitons/upgrades all happening simultaneously
       Special care needs to be taken when a invalidate is received. */
    virtual void handleInvalidate(MemEvent *event, CacheLine* cacheLine, Command cmd);

    /** Process GetSResp/GetXResp.  Update the cache line with the 
        provided/given state and payload */
    virtual void handleResponse(MemEvent* ackEvent, CacheLine* cacheLine, const vector<mshrType> mshrEntry);
    
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

    /** Send response memEvent to lower level caches */
    void sendResponse(MemEvent* _event, CacheLine* _cacheLine, int _parentId, bool _mshrHit);

    /** Send writeback request to lower level caches */
    void sendWriteback(Command cmd, CacheLine* cacheLine);

    /** Print statistics at the end of simulation */
    void printStats(int _stats, uint64 _GetSExReceived, uint64 _invalidateWaitingForUserLock, uint64 _totalReqsReceived, uint64 _mshrHits, uint64 _updgradeLatency);
    
    /** Sets the name of the next level cache */
    void setNextLevelCache(string _nlc){ nextLevelCacheName_ = _nlc; }


    /* Send andy outgoing messages directed to lower level caches or 
       directory controller (if one exists) */
    void sendOutgoingCommands(){
        while(!outgoingEventQueue_.empty() && outgoingEventQueue_.front().deliveryTime <= timestamp_) {
            MemEvent *outgoingEvent = outgoingEventQueue_.front().event;
            if(directoryLink_) {
                outgoingEvent->setDst(directoryLink_->findTargetDirectory(outgoingEvent->getBaseAddr()));
                directoryLink_->send(outgoingEvent);
            } else {
                lowNetPorts_->at(0)->send(outgoingEvent);
            }
            outgoingEventQueue_.pop();
        }
    }

private:
    vector<Link*>* lowNetPorts_;

    CacheListener* listener_;
    uint    GETSMissIS_,
            GETXMissSM_,
            GETXMissIM_,
            GETSHit_,
            GETXHit_,
            PUTSReqsReceived_,
            PUTEReqsReceived_,
            PUTMReqsReceived_,
            PUTXReqsReceived_,
            GetSExReqsReceived_,
            GetXReqsReceived_,
            GetSReqsReceived_,
            EvictionPUTSReqSent_,
            EvictionPUTMReqSent_,
            EvictionPUTEReqSent_,
            InvalidatePUTMReqSent_,
            InvalidatePUTEReqSent_,
            InvalidatePUTXReqSent_,
            FetchInvalidateReqSent_,
            FetchInvalidateXReqSent_;
    string ownerName_;
    string nextLevelCacheName_;
    
    void inc_GETXMissSM(Addr addr, bool pf);
    void inc_GETXMissIM(Addr addr, bool pf);
    void inc_GETSHit(Addr addr, bool pf);
    void inc_GETXHit(Addr addr, bool pf);
    void inc_GETSMissIS(Addr addr, bool pf);
    

};


}}


#endif

