/*
 * File:   MESITopCoherenceController.h
 * Author: Caesar De la Paz III
 * Email:  caesar.sst@gmail.com
 */

#ifndef MESITOPCOHERENCECONTROLLERS_H
#define MESITOPCOHERENCECONTROLLERS_H

#include <iostream>


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
    
    TopCacheController(const Cache* _cache, Output* _dbg, uint _lineSize, uint64_t _accessLatency, vector<Link*>* _childrenLinks)
                        : CoherencyController(_cache, _dbg, _lineSize){
        d_->debug(_INFO_,"--------------------------- Initializing [TopCC] ...\n\n");
        L1_ = true;
        accessLatency_ = _accessLatency;
    }
    
    virtual TCC_MESIState getState(int lineIndex) { return V; }
    virtual uint numSharers(int lineIndex){ return 0; }
    virtual bool handleEviction(int lineIndex, BCC_MESIState _state) {return false;}
    virtual void handleFetchInvalidate(CacheLine* _cacheLine, Command _cmd) {}
    virtual bool handleAccess(MemEvent* event, CacheLine* cacheLine, int _childId);
    virtual void handleInvalidate(int lineIndex, Command cmd){return;}
    virtual void handleInvAck(MemEvent* event, CCLine* ccLine, int childId){return;};
    virtual int getChildId(Link* _childLink){ return -1; }
    virtual void printStats(int _stats){};
    
    virtual void sendOutgoingCommands(){
        while(!outgoingEventQueue_.empty() && outgoingEventQueue_.front().deliveryTime <= timestamp_) {
            outgoingEventQueue_.front().deliveryLink->send(outgoingEventQueue_.front().event);
            outgoingEventQueue_.pop();
        }
    }
    
    bool sendResponse(MemEvent* _event, BCC_MESIState _newState, vector<uint8_t>* _data, int _childId);

};

/*--------------------------------------------------------------------------------------------
 * Top Controller that implements the MESI protocol, should be use by any cache that is not L1
 * Keeps sharers state, and handles downgrades and invalidates to lower level hierarchies
  -------------------------------------------------------------------------------------------*/
class MESITopCC : public TopCacheController{
private:
    uint numLines_;
    int protocol_;
    vector<Link*>* childrenLinks_;
    void sendInvalidates(Command cmd, int lineIndex, bool eviction, int requestLink, bool acksNeeded);
    
public:
    MESITopCC(const SST::MemHierarchy::Cache* _cache, Output* _dbg, uint _protocol, uint _numLines, uint _lineSize, uint64_t _accessLatency, vector<Link*>* _childrenLinks) :
           TopCacheController(_cache, _dbg, _lineSize, _accessLatency, _childrenLinks), numLines_(_numLines), childrenLinks_(_childrenLinks){
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
    
    uint InvReqsSent_;
    uint EvictionInvReqsSent_;
    vector<CCLine*> ccLines_;
    TCC_MESIState getState(int lineIndex) { return ccLines_[lineIndex]->getState(); }
    uint numSharers(int lineIndex){return ccLines_[lineIndex]->numSharers();}
    bool handleEviction(int lineIndex, BCC_MESIState _state);
    void handleFetchInvalidate(CacheLine* _cacheLine, Command _cmd);
    bool handleAccess(MemEvent* event, CacheLine* cacheLine, int _childId);
    void handleInvalidate(int lineIndex, Command cmd);
    void handleInvAck(MemEvent* event, CCLine* ccLine, int childId);
    int getChildId(Link* _childLink);
    void printStats(int _stats);
    void sendOutgoingCommands(){ TopCacheController::sendOutgoingCommands(); }
    void processGetSRequest(MemEvent* _event, CacheLine* _cacheLine, int _childId, bool& _ret);
    void processGetXRequest(MemEvent* _event, CacheLine* _cacheLine, int _childId, bool& _ret);
    void processPutMRequest(CCLine* _ccLine, BCC_MESIState _state, int _childId, bool& _ret);
    void processPutSRequest(CCLine* _ccLine, int _childId, bool& _ret);
};

}}

#endif
