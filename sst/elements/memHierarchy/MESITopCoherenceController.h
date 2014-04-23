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
 * File:   MESITopCoherenceController.h
 * Author: Caesar De la Paz III
 * Email:  caesar.sst@gmail.com
 */

#ifndef MESITOPCOHERENCECONTROLLERS_H
#define MESITOPCOHERENCECONTROLLERS_H

#include <iostream>
#include "MESITopCoherenceController.h"


namespace SST { namespace MemHierarchy {

/*--------------------------------------------------------------------------------------------
 * Dummy class intented to be use by any L1 Cache. L1 caches don't need to invalidate higher
 * levels or store information about sharers. However, this class neededs to hold all the data
 * to fake that there's no sharers and lines are valid at all times.
 * Keeps sharers state, and handles downgrades and invalidates to lower level hierarchies
  -------------------------------------------------------------------------------------------*/
  
class TopCacheController : public CoherencyController {
public:
    #include "ccLine.h"
    
    TopCacheController(const Cache* _cache, Output* _dbg, uint _lineSize, uint64_t _accessLatency,
                       uint64_t _mshrLatency, vector<Link*>* _childrenLinks)
                       : CoherencyController(_cache, _dbg, _lineSize){
        d_->debug(_INFO_,"--------------------------- Initializing [TopCC] ...\n\n");
        L1_ = true;
        accessLatency_ = _accessLatency;
        mshrLatency_   = _mshrLatency;
        highNetPorts_  = _childrenLinks;
    }
    
    virtual TCC_MESIState getState(int lineIndex) { return V; }
    virtual uint numSharers(int lineIndex){ return 0; }
    virtual bool handleEviction(int lineIndex, BCC_MESIState _state) {return false;}
    virtual void handleFetchInvalidate(CacheLine* _cacheLine, Command _cmd) {}
    virtual bool handleAccess(MemEvent* event, CacheLine* cacheLine, bool _mshrHit);
    virtual void handleInvalidate(int lineIndex, Command cmd){return;}
    //virtual void handleInvAck(MemEvent* event, CCLine* ccLine){return;};
    virtual void printStats(int _stats){};
    
    void sendOutgoingCommands(){
        while(!outgoingEventQueue_.empty() && outgoingEventQueue_.front().deliveryTime <= timestamp_) {
            highNetPorts_->at(0)->send(outgoingEventQueue_.front().event);
            outgoingEventQueue_.pop();
        }
    }
    
    bool sendResponse(MemEvent* _event, BCC_MESIState _newState, vector<uint8_t>* _data, bool _mshrHit);
    void sendNack(MemEvent*);
    vector<Link*>* highNetPorts_;

};

/*--------------------------------------------------------------------------------------------
 * Top Controller that implements the MESI protocol, should be use by any cache that is not L1
 * keeps sharers state, and handles downgrades and invalidates to lower level hierarchies
  -------------------------------------------------------------------------------------------*/
class MESITopCC : public TopCacheController{
private:
    uint numLines_;
    uint lowNetworkNodeCount_;
    map<string, int> lowNetworkNameMap_;
    int protocol_;
    void sendInvalidates(Command cmd, int lineIndex, bool eviction, string requestingNode, bool acksNeeded);
    
public:
    MESITopCC(const SST::MemHierarchy::Cache* _cache, Output* _dbg, uint _protocol, uint _numLines,
              uint _lineSize, uint64_t _accessLatency, uint64_t _mshrLatency, vector<Link*>* _childrenLinks) :
              TopCacheController(_cache, _dbg, _lineSize, _accessLatency, _mshrLatency, _childrenLinks),
              numLines_(_numLines), lowNetworkNodeCount_(0){
        d_->debug(_INFO_,"--------------------------- Initializing [MESITopCC] ...\n");
        d_->debug(_INFO_, "CCLines:  %d \n", numLines_);
        ccLines_.resize(numLines_);
        for(uint i = 0; i < ccLines_.size(); i++){
            ccLines_[i] = new CCLine(_dbg);
        }
        L1_ = false;
        protocol_ = _protocol;
        InvReqsSent_ = 0, EvictionInvReqsSent_ = 0;
        protocol_ = _protocol;
    }
    
    ~MESITopCC(){
        for(unsigned int i = 0; i < ccLines_.size(); i++) delete ccLines_[i];
        lowNetworkNameMap_.clear();
    }
    
    uint InvReqsSent_;
    uint EvictionInvReqsSent_;
    vector<CCLine*> ccLines_;

    virtual bool handleEviction(int lineIndex, BCC_MESIState _state);
    virtual void handleFetchInvalidate(CacheLine* _cacheLine, Command _cmd);
    virtual bool handleAccess(MemEvent* event, CacheLine* cacheLine, bool _mshrHit);
    virtual void handleInvalidate(int lineIndex, Command cmd);
    //virtual void handleInvAck(MemEvent* event, CCLine* ccLine);
    
    void printStats(int _stats);
    int  lowNetworkNodeLookup(const std::string &name);
    void processGetSRequest(MemEvent* _event, CacheLine* _cacheLine, int _childId, bool _mshrHit, bool& _ret);
    void processGetXRequest(MemEvent* _event, CacheLine* _cacheLine, int _childId, bool _mshrHit, bool& _ret);
    void processPutMRequest(CCLine* _ccLine, Command _cmd, BCC_MESIState _state, int _childId, bool& _ret);
    void processPutSRequest(CCLine* _ccLine, int _childId, bool& _ret);
    
    TCC_MESIState getState(int lineIndex) { return ccLines_[lineIndex]->getState(); }
    uint numSharers(int lineIndex){return ccLines_[lineIndex]->numSharers();}
    
};

}}

#endif
