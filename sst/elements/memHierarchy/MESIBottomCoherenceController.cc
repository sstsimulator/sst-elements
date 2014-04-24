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
	    
void MESIBottomCC::handleEviction(CacheLine* _wbCacheLine){
	BCC_MESIState state = _wbCacheLine->getState();
    updateEvictionStats(state);
    
    switch(state){
	case S:
		_wbCacheLine->setState(I);
        sendWriteback(PutS, _wbCacheLine);
		break;
	case M:
		_wbCacheLine->setState(I);
		sendWriteback(PutM, _wbCacheLine);
		break;
    case E:
		_wbCacheLine->setState(I);
        sendWriteback(PutE, _wbCacheLine);
        break;
	default:
		_abort(MemHierarchy::CacheController, "Not a valid state: %s", BccLineString[state]);
    }
}


void MESIBottomCC::handleRequest(MemEvent* _event, CacheLine* _cacheLine, Command _cmd){
    switch(_cmd){
    case GetS:
        handleGetSRequest(_event, _cacheLine);
        break;
    case GetX:
    case GetSEx:
        if(isUpgradeNeeded(_event, _cacheLine)) return;
        handleGetXRequest(_event, _cacheLine);
        GetSExReqsReceived_++;
        break;
    case PutS:
        PUTSReqsReceived_++;
        break;
    case PutM:
    case PutX:                                 //TODO:  Case PutX:  PUTXReqsReceived_++;
        handlePutMRequest(_event, _cacheLine);
        break;
    case PutE:
        handlePutERequest(_cacheLine);
        break;
    default:
        _abort(MemHierarchy::CacheController, "Wrong command type!");
    }
}


void MESIBottomCC::handleInvalidate(MemEvent* _event, CacheLine* _cacheLine, Command _cmd){
    /* Important note: If cache line is in transition, ignore request (remove from MSHR and drop this event)
       Since this cache line is in transition, any other requests will not proceed. This makes the cache line 
       behave as if it was in I state.  Because of this lower level cache can proceed even though this cache line
       is not actually invalidated.  No need for an ack sent to lower level cache since only possible 
       transitional state is (SM);  lower level cache knows this state is in S state so it proceeds (weak consistency).  
       This cache eventually gets M state since the requests actually gets store in the MSHR of the lwlvl cache */
    if(!canInvalidateRequestProceed(_event, _cacheLine, false)) return;
    
    switch(_cmd){
        case Inv:
			processInvRequest(_event, _cacheLine);
            break;
        case InvX:
			processInvXRequest(_event, _cacheLine);
            break;
	    default:
            _abort(MemHierarchy::CacheController, "Command not supported.\n");
	}

}

void MESIBottomCC::handleResponse(MemEvent* _responseEvent, CacheLine* _cacheLine, const vector<mshrType> _mshrEntry){
    _cacheLine->updateState();
    printData(d_, "Response Data", &_responseEvent->getPayload());
    
    assert(_mshrEntry.front().elem.type() == typeid(MemEvent*));
    
    MemEvent* origEv = boost::get<MemEvent*>(_mshrEntry.front().elem);
    Command origCmd  = origEv->getCmd();
    assert(MemEvent::isDataRequest(origCmd));
    _cacheLine->setData(_responseEvent->getPayload(), _responseEvent);
    d_->debug(_L4_,"CacheLine State: %s, Granted State: %s \n", BccLineString[_cacheLine->getState()], BccLineString[_responseEvent->getGrantedState()]);
    if(_cacheLine->getState() == S && _responseEvent->getGrantedState() == E) _cacheLine->setState(E);
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

bool MESIBottomCC::isUpgradeNeeded(MemEvent* _event, CacheLine* _cacheLine){
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
    
    return ret;
}


void MESIBottomCC::handleGetXRequest(MemEvent* _event, CacheLine* _cacheLine){
    BCC_MESIState state = _cacheLine->getState();
    Command cmd = _event->getCmd();
    
    Addr addr = _cacheLine->getBaseAddr();
    bool pf = _event->isPrefetch();
  
    if(state == E) _cacheLine->setState(M);    /* upgrade */
    assert(_cacheLine->getState() == M);
    
    if(cmd == GetX){
        _cacheLine->setData(_event->getPayload(), _event);
        if(L1_ && _event->queryFlag(MemEvent::F_LOCKED)){
            assert(_cacheLine->isLockedByUser());
            _cacheLine->decLock();
        }
    }
    if(L1_ && cmd == GetSEx) _cacheLine->incLock();
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
        sendWriteback(PutX, _cacheLine);
    }
    else{
        _cacheLine->setState(I);
    }
}


void MESIBottomCC::handleGetSRequest(MemEvent* _event, CacheLine* _cacheLine){
    BCC_MESIState state = _cacheLine->getState();
    Addr addr = _cacheLine->getBaseAddr();
    bool pf = _event->isPrefetch();

    if(state != I) inc_GETSHit(addr, pf);
    else{
        _cacheLine->setState(IS);
        forwardMessage(_event, _cacheLine, NULL);
        inc_GETSMissIS(addr, pf);
    }
}


void MESIBottomCC::handlePutMRequest(MemEvent* _event, CacheLine* _cacheLine){
    BCC_MESIState state = _cacheLine->getState();
    assert(state == M || state == E);
    if(state == E) _cacheLine->setState(M);
    _cacheLine->setData(_event->getPayload(), _event);
    PUTMReqsReceived_++;
}


void MESIBottomCC::handlePutERequest(CacheLine* _cacheLine){
    BCC_MESIState state = _cacheLine->getState();
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
    dbg->output(C,"Invalidates received that locked due to user atomic lock: %llu\n", _invalidateWaitingForUserLock);
    dbg->output(C,"Total instructions received: %llu\n", _totalInstReceived);
    dbg->output(C,"Total MSHR hits: %llu\n", _mshrHits);
    dbg->output(C,"Memory requests received (non-coherency related): %llu\n\n", _nonCoherenceReqsReceived);
}

void MESIBottomCC::updateEvictionStats(BCC_MESIState _state){
    if(_state == S)      EvictionPUTSReqSent_++;
    else if(_state == M) EvictionPUTMReqSent_++;
    else if(_state == E) EvictionPUTSReqSent_++;
    else _abort(MemHierarchy::CacheController, "State not supported for eviction.\n");
}


void MESIBottomCC::inc_GETXMissSM(Addr _addr, bool _prefetchRequest){
    if(!_prefetchRequest){
        GETXMissSM_++;
        listener_->notifyAccess(CacheListener::WRITE, CacheListener::MISS, _addr);
    }
}
void MESIBottomCC::inc_GETXMissIM(Addr _addr, bool _prefetchRequest){
    if(!_prefetchRequest){
        GETXMissIM_++;
        listener_->notifyAccess(CacheListener::WRITE, CacheListener::MISS, _addr);
    }
}
void MESIBottomCC::inc_GETSHit(Addr _addr, bool _prefetchRequest){
    if(!_prefetchRequest){
        GETSHit_++;
        listener_->notifyAccess(CacheListener::READ, CacheListener::HIT, _addr);
    }
}
void MESIBottomCC::inc_GETXHit(Addr _addr, bool _prefetchRequest){
    if(!_prefetchRequest){
        GETXHit_++;
        listener_->notifyAccess(CacheListener::WRITE, CacheListener::HIT, _addr);
    }
}
void MESIBottomCC::inc_GETSMissIS(Addr _addr, bool _prefetchRequest){
    if(!_prefetchRequest){
        GETSMissIS_++;
        listener_->notifyAccess(CacheListener::READ, CacheListener::MISS, _addr);
    }
}
