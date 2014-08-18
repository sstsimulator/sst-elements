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
 * File:   mshr.h
 * Author: Caesar De la Paz III
 * Email:  caesar.sst@gmail.com
 */

#ifndef SST_mshr_h
#define SST_mshr_h

#include "util.h"

namespace SST { namespace MemHierarchy {

/*--------------------------------------------------------------------------------------------------------
 *  MSHR Methods
 *--------------------------------------------------------------------------------------------------------*/

Cache::MSHR::MSHR(Cache* _cache, int _maxSize): cache_(_cache), size_(0), maxSize_(_maxSize){}

bool Cache::MSHR::exists(Addr _baseAddr){
    mshrTable::iterator it = map_.find(_baseAddr);
    return (it != map_.end() && it->second.front().elem.type() == typeid(MemEvent*));
}

const vector<mshrType> Cache::MSHR::lookup(Addr _baseAddr){
    mshrTable::iterator it = map_.find(_baseAddr);
    assert(it != map_.end());
    vector<mshrType> res = it->second;
    return res;
}


MemEvent* Cache::MSHR::lookupFront(Addr _baseAddr){
    mshrTable::iterator it = map_.find(_baseAddr);
    assert(it != map_.end());
    vector<mshrType> mshrEntry = it->second;
    assert(mshrEntry.front().elem.type() == typeid(MemEvent*));
    return boost::get<MemEvent*>(mshrEntry.front().elem);
}

bool Cache::MSHR::insertPointer(Addr _keyAddr, Addr _pointerAddr){
    assert(_keyAddr != _pointerAddr);
    return insert(_keyAddr, _pointerAddr);
}

bool Cache::MSHR::isFull(){
    if(size_ >= maxSize_){
        cache_->d_->debug(_L9_,"MSHR Full. Event could not be inserted. Max Size = %u\n", maxSize_);
        return true;
    }
    return false;
}

MemEvent* Cache::MSHR::getOldestRequest() const {
    MemEvent *ev = NULL;
    for ( mshrTable::const_iterator it = map_.begin() ; it != map_.end() ; ++it ) {
        for ( vector<mshrType>::const_iterator jt = it->second.begin() ; jt != it->second.end() ; jt++ ) {
            if ( jt->elem.type() == typeid(MemEvent*) ) {
                MemEvent *me = boost::get<MemEvent*>(jt->elem);
                if ( !ev || ( me->getInitializationTime() < ev->getInitializationTime() ) ) {
                    ev = me;
                }
            }
        }
    }

    return ev;
};


bool Cache::MSHR::insert(Addr _baseAddr, MemEvent* _event){
    _event->setInMSHR(true);
    bool ret = insert(_baseAddr, mshrType(_event));
    if(LIKELY(ret)){
        cache_->d_->debug(_L9_, "MSHR: Event Inserted. Key addr = %"PRIx64", event Addr = %"PRIx64", Cmd = %s, MSHR Size = %u, Entry Size = %lu\n", _baseAddr, _event->getAddr(), CommandString[_event->getCmd()], size_, map_[_baseAddr].size());
        _event->setStartTime(cache_->getTimestamp());
    }
    else cache_->d_->debug(_L9_, "MSHR Full.  Event could not be inserted.\n");
    return ret;
}

bool Cache::MSHR::insert(Addr _keyAddr, Addr _pointerAddr){
    bool ret = insert(_keyAddr, mshrType(_pointerAddr));
    if(LIKELY(ret))
        cache_->d_->debug(_L9_, "MSHR: Inserted pointer.  Key Addr = %"PRIx64", Pointer Addr = %"PRIx64"\n", _keyAddr, _pointerAddr);
    else
        cache_->d_->debug(_L9_, "MSHR Full. Pointer could not be inserted.\n");

    return ret;
}

bool Cache::MSHR::insert(Addr _baseAddr, mshrType _mshrEntry){
    if(size_ >= maxSize_) return false;
    map_[_baseAddr].push_back(_mshrEntry);
    if(_mshrEntry.elem.type() == typeid(MemEvent*)) size_++;
    return true;
}

bool Cache::MSHR::insertAll(Addr _baseAddr, vector<mshrType> _events){
    if(_events.empty()) return false;
    mshrTable::iterator it = map_.find(_baseAddr);
    
    if(it != map_.end()) it->second.insert(it->second.end(), _events.begin(), _events.end());
    else {
        map_[_baseAddr] = _events;
    }
    
    int trueSize = 0;
    for(vector<mshrType>::iterator it = _events.begin(); it != _events.end(); it++){
        if((*it).elem.type() == typeid(MemEvent*)) trueSize++;
    }
    
    size_ += trueSize;
    return true;
}

vector<mshrType> Cache::MSHR::removeAll(Addr _baseAddr){
    mshrTable::iterator it = map_.find(_baseAddr);
    assert(it != map_.end());
    vector<mshrType> res = it->second;
    map_.erase(it);
    
    int trueSize = 0;
    for(vector<mshrType>::iterator it = res.begin(); it != res.end(); it++){
        if((*it).elem.type() == typeid(MemEvent*)) trueSize++;
    }
    
    size_ -= trueSize; assert(size_ >= 0);
    return res;
}

MemEvent* Cache::MSHR::removeFront(Addr _baseAddr){
    mshrTable::iterator it = map_.find(_baseAddr);
    assert(it != map_.end());
    //assert(!it->second.empty()); assert(it->second.front().elem.type() == typeid(MemEvent*));
    
    MemEvent* ret = boost::get<MemEvent*>(it->second.front().elem);
    
    it->second.erase(it->second.begin());
    if(it->second.empty()) map_.erase(it);
    
    size_--;
    
    cache_->d_->debug(_L9_,"MSHR: Removed front event, Key Addr = %"PRIx64"\n", _baseAddr);
    return ret;
}


bool Cache::MSHR::isHit(Addr _baseAddr){ return map_.find(_baseAddr) != map_.end(); }


/* Function checks if incoming request should be stalled.  If there is mshr hit the
 * a pending instruction for the same address exists so new request should stall till
 * the this requests finishes */
bool Cache::MSHR::isHitAndStallNeeded(Addr _baseAddr, Command _cmd){
    mshrTable::iterator it = map_.find(_baseAddr);
    /* GetX does not stall if there is there is an Inv at the front of MSHR queue
     * since cache line can be user-locked.  If user-locked, Invalidates can pile up in the MSHR
     * but GetX still needs to continue since Inv won't get re-activated until user-lock is released */
    if(it == map_.end()) return false;
    if(it->second.front().elem.type() == typeid(Addr)) return false;  //front-of-the-vector event cannot be of pointer type
    MemEvent* frontEvent = boost::get<MemEvent*>(it->second.front().elem);
    if(_cmd == GetX) return (frontEvent->getCmd() != Inv && frontEvent->getCmd() != InvX);  //An Inv is at the front of the queue, wait for Inv Responses from higher network
    else{
        cache_->d_->debug(_L9_, "Blocking Request: MSHR entry exists and waiting to complete. TopOfQueue Request Cmd: %s\n", CommandString[frontEvent->getCmd()]);
        return true;
    }
}

struct MSHREntryCompare {
    enum Type {Event, Pointer};
    MSHREntryCompare( mshrType* _m ) : m_(_m) {
        if(m_->elem.type() == typeid(MemEvent*)) type_ = Event;
        else type_ = Pointer;
    }
    bool operator() (mshrType& _n){
        if(type_ == Event){
            if(_n.elem.type() == typeid(MemEvent*)) return boost::get<MemEvent*>(m_->elem) == boost::get<MemEvent*>(_n.elem);
            return false;
        }
        else{
            if(_n.elem.type() == typeid(Addr)) return boost::get<Addr>(m_->elem) == boost::get<Addr>(_n.elem);
            return false;
        }
    }
    mshrType* m_;
    Type type_;
};


void Cache::MSHR::removeElement(Addr _baseAddr, MemEvent* _event){
    removeElement(_baseAddr, mshrType(_event));
}

void Cache::MSHR::removeElement(Addr _baseAddr, Addr _pointer){
    removeElement(_baseAddr, mshrType(_pointer));
}

void Cache::MSHR::removeElement(Addr _baseAddr, mshrType _mshrEntry){

    mshrTable::iterator it = map_.find(_baseAddr);
    if(it == map_.end()) return;    
    cache_->d_->debug(_L9_,"MSHR Entry size = %lu\n", it->second.size());
    vector<mshrType>& res = it->second;
    vector<mshrType>::iterator itv = std::find_if(res.begin(), res.end(), MSHREntryCompare(&_mshrEntry));
    
    if(itv == res.end()) return;
    res.erase(std::remove_if(res.begin(), res.end(), MSHREntryCompare(&_mshrEntry)), res.end());

    if(res.empty()) map_.erase(it);
    size_--; assert(size_ >= 0);
    cache_->d_->debug(_L9_, "MSHR Removed Event\n");
}

/*
void Cache::MSHR::printEntry(Addr baseAddr){
    mshrTable::iterator it = map_.find(baseAddr);
    if(it == map_.end()) return;
    int i = 0;
    for(vector<MemEvent*>::iterator it = map_[baseAddr].begin(); it != map_[baseAddr].end(); ++it, ++i) {
        MemEvent* event = (MemEvent*)(*it);
        cache_->d_->debug(C,L5,0, "Entry %i:  Key Addr: %#016lllx, Event Addr: %#016lllx, Event Cmd = %s\n", i, baseAddr, event->getAddr(), CommandString[event->getCmd()]);
    }
}
*/

}};



#endif
