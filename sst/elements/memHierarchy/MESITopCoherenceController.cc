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

/*-------------------------------------------------------------------------------------
 * MESI Top Coherence Controller Implementation
 *-------------------------------------------------------------------------------------*/

bool MESITopCC::handleRequest(MemEvent* _event, CacheLine* _cacheLine, bool _mshrHit) {
    Command cmd = _event->getCmd();
    int id = lowNetworkNodeLookupByName(_event->getSrc());
    CCLine* ccLine = ccLines_[_cacheLine->index()];
    if(ccLine->inTransition() && !_event->isWriteback()){
        d_->debug(_L1_,"TopCC: Stalling request, ccLine in transition \n");
        return false;
    }
    
    bool ret = false;

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
        case PutXE:
            handlePutMRequest(ccLine, cmd, _cacheLine->getState(), id, ret);
            break;

        default:
            _abort(MemHierarchy::CacheController, "Wrong command type!");
    }

    return ret;
}


void MESITopCC::handleInvalidate(int _lineIndex, Command _cmd){
    CCLine* ccLine = ccLines_[_lineIndex];
    if(ccLine->isShareless() && !ccLine->ownerExists()) return;
    
    switch(_cmd){
        case Inv:
        case FetchInv:
            sendInvalidates(_lineIndex, "");
            break;
        case InvX:
        case FetchInvX:
            sendInvalidateX(_lineIndex);
            break;
        default:
            _abort(MemHierarchy::CacheController, "Command not supported, Cmd = %s", CommandString[_cmd]);
    }
}

void MESITopCC::handleEviction(int _lineIndex,  BCC_MESIState _state){

    if(_state == I) return;
    assert(!CacheArray::CacheLine::inTransition(_state));
    
    CCLine* ccLine = ccLines_[_lineIndex]; assert(ccLine->valid());
    
    if(ccLine->isShareless() && !ccLine->ownerExists()) return;
    
    sendEvictionInvalidates(_lineIndex);
    
    if(ccLine->inTransition()){
        d_->debug(_L1_,"TopCC: Stalling request. Eviction requires invalidation of lw lvl caches. St = %s, OwnerExists = %s \n",
                        BccLineString[_state], ccLine->ownerExists() ? "True" : "False");
    }
}


int MESITopCC::sendInvalidates(int _lineIndex, string _requestingNode){
    CCLine* ccLine = ccLines_[_lineIndex];
    
    int sentInvalidates = 0;
    map<string, int>::iterator sharer;
    int requestingId = _requestingNode.empty() ? -1 : lowNetworkNodeLookupByName(_requestingNode);
    
    bool acksNeeded = (ccLine->ownerExists() || ccLine->acksNeeded_);
    
    if(ccLine->ownerExists()){
        assert(ccLine->isShareless());
        sentInvalidates = 1;
        string onwerName = lowNetworkNodeLookupById(ccLine->ownerId_);
        sendInvalidate(ccLine, onwerName, acksNeeded);
    }
    else{
        for(sharer = lowNetworkNameMap_.begin(); sharer != lowNetworkNameMap_.end(); sharer++){
            int sharerId = sharer->second;
            if(requestingId == sharerId){
                if(ccLine->isSharer(sharerId)) ccLine->removeSharer(sharerId);
                continue;
            }
            if(ccLine->isSharer(sharerId)){
                sentInvalidates++;
                sendInvalidate(ccLine, sharer->first, acksNeeded);
            }
        }
    }
    
    
    if(!acksNeeded) ccLine->removeAllSharers();
    else if(acksNeeded && sentInvalidates > 0) ccLine->setState(Inv_A);
    
    ccLine->acksNeeded_ = false;
    
    d_->debug(_L1_,"Number of invalidates sent: %u, number of sharers = %u\n", sentInvalidates,  ccLine->numSharers());
    return sentInvalidates;
}



void MESITopCC::sendInvalidate(CCLine* _cLine, string destination, bool _acksNeeded){
    MemEvent* invalidateEvent = new MemEvent((Component*)owner_, _cLine->getBaseAddr(), Inv);
    if(_acksNeeded) invalidateEvent->setAckNeeded();            //TODO: add comment as to why this is needed (ie weak vs strong consistency)
    invalidateEvent->setDst(destination);
    response resp = {invalidateEvent, timestamp_ + accessLatency_, false};
    outgoingEventQueue_.push(resp);
    
    d_->debug(_L1_,"Invalidate sent: Addr = %"PRIx64", Dst = %s\n", _cLine->getBaseAddr(),  destination.c_str());
}

void MESITopCC::sendEvictionInvalidates(int _lineIndex){
    int invalidatesSent = sendInvalidates(_lineIndex, "");
    evictionInvReqsSent_ += invalidatesSent;
}

void MESITopCC::sendCCInvalidates(int _lineIndex, string _requestingNode){
    int invalidatesSent = sendInvalidates(_lineIndex, _requestingNode);
    invReqsSent_ += invalidatesSent;
}

void MESITopCC::sendInvalidateX(int _lineIndex){
    CCLine* ccLine = ccLines_[_lineIndex];
    if(!ccLine->ownerExists()) return;
    
    ccLine->setState(InvX_A);
    string ownerName = lowNetworkNodeLookupById(ccLine->getOwnerId());

    invReqsSent_++;
    
    MemEvent* invalidateEvent = new MemEvent((Component*)owner_, ccLine->getBaseAddr(), InvX);
    invalidateEvent->setAckNeeded();
    invalidateEvent->setDst(ownerName);
    
    response resp = {invalidateEvent, timestamp_ + accessLatency_, false};
    outgoingEventQueue_.push(resp);
    
    d_->debug(_L1_,"InvalidateX sent: Addr = %"PRIx64", Dst = %s\n", ccLine->getBaseAddr(),  ownerName.c_str());
}




/*---------------------------------------------------------------------------------------------------
 * Helper Functions
 *--------------------------------------------------------------------------------------------------*/



void MESITopCC::handleGetSRequest(MemEvent* _event, CacheLine* _cacheLine, int _sharerId, bool _mshrHit, bool& _ret){
    vector<uint8_t>* data = _cacheLine->getData();
    BCC_MESIState state   = _cacheLine->getState();
    int lineIndex         = _cacheLine->index();
    CCLine* l             = ccLines_[lineIndex];

    /* Send Data in E state */
    if(protocol_ &&  !l->ownerExists() && l->isShareless() && (state == E || state == M)){
        l->setOwner(_sharerId);
        _ret = sendResponse(_event, E, data, _mshrHit);
    }
    
    /* If exclusive sharer exists, downgrade it to S state */
    else if(l->ownerExists()) {
        d_->debug(_L5_,"GetS Req: Exclusive sharer exists \n");
        assert(!l->isSharer(_sharerId));                    /* Cache should not ask for 'S' if its already Exclusive */
        sendInvalidateX(lineIndex);
    }
    /* Send Data in S state */
    else if(state == S || state == M || state == E){
        assert(!l->isSharer(_sharerId));
        l->addSharer(_sharerId);
        _ret = sendResponse(_event, S, data, _mshrHit);
    }
    else{
        _abort(MemHierarchy::CacheController, "Unkwown state!");
    }
}

void MESITopCC::handleGetXRequest(MemEvent* _event, CacheLine* _cacheLine, int _sharerId, bool _mshrHit, bool& _ret){
    BCC_MESIState state   = _cacheLine->getState();
    int lineIndex         = _cacheLine->index();
    CCLine* ccLine        = ccLines_[lineIndex];
    Command cmd           = _event->getCmd();

    /* Invalidate any exclusive sharers before responding to GetX request */
    if(ccLine->ownerExists()){
        d_->debug(_L5_,"GetX Req: Exclusive sharer exists \n");
        assert(ccLine->isShareless());
        sendCCInvalidates(lineIndex, _event->getSrc());
        return;
    }
    /* Sharers exist */
    else if(ccLine->numSharers() > 0){
        d_->debug(_L5_,"GetX Req:  Sharers 'S' exists \n");
        if(cmd == GetX){
            sendCCInvalidates(lineIndex, _event->getSrc());
            ccLine->removeAllSharers();   //Weak consistency model, no need to wait for InvAcks to proceed with request
        }
        else if(cmd == GetSEx){
            ccLine->setAcksNeeded();
            int invSent = sendInvalidates(lineIndex, _event->getSrc());
            if(invSent > 0) return;
        }
    }
    
    if(state == E || state == M){
        ccLine->setOwner(_sharerId);
        sendResponse(_event, M, _cacheLine->getData(), _mshrHit);
        _ret = true;
    }
}

void MESITopCC::handlePutMRequest(CCLine* _ccLine, Command _cmd, BCC_MESIState _state, int _sharerId, bool& _ret){
    _ret = true;
    assert(_state == M || _state == E);

    /* Remove exclusivity for all: PutM, PutX, PutE */
    if(_ccLine->ownerExists())  _ccLine->clearOwner();

    /*If PutX, keep as sharer since transition ia M->S */
    if(_cmd == PutX || _cmd == PutXE){  /* If PutX, then state = V since we only wanted to get rid of the M-state hglv cache */
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
    dbg->output(C,"- NACKs sent (MSHR Full, TopCC): %u\n", NACKsSent_);
    dbg->output(C,"- Invalidates sent (non-eviction): %u\n", invReqsSent_);
    dbg->output(C,"- Invalidates sent due to evictions: %u\n", evictionInvReqsSent_);
}


bool MESITopCC::willRequestPossiblyStall(int lineIndex, MemEvent* _event){
    CCLine* ccLine = ccLines_[lineIndex];
    if(ccLine->ownerExists() || !ccLine->isShareless()) return true;
    if(ccLine->getState() != V) return true;
    
    return false;
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
                base            = (_event->getAddr()) & ~(lineSize_ - 1);
                offset          = _event->getAddr() - base;
                responseEvent   = _event->makeResponse((SST::Component*)owner_);
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
    response resp  = {responseEvent, latency, true};
    outgoingEventQueue_.push(resp);
    return true;
}


void TopCacheController::sendNACK(MemEvent *_event){
    MemEvent *NACKevent = _event->makeNACKResponse((Component*)owner_, _event);
    uint64 latency      = timestamp_ + accessLatency_;
    response resp       = {NACKevent, latency, true};
    d_->debug(_L1_,"TopCC: Sending NACK: Cmd = %s, EventID = %"PRIx64", Addr = %"PRIx64", RespToID = %"PRIx64"\n", CommandString[_event->getCmd()], _event->getID().first, _event->getAddr(), NACKevent->getResponseToID().first);
    NACKsSent_++;
    outgoingEventQueue_.push(resp);

}


void TopCacheController::sendEvent(MemEvent *_event){
    uint64 latency      = timestamp_ + accessLatency_;
    response resp       = {_event, latency, true};
    d_->debug(_L1_,"TopCC: Sending Event To HgLvLCache. Cmd = %s, Src = %s, Addr = %"PRIx64"\n", CommandString[_event->getCmd()], _event->getSrc().c_str(), _event->getAddr());
    outgoingEventQueue_.push(resp);
}



int MESITopCC::lowNetworkNodeLookupByName(const std::string& _name){
	int id = -1;
	std::map<string, int>::iterator it = lowNetworkNameMap_.find(_name);
	if(lowNetworkNameMap_.end() == it) {
        id = lowNetworkNodeCount_++;
		lowNetworkNameMap_[_name] = id;
        lowNetworkIdMap_[id] = _name;
	} else {
		id = it->second;
	}
	return id;
}

std::string MESITopCC::lowNetworkNodeLookupById(int _id){
    std::map<int, string>::iterator it = lowNetworkIdMap_.find(_id);
    assert(lowNetworkIdMap_.end() != it);
    return it->second;
}




