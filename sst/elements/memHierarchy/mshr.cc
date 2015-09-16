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

MSHR::MSHR(Output* _d, int _maxSize, string cacheName, bool debugAll, Addr debugAddr) {
    d_ = _d;
    maxSize_ = _maxSize;
    size_ = 0;
    ownerName_ = cacheName;

    d2_ = new Output();
    d2_->init("", 10, 0, (Output::output_location_t)1);

    DEBUG_ALL = debugAll;
    DEBUG_ADDR = debugAddr;
}


bool MSHR::isAlmostFull(){
    if(size_ >= (maxSize_-1)){
        return true;
    }
    return false;
}

bool MSHR::isFull(){
    if(size_ >= maxSize_){
        return true;
    }
    return false;
}


int MSHR::getAcksNeeded(Addr _baseAddr) {
    mshrTable::iterator it = map_.find(_baseAddr);
    if (it == map_.end()) return 0;
    return (it->second).acksNeeded;
}


void MSHR::setAcksNeeded(Addr _baseAddr, int acksNeeded) {
    mshrTable::iterator it = map_.find(_baseAddr);
    if (it == map_.end()) {
        if (DEBUG_ALL || _baseAddr == DEBUG_ADDR) d_->debug(_L6_, "Creating new MSHR holder for acks\n");
        mshrEntry entry;
        entry.acksNeeded = acksNeeded;
        entry.tempData.clear();
        map_[_baseAddr] = entry;
        return;
    }
    (it->second).acksNeeded = acksNeeded;
}

void MSHR::incrementAcksNeeded(Addr _baseAddr) {
    mshrTable::iterator it = map_.find(_baseAddr);
    if (it == map_.end()) {
        mshrEntry entry;
        entry.acksNeeded = 1;
        map_[_baseAddr] = entry;
    } else {
        (it->second).acksNeeded++;
    }
}

void MSHR::decrementAcksNeeded(Addr _baseAddr) {
    mshrTable::iterator it = map_.find(_baseAddr);
    if (it == map_.end()) return;
    (it->second).acksNeeded--;
    if (((it->second).acksNeeded == 0) && (it->second).tempData.empty() && (it->second).mshrQueue.empty()) {
        if (DEBUG_ALL || DEBUG_ADDR == _baseAddr) d_->debug(_L9_, "MSHR erasing 0x%" PRIx64 "\n", _baseAddr);
        map_.erase(it);
    }
}

void MSHR::setTempData(Addr baseAddr, vector<uint8_t>& data) {
    mshrTable::iterator it = map_.find(baseAddr);
    if (it == map_.end()) d2_->fatal(CALL_INFO,-1, "%s (MSHR), Error: No pending request for response event. Addr = 0x%" PRIx64 "\n", ownerName_.c_str(), baseAddr);
    (it->second).tempData = data;
}

vector<uint8_t> * MSHR::getTempData(Addr baseAddr) {
    mshrTable::iterator it = map_.find(baseAddr);
    if (it == map_.end()) return NULL;
    return &((it->second).tempData);
}

void MSHR::clearTempData(Addr baseAddr) {
    mshrTable::iterator it = map_.find(baseAddr);
    if (it != map_.end()) (it->second).tempData.clear();
    if (((it->second).acksNeeded == 0) && (it->second).tempData.empty() && (it->second).mshrQueue.empty()) {
        if (DEBUG_ALL || DEBUG_ADDR == baseAddr) d_->debug(_L9_, "MSHR erasing 0x%" PRIx64 "\n", baseAddr);
        map_.erase(it);
    }
}

bool MSHR::exists(Addr _baseAddr){
    mshrTable::iterator it = map_.find(_baseAddr);
    if (it == map_.end()) return false;
    vector<mshrType> * queue = &((it->second).mshrQueue);
    vector<mshrType>::iterator frontEntry = queue->begin();
    if (frontEntry == queue->end()) return false;
    return (frontEntry->elem.type() == typeid(MemEvent*));
}

bool MSHR::isHit(Addr _baseAddr){ return (map_.find(_baseAddr) != map_.end()) && (map_.find(_baseAddr)->second.mshrQueue.size() > 0); }

bool MSHR::pendingWriteback(Addr _baseAddr) {
    mshrType entry = mshrType(_baseAddr);
    mshrTable::iterator it = map_.find(_baseAddr);
    if (it == map_.end()) return false;

    vector<mshrType>& res = (it->second).mshrQueue;
    vector<mshrType>::iterator itv = std::find_if(res.begin(), res.end(), MSHREntryCompare(&entry));
    return (itv != res.end());
}

const vector<mshrType> MSHR::lookup(Addr _baseAddr){
    mshrTable::iterator it = map_.find(_baseAddr);
    if (it == map_.end()) {
        d2_->fatal(CALL_INFO,-1, "%s (MSHR), Error: mshr did not find entry with address 0x%" PRIx64 "\n", ownerName_.c_str(), _baseAddr);
    }
    vector<mshrType> res = (it->second).mshrQueue;
    return res;
}


MemEvent* MSHR::lookupFront(Addr _baseAddr){
    mshrTable::iterator it = map_.find(_baseAddr);
    if (it == map_.end()) {
        d2_->fatal(CALL_INFO,-1, "%s (MSHR), Error: mshr did not find entry with address 0x%" PRIx64 "\n", ownerName_.c_str(), _baseAddr);
    }
    vector<mshrType> queue = (it->second).mshrQueue;
    if (queue.front().elem.type() != typeid(MemEvent*)) {
        d2_->fatal(CALL_INFO,-1, "%s (MSHR), Error: front entry in mshr is not of type MemEvent. Addr = 0x%" PRIx64 "\n", ownerName_.c_str(), _baseAddr);
    }
    return boost::get<MemEvent*>(queue.front().elem);
}

/* Public insertion methods
 * insert(Addr, MemEvent) -> insert stalled event
 * insertPointer
 * insertWriteback
 * insertInv
 */
bool MSHR::insert(Addr _baseAddr, MemEvent* _event) {
    _event->setInMSHR(true);
    bool ret = insert(_baseAddr, mshrType(_event));
    if(LIKELY(ret)){
        if (DEBUG_ALL || DEBUG_ADDR == _baseAddr)
            d_->debug(_L9_, "MSHR: Event Inserted. Key addr = %" PRIx64 ", event Addr = %" PRIx64 ", Cmd = %s, MSHR Size = %u, Entry Size = %lu\n", _baseAddr, _event->getAddr(), CommandString[_event->getCmd()], size_, map_[_baseAddr].mshrQueue.size());
     }
    else if (DEBUG_ALL || DEBUG_ADDR == _baseAddr) d_->debug(_L9_, "MSHR Full.  Event could not be inserted.\n");
    return ret;
}


bool MSHR::insertPointer(Addr _keyAddr, Addr _pointerAddr) {
    if (_keyAddr == _pointerAddr) {
        d2_->fatal(CALL_INFO,-1, "%s (MSHR), Error: attempting to insert a self-pointer into the mshr. Addr = 0x%" PRIx64 "\n", ownerName_.c_str(), _keyAddr);
    }
    if (DEBUG_ALL || DEBUG_ADDR == _keyAddr || DEBUG_ADDR == _pointerAddr) d_->debug(_L9_, "MSHR: Inserted pointer.  Key Addr = %" PRIx64 ", Pointer Addr = %" PRIx64 "\n", _keyAddr, _pointerAddr);
    return insert(_keyAddr, mshrType(_pointerAddr));
}


bool MSHR::insertWriteback(Addr _keyAddr) {
    if (DEBUG_ALL || DEBUG_ADDR == _keyAddr) d_->debug(_L9_, "MSHR: Inserted writeback.  Key Addr = %" PRIx64 "\n", _keyAddr);
    mshrType mshrElement = mshrType(_keyAddr);

    mshrTable::iterator it = map_.find(_keyAddr);
    if (it == map_.end()) {
        mshrEntry newEntry;
        newEntry.acksNeeded = 0;
        newEntry.tempData.clear();
        map_[_keyAddr] = newEntry;
    }

    vector<mshrType>::iterator itv = map_[_keyAddr].mshrQueue.begin();
    map_[_keyAddr].mshrQueue.insert(itv, mshrElement);
    
    return true;
}

bool MSHR::insertInv(Addr _baseAddr, MemEvent* _event, bool inProgress) {
    _event->setInMSHR(true);
    bool ret = insertInv(_baseAddr, mshrType(_event), inProgress);
    if(LIKELY(ret)){
        if (DEBUG_ALL || DEBUG_ADDR == _baseAddr)
            d_->debug(_L9_, "MSHR: Event Inserted. Key addr = %" PRIx64 ", event Addr = %" PRIx64 ", Cmd = %s, MSHR Size = %u, Entry Size = %lu\n", _baseAddr, _event->getAddr(), CommandString[_event->getCmd()], size_, map_[_baseAddr].mshrQueue.size());
     }
    else if (DEBUG_ALL || DEBUG_ADDR == _baseAddr) d_->debug(_L9_, "MSHR Full.  Event could not be inserted.\n");
    return ret;
}

bool MSHR::insertAll(Addr _baseAddr, vector<mshrType>& _events){
    if(_events.empty()) return false;
    mshrTable::iterator it = map_.find(_baseAddr);
    
    if(it != map_.end()) (it->second).mshrQueue.insert((it->second).mshrQueue.end(), _events.begin(), _events.end());
    else {
        mshrEntry entry;
        entry.mshrQueue = _events;
        entry.acksNeeded = 0;
        entry.tempData.clear();
        map_[_baseAddr] = entry;
    }
    
    int trueSize = 0;
    for(vector<mshrType>::iterator it = _events.begin(); it != _events.end(); it++){
        if((*it).elem.type() == typeid(MemEvent*)) trueSize++;
    }
    
    size_ += trueSize;
    return true;
}


/* Private insertion methods called by public inserts */
bool MSHR::insert(Addr _baseAddr, mshrType _entry){
    if (_entry.elem.type() == typeid(MemEvent*)) {
        if (size_ >= maxSize_) return false;
        size_++;
    }
    mshrTable::iterator it = map_.find(_baseAddr);
    if (it == map_.end()) {
        mshrEntry entry;
        entry.acksNeeded = 0;
        entry.tempData.clear();
        map_[_baseAddr] = entry;
    }
    map_[_baseAddr].mshrQueue.push_back(_entry);
    
    return true;
}

bool MSHR::insertInv(Addr _baseAddr, mshrType _entry, bool inProgress) {
    if(size_ >= maxSize_) return false;
    
    vector<mshrType>::iterator it = map_[_baseAddr].mshrQueue.begin();
    /*int i = 0;
    for (it = map_[_baseAddr].mshrQueue.begin(); it != map_[_baseAddr].mshrQueue.end(); it++) {
        if (inProgress && it == map_[_baseAddr].mshrQueue.begin()) continue;
        if (it->elem.type() == typeid(MemEvent*)) {
            MemEvent * ev = boost::get<MemEvent*>(it->elem);
            if (ev->getCmd() == GetS || ev->getCmd() == GetSEx || ev->getCmd() == GetX) {
                break;
            }
        }
        i++;
    }*/
    if (inProgress && map_[_baseAddr].mshrQueue.size() > 0) it++;
    map_[_baseAddr].mshrQueue.insert(it, _entry);
    if(_entry.elem.type() == typeid(MemEvent*)) size_++;
    //printTable();
    return true;
}







MemEvent* MSHR::getOldestRequest() const {
    MemEvent *ev = NULL;
    for ( mshrTable::const_iterator it = map_.begin() ; it != map_.end() ; ++it ) {
        for ( vector<mshrType>::const_iterator jt = (it->second).mshrQueue.begin() ; jt != (it->second).mshrQueue.end() ; jt++ ) {
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


vector<mshrType> MSHR::removeAll(Addr _baseAddr) {
    mshrTable::iterator it = map_.find(_baseAddr);
    if (it == map_.end()) {
        d2_->fatal(CALL_INFO,-1, "%s (MSHR), Error: mshr did not find entry with address 0x%" PRIx64 "\n", ownerName_.c_str(), _baseAddr);
    }
    vector<mshrType> res = (it->second).mshrQueue;
    if ((it->second).acksNeeded == 0 && (it->second).tempData.empty()) {
        if (DEBUG_ALL || DEBUG_ADDR == _baseAddr) d_->debug(_L9_, "MSHR erasing 0x%" PRIx64 "\n", _baseAddr);
        map_.erase(it);
    } else {
        (it->second).mshrQueue.clear();
    }
    int trueSize = 0;
    for(vector<mshrType>::iterator it = res.begin(); it != res.end(); it++){
        if((*it).elem.type() == typeid(MemEvent*)) trueSize++;
    }
    
    size_ -= trueSize; 
    if (size_ < 0) {
        d2_->fatal(CALL_INFO, -1, "%s (MSHR), Error: mshr size < 0 after removing all elements for addr = 0x%" PRIx64 "\n", ownerName_.c_str(), _baseAddr);
    }
    return res;
}

MemEvent* MSHR::removeFront(Addr _baseAddr){
    mshrTable::iterator it = map_.find(_baseAddr);
    if (it == map_.end()) {
        d2_->fatal(CALL_INFO,-1, "%s (MSHR), Error: mshr did not find entry with address 0x%" PRIx64 "\n", ownerName_.c_str(), _baseAddr);
    }
    //if (it->second.empty()) {
    //  d2_->fatal(CALL_INFO,-1, "%s (MSHR), Error: no front entry to remove in mshr for addr = 0x%" PRIx64 "\n", ownerName_.c_str(), _baseAddr);
    //}
    //
    // if (it->second.front().elem.type() != typeid(MemEvent*)) {
    //     d2_->fatal(CALL_INFO,-1, "%s (MSHR), Error: front entry in mshr is not of type MemEvent. Addr = 0x%" PRIx64 "\n", ownerName_.c_str(), _baseAddr);
    // }
    
    MemEvent* ret = boost::get<MemEvent*>((it->second).mshrQueue.front().elem);
    
    (it->second).mshrQueue.erase(it->second.mshrQueue.begin());
    if (((it->second).acksNeeded == 0) && (it->second).tempData.empty() && (it->second).mshrQueue.empty()) {
        if (DEBUG_ALL || DEBUG_ADDR == _baseAddr) d_->debug(_L9_, "MSHR erasing 0x%" PRIx64 "\n", _baseAddr);
        map_.erase(it);
    }
    
    size_--;
    
    if (DEBUG_ALL || DEBUG_ADDR == _baseAddr) d_->debug(_L9_,"MSHR: Removed front event, Key Addr = %" PRIx64 "\n", _baseAddr);
    return ret;
}




void MSHR::removeElement(Addr _baseAddr, MemEvent* _event){
    removeElement(_baseAddr, mshrType(_event));
}

void MSHR::removeElement(Addr _baseAddr, Addr _pointer){
    removeElement(_baseAddr, mshrType(_pointer));
}

void MSHR::removeElement(Addr _baseAddr, mshrType _entry){

    mshrTable::iterator it = map_.find(_baseAddr);
    if (it == map_.end()) return;    
    if (DEBUG_ALL || DEBUG_ADDR == _baseAddr) d_->debug(_L9_,"MSHR Entry size = %lu\n", it->second.mshrQueue.size());
    vector<mshrType>& res = (it->second).mshrQueue;
    vector<mshrType>::iterator itv = std::find_if(res.begin(), res.end(), MSHREntryCompare(&_entry));
    
    if(itv == res.end()) return;
    res.erase(std::remove_if(res.begin(), res.end(), MSHREntryCompare(&_entry)), res.end());

    if (((it->second).acksNeeded == 0) && (it->second).tempData.empty() && (it->second).mshrQueue.empty()) {
        if (DEBUG_ALL || DEBUG_ADDR == _baseAddr) d_->debug(_L9_, "MSHR erasing 0x%" PRIx64 "\n", _baseAddr);
        map_.erase(it);
    }
    
    if (_entry.elem.type() == typeid(MemEvent*)) size_--; 
    if (size_ < 0) {
        d2_->fatal(CALL_INFO, -1, "%s (MSHR), Error: mshr size < 0 after removing element with address = 0x%" PRIx64 "\n", ownerName_.c_str(), _baseAddr);
    }
    if (DEBUG_ALL || DEBUG_ADDR == _baseAddr) d_->debug(_L9_, "MSHR Removed Event\n");
}


void MSHR::removeWriteback(Addr baseAddr) {
    MSHR::removeElement(baseAddr, mshrType(baseAddr));
}

bool MSHR::elementIsHit(Addr _baseAddr, MemEvent *_event) {
    mshrType _entry = mshrType(_event);

    mshrTable::iterator it = map_.find(_baseAddr);
    if(it == map_.end()) return false;    
    if (DEBUG_ALL || DEBUG_ADDR == _baseAddr) d_->debug(_L9_,"MSHR Entry size = %lu\n", it->second.mshrQueue.size());
    vector<mshrType>& res = (it->second).mshrQueue;
    vector<mshrType>::iterator itv = std::find_if(res.begin(), res.end(), MSHREntryCompare(&_entry));
    
    if(itv == res.end()) return false;
    return true;
}


void MSHR::printTable() {
    for (mshrTable::iterator it = map_.begin(); it != map_.end(); it++) {
        vector<mshrType> entries = (it->second).mshrQueue;
        d_->debug(_L9_, "MSHR: Addr = 0x%" PRIx64 "\n", (it->first));
        for (vector<mshrType>::iterator it2 = entries.begin(); it2 != entries.end(); it2++) {
            if (it2->elem.type() != typeid(MemEvent*)) {
                Addr ptr = boost::get<Addr>(it2->elem);
                d_->debug(_L9_, "\t0x%" PRIx64 "\n", ptr);
            } else {
                MemEvent * ev = boost::get<MemEvent*>(it2->elem);
                d_->debug(_L9_, "\t%s, %s\n", ev->getSrc().c_str(), CommandString[ev->getCmd()]);
            }
        }
    }
}


/*void MSHR::printEntry(Addr baseAddr){
    mshrTable::iterator it = map_.find(baseAddr);
    if(it == map_.end()) return;
    int i = 0;
    for(vector<MemEvent*>::iterator it = map_[baseAddr].begin(); it != map_[baseAddr].end(); ++it, ++i) {
        MemEvent* event = (MemEvent*)(*it);
        d_->debug(C,L5,0, "Entry %i:  Key Addr: %#016lllx, Event Addr: %#016lllx, Event Cmd = %s\n", i, baseAddr, event->getAddr(), CommandString[event->getCmd()]);
    }
}*/






