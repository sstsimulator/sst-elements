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
    
    switch(state){
	case S:
        EvictionPUTSReqSent_++;
		_wbCacheLine->setState(I);
        sendWriteback(PutS, _wbCacheLine);
		break;
	case M:
        EvictionPUTMReqSent_++;
        _wbCacheLine->setState(I);
		sendWriteback(PutM, _wbCacheLine);
		break;
    case E:
        EvictionPUTEReqSent_++;
		_wbCacheLine->setState(I);
        sendWriteback(PutE, _wbCacheLine);
        break;
	default:
		_abort(MemHierarchy::CacheController, "Eviction: Not a valid state: %s \n", BccLineString[state]);
    }
}


void MESIBottomCC::handleRequest(MemEvent* _event, CacheLine* _cacheLine, Command _cmd){
    bool upgrade;
    d_->debug(_L0_,"BottomCC State = %s\n", BccLineString[_cacheLine->getState()]);
    
    switch(_cmd){
    case GetS:
        handleGetSRequest(_event, _cacheLine);
        break;
    case GetX:
    case GetSEx:
        upgrade = isUpgradeToModifiedNeeded(_event, _cacheLine);
        if(upgrade){
            forwardMessage(_event, _cacheLine, &_event->getPayload());
            return;
        }
        handleGetXRequest(_event, _cacheLine);
        break;
    case PutS:
        PUTSReqsReceived_++;
        break;
    case PutM:
    case PutX:
    case PutXE:                             //TODO:  Case PutX:  PUTXReqsReceived_++;
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
       behave as if it was in Invalid state.  Atomiticy is maintained because this request in transition wont proceed 
       till AFTER the request that ignited the INV finishes.
       
       Because of this, the lower level cache can proceed even though this cache line
       is not actually invalidated.  No need for an ack sent to lower level cache since only possible 
       transitional state is (SM);  lower level cache knows this state is in S state so it proceeds (weak consistency).  
       This cache eventually gets M state since the requests actually gets store in the MSHR of the lwlvl cache */
    
    //if(!_cacheLine){
    //    d_->debug(_L0_,"Handling Invalidate. Cacheline already invalid\n");
    //    return;
    //}
    
    if(_cacheLine->inTransition()){
        d_->debug(_L0_,"Handling Invalidate. Ack Needed: %s\n", _event->getAckNeeded() ? "true" : "false");
        if(_event->getAckNeeded()){
            InvalidatePUTSReqSent_++;
            sendWriteback(PutS, _cacheLine);
        }
        return;
    }
    
    d_->debug(_L0_,"Handling Invalidate. Cache line state = %s\n", BccLineString[_cacheLine->getState()]);

    
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

void MESIBottomCC::handleResponse(MemEvent* _responseEvent, CacheLine* _cacheLine, MemEvent* _origRequest){
    _cacheLine->updateState();
    //printData(d_, "Response Data", &_responseEvent->getPayload());
    
    Command origCmd = _origRequest->getCmd();
    if(!MemEvent::isDataRequest(origCmd)){
        d_->debug(_L0_,"Error:  Command = %s not of request-type\n", CommandString[origCmd]);
        cout << flush;
        _abort(MemHierarchy::CacheController, "");
    }
    _cacheLine->setData(_responseEvent->getPayload(), _responseEvent);
    if(_cacheLine->getState() == S && _responseEvent->getGrantedState() == E) _cacheLine->setState(E);

    d_->debug(_L4_,"CacheLine State: %s, Granted State: %s \n", BccLineString[_cacheLine->getState()], BccLineString[_responseEvent->getGrantedState()]);
}


void MESIBottomCC::handleFetchInvalidate(MemEvent* _event, CacheLine* _cacheLine, int _parentId, bool _mshrHit){
    if(!_cacheLine) return;
    if(_cacheLine->inTransition()) return;  //TODO:  test this case, might need a response (ack)
    
    Command cmd = _event->getCmd();

    switch(cmd){
        case FetchInv:
            _cacheLine->setState(I);
            FetchInvReqSent_++;
            break;
        case FetchInvX:
            _cacheLine->setState(S);
            FetchInvXReqSent_++;
            break;
	    default:
            _abort(MemHierarchy::CacheController, "Command not supported.\n");
	}
    
    sendResponse(_event, _cacheLine, _parentId, _mshrHit);
}


/*---------------------------------------------------------------------------------------------------
 * Helper Functions
 *--------------------------------------------------------------------------------------------------*/

bool MESIBottomCC::isUpgradeToModifiedNeeded(MemEvent* _event, CacheLine* _cacheLine){
    bool pf   = _event->isPrefetch();
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
        return true;
    }
    return false;
}


bool MESIBottomCC::canInvalidateRequestProceed(MemEvent* _event, CacheLine* _cacheLine, bool _sendAcks){
    return true;
}


void MESIBottomCC::handleGetXRequest(MemEvent* _event, CacheLine* _cacheLine){
    BCC_MESIState state = _cacheLine->getState();
    Command cmd         = _event->getCmd();
    
    Addr addr = _cacheLine->getBaseAddr();
    bool pf   = _event->isPrefetch();
  
    if(state == E) _cacheLine->setState(M);    /* upgrade */
    assert(_cacheLine->getState() == M);
    
    if(cmd == GetX){
        GetXReqsReceived_++;
        _cacheLine->setData(_event->getPayload(), _event);
        if(L1_ && _event->queryFlag(MemEvent::F_LOCKED)){
            assert(_cacheLine->isLockedByUser());
            _cacheLine->decLock();
        }
    }
    else{
        GetSExReqsReceived_++;
        if(L1_) _cacheLine->incLock();
    }
    inc_GETXHit(addr, pf);
}


void MESIBottomCC::processInvRequest(MemEvent* _event, CacheLine* _cacheLine){
    BCC_MESIState state = _cacheLine->getState();
    
    if(state == M || state == E){
        _cacheLine->setState(I);
        if(state == M){
            InvalidatePUTMReqSent_++;
            sendWriteback(PutM, _cacheLine);
        }
        else{
            InvalidatePUTEReqSent_++;
            sendWriteback(PutE, _cacheLine);
        }
    }
    else if(state == S){
        _cacheLine->setState(I);
        if(_event->getAckNeeded()) sendWriteback(PutS, _cacheLine);
    }
    else _abort(MemHierarchy::CacheController, "BottomCC Invalidate: Not a valid state: %s \n", BccLineString[state]);
}


void MESIBottomCC::processInvXRequest(MemEvent* _event, CacheLine* _cacheLine){
    BCC_MESIState state = _cacheLine->getState();
    
    if(state == M || state == E){
        _cacheLine->setState(S);
        InvalidatePUTXReqSent_++;
        if(state == E) sendWriteback(PutXE, _cacheLine);
        else           sendWriteback(PutX, _cacheLine);
    }
    else if(state == S){
        _cacheLine->setState(I);
        if(_event->getAckNeeded()) sendWriteback(PutS, _cacheLine);
    }
    else _abort(MemHierarchy::CacheController, "Not a valid state: %s", BccLineString[state]);
}


void MESIBottomCC::handleGetSRequest(MemEvent* _event, CacheLine* _cacheLine){
    BCC_MESIState state = _cacheLine->getState();
    Addr addr = _cacheLine->getBaseAddr();
    bool pf = _event->isPrefetch();

    if(state != I){
        inc_GETSHit(addr, pf);
        GetSReqsReceived_++;
    }
    else{
        _cacheLine->setState(IS);
        forwardMessage(_event, _cacheLine, NULL);
        inc_GETSMissIS(addr, pf);
    }
}


void MESIBottomCC::handlePutMRequest(MemEvent* _event, CacheLine* _cacheLine){
    updateCacheLineRxWriteback(_event, _cacheLine);
    PUTMReqsReceived_++;
}

void MESIBottomCC::handlePutXRequest(MemEvent* _event, CacheLine* _cacheLine){
    updateCacheLineRxWriteback(_event, _cacheLine);
    PUTXReqsReceived_++;
}

void MESIBottomCC::updateCacheLineRxWriteback(MemEvent* _event, CacheLine* _cacheLine){
    BCC_MESIState state = _cacheLine->getState();
    assert(state == M || state == E);
    if(state == E) _cacheLine->setState(M);
    if(_event->getCmd() != PutXE){
        _cacheLine->setData(_event->getPayload(), _event);   //Only PutM/PutX write data in the cache line
        d_->debug(_L1_,"Data written to cache line\n");
    }
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

void MESIBottomCC::sendEvent(MemEvent* _event){
    uint64 deliveryTime =  timestamp_ + accessLatency_;
    response resp = {_event, deliveryTime, false};
    outgoingEventQueue_.push(resp);
    
    d_->debug(_L1_,"Sending Event: Addr = %"PRIx64", BaseAddr = %"PRIx64", Cmd = %s\n",
             _event->getAddr(), _event->getBaseAddr(), CommandString[_event->getCmd()]);
}

void MESIBottomCC::forwardMessage(MemEvent* _event, Addr _baseAddr, unsigned int _lineSize, vector<uint8_t>* _data){
    Command cmd = _event->getCmd();
    
    /* Create event to be forwarded */
    MemEvent* forwardEvent;
    if(cmd == GetX) forwardEvent = new MemEvent((SST::Component*)owner_, _event->getAddr(), _baseAddr, cmd, *_data);
    else            forwardEvent = new MemEvent((SST::Component*)owner_, _event->getAddr(), _baseAddr, cmd, _lineSize);
    forwardEvent->setDst(nextLevelCacheName_);
    
    /* Determine latency in cycles */
    uint64 deliveryTime;
    if(_event->queryFlag(MemEvent::F_UNCACHED)){
        forwardEvent->setFlag(MemEvent::F_UNCACHED);
        deliveryTime = timestamp_;
    }
    else deliveryTime = timestamp_ + accessLatency_;
    
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


void MESIBottomCC::sendWriteback(Command _cmd, CacheLine* _cacheLine){
    d_->debug(_L1_,"Sending writeback:  Cmd = %s\n", CommandString[_cmd]);
    vector<uint8_t>* data = _cacheLine->getData();
    MemEvent* newCommandEvent = new MemEvent((SST::Component*)owner_, _cacheLine->getBaseAddr(), _cacheLine->getBaseAddr(), _cmd, *data);
    newCommandEvent->setDst(nextLevelCacheName_);
    response resp = {newCommandEvent, timestamp_, false};
    outgoingEventQueue_.push(resp);
}

void MESIBottomCC::sendNACK(MemEvent* _event){
    MemEvent *NACKevent = _event->makeNACKResponse((Component*)owner_, _event);
    uint64 latency      = timestamp_ + accessLatency_;
    response resp       = {NACKevent, latency, true};
    d_->debug(_L1_,"BottomCC: Sending NACK: EventID = %"PRIx64", Addr = %"PRIx64", RespToID = %"PRIx64"\n", _event->getID().first, _event->getAddr(), NACKevent->getResponseToID().first);
    NACKsSent_++;
    outgoingEventQueue_.push(resp);
}


void MESIBottomCC::printStats(int _stats, uint64 _GetSExReceived,
                               uint64 _invalidateWaitingForUserLock, uint64 _totalReqReceived,
                               uint64 _mshrHits, uint64 _updgradeLatency){
    Output* dbg = new Output();
    dbg->init("", 0, 0, (Output::output_location_t)_stats);
    int totalMisses = GETXMissIM_ + GETXMissSM_ + GETSMissIS_;
    int totalHits = GETSHit_ + GETXHit_;
    double hitRatio = (totalHits / ( totalHits + (double)totalMisses)) * 100;
    dbg->output(C,"\n--------------------------------------------------------------------\n");
    dbg->output(C,"--- Cache Stats\n");
    dbg->output(C,"--- Name: %s\n", ownerName_.c_str());
    dbg->output(C,"--------------------------------------------------------------------\n");
    dbg->output(C,"- Total misses: %i\n", totalMisses);
    dbg->output(C,"- Total hits:  %i\n", totalHits);
    dbg->output(C,"- Hit ratio:  %.3f%%\n", hitRatio);
    dbg->output(C,"- Miss ratio:  %.3f%%\n", 100 - hitRatio);
    dbg->output(C,"- Read misses:  %i\n", GETSMissIS_);
    dbg->output(C,"- Write misses:  %i\n", GETXMissSM_ + GETXMissIM_);
    dbg->output(C,"- GetS received: %i\n", GetSReqsReceived_);
    dbg->output(C,"- GetX received: %i\n", GetXReqsReceived_);
    dbg->output(C,"- GetSEx received: %i\n", GetSExReqsReceived_);
    dbg->output(C,"- GetS-IS misses:  %i\n", GETSMissIS_);
    dbg->output(C,"- GetX-SM misses:  %i\n", GETXMissSM_);
    dbg->output(C,"- GetX-IM misses:  %i\n", GETXMissIM_);
    dbg->output(C,"- GetS hits:  %i\n", GETSHit_);
    dbg->output(C,"- GetX hits:  %i\n", GETXHit_);
    dbg->output(C,"- Average updgrade latency:  %llu cycles\n", _updgradeLatency);
    dbg->output(C,"- PutS received:  %i\n", PUTSReqsReceived_);
    dbg->output(C,"- PutM received:  %i\n", PUTMReqsReceived_);
    dbg->output(C,"- PUTM sent due to invalidations:  %u\n", InvalidatePUTMReqSent_);
    dbg->output(C,"- PUTE sent due to invalidations:  %u\n", InvalidatePUTEReqSent_);
    dbg->output(C,"- PUTX sent due to invalidations:  %u\n", InvalidatePUTXReqSent_);
    dbg->output(C,"- PUTS sent due to evictions:  %u\n", EvictionPUTSReqSent_);
    dbg->output(C,"- PUTM sent due to evictions:  %u\n", EvictionPUTMReqSent_);
    dbg->output(C,"- PUTE sent due to evictions:  %u\n", EvictionPUTEReqSent_);
    dbg->output(C,"- Inv received stalled bc atomic lock:  %llu\n", _invalidateWaitingForUserLock);
    dbg->output(C,"- Total requests received:  %llu\n", _totalReqReceived);
    dbg->output(C,"- Total requests handled by MSHR (MSHR hits):  %llu\n", _mshrHits);
    dbg->output(C,"- NACKs sent (MSHR Full, BottomCC):  %u\n", NACKsSent_);

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
