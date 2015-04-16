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
                       uint64_t _tagLatency, uint64_t _mshrLatency, vector<Link*>* _childrenLinks, bool _snoopL1Invs, bool debugAll, Addr debugAddr)
                       : CoherencyController(_cache, _dbg, _lineSize, _accessLatency, _tagLatency, _mshrLatency, debugAll, debugAddr){
        d_->debug(_INFO_,"--------------------------- Initializing [TopCC] ...\n\n");
        L1_                 = true;
        highNetPorts_       = _childrenLinks;
        NACKsSent_          = 0;
        dummyCCLine_        = new CCLine(_dbg);
        topNetworkLink_     = NULL;
        snoopL1Invs_        = _snoopL1Invs;
    }
    
    /** Upon a request, this function returns true if a response was sent back.
        In order for function to send response, the state of the cache line and the type of request
        have to align */
    virtual bool handleRequest(MemEvent* event, CacheLine* cacheLine, bool _mshrHit);

    /** Determines whether the Inv request will ultimately require an MSHR entry (ie. request will stall) */
    virtual bool willRequestPossiblyStall(int lineIndex, MemEvent* _event){return false;}
    
    /** Returns the state of the CCLine.
        Dummy TopCC always has all CCLines in V states since there are no sharers */
    virtual TCC_State getState(int lineIndex) { return V; }
    
    /** Returns number of sharers.
        Dummy TopCC always returns 0 */
    virtual uint numSharers(int lineIndex){ return 0; }
   
    virtual uint ownerExists(int lineIndex){ return 0; }

    /** Handle Eviction. Return true if invalidates are sent and 'this' cache needs
       to wait for akcks Dummy TopCC always returns false */
    virtual void handleEviction(int lineIndex, string _origRqstr, State _state) {return;}
    
    virtual bool isCoherenceMiss(MemEvent* event, CacheLine * line) { return false; }

    /* Incoming Inv/InvX/FetchInv/FetchInvX received.  TopCC sends invalidates up the hierarchy if necessary */
    virtual void handleInvalidate(int lineIndex, MemEvent* event, string _origRqstr, Command cmd, bool _mshrHit) {
        if (L1_ && snoopL1Invs_) {
            MemEvent* snoop = new MemEvent((Component*)owner_, event->getAddr(), event->getBaseAddr(), Inv);
            uint64_t deliveryTime = (_mshrHit) ? timestamp_ + mshrLatency_ : timestamp_ + accessLatency_;
            Response resp = {snoop, deliveryTime, true};
            addToOutgoingQueue(resp);
        }
        
        return;
    }
    virtual void handleFetchResp(MemEvent * ev, CacheLine * line) { return; }

    /** Create MemEvent and send Response to HgLvl caches */
    void sendResponse(MemEvent* _event, State _newState, vector<uint8_t>* _data, bool _mshrHit, bool atomic = false);

    /** Look at the outgoing queue buffer to see if we need to send an memEvent through the SST Links */
    void sendOutgoingCommands(SimTime_t curTime){
        timestamp_++;
        
        while(!outgoingEventQueue_.empty() && outgoingEventQueue_.front().deliveryTime <= timestamp_) {
            MemEvent *outgoingEvent = outgoingEventQueue_.front().event;
            
            if (DEBUG_ALL || DEBUG_ADDR == outgoingEvent->getBaseAddr()) {
            d_->debug(_L4_,"SEND. Cmd: %s, BsAddr: %" PRIx64 ", Addr: %" PRIx64 ", Rqstr: %s, Src: %s, Dst: %s, PreF:%s, Size = %u, time: (%" PRIu64 ", %" PRIu64 ")\n",
                   CommandString[outgoingEvent->getCmd()], outgoingEvent->getBaseAddr(), outgoingEvent->getAddr(), outgoingEvent->getRqstr().c_str(), outgoingEvent->getSrc().c_str(), 
                   outgoingEvent->getDst().c_str(), outgoingEvent->isPrefetch() ? "true" : "false", outgoingEvent->getSize(), timestamp_, curTime);
            }

            if (topNetworkLink_) {
                topNetworkLink_->send(outgoingEvent);
            } else {
                highNetPorts_->at(0)->send(outgoingEvent);
            }
            outgoingEventQueue_.pop_front();
        }
    }
    
    virtual CCLine* getCCLine(int index) { return dummyCCLine_; }
    
    /** Create MemEvent and send NACK cmd to HgLvl caches */
    void sendNACK(MemEvent*);
    
    /** Send event to Higher level caches */
    void resendEvent(MemEvent*);
    
    virtual void printStats(int _stats){};
   
    vector<Link*>*  highNetPorts_;
    uint            NACKsSent_;
    CCLine*         dummyCCLine_;
    bool            snoopL1Invs_;

//    virtual void profileReqSent(Command _cmd, bool _eviction) {
    void profileReqSent(Command _cmd, bool _eviction) {
        if (_cmd == NACK) NACKsSent_++;   
    }
    
};

/*--------------------------------------------------------------------------------------------
 * Top Controller that implements the MESI protocol, should be use by any cache that is not L1
 * keeps sharers state, and handles downgrades and invalidates to lower level hierarchies
  -------------------------------------------------------------------------------------------*/
class MESITopCC : public TopCacheController{

public:
    MESITopCC(const SST::MemHierarchy::Cache* _cache, Output* _dbg, uint _protocol, uint _numLines,
              uint _lineSize, uint64_t _accessLatency, uint64_t _tagLatency, uint64_t _mshrLatency, vector<Link*>* _childrenLinks, MemNIC* _topNetworkLink, bool debugAll, Addr debugAddr) :
              TopCacheController(_cache, _dbg, _lineSize, _accessLatency, _tagLatency, _mshrLatency, _childrenLinks, false, debugAll, debugAddr),
              numLines_(_numLines), lowNetworkNodeCount_(0){
        d_->debug(_INFO_,"--------------------------- Initializing [MESITopCC] ...\n");
        d_->debug(_INFO_, "CCLines:  %d \n", numLines_);
        
        L1_                     = false;
        snoopL1Invs_            = false;
        invReqsSent_            = 0;
        evictionInvReqsSent_    = 0;
        invXReqsSent_           = 0;
        evictionRequiredInv     = 0;
        protocol_               = _protocol;
        ccLines_.resize(numLines_);
        topNetworkLink_         = _topNetworkLink;

        for(uint i = 0; i < ccLines_.size(); i++)
            ccLines_[i] = new CCLine(_dbg);
    }
    
    ~MESITopCC(){
        for(unsigned int i = 0; i < ccLines_.size(); i++)
            delete ccLines_[i];
        lowNetworkNameMap_.clear();
        lowNetworkIdMap_.clear();
    }
    
    // profiling
    uint invReqsSent_;
    uint invXReqsSent_;
    uint evictionInvReqsSent_;
    
    /** Return the specified ccLines array element */
    virtual CCLine* getCCLine(int _index) { return ccLines_[_index];}
    
    /** Upon a request, this function returns true if a response was sent back. In order for function to
    send response, the state of the cache line and the type of request have to align */
    virtual bool handleRequest(MemEvent* event, CacheLine* cacheLine, bool mshrHit);
    
    /* Handle Eviction. Return true if invalidates are sent and 'this' cache needs
       to wait for akcks Dummy TopCC always returns false */
    virtual void handleEviction(int lineIndex, string origRqstr, State state);
    
    /* Incoming Inv/FetchInv received.  TopCC sends invalidates up the hierarchy if necessary */
    virtual void handleInvalidate(int lineIndex, MemEvent* event, string origRqstr, Command cmd, bool mshrHit);
    
    /** Determines whether the Inv request will ultimately require an MSHR entry (ie. request will stall) */
    virtual bool willRequestPossiblyStall(int lineIndex, MemEvent* event);

    /** Print statistics at end of simulation */
    void printStats(int _stats);
    
    /** Lookup link to lwLvl cache/bus by name */
    int  lowNetworkNodeLookupByName(const std::string &name);
    
    /** Lookup link to lwLvl cache/bus by ID */
    std::string lowNetworkNodeLookupById(int id);
    
    /** Handle incoming GetS Request.
        TopCC sends invalidates if needed, and add sharer appropriately */
    bool handleGetSRequest(MemEvent* _event, CacheLine* _cacheLine, int _childId, bool _mshrHit);
    
    /** Handle incoming GetX Request.
        TopCC sends invalidates if needed, add sharer and mark sharer as exclusive */
    bool handleGetXRequest(MemEvent* _event, CacheLine* _cacheLine, int _childId, bool _mshrHit);
   
    /** Handle incoming PutM Request.
        Clear exclusive sharer */
    void handlePutMRequest(CCLine* _ccLine, Command _cmd, State _state, int _childId);
    
    /** Handle incoming GetS Request.
        Clear sharer */
    bool handlePutSRequest(CCLine* _ccLine, int _childId);
   
    void handleFetchResp(MemEvent * _event, CacheLine* _cacheLine);

    /** Returns the state of the CCLine. */
    TCC_State getState(int lineIndex) { return ccLines_[lineIndex]->getState(); }
    
    
    /** Returns number of sharers. */
    uint numSharers(int lineIndex){return ccLines_[lineIndex]->numSharers();}
    
    uint ownerExists(int lineIndex){return ccLines_[lineIndex]->ownerExists();}
    
    bool isCoherenceMiss(MemEvent* event, CacheLine* cacheLine);

private:
    uint                numLines_;
    uint                lowNetworkNodeCount_;
    map<string, int>    lowNetworkNameMap_;
    map<int, string>    lowNetworkIdMap_;
    int                 protocol_;
    vector<CCLine*>     ccLines_;
    uint                evictionRequiredInv;

    int sendInvalidates(int lineIndex, string srcNode, string origRqstr, bool _mshrHit);
    void sendFetchInvX(int lineIndex, string _origRqstr, bool _mshrHit);
    void sendFetchInv(CCLine* _cLine, string origRqstr, bool mshrHit);
    void sendInvalidate(CCLine* _cLine, string destination, string origRqstr, bool _acksNeeded, bool _mshrHit);
    
    void sendEvictionInvalidates(int _lineIndex, string _origRqstr, bool _mshrHit);
    void sendCCInvalidates(int _lineIndex, string _srcNode, string _origRqstr, bool _mshrHit);
    
    virtual void profileReqSent(Command _cmd, bool _eviction, int _num);
    };

}}

#endif
