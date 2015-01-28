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
 * TCC for the L1 cache
 * Handles all protocols
 *-------------------------------------------------------------------------------------*/

/**
 *  Handles requests to the L1 cache, protocol agnostic
 *  @return whether the L1 sent a response to the processor
 */
bool TopCacheController::handleRequest(MemEvent* _event, CacheLine* _cacheLine, bool _mshrHit){
    Command cmd           = _event->getCmd();
    vector<uint8_t>* data = _cacheLine->getData();
    State state           = _cacheLine->getState();
    bool atomic           = _cacheLine->isAtomic();

    switch(cmd){
        case GetS:
            if(state == S || state == M || state == E || state == O) {
                sendResponse(_event, S, data,  _mshrHit);
                return true;
            }
            break;
        case GetX:
        case GetSEx:
            if(state == M){
                if(_event->isStoreConditional()) sendResponse(_event, M, data, _mshrHit, atomic);
                else                             sendResponse(_event, M, data, _mshrHit);
                return true;
            }
            break;
        default:
            d_->fatal(CALL_INFO,-1,"TCC received an unrecognized request: %s\n",CommandString[cmd]);
    }
    return false;
}


/*-------------------------------------------------------------------------------------
 * MESI Top Coherence Controller Implementation
 * TCC for all caches below the L1
 * Handles MSI & MESI protocols
 *-------------------------------------------------------------------------------------*/

/*********************************
 * Methods for request handling
 *********************************/
/**
 * Send requests to appropriate handler
 * @return whether TCC was able to handle the request given the current coherence state
 */
bool MESITopCC::handleRequest(MemEvent* _event, CacheLine* _cacheLine, bool _mshrHit) {
    Command cmd    = _event->getCmd();
    int id         = lowNetworkNodeLookupByName(_event->getSrc());
    CCLine* ccLine = ccLines_[_cacheLine->getIndex()];

    if(ccLine->inTransition() && !_event->isWriteback()){
        d_->debug(_L7_,"TopCC: Stalling request, ccLine in transition \n");
        return false;
    }

    switch(cmd){
        case GetS:
            return handleGetSRequest(_event, _cacheLine, id, _mshrHit);
            break;
        case GetX:
        case GetSEx:
            return handleGetXRequest(_event, _cacheLine, id, _mshrHit);
            break;
        case PutS:
            return handlePutSRequest(ccLine, id);
            break;
        case PutM:
        case PutE:
        case PutX:
        case PutXE:
            handlePutMRequest(ccLine, cmd, _cacheLine->getState(), id);
            return true;
            break;
        default:
            d_->fatal(CALL_INFO,-1,"TCC received an unrecognized request: %s\n",CommandString[cmd]);
    }

    return false;
}


/**
 *  Handles GetS requests
 *      Only called when the line is present in the cache
 *      Un-owned: Update sharer/owner info & send data to requestor
 *      Owned: send invalidation to line owner
 *  @return whether the handler responded to the requestor
 */
bool MESITopCC::handleGetSRequest(MemEvent* _event, CacheLine* _cacheLine, int _sharerId, bool _mshrHit) {
    vector<uint8_t>* data = _cacheLine->getData();
    State state           = _cacheLine->getState();
    int lineIndex         = _cacheLine->getIndex();
    CCLine* l             = ccLines_[lineIndex];

    /* Send Data in E state */
    if(protocol_ &&  !l->ownerExists() && l->isShareless() && (state == E || state == M)){
        l->setOwner(_sharerId);
        sendResponse(_event, E, data, _mshrHit);
        return true;
    }
    /* If exclusive owner exists, downgrade it to S state */
    else if(l->ownerExists()) {
        d_->debug(_L7_,"GetS request but exclusive cache exists \n");
        sendInvalidateX(lineIndex, _event->getRqstr(), _mshrHit);
        return false;
    }
    /* Send Data in S state */
    else if(state == S || state == M || state == E){
        assert(!l->isSharer(_sharerId));
        l->addSharer(_sharerId);
        sendResponse(_event, S, data, _mshrHit);
        return true;
    }
    else{
        d_->fatal(CALL_INFO,-1,"TCC is handling a GetS request but coherence state is not valid and stable: %s\n",BccLineString[state]);
    }
    return false;
}


/**
 *  Handles GetX requests
 *      Only called when line is present in the cache
 *      If no write permission - return false
 *      If owner exists - invalidate owner
 *      If sharers exist - invalidate sharers
 *      If don't need to wait for acks, respond to requestor
 *  @return whether response was sent to requestor 
 */
bool MESITopCC::handleGetXRequest(MemEvent* _event, CacheLine* _cacheLine, int _sharerId, bool _mshrHit){
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
                d_->fatal(CALL_INFO,-1,"TCC received an unrecognized GetX request: %s\n",CommandString[cmd]);
        }
    }
    /* Sharers don't exist, no need to send invalidates */
    else respond = true;
    
    if(respond){
        ccLine->setOwner(_sharerId);
        sendResponse(_event, M, _cacheLine->getData(), _mshrHit);
        return true;
    }
    return false;
}


/**
 *  Handles replacement & downgrade requests when cache has exclusive access (PutM, PutX, PutE, PutXE)
 */
void MESITopCC::handlePutMRequest(CCLine* _ccLine, Command _cmd, State _state, int _sharerId){
    assert(_state == M || _state == E);

    /* Remove exclusivity for all: PutM, PutX, PutE */
    if(_ccLine->ownerExists())  _ccLine->clearOwner();

    /*If PutX, keep as sharer since transition ia M->S */
    if(_cmd == PutX || _cmd == PutXE){  /* If PutX, then state = V since we only wanted to get rid of the M-state hglv cache */
        _ccLine->addSharer(_sharerId);
        _ccLine->setState(V);
     }
    else _ccLine->updateState();        /* If PutE, PutM -> num sharers should be 0 before state goes to (V) */

}


/**
 *  Handles replacement requests when cache has shared access (PutS)
 *  @return whether the request was handled & the block is now idle 
 *  Because the MESI protocol uses Put requests in place of Acks, we need to know when to attempt
 *  retrying a waiting request that sent out invalidations.
 */
bool MESITopCC::handlePutSRequest(CCLine* _ccLine, int _sharerId){
    if(_ccLine->isSharer(_sharerId)) _ccLine->removeSharer(_sharerId);
    else{
        /* Corner case example:  L2 sends invalidates and at the same time L1 sends eviction.  In the case, it is important to ignore this PutS request */
        return false;                   
    }
    
    if(_ccLine->isShareless()) return true;
    else                       return false;
}





/*
 * Send invalidation requests to appropriate handler
 *  For data invalidations -> sendInvalidates
 *  For write permission invalidation -> sendInvalidateX
 */
void MESITopCC::handleInvalidate(int _lineIndex, MemEvent* event, string _origRqstr, Command _cmd, bool _mshrHit){
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
            d_->fatal(CALL_INFO,-1,"TCC received an unrecognized invalidation request: %s\n",CommandString[_cmd]);
    }
}


/*
 * Handle evictions
 */
void MESITopCC::handleEviction(int _lineIndex, string _origRqstr, State _state){
    CCLine* ccLine = ccLines_[_lineIndex];
    assert(!CacheArray::CacheLine::inTransition(_state));
    assert(ccLine->valid());
    
    /* if state is invalid OR, there's no sharers or owner, then no need to send invalidations */
    if(_state == I || (ccLine->isShareless() && !ccLine->ownerExists())) return;
    
    /* Send invalidates */
    sendEvictionInvalidates(_lineIndex, _origRqstr, false);
    
    if(ccLine->inTransition()){
        d_->debug(_L7_,"TopCC: Stalling request. Eviction requires invalidation of upper level caches. St = %s, OwnerExists = %s \n",
                        BccLineString[_state], ccLine->ownerExists() ? "True" : "False");
    }
}

/**********************************
 *  Methods for sending messages
 **********************************/
/*
 *  Send invalidates
 *  Return: count of invalidates sent
 */
int MESITopCC::sendInvalidates(int _lineIndex, string _srcNode, string _origRqstr, bool _mshrHit){
    CCLine* ccLine = ccLines_[_lineIndex];
    
    int sentInvalidates = 0;
    map<string, int>::iterator sharer;
    int requestingId = _srcNode.empty() ? -1 : lowNetworkNodeLookupByName(_srcNode);
    
    bool acksNeeded = (ccLine->ownerExists() || ccLine->acksNeeded_);
    
    if (ccLine->ownerExists()) {
        sentInvalidates = 1;
        string ownerName = lowNetworkNodeLookupById(ccLine->ownerId_);
        sendInvalidate(ccLine, ownerName, _origRqstr, acksNeeded, _mshrHit);
    } else {
        for(sharer = lowNetworkNameMap_.begin(); sharer != lowNetworkNameMap_.end(); sharer++) {
            int sharerId = sharer->second;
            if (requestingId == sharerId) {
                if (ccLine->isSharer(sharerId)) ccLine->removeSharer(sharerId);
                continue;
            }
            if(ccLine->isSharer(sharerId)){
                sentInvalidates++;
                sendInvalidate(ccLine, sharer->first, _origRqstr, acksNeeded, _mshrHit);
            }
        }
    }
    
    if (!acksNeeded) ccLine->removeAllSharers();
    else if (acksNeeded && sentInvalidates > 0) ccLine->setState(Inv_A);
    
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
    
    d_->debug(_L7_,"TCC - Invalidate sent: Delivery = %" PRIu64 ", Current = %" PRIu64 ", Addr = %" PRIx64 ", Dst = %s\n", deliveryTime, timestamp_, _cLine->getBaseAddr(),  destination.c_str());
}



void MESITopCC::sendEvictionInvalidates(int _lineIndex, string _origRqstr, bool _mshrHit){
    int invalidatesSent = sendInvalidates(_lineIndex, "", _origRqstr, _mshrHit);
    profileReqSent(Inv, true, invalidatesSent);
}



void MESITopCC::sendCCInvalidates(int _lineIndex, string _srcNode, string _origRqstr, bool _mshrHit){
    int invalidatesSent = sendInvalidates(_lineIndex, _srcNode, _origRqstr, _mshrHit);
    profileReqSent(Inv, false, invalidatesSent);
}



void MESITopCC::sendInvalidateX(int _lineIndex, string _origRqstr, bool _mshrHit){
    CCLine* ccLine = ccLines_[_lineIndex];
    if(!ccLine->ownerExists()) return;
    
    ccLine->setState(InvX_A);
    string ownerName = lowNetworkNodeLookupById(ccLine->getOwnerId());

    
    MemEvent* invalidateEvent = new MemEvent((Component*)owner_, ccLine->getBaseAddr(), ccLine->getBaseAddr(), InvX);
    invalidateEvent->setAckNeeded();
    invalidateEvent->setDst(ownerName);
    invalidateEvent->setRqstr(_origRqstr); 

    uint64_t deliveryTime = (_mshrHit) ? timestamp_ + mshrLatency_ : timestamp_ + accessLatency_;

    Response resp = {invalidateEvent, deliveryTime, false};
    addToOutgoingQueue(resp);
    profileReqSent(InvX, false, 1);
    
    d_->debug(_L7_,"InvalidateX sent: Addr = %" PRIx64 ", Dst = %s\n", ccLine->getBaseAddr(),  ownerName.c_str());
}




/*---------------------------------------------------------------------------------------------------
 * Helper Functions
 *--------------------------------------------------------------------------------------------------*/

bool MESITopCC::isCoherenceMiss(MemEvent* _event, CacheLine* _cacheLine) {
    State state     = _cacheLine->getState();
    int lineIndex   = _cacheLine->getIndex();
    CCLine* l       = ccLines_[lineIndex];
    Command cmd     = _event->getCmd();
    int requestor =  lowNetworkNodeLookupByName(_event->getSrc());
    
    if (l->inTransition()) return true;
    if (l->ownerExists() || state == I) return true;
    if (state == S && cmd != GetS) return true;
    if (cmd == GetSEx && l->numSharers() > 0 && !(l->numSharers() == 1 && l->isSharer(requestor))) return true;
    return false;
}

void MESITopCC::printStats(int _stats){
    Output* dbg = new Output();
    dbg->init("", 0, 0, (Output::output_location_t)_stats);
    dbg->output(CALL_INFO,"- NACKs sent (MSHR Full, TopCC):                  %u\n", NACKsSent_);
    dbg->output(CALL_INFO,"- InvalidateX sent                                %u\n", invXReqsSent_);
    dbg->output(CALL_INFO,"- Invalidates sent (non-eviction):                %u\n", invReqsSent_);
    dbg->output(CALL_INFO,"- Invalidates sent due to evictions:              %u\n", evictionInvReqsSent_);
}



bool MESITopCC::willRequestPossiblyStall(int lineIndex, MemEvent* _event){
    CCLine* ccLine = ccLines_[lineIndex];
    if(ccLine->ownerExists() || !ccLine->isShareless()) return true;
    if(ccLine->getState() != V) return true;
    
    return false;
}


/**
 *  Send response to requestor
 *  Latency: 
 *      MSHR hit: means we got a response with data from some other cache so no need for cache array access -> mshr latency
 *      MSHR miss: access to cache array to get data
 */
void TopCacheController::sendResponse(MemEvent *_event, State _newState, std::vector<uint8_t>* _data, bool _mshrHit, bool _finishedAtomically){
    
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
    
    uint64_t deliveryTime = timestamp_ + (_mshrHit ? mshrLatency_ : accessLatency_);
    Response resp = {responseEvent, deliveryTime, true};
    addToOutgoingQueue(resp);
    
    d_->debug(_L3_,"TCC - Sending Response at cycle = %" PRIu64 ". Current Time = %" PRIu64 ", Addr = %" PRIx64 ", Dst = %s, Size = %i, Granted State = %s\n", deliveryTime, timestamp_, _event->getAddr(), responseEvent->getDst().c_str(), responseEvent->getSize(), BccLineString[responseEvent->getGrantedState()]);
}


/**
 *  Handles: sending a NACK to an upper level cache
 *  Latency: tag access to determine whether event needs to be NACKed
 */
void TopCacheController::sendNACK(MemEvent *_event){
    MemEvent *NACKevent = _event->makeNACKResponse((Component*)owner_, _event);
    
    uint64 deliveryTime      = timestamp_ + tagLatency_;
    Response resp       = {NACKevent, deliveryTime, true};
    addToOutgoingQueue(resp);
    
    profileReqSent(NACK,false);
    d_->debug(_L3_,"TCC - Sending NACK\n");
}


/*
 *  Handles: resending a nacked event
 *  Latency: Find original event in MSHRs
 */
void TopCacheController::resendEvent(MemEvent *_event){
    uint64 deliveryTime = timestamp_ + mshrLatency_;
    Response resp       = {_event, deliveryTime, true};
    addToOutgoingQueue(resp);
    
    d_->debug(_L3_,"TCC - Sending Event To HgLvLCache at cycle = %" PRIu64 ". Cmd = %s\n", deliveryTime, CommandString[_event->getCmd()]);
}


void MESITopCC::profileReqSent(Command _cmd, bool _eviction, int _num) {
    switch(_cmd) {
        case Inv:
            if (_eviction) evictionInvReqsSent_ += _num;
            else invReqsSent_ += _num;
            break;
        case InvX:
            invXReqsSent_ += _num;
            break;
        case NACK:
            NACKsSent_ += _num;
            break;
        default:
            break;
    }
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




