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
#include "MESIBottomCoherenceController.h"
using namespace SST;
using namespace SST::MemHierarchy;

/*----------------------------------------------------------------------------------------------------------------------
 * Bottom Coherence Controller Implementation
 *---------------------------------------------------------------------------------------------------------------------*/
	    
void MESIBottomCC::handleEviction(CacheLine* _wbCacheLine, uint32_t _groupId){
	BCC_MESIState state = _wbCacheLine->getState();
    setGroupId(_groupId);

    switch(state){
	case S:
        inc_EvictionPUTSReqSent();
		_wbCacheLine->setState(I);
        sendWriteback(PutS, _wbCacheLine);
		break;
	case M:
        inc_EvictionPUTMReqSent();
        _wbCacheLine->setState(I);
		sendWriteback(PutM, _wbCacheLine);
		break;
    case E:
        inc_EvictionPUTEReqSent();
		_wbCacheLine->setState(I);
        sendWriteback(PutE, _wbCacheLine);
        break;
	default:
		_abort(MemHierarchy::CacheController, "Eviction: Not a valid state: %s \n", BccLineString[state]);
    }
}


void MESIBottomCC::handleRequest(MemEvent* _event, CacheLine* _cacheLine, Command _cmd){
    bool upgrade;
    setGroupId(_event->getGroupId());
    d_->debug(_L0_,"BottomCC State = %s\n", BccLineString[_cacheLine->getState()]);
    assert(!_cacheLine->inTransition());   //MSHR should catch this..
    
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
        inc_PUTSReqsReceived();
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
    
    setGroupId(_event->getGroupId());
    
    if(_cacheLine->inTransition()){
        d_->debug(_L0_,"Handling Invalidate. Ack Needed: %s\n", _event->getAckNeeded() ? "true" : "false");
        if(_event->getAckNeeded()){
            inc_InvalidatePUTSReqSent();
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
        _abort(MemHierarchy::CacheController, "");
    }
    _cacheLine->setData(_responseEvent->getPayload(), _responseEvent);
    if(_cacheLine->getState() == S && _responseEvent->getGrantedState() == E) _cacheLine->setState(E);

    d_->debug(_L4_,"CacheLine State: %s, Granted State: %s \n", BccLineString[_cacheLine->getState()], BccLineString[_responseEvent->getGrantedState()]);
}


void MESIBottomCC::handleFetchInvalidate(MemEvent* _event, CacheLine* _cacheLine, int _parentId, bool _mshrHit){
    setGroupId(_event->getGroupId());
    if(!_cacheLine) return;
    if(_cacheLine->inTransition()) return;  //TODO:  test this case, might need a response (ack)
    
    Command cmd = _event->getCmd();

    switch(cmd){
        case FetchInv:
            _cacheLine->setState(I);
            inc_FetchInvReqSent();
            break;
        case FetchInvX:
            _cacheLine->setState(S);
            inc_FetchInvXReqSent();
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
        _cacheLine->setData(_event->getPayload(), _event);
        if(L1_ && _event->queryFlag(MemEvent::F_LOCKED)){
            assert(_cacheLine->isLockedByUser());
            _cacheLine->decLock();
        }
    }
    else{
        inc_GetSExReqsReceived();
        if(L1_) _cacheLine->incLock();
    }
    inc_GETXHit(addr, pf);
}


void MESIBottomCC::processInvRequest(MemEvent* _event, CacheLine* _cacheLine){
    BCC_MESIState state = _cacheLine->getState();
    
    if(state == M || state == E){
        _cacheLine->setState(I);
        if(state == M){
            inc_InvalidatePUTMReqSent();
            sendWriteback(PutM, _cacheLine);
        }
        else{
            inc_InvalidatePUTEReqSent();
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
        inc_InvalidatePUTXReqSent();
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

    if(state != I) inc_GETSHit(addr, pf);
    else{
        _cacheLine->setState(IS);
        forwardMessage(_event, _cacheLine, NULL);
        inc_GETSMissIS(addr, pf);
    }
}


void MESIBottomCC::handlePutMRequest(MemEvent* _event, CacheLine* _cacheLine){
    updateCacheLineRxWriteback(_event, _cacheLine);
    inc_PUTMReqsReceived();
}

void MESIBottomCC::handlePutXRequest(MemEvent* _event, CacheLine* _cacheLine){
    updateCacheLineRxWriteback(_event, _cacheLine);
    inc_PUTXReqsReceived();
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
    inc_PUTEReqsReceived();
    
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
    forwardEvent->setFlag(_event->getFlags());
    
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
    vector<uint8_t>* data = _cacheLine->getData();
    MemEvent* newCommandEvent = new MemEvent((SST::Component*)owner_, _cacheLine->getBaseAddr(), _cacheLine->getBaseAddr(), _cmd, *data);
    newCommandEvent->setDst(nextLevelCacheName_);
    response resp = {newCommandEvent, timestamp_, false};
    outgoingEventQueue_.push(resp);
    d_->debug(_L1_,"Sending writeback:  Cmd = %s\n", CommandString[_cmd]);
}

void MESIBottomCC::sendNACK(MemEvent* _event){
    setGroupId(_event->getGroupId());
    MemEvent *NACKevent = _event->makeNACKResponse((Component*)owner_, _event);
    uint64 latency      = timestamp_ + accessLatency_;
    response resp       = {NACKevent, latency, true};
    inc_NACKsSent();
    outgoingEventQueue_.push(resp);
    d_->debug(_L1_,"BottomCC: Sending NACK: EventID = %"PRIx64", Addr = %"PRIx64", RespToID = %"PRIx64"\n", _event->getID().first, _event->getAddr(), NACKevent->getResponseToID().first);
}


void MESIBottomCC::printStats(int _stats, vector<int> _groupIds, map<int, CtrlStats> _ctrlStats, uint64_t _updgradeLatency){
    Output* dbg = new Output();
    dbg->init("", 0, 0, (Output::output_location_t)_stats);
    dbg->output(C,"\n--------------------------------------------------------------------\n");
    dbg->output(C,"--- Cache Stats\n");
    dbg->output(C,"--- Name: %s\n", ownerName_.c_str());
    dbg->output(C,"--- Overall Statistics\n");
    dbg->output(C,"--------------------------------------------------------------------\n");

    for(unsigned int i = 0; i < _groupIds.size(); i++){
        int totalMisses = stats_[_groupIds[i]].GETXMissIM_ + stats_[_groupIds[i]].GETXMissSM_ + stats_[_groupIds[i]].GETSMissIS_;
        int totalHits = stats_[_groupIds[i]].GETSHit_ + stats_[_groupIds[i]].GETXHit_;
        double hitRatio = (totalHits / ( totalHits + (double)totalMisses)) * 100;
        
        if(i != 0){
            dbg->output(C,"--------------------------------------------------------------------\n");
            dbg->output(C,"--- Cache Stats\n");
            dbg->output(C,"--- Name: %s\n", ownerName_.c_str());
            dbg->output(C,"--- Group Statistics, Group ID = %i\n", _groupIds[i]);
            dbg->output(C,"--------------------------------------------------------------------\n");
        }
        dbg->output(C,"- Total misses: %i\n", totalMisses);
        dbg->output(C,"- Total hits: %i\n", totalHits);
        dbg->output(C,"- Hit ratio: %.3f%%\n", hitRatio);
        dbg->output(C,"- Miss ratio: %.3f%%\n", 100 - hitRatio);
        dbg->output(C,"- Read misses: %i\n", stats_[_groupIds[i]].GETSMissIS_);
        dbg->output(C,"- Write misses: %i\n", stats_[_groupIds[i]].GETXMissSM_ + stats_[_groupIds[i]].GETXMissIM_);
        dbg->output(C,"- GetS received: %i\n", stats_[_groupIds[i]].GETSMissIS_ + stats_[_groupIds[i]].GETSHit_);
        dbg->output(C,"- GetX received: %i\n", stats_[_groupIds[i]].GETXMissSM_ + stats_[_groupIds[i]].GETXMissIM_ + stats_[_groupIds[i]].GETXHit_);
        dbg->output(C,"- GetSEx received: %i\n", stats_[_groupIds[i]].GetSExReqsReceived_);
        dbg->output(C,"- GetS-IS misses: %i\n", stats_[_groupIds[i]].GETSMissIS_);
        dbg->output(C,"- GetX-SM misses: %i\n", stats_[_groupIds[i]].GETXMissSM_);
        dbg->output(C,"- GetX-IM misses: %i\n", stats_[_groupIds[i]].GETXMissIM_);
        dbg->output(C,"- GetS hits: %i\n", stats_[_groupIds[i]].GETSHit_);
        dbg->output(C,"- GetX hits: %i\n", stats_[_groupIds[i]].GETXHit_);
        dbg->output(C,"- Average updgrade latency: %"PRIu64" cycles\n", _updgradeLatency);
        dbg->output(C,"- PutS received: %i\n", stats_[_groupIds[i]].PUTSReqsReceived_);
        dbg->output(C,"- PutM received: %i\n", stats_[_groupIds[i]].PUTMReqsReceived_);
        dbg->output(C,"- PutX received: %i\n", stats_[_groupIds[i]].PUTXReqsReceived_);
        dbg->output(C,"- PUTM sent due to invalidations: %u\n", stats_[_groupIds[i]].InvalidatePUTMReqSent_);
        dbg->output(C,"- PUTE sent due to invalidations: %u\n", stats_[_groupIds[i]].InvalidatePUTEReqSent_);
        dbg->output(C,"- PUTX sent due to invalidations: %u\n", stats_[_groupIds[i]].InvalidatePUTXReqSent_);
        dbg->output(C,"- PUTS sent due to evictions: %u\n", stats_[_groupIds[i]].EvictionPUTSReqSent_);
        dbg->output(C,"- PUTM sent due to evictions: %u\n", stats_[_groupIds[i]].EvictionPUTMReqSent_);
        dbg->output(C,"- PUTE sent due to evictions: %u\n", stats_[_groupIds[i]].EvictionPUTEReqSent_);
        dbg->output(C,"- Inv received stalled bc atomic lock: %"PRIu64"\n", _ctrlStats[_groupIds[i]].InvWaitingForUserLock_);
        dbg->output(C,"- Total requests received: %"PRIu64"\n", _ctrlStats[_groupIds[i]].TotalRequestsReceived_);
        dbg->output(C,"- Total requests handled by MSHR (MSHR hits): %"PRIu64"\n", _ctrlStats[_groupIds[i]].TotalMSHRHits_);
        dbg->output(C,"- NACKs sent (MSHR Full, BottomCC): %u\n", stats_[_groupIds[i]].NACKsSent_);
    }

}

