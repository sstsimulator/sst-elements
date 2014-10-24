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
    State state           = _cacheLine->getState();
    bool atomic           = _cacheLine->isAtomic();

    switch(cmd){
        case GetS:
            if(state == S || state == M || state == E)
                return sendResponse(_event, S, data,  _mshrHit);
            break;
        case GetX:
        case GetSEx:
            if(state == M){
                if(_event->isStoreConditional()) return sendResponse(_event, M, data, _mshrHit, atomic);
                else                             return sendResponse(_event, M, data, _mshrHit);
            }
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
    Command cmd    = _event->getCmd();
    int id         = lowNetworkNodeLookupByName(_event->getSrc());
    CCLine* ccLine = ccLines_[_cacheLine->getIndex()];
    bool ret       = false;

    if(ccLine->inTransition() && !_event->isWriteback()){
        d_->debug(_L7_,"TopCC: Stalling request, ccLine in transition \n");
        return false;
    }

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



void MESITopCC::handleInvalidate(int _lineIndex, string _origRqstr, Command _cmd, bool _mshrHit){
    CCLine* ccLine = ccLines_[_lineIndex];
    if(ccLine->isShareless() && !ccLine->ownerExists()) return;
    
    switch(_cmd){
        case Inv:
        case FetchInv:
            sendInvalidates(_lineIndex, "", _origRqstr, _mshrHit);
            break;
        case InvX:
        case FetchInvX:
            sendInvalidateX(_lineIndex, _origRqstr, _mshrHit);
            break;
        default:
            _abort(MemHierarchy::CacheController, "Command not supported, Cmd = %s", CommandString[_cmd]);
    }
}



void MESITopCC::handleEviction(int _lineIndex, string _origRqstr, State _state){
    CCLine* ccLine = ccLines_[_lineIndex];
    assert(!CacheArray::CacheLine::inTransition(_state));
    assert(ccLine->valid());
    
    /* if state is invalid OR, there's no shares or owner, then no need to send invalidations */
    if(_state == I || (ccLine->isShareless() && !ccLine->ownerExists())) return;
    
    /* Send invalidates */
    sendEvictionInvalidates(_lineIndex, _origRqstr, false);
    
    if(ccLine->inTransition()){
        d_->debug(_L7_,"TopCC: Stalling request. Eviction requires invalidation of lw lvl caches. St = %s, OwnerExists = %s \n",
                        BccLineString[_state], ccLine->ownerExists() ? "True" : "False");
    }
}



int MESITopCC::sendInvalidates(int _lineIndex, string _srcNode, string _origRqstr, bool _mshrHit){
    CCLine* ccLine = ccLines_[_lineIndex];
    
    int sentInvalidates = 0;
    map<string, int>::iterator sharer;
    int requestingId = _srcNode.empty() ? -1 : lowNetworkNodeLookupByName(_srcNode);
    
    bool acksNeeded = (ccLine->ownerExists() || ccLine->acksNeeded_);
    
    if(ccLine->ownerExists()){
        sentInvalidates = 1;
        string ownerName = lowNetworkNodeLookupById(ccLine->ownerId_);
        sendInvalidate(ccLine, ownerName, _origRqstr, acksNeeded, _mshrHit);
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
                sendInvalidate(ccLine, sharer->first, _origRqstr, acksNeeded, _mshrHit);
            }
        }
    }
    
    
    if(!acksNeeded) ccLine->removeAllSharers();
    else if(acksNeeded && sentInvalidates > 0) ccLine->setState(Inv_A);
    
    ccLine->acksNeeded_ = false;
    
    d_->debug(_L7_,"Number of invalidates sent: %u, number of sharers = %u\n", sentInvalidates,  ccLine->numSharers());
    return sentInvalidates;
}



void MESITopCC::sendInvalidate(CCLine* _cLine, string destination, string _origRqstr, bool _acksNeeded, bool _mshrHit){
    MemEvent* invalidateEvent = new MemEvent((Component*)owner_, _cLine->getBaseAddr(), _cLine->getBaseAddr(), Inv);
    if(_acksNeeded) invalidateEvent->setAckNeeded();            //TODO: add comment as to why this is needed (ie weak vs strong consistency)
    invalidateEvent->setDst(destination);
    invalidateEvent->setRqstr(_origRqstr);

    uint64_t deliveryTime = (_mshrHit) ? timestamp_ + mshrLatency_ : timestamp_ + accessLatency_;
    Response resp = {invalidateEvent, deliveryTime, false};
    addToOutgoingQueue(resp);
    
    d_->debug(_L7_,"TCC - Invalidate sent: Delivery = %"PRIu64", Current = %"PRIu64", Addr = %"PRIx64", Dst = %s\n", deliveryTime, timestamp_, _cLine->getBaseAddr(),  destination.c_str());
}



void MESITopCC::sendEvictionInvalidates(int _lineIndex, string _origRqstr, bool _mshrHit){
    int invalidatesSent = sendInvalidates(_lineIndex, "", _origRqstr, _mshrHit);
    evictionInvReqsSent_ += invalidatesSent;
}



void MESITopCC::sendCCInvalidates(int _lineIndex, string _srcNode, string _origRqstr, bool _mshrHit){
    int invalidatesSent = sendInvalidates(_lineIndex, _srcNode, _origRqstr, _mshrHit);
    invReqsSent_ += invalidatesSent;
}



void MESITopCC::sendInvalidateX(int _lineIndex, string _origRqstr, bool _mshrHit){
    CCLine* ccLine = ccLines_[_lineIndex];
    if(!ccLine->ownerExists()) return;
    
    ccLine->setState(InvX_A);
    string ownerName = lowNetworkNodeLookupById(ccLine->getOwnerId());

    invReqsSent_++;
    
    MemEvent* invalidateEvent = new MemEvent((Component*)owner_, ccLine->getBaseAddr(), ccLine->getBaseAddr(), InvX);
    invalidateEvent->setAckNeeded();
    invalidateEvent->setDst(ownerName);
    invalidateEvent->setRqstr(_origRqstr); 

    uint64_t deliveryTime = (_mshrHit) ? timestamp_ + mshrLatency_ : timestamp_ + accessLatency_;

    Response resp = {invalidateEvent, deliveryTime, false};
    addToOutgoingQueue(resp);
    
    d_->debug(_L7_,"InvalidateX sent: Addr = %"PRIx64", Dst = %s\n", ccLine->getBaseAddr(),  ownerName.c_str());
}




/*---------------------------------------------------------------------------------------------------
 * Helper Functions
 *--------------------------------------------------------------------------------------------------*/

void MESITopCC::handleGetSRequest(MemEvent* _event, CacheLine* _cacheLine, int _sharerId, bool _mshrHit, bool& _ret){
    vector<uint8_t>* data = _cacheLine->getData();
    State state           = _cacheLine->getState();
    int lineIndex         = _cacheLine->getIndex();
    CCLine* l             = ccLines_[lineIndex];

    /* Send Data in E state */
    if(protocol_ &&  !l->ownerExists() && l->isShareless() && (state == E || state == M)){
        l->setOwner(_sharerId);
        _ret = sendResponse(_event, E, data, _mshrHit);
    }
    
    /* If exclusive sharer exists, downgrade it to S state */
    else if(l->ownerExists()) {
        d_->debug(_L7_,"GetS Req but exclusive cache exists \n");
        sendInvalidateX(lineIndex, _event->getRqstr(), _mshrHit);
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
    State state     = _cacheLine->getState();
    int lineIndex   = _cacheLine->getIndex();
    CCLine* ccLine  = ccLines_[lineIndex];
    Command cmd     = _event->getCmd();
    bool respond    = true;
    int invSent;
    
    /* Do we have the appropriate state */
    if(!(state == M || state == E)) respond = false;
    /* Invalidate any exclusive sharers before responding to GetX request */
    else if(ccLine->ownerExists()){
        d_->debug(_L7_,"GetX Req recieived but exclusive cache exists \n");
        assert(ccLine->isShareless());
        sendCCInvalidates(lineIndex, _event->getSrc(), _event->getRqstr(), _mshrHit);
        respond = false;
    }
    /* Sharers exist */
    else if(ccLine->numSharers() > 0){
        d_->debug(_L7_,"GetX Req recieved but sharers exists \n");
        switch(cmd){
            case GetX:
                sendCCInvalidates(lineIndex, _event->getSrc(), _event->getRqstr(), _mshrHit);
                ccLine->removeAllSharers();   //Weak consistency model, no need to wait for InvAcks to proceed with request
                respond = true;
                break;
            case GetSEx:
                ccLine->setAcksNeeded();
                invSent = sendInvalidates(lineIndex, _event->getSrc(), _event->getRqstr(), _mshrHit);
                respond = (invSent > 0) ? false : true;
                break;
            default:
                _abort(MemHierarchy::CacheController, "Unkwown state!");
        }
    }
    /* Sharers don't exist, no need to send invalidates */
    else respond = true;
    
    if(respond){
        ccLine->setOwner(_sharerId);
        sendResponse(_event, M, _cacheLine->getData(), _mshrHit);
        _ret = true;
    }

}



void MESITopCC::handlePutMRequest(CCLine* _ccLine, Command _cmd, State _state, int _sharerId, bool& _ret){
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
    if(_ccLine->isSharer(_sharerId)) _ccLine->removeSharer(_sharerId);
    else{
        _ret = false;                   /* Corner case example:  L2 sends invalidates and at the same time L1 sends evict.  In the case, it is important to ignore this PutS request */
        return;
    }
    
    if(_ccLine->isShareless()) _ret = true;
    else                       _ret = false;
}



void MESITopCC::printStats(int _stats){
    Output* dbg = new Output();
    dbg->init("", 0, 0, (Output::output_location_t)_stats);
    dbg->output(CALL_INFO,"- NACKs sent (MSHR Full, TopCC):                  %u\n", NACKsSent_);
    dbg->output(CALL_INFO,"- Invalidates sent (non-eviction):                %u\n", invReqsSent_);
    dbg->output(CALL_INFO,"- Invalidates sent due to evictions:              %u\n", evictionInvReqsSent_);
}



bool MESITopCC::willRequestPossiblyStall(int lineIndex, MemEvent* _event){
    CCLine* ccLine = ccLines_[lineIndex];
    if(ccLine->ownerExists() || !ccLine->isShareless()) return true;
    if(ccLine->getState() != V) return true;
    
    return false;
}



bool TopCacheController::sendResponse(MemEvent *_event, State _newState, std::vector<uint8_t>* _data, bool _mshrHit, bool _finishedAtomically){
    //if(_event->isPrefetch()) return true;
    
    Command cmd = _event->getCmd();
    MemEvent * responseEvent = _event->makeResponse(_newState);
    responseEvent->setDst(_event->getSrc());
    bool noncacheable = _event->queryFlag(MemEvent::F_NONCACHEABLE);
    
    if(L1_ && !noncacheable){
        /* Only return the desire word */
        Addr base    = (_event->getAddr()) & ~(((Addr)lineSize_) - 1);
        Addr offset  = _event->getAddr() - base;
        if(cmd != GetX) responseEvent->setPayload(_event->getSize(), &_data->at(offset));
        else{
            /* If write (GetX) and LLSC set, then check if operation was Atomic */
  	    if(_finishedAtomically) responseEvent->setAtomic(true);
            else responseEvent->setAtomic(false);
        }
    }
    else responseEvent->setPayload(*_data);
    
    uint64_t deliveryTime = (_mshrHit) ? timestamp_ + mshrLatency_ : timestamp_ + accessLatency_;
    Response resp = {responseEvent, deliveryTime, true};
    addToOutgoingQueue(resp);
    
    d_->debug(_L3_,"TCC - Sending Response at cycle = %"PRIu64". Current Time = %"PRIu64", Addr = %"PRIx64", Dst = %s, Size = %i, Granted State = %s\n", deliveryTime, timestamp_, _event->getAddr(), responseEvent->getDst().c_str(), responseEvent->getSize(), BccLineString[responseEvent->getGrantedState()]);
    return true;
}



void TopCacheController::sendNACK(MemEvent *_event){
    MemEvent *NACKevent = _event->makeNACKResponse((Component*)owner_, _event);
    
    uint64 deliveryTime      = timestamp_ + accessLatency_;
    Response resp       = {NACKevent, deliveryTime, true};
    addToOutgoingQueue(resp);
    
    NACKsSent_++;
    d_->debug(_L3_,"TCC - Sending NACK\n");
}



void TopCacheController::sendEvent(MemEvent *_event){
    uint64 deliveryTime = timestamp_ + accessLatency_;
    Response resp       = {_event, deliveryTime, true};
    addToOutgoingQueue(resp);
    
    d_->debug(_L3_,"TCC - Sending Event To HgLvLCache at cycle = %"PRIu64". Cmd = %s\n", deliveryTime, CommandString[_event->getCmd()]);
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




