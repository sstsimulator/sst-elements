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
private:
    vector<Link*>* lowNetPorts_;

    CacheListener* listener_;
    uint GETSMissIS_;
    uint GETXMissSM_;
    uint GETXMissIM_;
    uint GETSHit_;
    uint GETXHit_;
    uint PUTSReqsReceived_;
    uint PUTEReqsReceived_;
    uint PUTMReqsReceived_;
    uint GetSExReqsReceived_;
    uint EvictionPUTSReqSent_;
    uint EvictionPUTMReqSent_;
    uint InvalidatePUTMReqSent_;
    uint FetchInvalidateReqSent_;
    uint FetchInvalidateXReqSent_;
    string ownerName_;
    string nextLevelCacheName_;
    
    void inc_GETXMissSM(Addr addr, bool pf);
    void inc_GETXMissIM(Addr addr, bool pf);
    void inc_GETSHit(Addr addr, bool pf);
    void inc_GETXHit(Addr addr, bool pf);
    void inc_GETSMissIS(Addr addr, bool pf);
    bool isExclusive(CacheLine* cacheLine);
    
    unsigned int getParentId(CacheLine* wbCacheLine);
    unsigned int getParentId(Addr baseAddr);

public:
    MESIBottomCC(const SST::MemHierarchy::Cache* _cache, string _ownerName, Output* _dbg,
                 vector<Link*>* _parentLinks, CacheListener* _listener, unsigned int _lineSize,
                 uint64 _accessLatency, uint64 _mshrLatency, bool _L1, MemNIC* _directoryLink) :
                 CoherencyController(_cache, _dbg, _lineSize), lowNetPorts_(_parentLinks),
                 listener_(_listener), ownerName_(_ownerName) {
        d_->debug(_INFO_,"--------------------------- Initializing [BottomCC] ... \n\n");
        GETSMissIS_            = 0;
        GETXMissSM_            = 0;
        GETXMissIM_            = 0;
        GETSHit_               = 0;
        GETXHit_               = 0;
        PUTSReqsReceived_      = 0;
        PUTEReqsReceived_      = 0;
        PUTMReqsReceived_      = 0;
        GetSExReqsReceived_    = 0;
        EvictionPUTSReqSent_   = 0;
        EvictionPUTMReqSent_   = 0;
        InvalidatePUTMReqSent_ = 0;
        
        L1_ = _L1;
        accessLatency_ = _accessLatency;
        mshrLatency_   = _mshrLatency;
        directoryLink_ = _directoryLink;
    }

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

    
    void init(const char* name){}
    
    virtual void handleEviction(MemEvent* event, CacheLine* wbCacheLine);
    virtual void handleAccess(MemEvent* event, CacheLine* cacheLine, Command cmd);
    virtual void handleResponse(MemEvent* ackEvent, CacheLine* cacheLine, const vector<mshrType> mshrEntry);
    virtual void handleInvalidate(MemEvent *event, CacheLine* cacheLine, Command cmd);
    virtual void handleFetchInvalidate(MemEvent* _event, CacheLine* _cacheLine, int _parentId, bool _mshrHit);
    
    void printStats(int _stats, uint64 _GetSExReceived, uint64 _invalidateWaitingForUserLock, uint64 _totalInstReceived, uint64 _nonCoherenceReqsReceived, uint64 _mshrHits);
    void forwardMessage(MemEvent* _event, CacheLine* cacheLine, vector<uint8_t>* _data);
    void forwardMessage(MemEvent* _event, Addr _baseAddr, unsigned int _lineSize, vector<uint8_t>* _data);
    bool modifiedStateNeeded(MemEvent* _event, CacheLine* _cacheLine);
    void processGetXRequest(MemEvent* event, CacheLine* cacheLine, Command cmd);
    void processGetSRequest(MemEvent* event, CacheLine* cacheLine);
    void processPutMRequest(MemEvent* event, CacheLine* cacheLine);
    void processPutERequest(MemEvent* event, CacheLine* cacheLine);
    void processInvRequest(MemEvent* event, CacheLine* cacheLine);
    void processInvXRequest(MemEvent* event, CacheLine* cacheLine);
    void updateEvictionStats(BCC_MESIState _state);
    bool canInvalidateRequestProceed(MemEvent* _event, CacheLine* _cacheLine, bool _sendAcks);
    void sendResponse(MemEvent* _event, CacheLine* _cacheLine, int _parentId, bool _mshrHit);
    void sendWriteback(Command cmd, CacheLine* cacheLine);
    bool sendAckResponse(MemEvent *event);
    void setNextLevelCache(string _nlc){ nextLevelCacheName_ = _nlc; }
};


}}


#endif

