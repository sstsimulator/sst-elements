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
using namespace SST::Interfaces;

/*-------------------------------------------------------------------------------------
 * Top Coherence Controller Implementation
 *-------------------------------------------------------------------------------------*/

bool TopCacheController::handleAccess(MemEvent* _event, CacheLine* _cacheLine, int _childId){
    Command cmd           = _event->getCmd();
    vector<uint8_t>* data = _cacheLine->getData();
    BCC_MESIState state   = _cacheLine->getState();
    switch(cmd){
        case GetS:
            if(state == S || state == M || state == E)
                return sendResponse(_event, S, data, _childId);
            break;
        case GetSEx:
        case GetX:
            if(state == M) return sendResponse(_event, M, data, _childId);
            break;
        default:
            _abort(MemHierarchy::CacheController, "Wrong command type!");
    }
    return false;
}

bool MESITopCC::handleAccess(MemEvent* _event, CacheLine* _cacheLine, int _childId){
    Command cmd    = _event->getCmd();
    CCLine* ccLine = ccLines_[_cacheLine->index()];
    bool ret       = false;

    switch(cmd){
        case GetS:
            processGetSRequest(_event, _cacheLine, _childId, ret);
            break;
        case GetSEx:
        case GetX:
            processGetXRequest(_event, _cacheLine, _childId, ret);
            break;
        case PutM:
        case PutE:
            processPutMRequest(ccLine, _cacheLine->getState(), _childId, ret);
            break;
        case PutS:
            processPutSRequest(ccLine, _childId, ret);
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
        l->setState(Inv_A);
        sendInvalidates(_cmd, _lineIndex, false, -1, true);
    }
    else{
        sendInvalidates(_cmd, _lineIndex, false, -1, false);
        l->removeAllSharers();
    }
    return;
}

void MESITopCC::handleFetchInvalidate(CacheLine* _cacheLine, Command _cmd){
    CCLine* l = ccLines_[_cacheLine->index()];
    if(!l->exclusiveSharerExists() && l->numSharers() == 0) return;

    switch(_cmd){
        case FetchInvalidate:
            if(l->exclusiveSharerExists()) {
                assert(l->numSharers() == 1);
                l->setState(Inv_A);            //M_A state now; wait for invalidate acks
                sendInvalidates(Inv, _cacheLine->index(), false, -1, true);
            }
            else if(l->numSharers() > 0){
                sendInvalidates(Inv, _cacheLine->index(), false, -1, false);
                l->setAckCount(0);
                l->removeAllSharers();
            }else{
                _abort(MemHierarchy::CacheController, "Command not supported");
            }
            break;
        case FetchInvalidateX:
            assert(0); //TODO
        default:
            _abort(MemHierarchy::CacheController, "Command not supported");
    }
}

void MESITopCC::handleInvAck(MemEvent* event, CCLine* ccLine, int childId){
    assert(ccLine->getAckCount() > 0);
    if(ccLine->exclusiveSharerExists()) ccLine->clearExclusiveSharer(childId);
    else if(ccLine->isSharer(childId)) ccLine->removeSharer(childId);
    ccLine->decAckCount();
}

/* Function sends invalidates to lower level caches, removes sharers if needed.  
 * Currently it implements weak consistency, ie. invalidates to sharers do not need acknowledgment
 * Returns true if eviction requires a response from Child, and false if no response is expected */
bool MESITopCC::handleEviction(int lineIndex,  BCC_MESIState _state){
    if(_state == I) return false;
    bool waitForInvalidateAck = false;
    assert(!CacheArray::CacheLine::inTransition(_state));
    CCLine* ccLine = ccLines_[lineIndex];
    assert(ccLine->getAckCount() == 0 && ccLine->valid());
    
    if(ccLine->exclusiveSharerExists()){
        waitForInvalidateAck = true;
        assert(!ccLine->isShareless());
    }
    if(!ccLine->isShareless()){
        d_->debug(_L1_,"Stalling request: Eviction requires invalidation of lw lvl caches. St = %s, ExSharerFlag = %s \n", BccLineString[_state], waitForInvalidateAck ? "True" : "False");
        if(waitForInvalidateAck){
            ccLine->setState(Inv_A);
            sendInvalidates(Inv, lineIndex, true, -1, true);
            return true;
        }
        else{
            assert(ccLine->exclusiveSharerExists() || _state != IM);
            assert(ccLine->getState() == V);
            sendInvalidates(Inv, lineIndex, true, -1, false);
            ccLine->removeAllSharers();
        }
    }
    return false;
}


void MESITopCC::sendInvalidates(Command cmd, int lineIndex, bool eviction, int requestLink, bool acksNeeded){
    CCLine* ccLine = ccLines_[lineIndex];
    assert(ccLine->getAckCount() == 0);
    assert(!ccLine->isShareless());  //no sharers for this address in the cache
    unsigned int sentInvalidates = 0;
    
    d_->debug(_L1_,"Sending Invalidates: %u (numSharers), Invalidating Addr: %#016llx\n", ccLine->numSharers(), (uint64_t)ccLine->getBaseAddr());
    MemEvent* invalidateEvent; 
        for(vector<Link*>::iterator it = childrenLinks_->begin(); it != childrenLinks_->end(); it++){
            Link* link = *it;
            int childId = getChildId(link);
            if(requestLink == childId) continue;
            if(ccLine->isSharer(childId)){
                sentInvalidates++;
                if(!eviction) InvReqsSent_++;
                else EvictionInvReqsSent_++;
                invalidateEvent = new MemEvent((Component*)owner_, ccLine->getBaseAddr(), cmd);
                response resp = {link, invalidateEvent, timestamp_ + accessLatency_, false};
                outgoingEventQueue_.push(resp);
            }
    }
    //assert(sentInvalidates == ccLine->numSharers());   - 1 (requestLink)
    if(acksNeeded) ccLine->setAckCount(sentInvalidates);
}



/*---------------------------------------------------------------------------------------------------
 * Helper Functions
 *--------------------------------------------------------------------------------------------------*/



void MESITopCC::processGetSRequest(MemEvent* _event, CacheLine* _cacheLine, int _childId,bool& ret){
    vector<uint8_t>* data = _cacheLine->getData();
    BCC_MESIState state   = _cacheLine->getState();
    int lineIndex         = _cacheLine->index();
    CCLine* l             = ccLines_[_cacheLine->index()];

    /* Send Data in E state */
    if(l->isShareless() && (state == E || state == M)){
        if(protocol_){
            l->setExclusiveSharer(_childId);
            ret = sendResponse(_event, E, data, _childId);
        }
        else{
            l->addSharer(_childId);
            ret = sendResponse(_event, S, data, _childId);
        }
    }
    /*if(protocol_ && l->isShareless() && (state == E || state == M)){
        l->setExclusiveSharer(_childId);
        ret = sendResponse(_event, E, data, _childId);
    }
    */
    /* If exclusive sharer exists, downgrade it to S state */
    else if(l->exclusiveSharerExists()) {
        d_->debug(_L5_,"GetS Req: Exclusive sharer exists \n");
        assert(!l->isSharer(_childId));
        assert(l->numSharers() == 1);                      // TODO: l->setState(InvX_A);  //sendInvalidates(InvX, lineIndex);
        l->setState(Inv_A);
        sendInvalidates(Inv, lineIndex, false, -1, true);
    }
    /* Send Data in S state */
    else if(state == S || state == M || state == E){
        l->addSharer(_childId);
        ret = sendResponse(_event, S, data, _childId);
    }
    else{
        _abort(MemHierarchy::CacheController, "Unkwown state!");
    }
}

void MESITopCC::processGetXRequest(MemEvent* _event, CacheLine* _cacheLine, int _childId, bool& _ret){
    BCC_MESIState state   = _cacheLine->getState();
    int lineIndex         = _cacheLine->index();
    CCLine* ccLine        = ccLines_[lineIndex];

    /* Invalidate any exclusive sharers before responding to GetX request */
    if(ccLine->exclusiveSharerExists()){
        d_->debug(_L5_,"GetX Req: Exclusive sharer exists \n");
        ccLine->setState(Inv_A);         
        sendInvalidates(Inv, lineIndex, false, _childId, true);
        return;
    }
    /* Sharers exist */
    else if(ccLine->numSharers() > 0){
        d_->debug(_L5_,"GetX Req:  Sharers 'S' exists \n");
        sendInvalidates(Inv, lineIndex, false, _childId, false);
        ccLine->removeAllSharers();   //Weak consistency model, no need to wait for InvAcks to proceed with request
    }
    
    if(state == E || state == M){
        ccLine->setExclusiveSharer(_childId);
        sendResponse(_event, M, _cacheLine->getData(), _childId);
        _ret = true;
    }
}


void MESITopCC::processPutMRequest(CCLine* _ccLine, BCC_MESIState _state, int _childId, bool& _ret){
    _ret = true;
    assert(_state == M || _state == E);
    if(!(_ccLine->isSharer(_childId))) return;
    _ccLine->clearExclusiveSharer(_childId);
}

void MESITopCC::processPutSRequest(CCLine* _ccLine, int _childId, bool& _ret){
    _ret = true;
    if(_ccLine->isSharer(_childId)) _ccLine->removeSharer(_childId);
}

void MESITopCC::printStats(int _stats){
    Output* dbg = new Output();
    dbg->init("", 0, 0, (Output::output_location_t)_stats);
    dbg->output(C,"Invalidates sent (non-eviction): %u\n", InvReqsSent_);
    dbg->output(C,"Invalidates sent due to evictions: %u\n", EvictionInvReqsSent_);
}


int MESITopCC::getChildId(Link* _childLink){
    for(size_t i = 0; i < childrenLinks_->size(); i++){
        if((Link*)childrenLinks_->at(i) == _childLink){
            return i;
        }
    }
    _abort(MemHierarchy::Cache, "Panic at the disco! ?!?!?!?!");
    return -1;
}

//TODO: Fix/Refactor this mess!
bool TopCacheController::sendResponse(MemEvent *_event, BCC_MESIState _newState, std::vector<uint8_t>* _data, int _childId){
    if(_event->isPrefetch() || _childId == -1){
         d_->debug(_WARNING_,"Warning: No Response sent! Thi event is a prefetch or childId in -1");
        return true;
    }
    MemEvent *responseEvent; Command cmd = _event->getCmd();
    Addr offset, base;
    switch(cmd){
        case GetS: case GetSEx:
            assert(_data);
            if(L1_){
                base = (_event->getAddr()) & ~(lineSize_ - 1);
                offset = _event->getAddr() - base;
                responseEvent = _event->makeResponse((SST::Component*)owner_);
                responseEvent->setPayload(_event->getSize(), &_data->at(offset));
            }
            else responseEvent = _event->makeResponse((SST::Component*)owner_, *_data, _newState);
        break;
        case GetX:
            if(L1_){
                base = (_event->getAddr()) & ~(lineSize_ - 1);
                offset = _event->getAddr() - base;
                responseEvent = _event->makeResponse((SST::Component*)owner_);
                responseEvent->setPayload(_event->getSize(), &_data->at(offset));
            }
            else    responseEvent = _event->makeResponse((SST::Component*)owner_, *_data, _newState); //TODO: make sure to look at "Given State" before setting new state at receivingn end
            break;
        default:
            _abort(CoherencyController, "Command not valid as a response. \n");
    }
    if(L1_ && (cmd == GetS || cmd == GetSEx)) printData(d_, "Response Data", _data, offset, (int)_event->getSize());
    else printData(d_, "Response Data", _data);
    d_->debug(_L1_,"Sending Response:  Addr = %#016llx,  Dst = %s, Size = %i, Granted State = %s\n", (uint64_t)_event->getAddr(), responseEvent->getDst().c_str(), responseEvent->getSize(), BccLineString[responseEvent->getGrantedState()]);
    Link* deliveryLink = _event->getDeliveryLink();
    uint64_t deliveryTime = _event->queryFlag(MemEvent::F_UNCACHED) ? timestamp_ : timestamp_ + accessLatency_;
    response resp = {deliveryLink, responseEvent, deliveryTime, true};
    outgoingEventQueue_.push(resp);
    return true;
}

