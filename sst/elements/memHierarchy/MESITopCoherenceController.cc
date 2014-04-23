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
 * File:   MESITopCoherenceController.cc
 * Author: Caesar De la Paz III
 * Email:  caesar.sst@gmail.com
 */

#include <sst_config.h>
#include <vector>
#include "coherenceControllers.h"
#include "MESITopCoherenceController.h"
using namespace SST;
using namespace SST::MemHierarchy;

/*-------------------------------------------------------------------------------------
 * Top Coherence Controller Implementation
 *-------------------------------------------------------------------------------------*/

bool TopCacheController::handleRequest(MemEvent* _event, CacheLine* _cacheLine, bool _mshrHit){
    Command cmd           = _event->getCmd();
    vector<uint8_t>* data = _cacheLine->getData();
    BCC_MESIState state   = _cacheLine->getState();

    switch(cmd){
        case GetS:
            if(state == S || state == M || state == E)
                return sendResponse(_event, S, data, _mshrHit);
            break;
        case GetX:
        case GetSEx:
            if(state == M) return sendResponse(_event, M, data, _mshrHit);
            break;
        default:
            _abort(MemHierarchy::CacheController, "Wrong command type!");
    }
    return false;
}

bool MESITopCC::handleRequest(MemEvent* _event, CacheLine* _cacheLine, bool _mshrHit){
    Command cmd = _event->getCmd();
    int id = lowNetworkNodeLookup(_event->getSrc());
    CCLine* ccLine = ccLines_[_cacheLine->index()];
    bool ret       = false;

    switch(cmd){
        case GetS:
            handleGetSRequest(_event, _cacheLine, id, _mshrHit, ret);
            break;
        case GetX:
        case GetSEx:
            handleGetXRequest(_event, _cacheLine, id, _mshrHit, ret);
            break;
        case PutS:
            handlePutSRequest(ccLine, id, ret);
            break;
        case PutM:
        case PutE:
        case PutX:
            handlePutMRequest(ccLine, cmd, _cacheLine->getState(), id, ret);
            break;

        default:
            _abort(MemHierarchy::CacheController, "Wrong command type!");
    }

    return ret;
}

void MESITopCC::handleInvalidate(int _lineIndex, Command _cmd){
    CCLine* l = ccLines_[_lineIndex];
    if(l->isShareless()) return;
    
    if(l->exclusiveSharerExists()){
        sendInvalidates(_cmd, _lineIndex, false, "", true);
    }
    else{
        sendInvalidates(_cmd, _lineIndex, false, "", false);
        l->removeAllSharers();
    }
    return;
}

void MESITopCC::handleFetchInvalidate(CacheLine* _cacheLine, Command _cmd){
    CCLine* l = ccLines_[_cacheLine->index()];
    if(!l->exclusiveSharerExists() && l->numSharers() == 0) return;

    switch(_cmd){
        case FetchInvalidate:
            if(l->exclusiveSharerExists())
                sendInvalidates(Inv, _cacheLine->index(), false, "", true);
            else if(l->numSharers() > 0){
                sendInvalidates(Inv, _cacheLine->index(), false, "", false);
                l->removeAllSharers();
            }
            else _abort(MemHierarchy::CacheController, "Command not supported");
            break;
        case FetchInvalidateX:
            assert(0);
        default:
            _abort(MemHierarchy::CacheController, "Command not supported");
    }
}


/* Function sends invalidates to lower level caches, removes sharers if needed.  
 * Currently it implements weak consistency, ie. invalidates to sharers do not need acknowledgment
 * Returns true if eviction requires a response from Child, and false if no response is expected */
bool MESITopCC::handleEviction(int lineIndex,  BCC_MESIState _state){
    if(_state == I) return false;
    bool waitForInvalidateAck = false;
    assert(!CacheArray::CacheLine::inTransition(_state));
    CCLine* ccLine = ccLines_[lineIndex];
    assert(ccLine->valid());
    
    if(ccLine->exclusiveSharerExists()) waitForInvalidateAck = true;
    if(!ccLine->isShareless()){
        d_->debug(_L1_,"Stalling request: Eviction requires invalidation of lw lvl caches. St = %s, ExSharerFlag = %s \n", BccLineString[_state], waitForInvalidateAck ? "True" : "False");
        if(waitForInvalidateAck){
            sendInvalidates(Inv, lineIndex, true, "", true);
            return (ccLine->getState() != V);
        }
        else{
            assert(_state != IM || ccLine->exclusiveSharerExists());
            assert(ccLine->getState() == V);
            sendInvalidates(Inv, lineIndex, true, "", false);
            ccLine->removeAllSharers();
        }
    }
    return false;
}


void MESITopCC::sendInvalidates(Command cmd, int lineIndex, bool eviction, string requestingNode, bool acksNeeded){
    CCLine* ccLine = ccLines_[lineIndex];
    assert(!ccLine->isShareless());         //Make sure there's actually sharers
    unsigned int sentInvalidates = 0;
    int requestingId = requestingNode.empty() ? -1 : lowNetworkNodeLookup(requestingNode);
    
    d_->debug(_L1_,"Number of Sharers: %u \n", ccLine->numSharers());

    MemEvent* invalidateEvent;
    for(map<string, int>::iterator sharer = lowNetworkNameMap_.begin(); sharer != lowNetworkNameMap_.end(); sharer++){
        int sharerId = sharer->second;
        if(requestingId == sharerId) continue;
        if(ccLine->isSharer(sharerId)){
            if(acksNeeded){
                if(cmd == Inv) ccLine->setState(Inv_A);
                else ccLine->setState(InvX_A);
            }
            sentInvalidates++;
            
            if(!eviction) InvReqsSent_++;
            else EvictionInvReqsSent_++;
            
            invalidateEvent = new MemEvent((Component*)owner_, ccLine->getBaseAddr(), cmd);
            d_->debug(_L1_,"Invalidate sent: %u (numSharers), Invalidating Addr: %"PRIx64", Dst: %s\n", ccLine->numSharers(), ccLine->getBaseAddr(),  sharer->first.c_str());
            invalidateEvent->setDst(sharer->first);
            response resp = {invalidateEvent, timestamp_ + accessLatency_, false};
            outgoingEventQueue_.push(resp);
        }
    }
}



/*---------------------------------------------------------------------------------------------------
 * Helper Functions
 *--------------------------------------------------------------------------------------------------*/



void MESITopCC::handleGetSRequest(MemEvent* _event, CacheLine* _cacheLine, int _sharerId, bool _mshrHit, bool& ret){
    vector<uint8_t>* data = _cacheLine->getData();
    BCC_MESIState state   = _cacheLine->getState();
    int lineIndex         = _cacheLine->index();
    CCLine* l             = ccLines_[_cacheLine->index()];

    /* Send Data in E state */
    if(protocol_ && l->isShareless() && (state == E || state == M)){
        l->setExclusiveSharer(_sharerId);
        ret = sendResponse(_event, E, data, _mshrHit);
    }
    
    /* If exclusive sharer exists, downgrade it to S state */
    else if(l->exclusiveSharerExists()) {
        d_->debug(_L5_,"GetS Req: Exclusive sharer exists \n");
        assert(!l->isSharer(_sharerId));                    /* Cache should not ask for 'S' if its already Exclusive */
        sendInvalidates(InvX, lineIndex, false, "", true);  //TODO: ""? do we really need it?
    }
    /* Send Data in S state */
    else if(state == S || state == M || state == E){
        l->addSharer(_sharerId);
        ret = sendResponse(_event, S, data, _mshrHit);
    }
    else{
        _abort(MemHierarchy::CacheController, "Unkwown state!");
    }
}

void MESITopCC::handleGetXRequest(MemEvent* _event, CacheLine* _cacheLine, int _sharerId, bool _mshrHit, bool& _ret){
    BCC_MESIState state   = _cacheLine->getState();
    int lineIndex         = _cacheLine->index();
    CCLine* ccLine        = ccLines_[lineIndex];

    /* Invalidate any exclusive sharers before responding to GetX request */
    if(ccLine->exclusiveSharerExists()){
        d_->debug(_L5_,"GetX Req: Exclusive sharer exists \n");
        assert(!ccLine->isSharer(_sharerId));
        sendInvalidates(Inv, lineIndex, false, _event->getSrc(), true);
        return;
    }
    /* Sharers exist */
    else if(ccLine->numSharers() > 0){
        d_->debug(_L5_,"GetX Req:  Sharers 'S' exists \n");
        sendInvalidates(Inv, lineIndex, false, _event->getSrc(), false);
        ccLine->removeAllSharers();   //Weak consistency model, no need to wait for InvAcks to proceed with request
    }
    
    if(state == E || state == M){
        ccLine->setExclusiveSharer(_sharerId);
        sendResponse(_event, M, _cacheLine->getData(), _mshrHit);
        _ret = true;
    }
}

//TODO: create processPutXFunction to avoid that extra if inside here
void MESITopCC::handlePutMRequest(CCLine* _ccLine, Command _cmd, BCC_MESIState _state, int _sharerId, bool& _ret){
    _ret = true;
    assert(_state == M || _state == E);

    /* Remove exclusivity for all: PutM, PutX, PutE */
    if(_ccLine->exclusiveSharerExists())  _ccLine->clearExclusiveSharer(_sharerId);

    /*If PutX, keep as sharer since transition ia M->S */
    if(_cmd == PutX){                   /* If PutX, then state = V since we only wanted to get rid of the M-state hglv cache */
        _ccLine->addSharer(_sharerId);
        _ccLine->setState(V);
     }
    else _ccLine->updateState();        /* Id PutE, PutM -> num sharers should be 0 before state goes to (V) */

}

void MESITopCC::handlePutSRequest(CCLine* _ccLine, int _sharerId, bool& _ret){
    _ret = true;
    if(_ccLine->isSharer(_sharerId)) _ccLine->removeSharer(_sharerId);
}

void MESITopCC::printStats(int _stats){
    Output* dbg = new Output();
    dbg->init("", 0, 0, (Output::output_location_t)_stats);
    dbg->output(C,"Invalidates sent (non-eviction): %u\n", InvReqsSent_);
    dbg->output(C,"Invalidates sent due to evictions: %u\n", EvictionInvReqsSent_);
}


bool TopCacheController::sendResponse(MemEvent *_event, BCC_MESIState _newState, std::vector<uint8_t>* _data, bool _mshrHit){
    if(_event->isPrefetch()){
        d_->debug(_WARNING_,"Warning: No Response sent! Thi event is a prefetch or sharerId in -1");
        return true;
    }
    
    MemEvent *responseEvent; Command cmd = _event->getCmd();
    Addr offset = 0, base = 0;
    switch(cmd){
        case GetS: case GetSEx: case GetX:
            if(L1_){
                base = (_event->getAddr()) & ~(lineSize_ - 1);
                offset = _event->getAddr() - base;
                responseEvent = _event->makeResponse((SST::Component*)owner_);
                if(cmd != GetX) responseEvent->setPayload(_event->getSize(), &_data->at(offset));
            }
            else responseEvent = _event->makeResponse((SST::Component*)owner_, *_data, _newState);
            
            responseEvent->setDst(_event->getSrc());
            break;
        default:
            _abort(CoherencyController, "Command not valid as a response. \n");
    }
    
    d_->debug(_L1_,"Sending Response: Addr = %"PRIx64", Dst = %s, Size = %i, Granted State = %s\n", _event->getAddr(), responseEvent->getDst().c_str(), responseEvent->getSize(), BccLineString[responseEvent->getGrantedState()]);
    
    uint64 latency = (_mshrHit) ? timestamp_ + mshrLatency_ : timestamp_ + accessLatency_;
    response resp = {responseEvent, latency, true};
    outgoingEventQueue_.push(resp);
    return true;
}

int MESITopCC::lowNetworkNodeLookup(const std::string &name){
	int id = -1;
	std::map<string, int>::iterator it = lowNetworkNameMap_.find(name);
	if(lowNetworkNameMap_.end() == it) {
        id = lowNetworkNodeCount_++;
		lowNetworkNameMap_[name] = id;
	} else {
		id = it->second;
	}
	return id;
}

