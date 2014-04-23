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
 * File:   MESIBottomCoherenceController.cc
 * Author: Caesar De la Paz III
 * Email:  caesar.sst@gmail.com
 */

#include <sst_config.h>
#include <vector>
#include "coherenceControllers.h"
#include "MESIBottomCoherenceController.h"
using namespace SST;
using namespace SST::MemHierarchy;

/*----------------------------------------------------------------------------------------------------------------------
 * Bottom Coherence Controller Implementation
 *---------------------------------------------------------------------------------------------------------------------*/
	    
void MESIBottomCC::handleEviction(MemEvent* event, CacheLine* wbCacheLine){
	BCC_MESIState state = wbCacheLine->getState();
    updateEvictionStats(state);
    
    switch(state){
	case S:
		wbCacheLine->setState(I);
        sendWriteback(PutS, wbCacheLine);
		break;
	case M:
		wbCacheLine->setState(I);
		sendWriteback(PutM, wbCacheLine);
		break;
    case E:
		wbCacheLine->setState(I);
        sendWriteback(PutE, wbCacheLine);
        break;
	default:
		_abort(MemHierarchy::CacheController, "Not a valid state: %s", BccLineString[state]);
    }
}


void MESIBottomCC::handleAccess(MemEvent* _event, CacheLine* _cacheLine, Command _cmd){
    switch(_cmd){
    case GetS:
        processGetSRequest(_event, _cacheLine);
        break;
    case GetX:
    case GetSEx:
        if(modifiedStateNeeded(_event, _cacheLine)) return;
        processGetXRequest(_event, _cacheLine, _cmd);
        GetSExReqsReceived_++;
        break;
    case PutS:
        PUTSReqsReceived_++;
        break;
    //TODO:  Case PutX:  PUTXReqsReceived_++;
    case PutM:
    case PutX:
        processPutMRequest(_event, _cacheLine);
        break;
    case PutE:
        processPutERequest(_event, _cacheLine);
        break;
    default:
        _abort(MemHierarchy::CacheController, "Wrong command type!");
    }
}


void MESIBottomCC::handleInvalidate(MemEvent *event, CacheLine* cacheLine, Command cmd){
    if(!canInvalidateRequestProceed(event, cacheLine, false)) return;
    
    switch(cmd){
        case Inv:
			processInvRequest(event, cacheLine);
            break;
        case InvX:
			processInvXRequest(event, cacheLine);
            break;
	    default:
            _abort(MemHierarchy::CacheController, "Command not supported.\n");
	}

}

void MESIBottomCC::handleResponse(MemEvent* ackEvent, CacheLine* cacheLine, const vector<mshrType> mshrEntry){
    cacheLine->decAckCount();
    printData(d_, "Response Data", &ackEvent->getPayload());
    
    assert(cacheLine->getAckCount() == 0);
    assert(mshrEntry.front().elem.type() == typeid(MemEvent*));
    assert(cacheLine->unlocked());
    
    MemEvent* origEv = boost::get<MemEvent*>(mshrEntry.front().elem);
    Command origCmd  = origEv->getCmd();
    assert(MemEvent::isDataRequest(origCmd));
    cacheLine->setData(ackEvent->getPayload(), ackEvent);
    d_->debug(_L4_,"CacheLine State: %s, Granted State: %s \n", BccLineString[cacheLine->getState()], BccLineString[ackEvent->getGrantedState()]);
    if(cacheLine->getState() == S && ackEvent->getGrantedState() == E) cacheLine->setState(E);

}


void MESIBottomCC::handleFetchInvalidate(MemEvent* _event, CacheLine* _cacheLine, int _parentId, bool _mshrHit){
    if(!canInvalidateRequestProceed(_event, _cacheLine, false)) return;
    Command cmd = _event->getCmd();

    switch(cmd){
        case FetchInvalidate:
            _cacheLine->setState(I);
            FetchInvalidateReqSent_++;
            break;
        case FetchInvalidateX:  //TODO: not tested yet
            _cacheLine->setState(S);
            FetchInvalidateXReqSent_++;
            break;
	    default:
            _abort(MemHierarchy::CacheController, "Command not supported.\n");
	}
    
    sendResponse(_event, _cacheLine, _parentId, _mshrHit);
}


/*---------------------------------------------------------------------------------------------------
 * Helper Functions
 *--------------------------------------------------------------------------------------------------*/

bool MESIBottomCC::modifiedStateNeeded(MemEvent* _event, CacheLine* _cacheLine){
    bool pf = _event->isPrefetch();
    Addr addr = _cacheLine->getBaseAddr();
    BCC_MESIState state = _cacheLine->getState();

    if(state == S || state == I){
        if(state == S){
            inc_GETXMissSM(addr, pf);
            _cacheLine->setState(SM);
        }
        else{
            inc_GETXMissIM(addr, pf);
            _cacheLine->setState(IM);
        }
        forwardMessage(_event, _cacheLine, &_event->getPayload());
        return true;
    }
    return false;
}


bool MESIBottomCC::canInvalidateRequestProceed(MemEvent* _event, CacheLine* _cacheLine, bool _sendAcks){
    bool ret = true;
    if(!_cacheLine || _cacheLine->inTransition()){
        d_->debug(_WARNING_,"Warning: Inv/FetchInv Rx but line's invalid or in transition. State = %s\n", BccLineString[_cacheLine->getState()]);
        ret = false;
    }
    
    if(!ret && _sendAcks) sendAckResponse(_event);

    return ret;
}


void MESIBottomCC::processGetXRequest(MemEvent* event, CacheLine* cacheLine, Command cmd){
    BCC_MESIState state = cacheLine->getState();
    Addr addr = cacheLine->getBaseAddr();
    bool pf = event->isPrefetch();
  
    if(state == E) cacheLine->setState(M);    /* upgrade */
    assert(cacheLine->getState() == M);
    
    if(cmd == GetX){
        cacheLine->setData(event->getPayload(), event);
        if(L1_ && event->queryFlag(MemEvent::F_LOCKED)){
            assert(cacheLine->isLockedByUser());
            cacheLine->decLock();
        }
    }
    if(L1_ && cmd == GetSEx) cacheLine->incLock();
    inc_GETXHit(addr, pf);
}


void MESIBottomCC::processInvRequest(MemEvent* _event, CacheLine* _cacheLine){
    BCC_MESIState state = _cacheLine->getState();
    
    if(state == M || state == E){
        _cacheLine->setState(I);
        InvalidatePUTMReqSent_++;
        if(state == M) sendWriteback(PutM, _cacheLine);
        else           sendWriteback(PutE, _cacheLine);
    }
    else{
        _cacheLine->setState(I);
    }
}


void MESIBottomCC::processInvXRequest(MemEvent* _event, CacheLine* _cacheLine){
    BCC_MESIState state = _cacheLine->getState();
    
    if(state == M || state == E){
        _cacheLine->setState(S);
        InvalidatePUTMReqSent_++;
        sendWriteback(PutX, _cacheLine);  //TODO: send PutXE, PutXM;  lower level caches doesn't know if data needs to be actual written in their cache
    }
    else{
        _cacheLine->setState(I);
    }
}


void MESIBottomCC::processGetSRequest(MemEvent* event, CacheLine* cacheLine){
    BCC_MESIState state = cacheLine->getState();
    Addr addr = cacheLine->getBaseAddr();
    bool pf = event->isPrefetch();

    if(state != I) inc_GETSHit(addr, pf);
    else{
        cacheLine->setState(IS);
        forwardMessage(event, cacheLine, NULL);
        inc_GETSMissIS(addr, pf);
    }
}


void MESIBottomCC::processPutMRequest(MemEvent* event, CacheLine* cacheLine){
    BCC_MESIState state = cacheLine->getState();
    assert(state == M || state == E);
    if(state == E) cacheLine->setState(M);
    cacheLine->setData(event->getPayload(), event);
    PUTMReqsReceived_++;
}


void MESIBottomCC::processPutERequest(MemEvent* event, CacheLine* cacheLine){
    BCC_MESIState state = cacheLine->getState();
    assert(state == E || state == M);
    PUTEReqsReceived_++;
}


void MESIBottomCC::forwardMessage(MemEvent* _event, CacheLine* _cacheLine, vector<uint8_t>* _data){
    Addr baseAddr = _cacheLine->getBaseAddr();
    unsigned int lineSize = _cacheLine->getLineSize();
    forwardMessage(_event, baseAddr, lineSize, _data);
}


void MESIBottomCC::forwardMessage(MemEvent* _event, Addr _baseAddr, unsigned int _lineSize, vector<uint8_t>* _data){

    Command cmd = _event->getCmd();
    MemEvent* forwardEvent;
    if(cmd == GetX) forwardEvent = new MemEvent((SST::Component*)owner_, _event->getAddr(), _baseAddr, cmd, *_data);
    else forwardEvent = new MemEvent((SST::Component*)owner_, _event->getAddr(), _baseAddr, cmd, _lineSize);

    forwardEvent->setDst(nextLevelCacheName_);
    uint64 deliveryTime;
    if(_event->queryFlag(MemEvent::F_UNCACHED)){
        forwardEvent->setFlag(MemEvent::F_UNCACHED);
        deliveryTime = timestamp_;
    }
    else deliveryTime =  timestamp_ + accessLatency_;
    response resp = {forwardEvent, deliveryTime, false};
    outgoingEventQueue_.push(resp);
    
    d_->debug(_L1_,"Forwarding Message: Addr = %"PRIx64", BaseAddr = %"PRIx64", Cmd = %s, Size = %i, Dst = %s \n",
             _event->getAddr(), _baseAddr, CommandString[cmd], _event->getSize(), nextLevelCacheName_.c_str());
}


void MESIBottomCC::sendResponse(MemEvent* _event, CacheLine* _cacheLine, int _parentId, bool _mshrHit){
    Command cmd = _event->getCmd();

    MemEvent *responseEvent = _event->makeResponse((SST::Component*)owner_);
    responseEvent->setPayload(*_cacheLine->getData());

    uint64 latency = _mshrHit ? timestamp_ + mshrLatency_ : timestamp_ + accessLatency_;
    response resp  = {responseEvent, latency, true};
    outgoingEventQueue_.push(resp);
    
    d_->debug(_L1_,"Sending %s Response Message: Addr = %"PRIx64", BaseAddr = %"PRIx64", Cmd = %s, Size = %i \n",
              CommandString[cmd], _event->getBaseAddr(), _event->getBaseAddr(), CommandString[cmd], lineSize_);
}

void MESIBottomCC::sendWriteback(Command cmd, CacheLine* cacheLine){
    d_->debug(_L1_,"Sending Command:  Cmd = %s\n", CommandString[cmd]);
    vector<uint8_t>* data = cacheLine->getData();
    MemEvent* newCommandEvent = new MemEvent((SST::Component*)owner_, cacheLine->getBaseAddr(),cacheLine->getBaseAddr(), cmd, *data);
    newCommandEvent->setDst(nextLevelCacheName_);
    response resp = {newCommandEvent, timestamp_, false};
    outgoingEventQueue_.push(resp);
}

//TODO: change name
bool MESIBottomCC::sendAckResponse(MemEvent *_event){
    MemEvent *responseEvent;
    Command cmd = _event->getCmd();
    assert(cmd == Inv || cmd == PutM ||  cmd == PutS || cmd == InvX);
    responseEvent = _event->makeResponse((SST::Component*)owner_);
    d_->debug(_L1_,"Sending Ack Response:  Addr = %"PRIx64", Cmd = %s \n", responseEvent->getAddr(), CommandString[responseEvent->getCmd()]);
    response resp = {responseEvent, timestamp_, false};
    outgoingEventQueue_.push(resp);
    return true;
}


unsigned int MESIBottomCC::getParentId(CacheLine* wbCacheLine){
    return 0;
}


unsigned int MESIBottomCC::getParentId(Addr baseAddr){
    return 0;
}


void MESIBottomCC::printStats(int _stats, uint64 _GetSExReceived,
                               uint64 _invalidateWaitingForUserLock, uint64 _totalInstReceived,
                               uint64 _nonCoherenceReqsReceived, uint64 _mshrHits){
    Output* dbg = new Output();
    dbg->init("", 0, 0, (Output::output_location_t)_stats);
    int totalMisses = GETXMissIM_ + GETXMissSM_ + GETSMissIS_;
    int totalHits = GETSHit_ + GETXHit_;
    double hitRatio = (totalHits / ( totalHits + (double)totalMisses)) * 100;
    dbg->output(C,"--------------------------------------------------------------------\n");
    dbg->output(C,"Name: %s\n", ownerName_.c_str());
    dbg->output(C,"--------------------------------------------------------------------\n");
    dbg->output(C,"GetS-IS misses: %i\n", GETSMissIS_);
    dbg->output(C,"GetX-SM misses: %i\n", GETXMissSM_);
    dbg->output(C,"GetX-IM misses: %i\n", GETXMissIM_);
    dbg->output(C,"GetS hits: %i\n", GETSHit_);
    dbg->output(C,"GetX hits: %i\n", GETXHit_);
    dbg->output(C,"Total misses: %i\n", totalMisses);
    dbg->output(C,"Total hits: %i\n", totalHits);
    dbg->output(C,"Hit Ratio:  %.3f%%\n", hitRatio);
    dbg->output(C,"PutS received: %i\n", PUTSReqsReceived_);
    dbg->output(C,"PutM received: %i\n", PUTMReqsReceived_);
    dbg->output(C,"GetSEx received: %i\n", GetSExReqsReceived_);
    dbg->output(C,"PUTS sent due to evictions: %u\n", EvictionPUTSReqSent_);
    dbg->output(C,"PUTM sent due to evictions: %u\n", EvictionPUTMReqSent_);
    dbg->output(C,"PUTM sent due to invalidations: %u\n", InvalidatePUTMReqSent_);
    dbg->output(C,"Invalidates recieved that locked due to user atomic lock: %llu\n", _invalidateWaitingForUserLock);
    dbg->output(C,"Total instructions recieved: %llu\n", _totalInstReceived);
    dbg->output(C,"Total MSHR hits: %llu\n", _mshrHits);
    dbg->output(C,"Memory requests received (non-coherency related): %llu\n\n", _nonCoherenceReqsReceived);
}

void MESIBottomCC::updateEvictionStats(BCC_MESIState _state){
    if(_state == S)      EvictionPUTSReqSent_++;
    else if(_state == M) EvictionPUTMReqSent_++;
    else if(_state == E) EvictionPUTSReqSent_++;
    else _abort(MemHierarchy::CacheController, "State not supported for eviction.\n");
}


void MESIBottomCC::inc_GETXMissSM(Addr addr, bool prefetchRequest){
    if(!prefetchRequest){
        GETXMissSM_++;
        listener_->notifyAccess(CacheListener::WRITE, CacheListener::MISS, addr);
    }
}
void MESIBottomCC::inc_GETXMissIM(Addr addr, bool prefetchRequest){
    if(!prefetchRequest){
        GETXMissIM_++;
        listener_->notifyAccess(CacheListener::WRITE, CacheListener::MISS, addr);
    }
}
void MESIBottomCC::inc_GETSHit(Addr addr, bool prefetchRequest){
    if(!prefetchRequest){
        GETSHit_++;
        listener_->notifyAccess(CacheListener::READ, CacheListener::HIT, addr);
    }
}
void MESIBottomCC::inc_GETXHit(Addr addr, bool prefetchRequest){
    if(!prefetchRequest){
        GETXHit_++;
        listener_->notifyAccess(CacheListener::WRITE, CacheListener::HIT, addr);
    }
}
void MESIBottomCC::inc_GETSMissIS(Addr addr, bool prefetchRequest){
    if(!prefetchRequest){
        GETSMissIS_++;
        listener_->notifyAccess(CacheListener::READ, CacheListener::MISS, addr);
    }
}

bool MESIBottomCC::isExclusive(CacheLine* cacheLine) {
    BCC_MESIState state = cacheLine->getState();
    return (state == E) || (state == M);
}
