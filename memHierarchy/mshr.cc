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
        if (m_->elem.type() == typeid(MemEvent*)) type_ = Event;
        else type_ = Pointer;
    }
    bool operator() (mshrType& _n) {
        if (type_ == Event) {
            if (_n.elem.type() == typeid(MemEvent*)) return boost::get<MemEvent*>(m_->elem) == boost::get<MemEvent*>(_n.elem);
            return false;
        }
        else{
            if (_n.elem.type() == typeid(Addr)) return boost::get<Addr>(m_->elem) == boost::get<Addr>(_n.elem);
            return false;
        }
    }
    mshrType* m_;
    Type type_;
};

MSHR::MSHR(Output* debug, int maxSize, string cacheName, bool debugAll, Addr debugAddr) {
    d_ = debug;
    maxSize_ = maxSize;
    size_ = 0;
    ownerName_ = cacheName;

    d2_ = new Output();
    d2_->init("", 10, 0, (Output::output_location_t)1);

    DEBUG_ALL = debugAll;
    DEBUG_ADDR = debugAddr;
}


bool MSHR::isAlmostFull() {
    if (size_ >= (maxSize_-1)) {
        return true;
    }
    return false;
}

bool MSHR::isFull() {
    if (size_ >= maxSize_) {
        return true;
    }
    return false;
}


int MSHR::getAcksNeeded(Addr baseAddr) {
    mshrTable::iterator it = map_.find(baseAddr);
    if (it == map_.end()) return 0;
    return (it->second).acksNeeded;
}


void MSHR::setAcksNeeded(Addr baseAddr, int acksNeeded) {
    mshrTable::iterator it = map_.find(baseAddr);
    if (it == map_.end()) {
#ifdef __SST_DEBUG_OUTPUT__
        if (DEBUG_ALL || baseAddr == DEBUG_ADDR) d_->debug(_L6_, "Creating new MSHR holder for acks\n");
#endif
        mshrEntry entry;
        entry.acksNeeded = acksNeeded;
        entry.tempData.clear();
        map_[baseAddr] = entry;
        return;
    }
    (it->second).acksNeeded = acksNeeded;
}

void MSHR::incrementAcksNeeded(Addr baseAddr) {
    mshrTable::iterator it = map_.find(baseAddr);
    if (it == map_.end()) {
        mshrEntry entry;
        entry.acksNeeded = 1;
        map_[baseAddr] = entry;
    } else {
        (it->second).acksNeeded++;
    }
}

void MSHR::decrementAcksNeeded(Addr baseAddr) {
    mshrTable::iterator it = map_.find(baseAddr);
    if (it == map_.end()) return;
    (it->second).acksNeeded--;
    if (((it->second).acksNeeded == 0) && (it->second).tempData.empty() && (it->second).mshrQueue.empty()) {
#ifdef __SST_DEBUG_OUTPUT__
        if (DEBUG_ALL || DEBUG_ADDR == baseAddr) d_->debug(_L9_, "MSHR erasing 0x%" PRIx64 "\n", baseAddr);
#endif
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
#ifdef __SST_DEBUG_OUTPUT__
        if (DEBUG_ALL || DEBUG_ADDR == baseAddr) d_->debug(_L9_, "MSHR erasing 0x%" PRIx64 "\n", baseAddr);
#endif
        map_.erase(it);
    }
}

bool MSHR::exists(Addr baseAddr) {
    mshrTable::iterator it = map_.find(baseAddr);
    if (it == map_.end()) return false;
    vector<mshrType> * queue = &((it->second).mshrQueue);
    vector<mshrType>::iterator frontEntry = queue->begin();
    if (frontEntry == queue->end()) return false;
    return (frontEntry->elem.type() == typeid(MemEvent*));
}

bool MSHR::isHit(Addr baseAddr) { return (map_.find(baseAddr) != map_.end()) && (map_.find(baseAddr)->second.mshrQueue.size() > 0); }

bool MSHR::pendingWriteback(Addr baseAddr) {
    mshrType entry = mshrType(baseAddr);
    mshrTable::iterator it = map_.find(baseAddr);
    if (it == map_.end()) return false;

    vector<mshrType>& res = (it->second).mshrQueue;
    vector<mshrType>::iterator itv = std::find_if(res.begin(), res.end(), MSHREntryCompare(&entry));
    return (itv != res.end());
}

const vector<mshrType> MSHR::lookup(Addr baseAddr) {
    mshrTable::iterator it = map_.find(baseAddr);
    if (it == map_.end()) {
        d2_->fatal(CALL_INFO,-1, "%s (MSHR), Error: mshr did not find entry with address 0x%" PRIx64 "\n", ownerName_.c_str(), baseAddr);
    }
    vector<mshrType> res = (it->second).mshrQueue;
    return res;
}


MemEvent* MSHR::lookupFront(Addr baseAddr) {
    mshrTable::iterator it = map_.find(baseAddr);
    if (it == map_.end()) {
        d2_->fatal(CALL_INFO,-1, "%s (MSHR), Error: mshr did not find entry with address 0x%" PRIx64 "\n", ownerName_.c_str(), baseAddr);
    }
    vector<mshrType> queue = (it->second).mshrQueue;
    if (queue.front().elem.type() != typeid(MemEvent*)) {
        d2_->fatal(CALL_INFO,-1, "%s (MSHR), Error: front entry in mshr is not of type MemEvent. Addr = 0x%" PRIx64 "\n", ownerName_.c_str(), baseAddr);
    }
    return boost::get<MemEvent*>(queue.front().elem);
}

/* Public insertion methods
 * insert(Addr, MemEvent) -> insert stalled event
 * insertPointer
 * insertWriteback
 * insertInv
 */
bool MSHR::insert(Addr baseAddr, MemEvent* event) {
    bool ret = insert(baseAddr, mshrType(event));
#ifdef __SST_DEBUG_OUTPUT__
    if (LIKELY(ret)) {
        if (DEBUG_ALL || DEBUG_ADDR == baseAddr)
            d_->debug(_L9_, "MSHR: Event Inserted. Key addr = %" PRIx64 ", event Addr = %" PRIx64 ", Cmd = %s, MSHR Size = %u, Entry Size = %lu\n", baseAddr, event->getAddr(), CommandString[event->getCmd()], size_, map_[baseAddr].mshrQueue.size());
     }
    else if (DEBUG_ALL || DEBUG_ADDR == baseAddr) d_->debug(_L9_, "MSHR Full.  Event could not be inserted.\n");
#endif
    return ret;
}


bool MSHR::insertPointer(Addr keyAddr, Addr pointerAddr) {
    if (keyAddr == pointerAddr) {
        d2_->fatal(CALL_INFO,-1, "%s (MSHR), Error: attempting to insert a self-pointer into the mshr. Addr = 0x%" PRIx64 "\n", ownerName_.c_str(), keyAddr);
    }
#ifdef __SST_DEBUG_OUTPUT__
    if (DEBUG_ALL || DEBUG_ADDR == keyAddr || DEBUG_ADDR == pointerAddr) d_->debug(_L9_, "MSHR: Inserted pointer.  Key Addr = %" PRIx64 ", Pointer Addr = %" PRIx64 "\n", keyAddr, pointerAddr);
#endif
    return insert(keyAddr, mshrType(pointerAddr));
}


bool MSHR::insertWriteback(Addr keyAddr) {
#ifdef __SST_DEBUG_OUTPUT__
    if (DEBUG_ALL || DEBUG_ADDR == keyAddr) d_->debug(_L9_, "MSHR: Inserted writeback.  Key Addr = %" PRIx64 "\n", keyAddr);
#endif
    mshrType mshrElement = mshrType(keyAddr);

    mshrTable::iterator it = map_.find(keyAddr);
    if (it == map_.end()) {
        mshrEntry newEntry;
        newEntry.acksNeeded = 0;
        newEntry.tempData.clear();
        map_[keyAddr] = newEntry;
    }

    vector<mshrType>::iterator itv = map_[keyAddr].mshrQueue.begin();
    map_[keyAddr].mshrQueue.insert(itv, mshrElement);
    
    return true;
}

bool MSHR::insertInv(Addr baseAddr, MemEvent* event, bool inProgress) {
    bool ret = insertInv(baseAddr, mshrType(event), inProgress);
#ifdef __SST_DEBUG_OUTPUT__
    if (LIKELY(ret)) {
        if (DEBUG_ALL || DEBUG_ADDR == baseAddr)
            d_->debug(_L9_, "MSHR: Event Inserted. Key addr = %" PRIx64 ", event Addr = %" PRIx64 ", Cmd = %s, MSHR Size = %u, Entry Size = %lu\n", baseAddr, event->getAddr(), CommandString[event->getCmd()], size_, map_[baseAddr].mshrQueue.size());
     }
    else if (DEBUG_ALL || DEBUG_ADDR == baseAddr) d_->debug(_L9_, "MSHR Full.  Event could not be inserted.\n");
#endif
    return ret;
}

bool MSHR::insertAll(Addr baseAddr, vector<mshrType>& events) {
    if (events.empty()) return false;
    mshrTable::iterator it = map_.find(baseAddr);
    
    if (it != map_.end()) (it->second).mshrQueue.insert((it->second).mshrQueue.end(), events.begin(), events.end());
    else {
        mshrEntry entry;
        entry.mshrQueue = events;
        entry.acksNeeded = 0;
        entry.tempData.clear();
        map_[baseAddr] = entry;
    }
    
    int trueSize = 0;
    for (vector<mshrType>::iterator it = events.begin(); it != events.end(); it++) {
        if ((*it).elem.type() == typeid(MemEvent*)) trueSize++;
    }
    
    size_ += trueSize;
    return true;
}


/* Private insertion methods called by public inserts */
bool MSHR::insert(Addr baseAddr, mshrType entry) {
    if (entry.elem.type() == typeid(MemEvent*)) {
        if (size_ >= maxSize_) return false;
        size_++;
    }
    mshrTable::iterator it = map_.find(baseAddr);
    if (it == map_.end()) {
        mshrEntry entry;
        entry.acksNeeded = 0;
        entry.tempData.clear();
        map_[baseAddr] = entry;
    }
    map_[baseAddr].mshrQueue.push_back(entry);
    
    return true;
}

bool MSHR::insertInv(Addr baseAddr, mshrType entry, bool inProgress) {
    if (size_ >= maxSize_) return false;
    
    vector<mshrType>::iterator it = map_[baseAddr].mshrQueue.begin();
    /*int i = 0;
    for (it = map_[baseAddr].mshrQueue.begin(); it != map_[baseAddr].mshrQueue.end(); it++) {
        if (inProgress && it == map_[baseAddr].mshrQueue.begin()) continue;
        if (it->elem.type() == typeid(MemEvent*)) {
            MemEvent * ev = boost::get<MemEvent*>(it->elem);
            if (ev->getCmd() == GetS || ev->getCmd() == GetSEx || ev->getCmd() == GetX) {
                break;
            }
        }
        i++;
    }*/
    if (inProgress && map_[baseAddr].mshrQueue.size() > 0) it++;
    map_[baseAddr].mshrQueue.insert(it, entry);
    if (entry.elem.type() == typeid(MemEvent*)) size_++;
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


vector<mshrType> MSHR::removeAll(Addr baseAddr) {
    mshrTable::iterator it = map_.find(baseAddr);
    if (it == map_.end()) {
        d2_->fatal(CALL_INFO,-1, "%s (MSHR), Error: mshr did not find entry with address 0x%" PRIx64 "\n", ownerName_.c_str(), baseAddr);
    }
    vector<mshrType> res = (it->second).mshrQueue;
    if ((it->second).acksNeeded == 0 && (it->second).tempData.empty()) {
#ifdef __SST_DEBUG_OUTPUT__
        if (DEBUG_ALL || DEBUG_ADDR == baseAddr) d_->debug(_L9_, "MSHR erasing 0x%" PRIx64 "\n", baseAddr);
#endif
        map_.erase(it);
    } else {
        (it->second).mshrQueue.clear();
    }
    int trueSize = 0;
    for (vector<mshrType>::iterator it = res.begin(); it != res.end(); it++) {
        if ((*it).elem.type() == typeid(MemEvent*)) trueSize++;
    }
    
    size_ -= trueSize; 
    if (size_ < 0) {
        d2_->fatal(CALL_INFO, -1, "%s (MSHR), Error: mshr size < 0 after removing all elements for addr = 0x%" PRIx64 "\n", ownerName_.c_str(), baseAddr);
    }
    return res;
}

MemEvent* MSHR::removeFront(Addr baseAddr) {
    mshrTable::iterator it = map_.find(baseAddr);
    if (it == map_.end()) {
        d2_->fatal(CALL_INFO,-1, "%s (MSHR), Error: mshr did not find entry with address 0x%" PRIx64 "\n", ownerName_.c_str(), baseAddr);
    }
    //if (it->second.empty()) {
    //  d2_->fatal(CALL_INFO,-1, "%s (MSHR), Error: no front entry to remove in mshr for addr = 0x%" PRIx64 "\n", ownerName_.c_str(), baseAddr);
    //}
    //
    // if (it->second.front().elem.type() != typeid(MemEvent*)) {
    //     d2_->fatal(CALL_INFO,-1, "%s (MSHR), Error: front entry in mshr is not of type MemEvent. Addr = 0x%" PRIx64 "\n", ownerName_.c_str(), baseAddr);
    // }
    
    MemEvent* ret = boost::get<MemEvent*>((it->second).mshrQueue.front().elem);
    
    (it->second).mshrQueue.erase(it->second.mshrQueue.begin());
    if (((it->second).acksNeeded == 0) && (it->second).tempData.empty() && (it->second).mshrQueue.empty()) {
#ifdef __SST_DEBUG_OUTPUT__
        if (DEBUG_ALL || DEBUG_ADDR == baseAddr) d_->debug(_L9_, "MSHR erasing 0x%" PRIx64 "\n", baseAddr);
#endif
        map_.erase(it);
    }
    
    size_--;
    
#ifdef __SST_DEBUG_OUTPUT__
    if (DEBUG_ALL || DEBUG_ADDR == baseAddr) d_->debug(_L9_,"MSHR: Removed front event, Key Addr = %" PRIx64 "\n", baseAddr);
#endif
    return ret;
}




void MSHR::removeElement(Addr baseAddr, MemEvent* event) {
    removeElement(baseAddr, mshrType(event));
}

void MSHR::removeElement(Addr baseAddr, Addr pointer) {
    removeElement(baseAddr, mshrType(pointer));
}

void MSHR::removeElement(Addr baseAddr, mshrType entry) {

    mshrTable::iterator it = map_.find(baseAddr);
    if (it == map_.end()) return;    
#ifdef __SST_DEBUG_OUTPUT__
    if (DEBUG_ALL || DEBUG_ADDR == baseAddr) d_->debug(_L9_,"MSHR Entry size = %lu\n", it->second.mshrQueue.size());
#endif
    vector<mshrType>& res = (it->second).mshrQueue;
    vector<mshrType>::iterator itv = std::find_if(res.begin(), res.end(), MSHREntryCompare(&entry));
    
    if (itv == res.end()) return;
    res.erase(std::remove_if(res.begin(), res.end(), MSHREntryCompare(&entry)), res.end());

    if (((it->second).acksNeeded == 0) && (it->second).tempData.empty() && (it->second).mshrQueue.empty()) {
#ifdef __SST_DEBUG_OUTPUT__
        if (DEBUG_ALL || DEBUG_ADDR == baseAddr) d_->debug(_L9_, "MSHR erasing 0x%" PRIx64 "\n", baseAddr);
#endif
        map_.erase(it);
    }
    
    if (entry.elem.type() == typeid(MemEvent*)) size_--; 
    if (size_ < 0) {
        d2_->fatal(CALL_INFO, -1, "%s (MSHR), Error: mshr size < 0 after removing element with address = 0x%" PRIx64 "\n", ownerName_.c_str(), baseAddr);
    }
#ifdef __SST_DEBUG_OUTPUT__
    if (DEBUG_ALL || DEBUG_ADDR == baseAddr) d_->debug(_L9_, "MSHR Removed Event\n");
#endif
}


void MSHR::removeWriteback(Addr baseAddr) {
    MSHR::removeElement(baseAddr, mshrType(baseAddr));
}

bool MSHR::elementIsHit(Addr baseAddr, MemEvent *event) {
    mshrType entry = mshrType(event);

    mshrTable::iterator it = map_.find(baseAddr);
    if (it == map_.end()) return false;    
#ifdef __SST_DEBUG_OUTPUT__
    if (DEBUG_ALL || DEBUG_ADDR == baseAddr) d_->debug(_L9_,"MSHR Entry size = %lu\n", it->second.mshrQueue.size());
#endif
    vector<mshrType>& res = (it->second).mshrQueue;
    vector<mshrType>::iterator itv = std::find_if (res.begin(), res.end(), MSHREntryCompare(&entry));
    
    if (itv == res.end()) return false;
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


/*void MSHR::printEntry(Addr baseAddr) {
    mshrTable::iterator it = map_.find(baseAddr);
    if (it == map_.end()) return;
    int i = 0;
    for (vector<MemEvent*>::iterator it = map_[baseAddr].begin(); it != map_[baseAddr].end(); ++it, ++i) {
        MemEvent* event = (MemEvent*)(*it);
        d_->debug(C,L5,0, "Entry %i:  Key Addr: %#016lllx, Event Addr: %#016lllx, Event Cmd = %s\n", i, baseAddr, event->getAddr(), CommandString[event->getCmd()]);
    }
}*/






