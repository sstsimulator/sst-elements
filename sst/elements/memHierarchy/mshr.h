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


const vector<mshrType> Cache::MSHR::lookup(Addr _baseAddr){
    mshrTable::iterator it = map_.find(_baseAddr);
    assert(it != map_.end());
    vector<mshrType> res = it->second;
    return res;
}

bool Cache::MSHR::insertPointer(Addr _keyAddr, Addr _pointerAddr){
    return insert(_keyAddr, _pointerAddr);
}

bool Cache::MSHR::insert(Addr _baseAddr, MemEvent* _event){
    cache_->d_->debug(_L6_, "MSHR Event Inserted: Key Addr = %"PRIx64", Event Addr = %"PRIx64", Cmd = %s, MSHR Size = %u, Entry Size = %lu\n", _baseAddr, _event->getAddr(), CommandString[_event->getCmd()], size_, map_[_baseAddr].size());
    bool ret = insert(_baseAddr, mshrType(_event));
    if(LIKELY(ret)){
        _event->setStartTime(cache_->getCurrentSimTime());
    }
    return ret;
}

bool Cache::MSHR::insert(Addr _baseAddr, Addr _pointer){
    return insert(_baseAddr, mshrType(_pointer));
}

bool Cache::MSHR::insert(Addr _baseAddr, mshrType _mshrEntry) throw(mshrException){
    if(size_ >= maxSize_) throw mshrException();
    map_[_baseAddr].push_back(_mshrEntry);
    size_++;
    return true;
}

bool Cache::MSHR::insertAll(Addr _baseAddr, vector<mshrType> _events) throw(mshrException){
    if(_events.empty()) return false;
    mshrTable::iterator it = map_.find(_baseAddr);
    
    if(it != map_.end()) it->second.insert(it->second.end(), _events.begin(), _events.end());
    else {
        map_[_baseAddr] = _events;
    }

    size_ = size_ + _events.size();
    return true;
}

vector<mshrType> Cache::MSHR::removeAll(Addr _baseAddr){
    mshrTable::iterator it = map_.find(_baseAddr);
    assert(it != map_.end());
    vector<mshrType> res = it->second;
    map_.erase(it);
    size_ = size_ - res.size(); assert(size_ >= 0);
    return res;
}

bool Cache::MSHR::isHit(Addr _baseAddr){ return map_.find(_baseAddr) != map_.end(); }


/* Function checks if incoming request should be stalled.  If there is mshr hit the
 * a pending instruction for the same address exists so new request should stall till
 * the this requests finishes */
bool Cache::MSHR::isHitAndStallNeeded(Addr _baseAddr, Command _cmd){
    mshrTable::iterator it = map_.find(_baseAddr);
    /*GetX does not stall if there is there is an Inv at the front of MSHR queue 
     * since cache line can be user-locked.  If user-locked, Invalidates can pile up in the MSHR
     * but GetX still needs to continue since Inv won't get re-activated until user-lock is released */
    if(it == map_.end()) return false;
    if(it->second.front().elem.type() == typeid(Addr)) return false;  //front-of-the-vector event cannot be of pointer type
    MemEvent* frontEvent = boost::get<MemEvent*>(it->second.front().elem);
    if(_cmd == GetX) return (frontEvent->getCmd() != Inv && frontEvent->getCmd() != InvX);  //An Inv is at the front of the queue, wait for Inv Responses from higher network
    else{
        cache_->d_->debug(_WARNING_, "Blocking Request: MSHR entry exists and waiting to complete. TopOfQueue Request Cmd: %s\n", CommandString[frontEvent->getCmd()]);
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
    cache_->d_->debug(_L6_,"MSHR Entry size = %lu\n", it->second.size());
    vector<mshrType>& res = it->second;
    vector<mshrType>::iterator itv = std::find_if(res.begin(), res.end(), MSHREntryCompare(&_mshrEntry));
    
    if(itv == res.end()) return;
    res.erase(std::remove_if(res.begin(), res.end(), MSHREntryCompare(&_mshrEntry)), res.end());

    if(res.empty()) map_.erase(it);
    size_--; assert(size_ >= 0);
    cache_->d_->debug(_L6_, "MSHR Removed Event\n");
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
