// Copyright 2009-2019 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2019, NTESS
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
    MSHREntryCompare( MSHREntry* _m ) : m_(_m) {
        if (m_->elem.isEvent()) type_ = Event;
        else type_ = Pointer;
    }
    bool operator() (MSHREntry& _n) {
        if (type_ == Event) {
            if (_n.elem.isEvent()) return (m_->elem).getEvent() == (_n.elem).getEvent();
            return false;
        }
        else{
            if (_n.elem.isAddr()) return (m_->elem).getAddr() == (_n.elem).getAddr();
            return false;
        }
    }
    MSHREntry* m_;
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

// Get the size limit for the MSHR
int MSHR::getMaxSize() {
    return maxSize_;
}

// Get total number of entries in the MSHR, measured by 'size_'
int MSHR::getSize() {
    return size_;
}

// Get size of 'entries' list for address 'addr'
unsigned int MSHR::getSize(Addr addr) {
    if (mshr_.find(addr) == mshr_.end())
        return 0;
    else
        return mshr_.find(addr)->second.entries.size();
}

bool MSHR::exists(Addr baseAddr) {
    mshrTable::iterator it = mshr_.find(baseAddr);
    if (it == mshr_.end()) return false;
    list<MSHREntry> * queue = &((it->second).entries);
    list<MSHREntry>::iterator frontEntry = queue->begin();
    if (frontEntry == queue->end()) return false;
    return (frontEntry->elem.isEvent());
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
    mshrTable::iterator it = mshr_.find(baseAddr);
    if (it == mshr_.end()) return 0;
    return (it->second).acksNeeded;
}


void MSHR::setAcksNeeded(Addr baseAddr, int acksNeeded, MemEvent * event) {
    mshrTable::iterator it = mshr_.find(baseAddr);
    if (it == mshr_.end()) {
        if (is_debug_addr(baseAddr)) d_->debug(_L6_, "\tCreating new MSHR holder for acks\n");
        
        MSHRRegister reg;
        reg.acksNeeded = acksNeeded;
        reg.dataBuffer.clear();
        if (event != nullptr)
            reg.entries.push_back(MSHREntry(event));
        mshr_[baseAddr] = reg;
        return;
    }
    (it->second).acksNeeded = acksNeeded;
}

void MSHR::incrementAcksNeeded(Addr baseAddr) {
    mshrTable::iterator it = mshr_.find(baseAddr);
    if (it == mshr_.end()) {
        MSHRRegister reg;
        reg.acksNeeded = 1;
        mshr_[baseAddr] = reg;
    } else {
        (it->second).acksNeeded++;
    }
}

void MSHR::decrementAcksNeeded(Addr baseAddr) {
    mshrTable::iterator it = mshr_.find(baseAddr);
    if (it == mshr_.end()) return;
    (it->second).acksNeeded--;
    if (((it->second).acksNeeded == 0) && (it->second).dataBuffer.empty() && (it->second).entries.empty()) {
        if (is_debug_addr(baseAddr)) d_->debug(_L9_, "\tMSHR erasing 0x%" PRIx64 "\n", baseAddr);
        
        mshr_.erase(it);
    }
}

void MSHR::setDataBuffer(Addr baseAddr, vector<uint8_t>& data) {
    mshrTable::iterator it = mshr_.find(baseAddr);
    if (it == mshr_.end()) d2_->fatal(CALL_INFO,-1, "%s (MSHR), Error: No pending request for response event. Addr = 0x%" PRIx64 "\n", ownerName_.c_str(), baseAddr);
    (it->second).dataBuffer = data;
}

vector<uint8_t> * MSHR::getDataBuffer(Addr baseAddr) {
    mshrTable::iterator it = mshr_.find(baseAddr);
    if (it == mshr_.end()) return NULL;
    return &((it->second).dataBuffer);
}

void MSHR::clearDataBuffer(Addr baseAddr) {
    mshrTable::iterator it = mshr_.find(baseAddr);
    if (it != mshr_.end()) (it->second).dataBuffer.clear();
    if (((it->second).acksNeeded == 0) && (it->second).dataBuffer.empty() && (it->second).entries.empty()) {
        if (is_debug_addr(baseAddr)) d_->debug(_L9_, "\tMSHR erasing 0x%" PRIx64 "\n", baseAddr);
        
        mshr_.erase(it);
    }
}

bool MSHR::isDataBufferValid(Addr baseAddr) {
    mshrTable::iterator it = mshr_.find(baseAddr);
    if (it != mshr_.end()) (it->second).dataBuffer.clear();
    return false;
}


bool MSHR::isHit(Addr baseAddr) { return (mshr_.find(baseAddr) != mshr_.end()) && (mshr_.find(baseAddr)->second.entries.size() > 0); }

bool MSHR::pendingWriteback(Addr baseAddr) {
    MSHREntry entry = MSHREntry(baseAddr);
    mshrTable::iterator it = mshr_.find(baseAddr);
    if (it == mshr_.end()) return false;

    list<MSHREntry>& res = (it->second).entries;
    list<MSHREntry>::iterator itv = std::find_if(res.begin(), res.end(), MSHREntryCompare(&entry));
    return (itv != res.end());
}

const list<MSHREntry> MSHR::lookup(Addr baseAddr) {
    mshrTable::iterator it = mshr_.find(baseAddr);
    if (it == mshr_.end()) {
        d2_->fatal(CALL_INFO,-1, "%s (MSHR), Error: mshr did not find entry with address 0x%" PRIx64 "\n", ownerName_.c_str(), baseAddr);
    }
    list<MSHREntry> res = (it->second).entries;
    return res;
}


MemEvent* MSHR::lookupFront(Addr baseAddr) {
    mshrTable::iterator it = mshr_.find(baseAddr);
    if (it == mshr_.end()) {
        d2_->fatal(CALL_INFO,-1, "%s (MSHR), Error: mshr did not find entry with address 0x%" PRIx64 "\n", ownerName_.c_str(), baseAddr);
    }
    list<MSHREntry> queue = (it->second).entries;
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
    insert(baseAddr, MSHREntry(event));
    
    if (is_debug_addr(baseAddr)) {
        d_->debug(_L9_, "\tMSHR: Event Inserted. Key addr = %" PRIx64 ", event Addr = %" PRIx64 ", Cmd = %s, MSHR Size = %u, Entry Size = %zu\n", 
                baseAddr, event->getAddr(), CommandString[(int)event->getCmd()], size_, mshr_[baseAddr].entries.size());
    }

    return true;
}


bool MSHR::insertPointer(Addr keyAddr, Addr pointerAddr) {
    if (keyAddr == pointerAddr) {
        d2_->fatal(CALL_INFO,-1, "%s (MSHR), Error: attempting to insert a self-pointer into the mshr. Addr = 0x%" PRIx64 "\n", ownerName_.c_str(), keyAddr);
    }
    
    if (is_debug_addr(keyAddr) || is_debug_addr(pointerAddr))
        d_->debug(_L9_, "\tMSHR: Inserted pointer.  Key Addr = %" PRIx64 ", Pointer Addr = %" PRIx64 "\n", keyAddr, pointerAddr);
    
    return insert(keyAddr, MSHREntry(pointerAddr));
}


bool MSHR::insertWriteback(Addr keyAddr) {
    if (is_debug_addr(keyAddr)) d_->debug(_L9_, "\tMSHR: Inserted writeback.  Key Addr = %" PRIx64 "\n", keyAddr);
    
    MSHREntry mshrElement = MSHREntry(keyAddr);

    mshrTable::iterator it = mshr_.find(keyAddr);
    if (it == mshr_.end()) {
        MSHRRegister reg;
        reg.acksNeeded = 0;
        reg.dataBuffer.clear();
        mshr_[keyAddr] = reg;
    }

    list<MSHREntry>::iterator itv = mshr_[keyAddr].entries.begin();
    mshr_[keyAddr].entries.insert(itv, mshrElement);
    //printTable();
    
    return true;
}

bool MSHR::insertInv(Addr baseAddr, MemEvent* event, bool inProgress) {
    bool ret = insertInv(baseAddr, MSHREntry(event), inProgress);
    
    if (LIKELY(ret)) {
        if (is_debug_addr(baseAddr)) {
            d_->debug(_L9_, "\tMSHR: Event Inserted. Key addr = %" PRIx64 ", event Addr = %" PRIx64 ", Cmd = %s, MSHR Size = %u, Entry Size = %zu\n", 
                    baseAddr, event->getAddr(), CommandString[(int)event->getCmd()], size_, mshr_[baseAddr].entries.size());
        }
    } else if (is_debug_addr(baseAddr)) d_->debug(_L9_, "\tMSHR Full.  Event could not be inserted.\n");
    
    return ret;
}

bool MSHR::insertAll(Addr baseAddr, list<MSHREntry>& events) {
    if (events.empty()) return false;
    mshrTable::iterator it = mshr_.find(baseAddr);
    
    if (it != mshr_.end()) (it->second).entries.insert((it->second).entries.end(), events.begin(), events.end());
    else {
        MSHRRegister reg;
        reg.entries = events;
        reg.acksNeeded = 0;
        reg.dataBuffer.clear();
        mshr_[baseAddr] = reg;
    }
    
    int trueSize = 0;
    int prefetches = 0;
    for (list<MSHREntry>::iterator it = events.begin(); it != events.end(); it++) {
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
bool MSHR::insert(Addr baseAddr, MSHREntry entry) {
    mshrTable::iterator it = mshr_.find(baseAddr);
    if (it == mshr_.end()) {
        MSHRRegister reg;
        reg.acksNeeded = 0;
        reg.dataBuffer.clear();
        mshr_[baseAddr] = reg;
    }
    mshr_[baseAddr].entries.push_back(entry);
    //printTable();
    
    return true;
}

bool MSHR::insertInv(Addr baseAddr, MSHREntry entry, bool inProgress) {
    if (size_ >= maxSize_) return false;
    
    list<MSHREntry>::iterator it = mshr_[baseAddr].entries.begin();
    if (inProgress && mshr_[baseAddr].entries.size() > 0) it++;
    mshr_[baseAddr].entries.insert(it, entry);
    if (entry.elem.isEvent()) size_++;
    //printTable();
    return true;
}





const MSHREntry* MSHR::getOldestRequest() const {
    const MSHREntry* entry = nullptr;
    for ( mshrTable::const_iterator it = mshr_.begin() ; it != mshr_.end() ; ++it ) {
        for ( list<MSHREntry>::const_iterator jt = (it->second).entries.begin() ; jt != (it->second).entries.end() ; jt++ ) {
            if ( jt->elem.isEvent() ) {
                if ( !entry || ( jt->getInsertionTime() < entry->getInsertionTime())) {
                    entry = &*jt;
                }
            }
        }
    }

    return entry;
}

list<MSHREntry>* MSHR::getAll(Addr baseAddr) {
    mshrTable::iterator it = mshr_.find(baseAddr);
    if (it == mshr_.end()) {
        d2_->fatal(CALL_INFO,-1, "%s (MSHR), Error: mshr did not find entry with address 0x%" PRIx64 "\n", ownerName_.c_str(), baseAddr);
    }
    return &((it->second).entries); 
}


list<MSHREntry> MSHR::removeAll(Addr baseAddr) {
    mshrTable::iterator it = mshr_.find(baseAddr);
    if (it == mshr_.end()) {
        d2_->fatal(CALL_INFO,-1, "%s (MSHR), Error: mshr did not find entry with address 0x%" PRIx64 "\n", ownerName_.c_str(), baseAddr);
    }
    list<MSHREntry> res = (it->second).entries;
    if ((it->second).acksNeeded == 0 && (it->second).dataBuffer.empty()) {
        if (is_debug_addr(baseAddr)) d_->debug(_L9_, "\tMSHR erasing 0x%" PRIx64 "\n", baseAddr);
        
        mshr_.erase(it);
    } else {
        (it->second).entries.clear();
    }
    int trueSize = 0;
    int prefetches = 0;
    for (list<MSHREntry>::iterator it = res.begin(); it != res.end(); it++) {
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
    mshrTable::iterator it = mshr_.find(baseAddr);
    if (it == mshr_.end()) {
        d2_->fatal(CALL_INFO,-1, "%s (MSHR), Error: mshr did not find entry with address 0x%" PRIx64 "\n", ownerName_.c_str(), baseAddr);
    }
    //if (it->second.empty()) {
    //  d2_->fatal(CALL_INFO,-1, "%s (MSHR), Error: no front entry to remove in mshr for addr = 0x%" PRIx64 "\n", ownerName_.c_str(), baseAddr);
    //}
    //
    // if (it->second.front().elem.isAddr()) {
    //     d2_->fatal(CALL_INFO,-1, "%s (MSHR), Error: front entry in mshr is not of type MemEvent. Addr = 0x%" PRIx64 "\n", ownerName_.c_str(), baseAddr);
    // }
    
    MemEvent* ret = ((it->second).entries.front().elem).getEvent();
    
    if (ret->isPrefetch()) prefetchCount_--;
    
    (it->second).entries.erase(it->second.entries.begin());
    if (((it->second).acksNeeded == 0) && (it->second).dataBuffer.empty() && (it->second).entries.empty()) {
        if (is_debug_addr(baseAddr)) d_->debug(_L9_, "\tMSHR erasing 0x%" PRIx64 "\n", baseAddr);
        mshr_.erase(it);
    }
    
    size_--;
    
    if (is_debug_addr(baseAddr)) d_->debug(_L9_,"\tMSHR: Removed front event, Key Addr = %" PRIx64 "\n", baseAddr);
    //printTable();
    
    return ret;
}




void MSHR::removeElement(Addr baseAddr, MemEvent* event) {
    if (!removeElement(baseAddr, MSHREntry(event))) 
        return;
    
    if (event->isPrefetch())
        prefetchCount_--;
    size_--; 
    if (size_ < 0) {
        d2_->fatal(CALL_INFO, -1, "%s (MSHR), Error: mshr size < 0 after removing element with address = 0x%" PRIx64 "\n", ownerName_.c_str(), baseAddr);
    }
}

void MSHR::removeElement(Addr baseAddr, Addr pointer) {
    removeElement(baseAddr, MSHREntry(pointer));
}

bool MSHR::removeElement(Addr baseAddr, MSHREntry entry) {

    mshrTable::iterator it = mshr_.find(baseAddr);
    if (it == mshr_.end()) return false;    
   
    if (is_debug_addr(baseAddr)) d_->debug(_L9_,"\tMSHR Entry size = %zu\n", it->second.entries.size());
    
    list<MSHREntry>& res = (it->second).entries;
    list<MSHREntry>::iterator itv = std::find_if(res.begin(), res.end(), MSHREntryCompare(&entry));
    
    if (itv == res.end()) return false;
    res.erase(std::remove_if(res.begin(), res.end(), MSHREntryCompare(&entry)), res.end());

    if (((it->second).acksNeeded == 0) && (it->second).dataBuffer.empty() && (it->second).entries.empty()) {
        if (is_debug_addr(baseAddr)) d_->debug(_L9_, "\tMSHR erasing 0x%" PRIx64 "\n", baseAddr);
        
        mshr_.erase(it);
    }
    
    if (is_debug_addr(baseAddr)) d_->debug(_L9_, "\tMSHR Removed Event\n");
    //printTable();
    
    return true;
}


void MSHR::removeWriteback(Addr baseAddr) {
    MSHR::removeElement(baseAddr, MSHREntry(baseAddr));
}

bool MSHR::elementIsHit(Addr baseAddr, MemEvent *event) {
    MSHREntry entry = MSHREntry(event);

    mshrTable::iterator it = mshr_.find(baseAddr);
    if (it == mshr_.end()) return false;    
    
    if (is_debug_addr(baseAddr)) d_->debug(_L9_,"\tMSHR Entry size = %zu\n", it->second.entries.size());
    
    list<MSHREntry>& res = (it->second).entries;
    list<MSHREntry>::iterator itv = std::find_if (res.begin(), res.end(), MSHREntryCompare(&entry));
    
    if (itv == res.end()) return false;
    return true;
}


void MSHR::printTable() {
    for (mshrTable::iterator it = mshr_.begin(); it != mshr_.end(); it++) {
        list<MSHREntry> entries = (it->second).entries;
        d_->debug(_L9_, "\tMSHR: Addr = 0x%" PRIx64 "\n", (it->first));
        for (list<MSHREntry>::iterator it2 = entries.begin(); it2 != entries.end(); it2++) {
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
    mshrTable::iterator it = mshr_.find(baseAddr);
    if (it == mshr_.end()) return;
    int i = 0;
    for (vector<MemEvent*>::iterator it = mshr_[baseAddr].begin(); it != mshr_[baseAddr].end(); ++it, ++i) {
        MemEvent* event = (MemEvent*)(*it);
        d_->debug(C,L5,0, "Entry %i:  Key Addr: %#016lllx, Event Addr: %#016lllx, Event Cmd = %s\n", i, baseAddr, event->getAddr(), CommandString[(int)event->getCmd()]);
    }
}*/

void MSHR::printStatus(Output &out) {
    out.output("    MSHR Status for %s. Size: %u. Prefetches: %u\n", ownerName_.c_str(), size_, prefetchCount_);
    for (mshrTable::iterator it = mshr_.begin(); it != mshr_.end(); it++) {
        list<MSHREntry> entries = (it->second).entries;
        out.output("      Entry: Addr = 0x%" PRIx64 " Acks needed: %d\n", (it->first), it->second.acksNeeded);
        for (list<MSHREntry>::iterator it2 = entries.begin(); it2 != entries.end(); it2++) {
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





