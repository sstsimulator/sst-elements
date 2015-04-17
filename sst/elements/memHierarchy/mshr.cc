// Copyright 2009-2015 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2015, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include <sst_config.h>
#include "mshr.h"

using namespace SST;
using namespace SST::MemHierarchy;

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

MSHR::MSHR(Output* _d, int _maxSize) {
    d_ = _d;
    maxSize_ = _maxSize;
    size_ = 0;
    
    d2_ = new Output();
    d2_->init("", 10, 0, (Output::output_location_t)1);
}

bool MSHR::exists(Addr _baseAddr){
    mshrTable::iterator it = map_.find(_baseAddr);
    return (it != map_.end() && it->second.front().elem.type() == typeid(MemEvent*));
}

const vector<mshrType> MSHR::lookup(Addr _baseAddr){
    mshrTable::iterator it = map_.find(_baseAddr);
    if (it == map_.end()) {
        d_->fatal(CALL_INFO,-1, "Error: mshr did not find entry with address 0x%" PRIx64 "\n", _baseAddr);
    }
    vector<mshrType> res = it->second;
    return res;
}


MemEvent* MSHR::lookupFront(Addr _baseAddr){
    mshrTable::iterator it = map_.find(_baseAddr);
    if (it == map_.end()) {
        d_->fatal(CALL_INFO,-1, "Error: mshr did not find entry with address 0x%" PRIx64 "\n", _baseAddr);
    }
    vector<mshrType> mshrEntry = it->second;
    if (mshrEntry.front().elem.type() != typeid(MemEvent*)) {
        d_->fatal(CALL_INFO,-1, "Error: front entry in mshr is not of type MemEvent. Addr = 0x%" PRIx64 "\n", _baseAddr);
    }
    return boost::get<MemEvent*>(mshrEntry.front().elem);
}

bool MSHR::insertPointer(Addr _keyAddr, Addr _pointerAddr){
    if (_keyAddr == _pointerAddr) {
        d_->fatal(CALL_INFO,-1, "Error: attempting to insert a self-pointer into the mshr. Addr = 0x%" PRIx64 "\n", _keyAddr);
    }
    return insert(_keyAddr, _pointerAddr);
}

bool MSHR::isAlmostFull(){
    if(size_ >= (maxSize_-1)){
        d_->debug(_L9_,"MSHR is almost full. Request event will not be inserted to avoid deadlock. Max Size = %u\n", maxSize_);
        return true;
    }
    return false;
}

bool MSHR::isFull(){
    if(size_ >= maxSize_){
        d_->debug(_L9_,"MSHR Full. Event could not be inserted. Max Size = %u\n", maxSize_);
        return true;
    }
    return false;
}

MemEvent* MSHR::getOldestRequest() const {
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
}


bool MSHR::insert(Addr _baseAddr, MemEvent* _event){
    _event->setInMSHR(true);
    bool ret = insert(_baseAddr, mshrType(_event));
    if(LIKELY(ret)){
        d_->debug(_L9_, "MSHR: Event Inserted. Key addr = %" PRIx64 ", event Addr = %" PRIx64 ", Cmd = %s, MSHR Size = %u, Entry Size = %lu\n", _baseAddr, _event->getAddr(), CommandString[_event->getCmd()], size_, map_[_baseAddr].size());
     }
    else d_->debug(_L9_, "MSHR Full.  Event could not be inserted.\n");
    return ret;
}

bool MSHR::insertInv(Addr _baseAddr, MemEvent* _event){
    _event->setInMSHR(true);
    bool ret = insertInv(_baseAddr, mshrType(_event));
    if(LIKELY(ret)){
        d_->debug(_L9_, "MSHR: Event Inserted. Key addr = %" PRIx64 ", event Addr = %" PRIx64 ", Cmd = %s, MSHR Size = %u, Entry Size = %lu\n", _baseAddr, _event->getAddr(), CommandString[_event->getCmd()], size_, map_[_baseAddr].size());
     }
    else d_->debug(_L9_, "MSHR Full.  Event could not be inserted.\n");
    return ret;
}




bool MSHR::insert(Addr _keyAddr, Addr _pointerAddr){
    bool ret = insert(_keyAddr, mshrType(_pointerAddr));
    if(LIKELY(ret)) {
	d_->debug(_L9_, "MSHR: Inserted pointer.  Key Addr = %" PRIx64 ", Pointer Addr = %" PRIx64 "\n", _keyAddr, _pointerAddr);
    } else
        d_->debug(_L9_, "MSHR Full. Pointer could not be inserted.\n");

    return ret;
}

bool MSHR::insert(Addr _baseAddr, mshrType _mshrEntry){
    if(size_ >= maxSize_) return false;
    map_[_baseAddr].push_back(_mshrEntry);
    if(_mshrEntry.elem.type() == typeid(MemEvent*)) size_++;
    return true;
}

bool MSHR::insertInv(Addr _baseAddr, mshrType _mshrEntry){
    if(size_ >= maxSize_) return false;
    if (map_[_baseAddr].size() > 1) {
        map_[_baseAddr].insert((map_[_baseAddr].begin()+1),_mshrEntry);
    } else map_[_baseAddr].push_back(_mshrEntry);
    if(_mshrEntry.elem.type() == typeid(MemEvent*)) size_++;
    return true;
}


bool MSHR::insertAll(Addr _baseAddr, vector<mshrType> _events){
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

vector<mshrType> MSHR::removeAll(Addr _baseAddr){
    mshrTable::iterator it = map_.find(_baseAddr);
    if (it == map_.end()) {
        d_->fatal(CALL_INFO,-1, "Error: mshr did not find entry with address 0x%" PRIx64 "\n", _baseAddr);
    }
    vector<mshrType> res = it->second;
    map_.erase(it);
    
    int trueSize = 0;
    for(vector<mshrType>::iterator it = res.begin(); it != res.end(); it++){
        if((*it).elem.type() == typeid(MemEvent*)) trueSize++;
    }
    
    size_ -= trueSize; 
    if (size_ < 0) {
        d_->fatal(CALL_INFO, -1, "Error: mshr size < 0 after removing all elements for addr = 0x%" PRIx64 "\n", _baseAddr);
    }
    return res;
}

MemEvent* MSHR::removeFront(Addr _baseAddr){
    mshrTable::iterator it = map_.find(_baseAddr);
    if (it == map_.end()) {
        d_->fatal(CALL_INFO,-1, "Error: mshr did not find entry with address 0x%" PRIx64 "\n", _baseAddr);
    }
    //if (it->second.empty()) {
    //  d_->fatal(CALL_INFO,-1, "Error: no front entry to remove in mshr for addr = 0x%" PRIx64 "\n", _baseAddr);
    //}
    //
    // if (it->second.front().elem.type() != typeid(MemEvent*)) {
    //     d_->fatal(CALL_INFO,-1, "Error: front entry in mshr is not of type MemEvent. Addr = 0x%" PRIx64 "\n", _baseAddr);
    // }
    
    MemEvent* ret = boost::get<MemEvent*>(it->second.front().elem);
    
    it->second.erase(it->second.begin());
    if(it->second.empty()) map_.erase(it);
    
    size_--;
    
    d_->debug(_L9_,"MSHR: Removed front event, Key Addr = %" PRIx64 "\n", _baseAddr);
    return ret;
}


bool MSHR::isHit(Addr _baseAddr){ return map_.find(_baseAddr) != map_.end(); }


/* Function checks if incoming request should be stalled.  If there is mshr hit the
 * a pending instruction for the same address exists so new request should stall till
 * the this requests finishes */
bool MSHR::isHitAndStallNeeded(Addr _baseAddr, Command _cmd){
    mshrTable::iterator it = map_.find(_baseAddr);
    /* GetX does not stall if there is there is an Inv at the front of MSHR queue
     * since cache line can be user-locked.  If user-locked, Invalidates can pile up in the MSHR
     * but GetX still needs to continue since Inv won't get re-activated until user-lock is released */
    
    /* Miss */
    if(it == map_.end()) return false;

    if(it->second.front().elem.type() == typeid(Addr)) return false;  //front-of-the-vector event cannot be of pointer type
    MemEvent* frontEvent = boost::get<MemEvent*>(it->second.front().elem);
    

    if(_cmd == GetX) return (frontEvent->getCmd() != Inv && frontEvent->getCmd() != FetchInv && frontEvent->getCmd() != FetchInvX);  //An Inv is at the front of the queue, wait for Inv Responses from higher network
    else{
        d_->debug(_L9_, "Blocking Request: MSHR entry exists and waiting to complete. TopOfQueue Request Cmd: %s\n", CommandString[frontEvent->getCmd()]);
        return true;
    }
}




void MSHR::removeElement(Addr _baseAddr, MemEvent* _event){
    removeElement(_baseAddr, mshrType(_event));
}

void MSHR::removeElement(Addr _baseAddr, Addr _pointer){
    removeElement(_baseAddr, mshrType(_pointer));
}

void MSHR::removeElement(Addr _baseAddr, mshrType _mshrEntry){

    mshrTable::iterator it = map_.find(_baseAddr);
    if(it == map_.end()) return;    
    d_->debug(_L9_,"MSHR Entry size = %lu\n", it->second.size());
    vector<mshrType>& res = it->second;
    vector<mshrType>::iterator itv = std::find_if(res.begin(), res.end(), MSHREntryCompare(&_mshrEntry));
    
    if(itv == res.end()) return;
    res.erase(std::remove_if(res.begin(), res.end(), MSHREntryCompare(&_mshrEntry)), res.end());

    if(res.empty()) map_.erase(it);
    size_--; 
    if (size_ < 0) {
        d_->fatal(CALL_INFO, -1, "Error: mshr size < 0 after removing element with address = 0x%" PRIx64 "\n", _baseAddr);
    }
    d_->debug(_L9_, "MSHR Removed Event\n");
}

bool MSHR::elementIsHit(Addr _baseAddr, MemEvent *_event) {
    mshrType _mshrEntry = mshrType(_event);

    mshrTable::iterator it = map_.find(_baseAddr);
    if(it == map_.end()) return false;    
    d_->debug(_L9_,"MSHR Entry size = %lu\n", it->second.size());
    vector<mshrType>& res = it->second;
    vector<mshrType>::iterator itv = std::find_if(res.begin(), res.end(), MSHREntryCompare(&_mshrEntry));
    
    if(itv == res.end()) return false;
    return true;
}



/*
void MSHR::printEntry(Addr baseAddr){
    mshrTable::iterator it = map_.find(baseAddr);
    if(it == map_.end()) return;
    int i = 0;
    for(vector<MemEvent*>::iterator it = map_[baseAddr].begin(); it != map_[baseAddr].end(); ++it, ++i) {
        MemEvent* event = (MemEvent*)(*it);
        d_->debug(C,L5,0, "Entry %i:  Key Addr: %#016lllx, Event Addr: %#016lllx, Event Cmd = %s\n", i, baseAddr, event->getAddr(), CommandString[event->getCmd()]);
    }
}
*/





