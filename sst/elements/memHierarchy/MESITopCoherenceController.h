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
    
    /** Upon a request, this function returns true if a response was sent back.
        In order for function to send response, the state of the cache line and the type of request
        have to align */
    virtual bool handleRequest(MemEvent* event, CacheLine* cacheLine, bool _mshrHit);

    /** Returns the state of the CCLine.
        Dummy TopCC always has all CCLines in V states since there are no sharers */
    virtual TCC_MESIState getState(int lineIndex) { return V; }
    
    /** Returns number of sharers.
        Dummy TopCC always returns 0 */
    virtual uint numSharers(int lineIndex){ return 0; }
    
    /** Handle Eviction. Return true if invalidates are sent and 'this' cache needs
       to wait for akcks Dummy TopCC always returns false */
    virtual bool handleEviction(int lineIndex, BCC_MESIState _state) {return false;}
   
    /* Incoming Inv received.  TopCC sends invalidates up the hierarchy if necessary */
    virtual void handleInvalidate(int lineIndex, Command cmd){return;}
   
    /** Incoming FetchInv received.  TopCC sends FetchInv up the hierarchy if necessary */
    virtual void handleFetchInvalidate(CacheLine* _cacheLine, Command _cmd) {}

    /** Create MemEvent and send Response to HgLvl caches */
    bool sendResponse(MemEvent* _event, BCC_MESIState _newState, vector<uint8_t>* _data, bool _mshrHit);
   
    /** Look at the outgoing queue buffer to see if we need to send an memEvent through the SST Links */
    void sendOutgoingCommands(){
        while(!outgoingEventQueue_.empty() && outgoingEventQueue_.front().deliveryTime <= timestamp_) {
            highNetPorts_->at(0)->send(outgoingEventQueue_.front().event);
            outgoingEventQueue_.pop();
        }
    }
    
    /** Create MemEvent and send NACK cmd to HgLvl caches */
    void sendNack(MemEvent*);
    
    virtual void printStats(int _stats){};
   
    vector<Link*>* highNetPorts_;

};

/*--------------------------------------------------------------------------------------------
 * Top Controller that implements the MESI protocol, should be use by any cache that is not L1
 * keeps sharers state, and handles downgrades and invalidates to lower level hierarchies
  -------------------------------------------------------------------------------------------*/
class MESITopCC : public TopCacheController{

public:
    MESITopCC(const SST::MemHierarchy::Cache* _cache, Output* _dbg, uint _protocol, uint _numLines,
              uint _lineSize, uint64_t _accessLatency, uint64_t _mshrLatency, vector<Link*>* _childrenLinks) :
              TopCacheController(_cache, _dbg, _lineSize, _accessLatency, _mshrLatency, _childrenLinks),
              numLines_(_numLines), lowNetworkNodeCount_(0){
        d_->debug(_INFO_,"--------------------------- Initializing [MESITopCC] ...\n");
        d_->debug(_INFO_, "CCLines:  %d \n", numLines_);
        
        L1_             = false;
        InvReqsSent_    = 0, EvictionInvReqsSent_ = 0;
        protocol_       = _protocol;
        ccLines_.resize(numLines_);
        
        for(uint i = 0; i < ccLines_.size(); i++)
            ccLines_[i] = new CCLine(_dbg);
    }
    
    ~MESITopCC(){
        for(unsigned int i = 0; i < ccLines_.size(); i++)
            delete ccLines_[i];
        lowNetworkNameMap_.clear();
    }
    
    uint            InvReqsSent_;
    uint            EvictionInvReqsSent_;
    vector<CCLine*> ccLines_;
    
    /** Upon a request, this function returns true if a response was sent back. In order for function to
    send response, the state of the cache line and the type of request have to align */
    virtual bool handleRequest(MemEvent* event, CacheLine* cacheLine, bool _mshrHit);
    
    /* Handle Eviction. Return true if invalidates are sent and 'this' cache needs
       to wait for akcks Dummy TopCC always returns false */
    virtual bool handleEviction(int lineIndex, BCC_MESIState _state);
    
    /* Incoming Inv received.  TopCC sends invalidates up the hierarchy if necessary */
    virtual void handleInvalidate(int lineIndex, Command cmd);
    
    /* Incoming FetchInv received.  TopCC sends FetchInv up the hierarchy if necessary */
    virtual void handleFetchInvalidate(CacheLine* _cacheLine, Command _cmd);
    
    /** Print statistics at end of simulation */
    void printStats(int _stats);
    
    /** Lookup link to lwLvl cache/bus by name */
    int  lowNetworkNodeLookup(const std::string &name);
    
    /** Handle incoming GetS Request.
        TopCC sends invalidates if needed, and add sharer appropriately */
    void handleGetSRequest(MemEvent* _event, CacheLine* _cacheLine, int _childId, bool _mshrHit, bool& _ret);
    
    /** Handle incoming GetX Request.
        TopCC sends invalidates if needed, add sharer and mark sharer as exclusive */
    void handleGetXRequest(MemEvent* _event, CacheLine* _cacheLine, int _childId, bool _mshrHit, bool& _ret);
   
    /** Handle incoming PutM Request.
        Clear exclusive sharer */
    void handlePutMRequest(CCLine* _ccLine, Command _cmd, BCC_MESIState _state, int _childId, bool& _ret);
    
    /** Handle incoming GetS Request.
        Clear sharer */
    void handlePutSRequest(CCLine* _ccLine, int _childId, bool& _ret);
    
    /** Returns the state of the CCLine. */
    TCC_MESIState getState(int lineIndex) { return ccLines_[lineIndex]->getState(); }
    
    /** Returns number of sharers. */
    uint numSharers(int lineIndex){return ccLines_[lineIndex]->numSharers();}

private:
    uint                numLines_;
    uint                lowNetworkNodeCount_;
    map<string, int>    lowNetworkNameMap_;
    int                 protocol_;
    void sendInvalidates(Command cmd, int lineIndex, bool eviction, string requestingNode, bool acksNeeded);
    };

}}

#endif
