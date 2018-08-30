// Copyright 2009-2018 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2018, NTESS
// All rights reserved.
// 
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include <sst_config.h>
#include "mshr.h"

#include <algorithm>

using namespace SST;
using namespace SST::MemHierarchy;

/* Debug macros */
#ifdef __SST_DEBUG_OUTPUT__ /* From sst-core, enable with --enable-debug */
#define is_debug_addr(addr) (DEBUG_ADDR.empty() || DEBUG_ADDR.find(addr) != DEBUG_ADDR.end())
#define is_debug_event(ev) (DEBUG_ADDR.empty() || ev->doDebug(DEBUG_ADDR))
#else
#define is_debug_addr(addr) false
#define is_debug_event(ev) false
#endif

struct MSHREntryCompare {
    enum Type {Event, Pointer};
    MSHREntryCompare( mshrType* _m ) : m_(_m) {
        if (m_->elem.isEvent()) type_ = Event;
        else type_ = Pointer;
    }
    bool operator() (mshrType& _n) {
        if (type_ == Event) {
            if (_n.elem.isEvent()) return (m_->elem).getEvent() == (_n.elem).getEvent();
            return false;
        }
        else{
            if (_n.elem.isAddr()) return (m_->elem).getAddr() == (_n.elem).getAddr();
            return false;
        }
    }
    mshrType* m_;
    Type type_;
};

MSHR::MSHR(Output* debug, int maxSize, string cacheName, std::set<Addr> debugAddr) {
    d_ = debug;
    maxSize_ = maxSize;
    size_ = 0;
    prefetchCount_ = 0;
    ownerName_ = cacheName;

    d2_ = new Output();
    d2_->init("", 10, 0, (Output::output_location_t)1);

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


void MSHR::setAcksNeeded(Addr baseAddr, int acksNeeded, MemEvent * event) {
    mshrTable::iterator it = map_.find(baseAddr);
    if (it == map_.end()) {
        if (is_debug_addr(baseAddr)) d_->debug(_L6_, "\tCreating new MSHR holder for acks\n");
        
        mshrEntry entry;
        entry.acksNeeded = acksNeeded;
        entry.dataBuffer.clear();
        if (event != nullptr)
            entry.mshrQueue.push_back(mshrType(event));
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
    if (((it->second).acksNeeded == 0) && (it->second).dataBuffer.empty() && (it->second).mshrQueue.empty()) {
        if (is_debug_addr(baseAddr)) d_->debug(_L9_, "\tMSHR erasing 0x%" PRIx64 "\n", baseAddr);
        
        map_.erase(it);
    }
}

void MSHR::setDataBuffer(Addr baseAddr, vector<uint8_t>& data) {
    mshrTable::iterator it = map_.find(baseAddr);
    if (it == map_.end()) d2_->fatal(CALL_INFO,-1, "%s (MSHR), Error: No pending request for response event. Addr = 0x%" PRIx64 "\n", ownerName_.c_str(), baseAddr);
    (it->second).dataBuffer = data;
}

vector<uint8_t> * MSHR::getDataBuffer(Addr baseAddr) {
    mshrTable::iterator it = map_.find(baseAddr);
    if (it == map_.end()) return NULL;
    return &((it->second).dataBuffer);
}

void MSHR::clearDataBuffer(Addr baseAddr) {
    mshrTable::iterator it = map_.find(baseAddr);
    if (it != map_.end()) (it->second).dataBuffer.clear();
    if (((it->second).acksNeeded == 0) && (it->second).dataBuffer.empty() && (it->second).mshrQueue.empty()) {
        if (is_debug_addr(baseAddr)) d_->debug(_L9_, "\tMSHR erasing 0x%" PRIx64 "\n", baseAddr);
        
        map_.erase(it);
    }
}

bool MSHR::isDataBufferValid(Addr baseAddr) {
    mshrTable::iterator it = map_.find(baseAddr);
    if (it != map_.end()) (it->second).dataBuffer.clear();
    return false;
}

bool MSHR::exists(Addr baseAddr) {
    mshrTable::iterator it = map_.find(baseAddr);
    if (it == map_.end()) return false;
    vector<mshrType> * queue = &((it->second).mshrQueue);
    vector<mshrType>::iterator frontEntry = queue->begin();
    if (frontEntry == queue->end()) return false;
    return (frontEntry->elem.isEvent());
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
    if (queue.front().elem.isAddr()) {
        d2_->fatal(CALL_INFO,-1, "%s (MSHR), Error: front entry in mshr is not of type MemEvent. Addr = 0x%" PRIx64 "\n", ownerName_.c_str(), baseAddr);
    }
    return (queue.front().elem).getEvent();
}

/* Public insertion methods
 * insert(Addr, MemEvent) -> insert stalled event
 * insertPointer
 * insertWriteback
 * insertInv
 */
bool MSHR::insert(Addr baseAddr, MemEvent* event) {
    if (size_ >= maxSize_) {
        if (is_debug_addr(baseAddr)) d_->debug(_L9_, "\tMSHR Full.  Event could not be inserted.\n");
        return false;
    }

    // Success
    size_++;
    if (event->isPrefetch())
        prefetchCount_++;
    insert(baseAddr, mshrType(event));
    
    if (is_debug_addr(baseAddr)) {
        d_->debug(_L9_, "\tMSHR: Event Inserted. Key addr = %" PRIx64 ", event Addr = %" PRIx64 ", Cmd = %s, MSHR Size = %u, Entry Size = %zu\n", 
                baseAddr, event->getAddr(), CommandString[(int)event->getCmd()], size_, map_[baseAddr].mshrQueue.size());
    }

    return true;
}


bool MSHR::insertPointer(Addr keyAddr, Addr pointerAddr) {
    if (keyAddr == pointerAddr) {
        d2_->fatal(CALL_INFO,-1, "%s (MSHR), Error: attempting to insert a self-pointer into the mshr. Addr = 0x%" PRIx64 "\n", ownerName_.c_str(), keyAddr);
    }
    
    if (is_debug_addr(keyAddr) || is_debug_addr(pointerAddr))
        d_->debug(_L9_, "\tMSHR: Inserted pointer.  Key Addr = %" PRIx64 ", Pointer Addr = %" PRIx64 "\n", keyAddr, pointerAddr);
    
    return insert(keyAddr, mshrType(pointerAddr));
}


bool MSHR::insertWriteback(Addr keyAddr) {
    if (is_debug_addr(keyAddr)) d_->debug(_L9_, "\tMSHR: Inserted writeback.  Key Addr = %" PRIx64 "\n", keyAddr);
    
    mshrType mshrElement = mshrType(keyAddr);

    mshrTable::iterator it = map_.find(keyAddr);
    if (it == map_.end()) {
        mshrEntry newEntry;
        newEntry.acksNeeded = 0;
        newEntry.dataBuffer.clear();
        map_[keyAddr] = newEntry;
    }

    vector<mshrType>::iterator itv = map_[keyAddr].mshrQueue.begin();
    map_[keyAddr].mshrQueue.insert(itv, mshrElement);
    //printTable();
    
    return true;
}

bool MSHR::insertInv(Addr baseAddr, MemEvent* event, bool inProgress) {
    bool ret = insertInv(baseAddr, mshrType(event), inProgress);
    
    if (LIKELY(ret)) {
        if (is_debug_addr(baseAddr)) {
            d_->debug(_L9_, "\tMSHR: Event Inserted. Key addr = %" PRIx64 ", event Addr = %" PRIx64 ", Cmd = %s, MSHR Size = %u, Entry Size = %zu\n", 
                    baseAddr, event->getAddr(), CommandString[(int)event->getCmd()], size_, map_[baseAddr].mshrQueue.size());
        }
    } else if (is_debug_addr(baseAddr)) d_->debug(_L9_, "\tMSHR Full.  Event could not be inserted.\n");
    
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
        entry.dataBuffer.clear();
        map_[baseAddr] = entry;
    }
    
    int trueSize = 0;
    int prefetches = 0;
    for (vector<mshrType>::iterator it = events.begin(); it != events.end(); it++) {
        if ((*it).elem.isEvent()) {
            trueSize++;
            if (((it->elem)).getEvent()->isPrefetch()) 
                prefetches++;
        }
    }

    prefetchCount_ += prefetches;
    size_ += trueSize;
    //printTable();
    return true;
}


/* Private insertion methods called by public inserts */
bool MSHR::insert(Addr baseAddr, mshrType entry) {
    mshrTable::iterator it = map_.find(baseAddr);
    if (it == map_.end()) {
        mshrEntry entry;
        entry.acksNeeded = 0;
        entry.dataBuffer.clear();
        map_[baseAddr] = entry;
    }
    map_[baseAddr].mshrQueue.push_back(entry);
    //printTable();
    
    return true;
}

bool MSHR::insertInv(Addr baseAddr, mshrType entry, bool inProgress) {
    if (size_ >= maxSize_) return false;
    
    vector<mshrType>::iterator it = map_[baseAddr].mshrQueue.begin();
    if (inProgress && map_[baseAddr].mshrQueue.size() > 0) it++;
    map_[baseAddr].mshrQueue.insert(it, entry);
    if (entry.elem.isEvent()) size_++;
    //printTable();
    return true;
}







MemEvent* MSHR::getOldestRequest() const {
    MemEvent *ev = NULL;
    for ( mshrTable::const_iterator it = map_.begin() ; it != map_.end() ; ++it ) {
        for ( vector<mshrType>::const_iterator jt = (it->second).mshrQueue.begin() ; jt != (it->second).mshrQueue.end() ; jt++ ) {
            if ( jt->elem.isEvent() ) {
                MemEvent *me = (jt->elem).getEvent();
                if ( !ev || ( me->getInitializationTime() < ev->getInitializationTime() ) ) {
                    ev = me;
                }
            }
        }
    }

    return ev;
}

vector<mshrType>* MSHR::getAll(Addr baseAddr) {
    mshrTable::iterator it = map_.find(baseAddr);
    if (it == map_.end()) {
        d2_->fatal(CALL_INFO,-1, "%s (MSHR), Error: mshr did not find entry with address 0x%" PRIx64 "\n", ownerName_.c_str(), baseAddr);
    }
    return &((it->second).mshrQueue); 
}


vector<mshrType> MSHR::removeAll(Addr baseAddr) {
    mshrTable::iterator it = map_.find(baseAddr);
    if (it == map_.end()) {
        d2_->fatal(CALL_INFO,-1, "%s (MSHR), Error: mshr did not find entry with address 0x%" PRIx64 "\n", ownerName_.c_str(), baseAddr);
    }
    vector<mshrType> res = (it->second).mshrQueue;
    if ((it->second).acksNeeded == 0 && (it->second).dataBuffer.empty()) {
        if (is_debug_addr(baseAddr)) d_->debug(_L9_, "\tMSHR erasing 0x%" PRIx64 "\n", baseAddr);
        
        map_.erase(it);
    } else {
        (it->second).mshrQueue.clear();
    }
    int trueSize = 0;
    int prefetches = 0;
    for (vector<mshrType>::iterator it = res.begin(); it != res.end(); it++) {
        if ((*it).elem.isEvent()) {
            trueSize++;
            MemEvent * ev = (it->elem).getEvent();
            if (ev->isPrefetch()) prefetches++;
        }
    }
    
    prefetchCount_ -= prefetches;
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
    // if (it->second.front().elem.isAddr()) {
    //     d2_->fatal(CALL_INFO,-1, "%s (MSHR), Error: front entry in mshr is not of type MemEvent. Addr = 0x%" PRIx64 "\n", ownerName_.c_str(), baseAddr);
    // }
    
    MemEvent* ret = ((it->second).mshrQueue.front().elem).getEvent();
    
    if (ret->isPrefetch()) prefetchCount_--;
    
    (it->second).mshrQueue.erase(it->second.mshrQueue.begin());
    if (((it->second).acksNeeded == 0) && (it->second).dataBuffer.empty() && (it->second).mshrQueue.empty()) {
        if (is_debug_addr(baseAddr)) d_->debug(_L9_, "\tMSHR erasing 0x%" PRIx64 "\n", baseAddr);
        map_.erase(it);
    }
    
    size_--;
    
    if (is_debug_addr(baseAddr)) d_->debug(_L9_,"\tMSHR: Removed front event, Key Addr = %" PRIx64 "\n", baseAddr);
    //printTable();
    
    return ret;
}




void MSHR::removeElement(Addr baseAddr, MemEvent* event) {
    if (!removeElement(baseAddr, mshrType(event))) 
        return;
    
    if (event->isPrefetch())
        prefetchCount_--;
    size_--; 
    if (size_ < 0) {
        d2_->fatal(CALL_INFO, -1, "%s (MSHR), Error: mshr size < 0 after removing element with address = 0x%" PRIx64 "\n", ownerName_.c_str(), baseAddr);
    }
}

void MSHR::removeElement(Addr baseAddr, Addr pointer) {
    removeElement(baseAddr, mshrType(pointer));
}

bool MSHR::removeElement(Addr baseAddr, mshrType entry) {

    mshrTable::iterator it = map_.find(baseAddr);
    if (it == map_.end()) return false;    
   
    if (is_debug_addr(baseAddr)) d_->debug(_L9_,"\tMSHR Entry size = %zu\n", it->second.mshrQueue.size());
    
    vector<mshrType>& res = (it->second).mshrQueue;
    vector<mshrType>::iterator itv = std::find_if(res.begin(), res.end(), MSHREntryCompare(&entry));
    
    if (itv == res.end()) return false;
    res.erase(std::remove_if(res.begin(), res.end(), MSHREntryCompare(&entry)), res.end());

    if (((it->second).acksNeeded == 0) && (it->second).dataBuffer.empty() && (it->second).mshrQueue.empty()) {
        if (is_debug_addr(baseAddr)) d_->debug(_L9_, "\tMSHR erasing 0x%" PRIx64 "\n", baseAddr);
        
        map_.erase(it);
    }
    
    if (is_debug_addr(baseAddr)) d_->debug(_L9_, "\tMSHR Removed Event\n");
    //printTable();
    
    return true;
}


void MSHR::removeWriteback(Addr baseAddr) {
    MSHR::removeElement(baseAddr, mshrType(baseAddr));
}

bool MSHR::elementIsHit(Addr baseAddr, MemEvent *event) {
    mshrType entry = mshrType(event);

    mshrTable::iterator it = map_.find(baseAddr);
    if (it == map_.end()) return false;    
    
    if (is_debug_addr(baseAddr)) d_->debug(_L9_,"\tMSHR Entry size = %zu\n", it->second.mshrQueue.size());
    
    vector<mshrType>& res = (it->second).mshrQueue;
    vector<mshrType>::iterator itv = std::find_if (res.begin(), res.end(), MSHREntryCompare(&entry));
    
    if (itv == res.end()) return false;
    return true;
}


void MSHR::printTable() {
    for (mshrTable::iterator it = map_.begin(); it != map_.end(); it++) {
        vector<mshrType> entries = (it->second).mshrQueue;
        d_->debug(_L9_, "\tMSHR: Addr = 0x%" PRIx64 "\n", (it->first));
        for (vector<mshrType>::iterator it2 = entries.begin(); it2 != entries.end(); it2++) {
            if (it2->elem.isAddr()) {
                Addr ptr = (it2->elem).getAddr();
                d_->debug(_L9_, "\t\t0x%" PRIx64 "\n", ptr);

            } else {
                MemEvent * ev = (it2->elem).getEvent();
                d_->debug(_L9_, "\t\t%s, %s\n", ev->getSrc().c_str(), CommandString[(int)ev->getCmd()]);
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
        d_->debug(C,L5,0, "Entry %i:  Key Addr: %#016lllx, Event Addr: %#016lllx, Event Cmd = %s\n", i, baseAddr, event->getAddr(), CommandString[(int)event->getCmd()]);
    }
}*/

void MSHR::printStatus(Output &out) {
    out.output("    MSHR Status for %s. Size: %u. Prefetches: %u\n", ownerName_.c_str(), size_, prefetchCount_);
    for (mshrTable::iterator it = map_.begin(); it != map_.end(); it++) {
        vector<mshrType> entries = (it->second).mshrQueue;
        out.output("      Entry: Addr = 0x%" PRIx64 " Acks needed: %d\n", (it->first), it->second.acksNeeded);
        for (vector<mshrType>::iterator it2 = entries.begin(); it2 != entries.end(); it2++) {
            if (it2->elem.isAddr()) {
                Addr ptr = (it2->elem).getAddr();
                out.output("        0x%" PRIx64 "\n", ptr);
            } else {
                MemEvent * ev = (it2->elem).getEvent();
                out.output("        %s\n", ev->getVerboseString().c_str());
            }
        }
    }
    out.output("    End MSHR Status for %s\n", ownerName_.c_str());
}





