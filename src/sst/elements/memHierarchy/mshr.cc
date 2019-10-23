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

int MSHR::getMaxSize() {
    return maxSize_;
}

int MSHR::getSize() {
    return size_;
}

unsigned int MSHR::getSize(Addr addr) {
    if (mshr_.find(addr) == mshr_.end())
        return 0;
    else
        return mshr_.find(addr)->second.entries.size();
}

bool MSHR::exists(Addr addr) {
    return mshr_.find(addr) != mshr_.end();
}

MSHREntry MSHR::getEntry(Addr addr, size_t index) {
    if (mshr_.find(addr) == mshr_.end()) {
        d_->fatal(CALL_INFO, -1, "%s, Error: MSHR::getEntry(0x%" PRIx64 ", %zu). Address doesn't exist in MSHR.\n", ownerName_.c_str(), addr, index);
    }
    if (mshr_.find(addr)->second.entries.size() <= index) {
        d_->fatal(CALL_INFO, -1, "%s, Error: MSHR::getEntry(0x%" PRIx64 ", %zu). Entry list size is %zu.\n", ownerName_.c_str(), addr, index, mshr_.find(addr)->second.entries.size());
    }
    std::list<MSHREntry>::iterator it = mshr_.find(addr)->second.entries.begin();
    std::advance(it, index);
    return *it;
}

MSHREntry MSHR::getFront(Addr addr) {
    if (mshr_.find(addr) == mshr_.end()) {
        d_->fatal(CALL_INFO, -1, "%s, Error: MSHR::getFront(0x%" PRIx64 "). Address doesn't exist in MSHR.\n", ownerName_.c_str(), addr);
    }

    if (mshr_.find(addr)->second.entries.empty()) {
        d_->fatal(CALL_INFO, -1, "%s, Error: MSHR::getFront(0x%" PRIx64 "). Entry list is empty.\n", ownerName_.c_str(), addr);
    }
    return mshr_.find(addr)->second.entries.front();
}

void MSHR::removeEntry(Addr addr, size_t index) {
    if (mshr_.find(addr) == mshr_.end()) {
        d_->fatal(CALL_INFO, -1, "%s, Error: MSHR::removeEntry(0x%" PRIx64 ", %zu). Address doesn't exist in MSHR.\n", ownerName_.c_str(), addr, index);
    }
    MSHRRegister * reg = &(mshr_.find(addr)->second);
    if (reg->entries.size() <= index) {
        d_->fatal(CALL_INFO, -1, "%s, Error: MSHR::removeEntry(0x%" PRIx64 ", %zu). Entry list is shorter than requested index.\n", ownerName_.c_str(), addr, index);
    }

    std::list<MSHREntry>::iterator entry = reg->entries.begin();
    std::advance(entry, index);

    if (entry->getType() == MSHREntryType::Event)
        size_--;
    
    if (is_debug_addr(addr))
        printDebug(10, "Remove", addr, (*entry).getString().c_str());
    
    reg->entries.erase(entry);
    if (reg->entries.empty()) {
        if (is_debug_addr(addr))
            printDebug(10, "Erase", addr, "");
            //d_->debug(_L10_, "M: %-41" PRIu64 " %-20s Erase        0x%-16" PRIx64 " %-10d\n",
            //        Simulation::getSimulation()->getCurrentSimCycle(), ownerName_.c_str(), addr, size_);
            //d_->debug(_L10_, "    MSHR: erasing 0x%" PRIx64 " from MSHR\n", addr);
        mshr_.erase(addr);
    }
}

void MSHR::removeFront(Addr addr) {
    if (mshr_.find(addr) == mshr_.end()) {
        d_->fatal(CALL_INFO, -1, "%s, Error: MSHR::removeFront(0x%" PRIx64 "). Address doesn't exist in MSHR.\n", ownerName_.c_str(), addr);
    }
    MSHRRegister * reg = &(mshr_.find(addr)->second);
    if (reg->entries.empty()) {    
        d_->fatal(CALL_INFO, -1, "%s, Error: MSHR::removeFront(0x%" PRIx64 "). Entry list is empty.\n", ownerName_.c_str(), addr);
    }
    
   // if (is_debug_addr(addr))
   //     d_->debug(_L10_, "    MSHR::removeFront(0x%" PRIx64 ", %s)\n", addr, reg->entries.front().getString().c_str());

    if (getFrontType(addr) == MSHREntryType::Event)
        size_--;
    
    if (is_debug_addr(addr))
        printDebug(10, "RemFr", addr, (reg->entries.front()).getString().c_str());
    
    reg->entries.pop_front();
    if (reg->entries.empty()) {
        if (is_debug_addr(addr))
            printDebug(10, "Erase", addr, "");
            //d_->debug(_L10_, "    MSHR: erasing 0x%" PRIx64 " from MSHR\n", addr);
        mshr_.erase(addr);
    }
}

MSHREntryType MSHR::getEntryType(Addr addr, size_t index) {
    //if (is_debug_addr(addr))
    //    d_->debug(_L20_, "    MSHR::getEntryType(0x%" PRIx64 ", %zu)\n", addr, index);
    if (mshr_.find(addr) == mshr_.end()) {
        d_->fatal(CALL_INFO, -1, "%s, Error: MSHR::getEntryType(0x%" PRIx64 ", %zu). Address doesn't exist in MSHR.\n", ownerName_.c_str(), addr, index);
    }
    if (mshr_.find(addr)->second.entries.size() <= index) {
        d_->fatal(CALL_INFO, -1, "%s, Error: MSHR::getEntryType(0x%" PRIx64 ", %zu). Entry list is shoerter than index.\n", ownerName_.c_str(), addr, index);
    }
    std::list<MSHREntry>::iterator it = mshr_.find(addr)->second.entries.begin();
    std::advance(it, index);
    return it->getType();
}

MSHREntryType MSHR::getFrontType(Addr addr) {
    //if (is_debug_addr(addr))
    //    d_->debug(_L20_, "    MSHR::getFrontType(0x%" PRIx64 ")\n", addr);
    if (mshr_.find(addr) == mshr_.end()) {
        d_->fatal(CALL_INFO, -1, "%s, Error: MSHR::getFrontType(0x%" PRIx64 "). Address doesn't exist in MSHR.\n", ownerName_.c_str(), addr);
    }
    if (mshr_.find(addr)->second.entries.empty()) {
        d_->fatal(CALL_INFO, -1, "%s, Error: MSHR::getFrontType(0x%" PRIx64 "). Entry list is empty.\n", ownerName_.c_str(), addr);
    }
    return mshr_.find(addr)->second.entries.front().getType();
}

MemEventBase* MSHR::getEntryEvent(Addr addr, size_t index) {
    //if (is_debug_addr(addr))
    //    d_->debug(_L20_, "    MSHR::getEntryEvent(0x%" PRIx64 ", %zu)\n", addr, index);
    
    if (mshr_.find(addr) == mshr_.end() || mshr_.find(addr)->second.entries.size() <= index)
        return nullptr;
    
    std::list<MSHREntry>::iterator it = mshr_.find(addr)->second.entries.begin();
    std::advance(it, index);
    if (it->getType() != MSHREntryType::Event)
        return nullptr;
    return it->getEvent();
}


MemEventBase* MSHR::getFrontEvent(Addr addr) {
    //if (is_debug_addr(addr))
    //    d_->debug(_L20_, "    MSHR::getFrontEvent(0x%" PRIx64 ")\n", addr);
    if (getFrontType(addr) != MSHREntryType::Event) {
        return nullptr;
    }
    return mshr_.find(addr)->second.entries.front().getEvent();
}

MemEventBase* MSHR::getFirstEventEntry(Addr addr, Command cmd) {
//    if (is_debug_addr(addr))
//        d_->debug(_L20_, "    MSHR::getFirstEventEntry(0x%" PRIx64 ", %s)\n", addr, CommandString[(int)cmd]);
    
    if (mshr_.find(addr) == mshr_.end())
        return nullptr;

    for (std::list<MSHREntry>::iterator it = mshr_.find(addr)->second.entries.begin(); it != mshr_.find(addr)->second.entries.end(); it++) {
        if (it->getType() == MSHREntryType::Event && it->getEvent()->getCmd() == cmd)
            return it->getEvent();
    }
    return nullptr;
}

std::list<Addr>* MSHR::getEvictPointers(Addr addr) {
//    if (is_debug_addr(addr))
//        d_->debug(_L20_, "    MSHR::getEvictPointers(0x%" PRIx64 ")\n", addr);
    if (getFrontType(addr) != MSHREntryType::Evict)
        d_->fatal(CALL_INFO, -1, "%s, Error: MSHR::getEvictPointers(0x%" PRIx64 "). Entry type is not Evict.\n", ownerName_.c_str(), addr);

    return mshr_.find(addr)->second.entries.front().getPointers();
}

// Return whether we should retry a new event or not
bool MSHR::removeEvictPointer(Addr addr, Addr addrPtr) {
//    if (is_debug_addr(addr))
//        d_->debug(_L10_, "    MSHR::removeEvictPointer(0x%" PRIx64 ", 0x%" PRIx64 ")\n", addr, addrPtr);
    if (getFrontType(addr) == MSHREntryType::Event)
        d_->fatal(CALL_INFO, -1, "%s, Error: MSHR::removeEvictPointer(0x%" PRIx64 ", 0x%" PRIx64 "). Front entry type is not Evict or Writeback.\n", ownerName_.c_str(), addr, addrPtr);
        
    if (is_debug_addr(addr) || is_debug_addr(addrPtr)) {
        stringstream reason;
        reason << "to 0x" << std::hex << addrPtr;
        printDebug(10, "RemPtr", addr, reason.str());
    }

    // Sometimes we insert a WB before the Evict & then remove the Evict pointer, othertimes the Evict is front
    if (getFrontType(addr) == MSHREntryType::Evict) {
        MSHREntry * entry = &(mshr_.find(addr)->second.entries.front());
        entry->getPointers()->remove(addrPtr);
        if (entry->getPointers()->empty()) {
            removeFront(addr);
            return true;
        } 
    } else {
        std::list<MSHREntry>::iterator it = mshr_.find(addr)->second.entries.begin();
        it++;
        if (it->getType() != MSHREntryType::Evict)
            d_->fatal(CALL_INFO, -1, "%s, Error: MSHR::removeEvictPointer(0x%" PRIx64 ", 0x%" PRIx64 "). Entry type is not Evict.\n", ownerName_.c_str(), addr, addrPtr);
        it->getPointers()->remove(addrPtr);
        if (it->getPointers()->empty()) {
            removeEntry(addr, 1);
        }
    }
    return false;
}

bool MSHR::pendingWriteback(Addr addr) {
    return exists(addr) && getFrontType(addr) == MSHREntryType::Writeback;
}

bool MSHR::pendingWritebackIsDowngrade(Addr addr) {
    if (pendingWriteback(addr))
        return mshr_.find(addr)->second.entries.front().getDowngrade();
    return false;
}

int MSHR::insertEvent(Addr addr, MemEventBase* event, int pos, bool fwdRequest, bool stallEvict) {
    //if (is_debug_addr(addr))
    //    d_->debug(_L10_, "    MSHR::insertEvent(0x%" PRIx64 "). Size: %d\n", addr, size_);
    if ((size_ == maxSize_) || (!fwdRequest && (size_ == maxSize_-1))) {
        if (is_debug_addr(addr)) {
            stringstream reason;
            reason << "<" << event->getID().first << "," << event->getID().second << "> FAILED " << (fwdRequest ? "fwd, " : "") << "maxsz: " << maxSize_;
            printDebug(10, "InsEv", addr, reason.str());
        }
//        if (is_debug_addr(addr))
//            d_->debug(_L10_, "    MSHR::insertEvent, unable to insert (fwdRequest ? %s). size: %d. maxSize_: %d\n", fwdRequest ? "T" : "F", size_, maxSize_);
        return -1;
    }

    // Success
    size_++;
    
    if (mshr_.find(addr) == mshr_.end()) {
        MSHRRegister reg;
        reg.entries.push_back(MSHREntry(event, stallEvict));
        
        mshr_.insert(std::make_pair(addr, reg));
        if (is_debug_addr(addr)) {
            stringstream reason;
            reason << "<" << event->getID().first << "," << event->getID().second << ">, pos=0";
            printDebug(10, "InsEv", addr, reason.str());

        }
            
        return 0;
    } else {
        if (pos == -1 || pos > mshr_.find(addr)->second.entries.size()) {
            mshr_.find(addr)->second.entries.push_back(MSHREntry(event, stallEvict));
            if (is_debug_addr(addr)) {
                stringstream reason;
                reason << "<" << event->getID().first << "," << event->getID().second << ">, pos=" << (mshr_.find(addr)->second.entries.size() - 1);
                printDebug(10, "InsEv", addr, reason.str());
            }
            return (mshr_.find(addr)->second.entries.size() - 1);
        } else {
            std::list<MSHREntry>::iterator it = mshr_.find(addr)->second.entries.begin();
            std::advance(it, pos);
            mshr_.find(addr)->second.entries.insert(it, MSHREntry(event, stallEvict));
            if (is_debug_addr(addr)) {
                stringstream reason;
                reason << "<" << event->getID().first << "," << event->getID().second << ">, pos=" << pos;
                printDebug(10, "InsEv", addr, reason.str());
            }
            return pos;
        }
    }
}

MemEventBase* MSHR::swapFrontEvent(Addr addr, MemEventBase* event) {
    if (is_debug_addr(addr))
        printDebug(10, "SwpEv", addr, "");
   
    if (mshr_.find(addr)->second.entries.empty()) 
        return nullptr;

    return  mshr_.find(addr)->second.entries.front().swapEvent(event);
}

void MSHR::moveEntryToFront(Addr addr, unsigned int index) {
    if (mshr_.find(addr) == mshr_.end()) {
        d_->fatal(CALL_INFO, -1, "%s, Error: MSHR::moveEntryToFront(0x%" PRIx64 ", %zu). Address doesn't exist in MSHR.\n", ownerName_.c_str(), addr, index);
    }
    MSHRRegister * reg = &(mshr_.find(addr)->second);
    if (reg->entries.size() <= index) {
        d_->fatal(CALL_INFO, -1, "%s, Error: MSHR::moveEntryToFront(0x%" PRIx64 ", %zu). Entry list is shorter than requested index.\n", ownerName_.c_str(), addr, index);
    }

    std::list<MSHREntry>::iterator entry = reg->entries.begin();
    std::advance(entry, index);
   
    MSHREntry tmpEntry = *entry;

    if (is_debug_addr(addr))
        printDebug(10, "MvEnt", addr, entry->getString());
    reg->entries.erase(entry);
    reg->entries.push_front(tmpEntry);
}

bool MSHR::insertWriteback(Addr addr, bool downgrade) {
//    if (is_debug_addr(addr))
//        d_->debug(_L10_, "    MSHR::insertWriteback(0x%" PRIx64 ")\n", addr);
    
    if (is_debug_addr(addr)) {
        stringstream reason;
        reason << "Downgrade: " << downgrade ? "T" : "F";
        printDebug(10, "InsWB", addr, reason.str());
    }
    
    if (mshr_.find(addr) == mshr_.end()) {
        MSHRRegister reg;
        reg.entries.push_back(MSHREntry(downgrade));
        mshr_.insert(std::make_pair(addr, reg));
    } else {
        mshr_.find(addr)->second.entries.push_front(MSHREntry(downgrade));
    }

    if (downgrade && !pendingWritebackIsDowngrade(addr)) {
        printf("%s, Error: inserting downgrade in MSHR but pendingWritebackIsDowngrade returns false. 0x%" PRIx64 "\n", ownerName_.c_str(), addr);
    }
    return true;
}


bool MSHR::insertEviction(Addr oldAddr, Addr newAddr) {
//    if (is_debug_addr(oldAddr) || is_debug_addr(newAddr))
//        d_->debug(_L10_, "    MSHR::insertEviction(0x%" PRIx64 ", 0x%" PRIx64 ")\n", oldAddr, newAddr);
    
    if (is_debug_addr(oldAddr) || is_debug_addr(newAddr)) {
        stringstream reason;
        reason << "to 0x" << std::hex << newAddr;
        printDebug(10, "InsPtr", oldAddr, reason.str());
    }
    
    if (mshr_.find(oldAddr) == mshr_.end()) {  // No MSHR entry for oldAddr
        MSHRRegister reg;
        reg.entries.push_back(MSHREntry(newAddr));
        mshr_.insert(std::make_pair(oldAddr, reg));
    } else {
        list<MSHREntry>* entries = &(mshr_.find(oldAddr)->second.entries);
        if (!entries->empty() && entries->back().getType() == MSHREntryType::Evict) { // MSHR entry for oldAddr is an Evict
            entries->back().getPointers()->push_back(newAddr);
        } else { // MSHR entry for oldAddr is not an Evict (or no entry exists)
            entries->push_back(MSHREntry(newAddr));
        }
    }
    return true;
}

void MSHR::setInProgress(Addr addr, bool value) {
//    if (is_debug_addr(addr))
//        d_->debug(_L10_, "    MSHR::setInProgress(0x%" PRIx64 ")\n", addr);
    if (is_debug_addr(addr))
        printDebug(20, "InProg", addr, "");
    
    if (mshr_.find(addr) == mshr_.end()) {
        d_->fatal(CALL_INFO, -1, "%s, Error: MSHR::setInProgress(0x%" PRIx64 "). Address does not exist in MSHR.\n", ownerName_.c_str(), addr);
    }
    if (mshr_.find(addr)->second.entries.empty()) {
        d_->fatal(CALL_INFO, -1, "%s, Error: MSHR::setInProgress(0x%" PRIx64 "). Entry list is empty.\n", ownerName_.c_str(), addr);
    }
    mshr_.find(addr)->second.entries.front().setInProgress(value);
}

bool MSHR::getInProgress(Addr addr) {
//    if (is_debug_addr(addr))
//        d_->debug(_L20_, "    MSHR::getInProgress(0x%" PRIx64 ")\n", addr);
    if (mshr_.find(addr) == mshr_.end()) {
        d_->fatal(CALL_INFO, -1, "%s, Error: MSHR::getInProgress(0x%" PRIx64 "). Address does not exist in MSHR.\n", ownerName_.c_str(), addr);
    }
    if (mshr_.find(addr)->second.entries.empty()) {
        d_->fatal(CALL_INFO, -1, "%s, Error: MSHR::getInProgress(0x%" PRIx64 "). Entry list is empty.\n", ownerName_.c_str(), addr);
    }
    return mshr_.find(addr)->second.entries.front().getInProgress();
}

void MSHR::setStalledForEvict(Addr addr, bool set) {
//    if (is_debug_addr(addr))
//        d_->debug(_L10_, "    MSHR::setStalledForEvict(0x%" PRIx64 ")\n", addr);
    if (is_debug_addr(addr)) {
        if (set)
            printDebug(20, "Stall", addr, "");
        else
            printDebug(20, "Unstall", addr, "");
    }

    if (mshr_.find(addr) == mshr_.end()) {
        d_->fatal(CALL_INFO, -1, "%s, Error: MSHR::setStalledForEvict(0x%" PRIx64 "). Address does not exist in MSHR.\n", ownerName_.c_str(), addr);
    }
    if (mshr_.find(addr)->second.entries.empty()) {
        d_->fatal(CALL_INFO, -1, "%s, Error: MSHR::setStalledForEvict(0x%" PRIx64 "). Entry list is empty.\n", ownerName_.c_str(), addr);
    }
    mshr_.find(addr)->second.entries.front().setStalledForEvict(set);
}

bool MSHR::getStalledForEvict(Addr addr) {
//    if (is_debug_addr(addr))
//        d_->debug(_L20_, "    MSHR::getStalledForEvict(0x%" PRIx64 ")\n", addr);
    if (mshr_.find(addr) == mshr_.end()) {
        return false;
    }
    if (mshr_.find(addr)->second.entries.empty()) {
        return false;
    }
    return mshr_.find(addr)->second.entries.front().getStalledForEvict();
}

void MSHR::setProfiled(Addr addr) {
//    if (is_debug_addr(addr))
//        d_->debug(_L10_, "    MSHR::setProfiled(0x%" PRIx64 ")\n", addr);
    if (is_debug_addr(addr))
        printDebug(20, "Profile", addr, "");
    
    if (mshr_.find(addr) == mshr_.end()) {
        d_->fatal(CALL_INFO, -1, "%s, Error: MSHR::setProfiled(0x%" PRIx64 "). Address does not exist in MSHR.\n", ownerName_.c_str(), addr);
    }
    if (mshr_.find(addr)->second.entries.empty()) {
        d_->fatal(CALL_INFO, -1, "%s Error: MSHR::setProfiled(0x%" PRIx64 "). Entry list is empty.\n", ownerName_.c_str(), addr);
    }
    mshr_.find(addr)->second.entries.front().setProfiled();
}

bool MSHR::getProfiled(Addr addr) {
//    if (is_debug_addr(addr))
//        d_->debug(_L20_, "    MSHR::getProfiled(0x%" PRIx64 "\n", addr);
    if (mshr_.find(addr) == mshr_.end()) {
        d_->fatal(CALL_INFO, -1, "%s, Error: MSHR::getProfiled(0x%" PRIx64 "). Address does not exist in MSHR.\n", ownerName_.c_str(), addr);
    }
    if (mshr_.find(addr)->second.entries.empty()) {
        d_->fatal(CALL_INFO, -1, "%s, Error: MSHR::getProfiled(0x%" PRIx64 "). Entry list is empty.\n", ownerName_.c_str(), addr);
    }
    return mshr_.find(addr)->second.entries.front().getProfiled();
}

bool MSHR::getProfiled(Addr addr, SST::Event::id_type id) {
    if (mshr_.find(addr) == mshr_.end())
        d_->fatal(CALL_INFO, -1, "%s, Error: MSHR::getProfiled(0x%" PRIx64 ", (%" PRIu64 ", %" PRIu64 ")). Address does not exist in MSHR.\n", ownerName_.c_str(), addr, id.first, id.second);
    if (mshr_.find(addr)->second.entries.empty())
        d_->fatal(CALL_INFO, -1, "%s, Error: MSHR::getProfiled(0x%" PRIx64 ", (%" PRIu64 ", %" PRIu64 ")). Entry list is empty.\n", ownerName_.c_str(), addr, id.first, id.second);
    for (list<MSHREntry>::iterator jt = mshr_.find(addr)->second.entries.begin(); jt != mshr_.find(addr)->second.entries.end(); jt) {
        if (jt->getType() == MSHREntryType::Event && jt->getEvent()->getID() == id) {
            return jt->getProfiled();
        }
    }
    return true; // default so we don't attempt to profile what isn't there
}

void MSHR::setProfiled(Addr addr, SST::Event::id_type id) {
    if (is_debug_addr(addr))
        printDebug(20, "Profile", addr, "");
    
    if (mshr_.find(addr) == mshr_.end()) {
        d_->fatal(CALL_INFO, -1, "%s, Error: MSHR::setProfiled(0x%" PRIx64 ", (%" PRIu64 ", %" PRIu64 ")). Address does not exist in MSHR.\n", ownerName_.c_str(), addr, id.first, id.second);
    }
    if (mshr_.find(addr)->second.entries.empty()) {
        d_->fatal(CALL_INFO, -1, "%s Error: MSHR::setProfiled(0x%" PRIx64 ", (%" PRIu64 ", %" PRIu64 ")). Entry list is empty.\n", ownerName_.c_str(), addr, id.first, id.second);
    }
    for (list<MSHREntry>::iterator jt = mshr_.find(addr)->second.entries.begin(); jt != mshr_.find(addr)->second.entries.end(); jt) {
        if (jt->getType() == MSHREntryType::Event && jt->getEvent()->getID() == id) {
            jt->setProfiled();
            return;
        }
    }
}

MSHREntry* MSHR::getOldestEntry() {
    bool first = false;
    MSHREntry* entry = nullptr;
    uint64_t time;
    
    for (MSHRBlock::iterator it = mshr_.begin(); it != mshr_.end(); it++) {
        for (list<MSHREntry>::iterator jt = it->second.entries.begin(); jt != it->second.entries.end(); jt++) {
            if (jt->getType() == MSHREntryType::Event) {
                if (!first) {
                    entry = &(*jt);
                    time = jt->getStartTime();
                } else if (jt->getStartTime() < time) {
                    entry = &(*jt);
                    time = jt->getStartTime();
                }
            }
        }
    }
    return entry;
}

void MSHR::incrementAcksNeeded(Addr addr) {
   // if (is_debug_addr(addr))
   //     d_->debug(_L10_, "    MSHR::incrementAcksNeeded(0x%" PRIx64 ")\n", addr);
    if (mshr_.find(addr) == mshr_.end()) {
        MSHRRegister reg;
        mshr_.insert(std::make_pair(addr, reg));
    }
    mshr_.find(addr)->second.acksNeeded++;
    
    if (is_debug_addr(addr)) {
        std::stringstream reason;
        reason << mshr_.find(addr)->second.acksNeeded << " acks";
        printDebug(10, "IncAck", addr, reason.str());
    }
}

/* Decrement acks needed and return if we're done waiting (acksNeeded == 0) */
bool MSHR::decrementAcksNeeded(Addr addr) {
   // if (is_debug_addr(addr))
   //     d_->debug(_L10_, "    MSHR::decrementAcksNeeded(0x%" PRIx64 ")\n", addr);
    if (mshr_.find(addr) == mshr_.end()) {
        d_->fatal(CALL_INFO, -1, "%s, Error: MSHR::decrementAcksNeeded(0x%" PRIx64 "). Address does not exist in MSHR.\n", ownerName_.c_str(), addr);
    }
    if (mshr_.find(addr)->second.acksNeeded == 0) {
        d_->fatal(CALL_INFO, -1, "%s, Error: MSHR::decrementAcksNeeded(0x%" PRIx64 "). AcksNeeded is already 0.\n", ownerName_.c_str(), addr);
    }
    mshr_.find(addr)->second.acksNeeded--;
    
    if (is_debug_addr(addr)) {
        std::stringstream reason;
        reason << mshr_.find(addr)->second.acksNeeded << " acks";
        printDebug(10, "DecAck", addr, reason.str());
    }
    
    return (mshr_.find(addr)->second.acksNeeded == 0);
}

uint32_t MSHR::getAcksNeeded(Addr addr) {
//    if (is_debug_addr(addr))
//        d_->debug(_L20_, "    MSHR::getAcksNeeded(0x%" PRIx64 ")\n", addr);
    if (mshr_.find(addr) == mshr_.end()) {
        return 0;
    }
    return (mshr_.find(addr)->second.acksNeeded);
}
    
void MSHR::setData(Addr addr, vector<uint8_t>& data, bool dirty) {
//    if (is_debug_addr(addr))
//        d_->debug(_L10_, "    MSHR::setData(0x%" PRIx64 ")\n", addr);
    if (mshr_.find(addr) == mshr_.end()) {
        d_->fatal(CALL_INFO, -1, "%s, Error: MSHR::setData(0x%" PRIx64 "). Address does not exist in MSHR.\n", ownerName_.c_str(), addr);
    }
    
    if (is_debug_addr(addr))
        printDebug(10, "SetData", addr, (dirty ? "Dirty" : "Clean"));
    
    mshr_.find(addr)->second.dataBuffer = data;
    mshr_.find(addr)->second.dataDirty = dirty;
}

void MSHR::clearData(Addr addr) {
//    if (is_debug_addr(addr))
//        d_->debug(_L10_, "    MSHR::clearData(0x%" PRIx64 ")\n", addr);
    if (is_debug_addr(addr))
        printDebug(10, "ClrData", addr, "");
    
    mshr_.find(addr)->second.dataBuffer.clear();
    mshr_.find(addr)->second.dataDirty = false;
}

vector<uint8_t>& MSHR::getData(Addr addr) {
//    if (is_debug_addr(addr))
//        d_->debug(_L20_, "    MSHR::getData(0x%" PRIx64 ")\n", addr);
    if (mshr_.find(addr) == mshr_.end()) {
        d_->fatal(CALL_INFO, -1, "%s, Error: MSHR::getData(0x%" PRIx64 "). Address does not exist in MSHR.\n", ownerName_.c_str(), addr);
    }
    return mshr_.find(addr)->second.dataBuffer;
}

bool MSHR::hasData(Addr addr) {
    if (mshr_.find(addr) == mshr_.end())
        return false;
    return !(mshr_.find(addr)->second.dataBuffer.empty());
}

bool MSHR::getDataDirty(Addr addr) {
//    if (is_debug_addr(addr))
//        d_->debug(_L20_, "    MSHR::getDataDirty(0x%" PRIx64 ")\n", addr);
    if (mshr_.find(addr) == mshr_.end()) {
        d_->fatal(CALL_INFO, -1, "%s, Error: MSHR::getDataDirty(0x%" PRIx64 "). Address does not exist in MSHR.\n", ownerName_.c_str(), addr);
    }
    return mshr_.find(addr)->second.dataDirty;
}

void MSHR::setDataDirty(Addr addr, bool dirty) {
//    if (is_debug_addr(addr))
//        d_->debug(_L10_, "    MSHR::setDataDirty(0x%" PRIx64 ")\n", addr);
    
    if (is_debug_addr(addr))
        printDebug(20, "SetDirt", addr, (dirty ? "Dirty" : "Clean"));
    
    if (mshr_.find(addr) == mshr_.end()) {
        d_->fatal(CALL_INFO, -1, "%s, Error: MSHR::setDataDirty(0x%" PRIx64 "). Address does not exist in MSHR.\n", ownerName_.c_str(), addr);
    }
    mshr_.find(addr)->second.dataDirty = dirty;
    
}

// Easier to adjust format if we do this in one place!
void MSHR::printDebug(uint32_t lev, std::string action, Addr addr, std::string reason) {
    if (lev == 10) {
        if (reason.empty())
            d_->debug(_L10_, "M: %-41" PRIu64 " %-20s MSHR:%-8s 0x%-16" PRIx64 " Sz: %-6d\n",
                    Simulation::getSimulation()->getCurrentSimCycle(), ownerName_.c_str(), action.c_str(), addr, size_);
        else 
            d_->debug(_L10_, "M: %-41" PRIu64 " %-20s MSHR:%-8s 0x%-16" PRIx64 " Sz: %-6d (%s)\n",
                    Simulation::getSimulation()->getCurrentSimCycle(), ownerName_.c_str(), action.c_str(), addr, size_, reason.c_str());
    } else {
        if (reason.empty())
            d_->debug(_L20_, "M: %-41" PRIu64 " %-20s MSHR:%-8s 0x%-16" PRIx64 " Sz: %-6d\n",
                    Simulation::getSimulation()->getCurrentSimCycle(), ownerName_.c_str(), action.c_str(), addr, size_);
        else 
            d_->debug(_L20_, "M: %-41" PRIu64 " %-20s MSHR:%-8s 0x%-16" PRIx64 " Sz: %-6d (%s)\n",
                    Simulation::getSimulation()->getCurrentSimCycle(), ownerName_.c_str(), action.c_str(), addr, size_, reason.c_str());
    }
}

// Print status. Called by cache controller on EmergencyShutdown and printStatus()
void MSHR::printStatus(Output &out) {
    out.output("    MSHR Status for %s. Size: %u. Prefetches: %u\b", ownerName_.c_str(), size_, prefetchCount_);
    for (std::map<Addr,MSHRRegister>::iterator it = mshr_.begin(); it != mshr_.end(); it++) {   // Iterate over addresses
        out.output("      Entry: Addr = 0x%" PRIx64 "\n", (it->first));
        for (std::list<MSHREntry>::iterator it2 = it->second.entries.begin(); it2 != it->second.entries.end(); it2++) { // Iterate over entries for each address
            out.output("        %s\n", it2->getString().c_str());
        }
    }
    out.output("    End MSHR Status for %s\n", ownerName_.c_str());
}


/*struct MSHREntryCompare {
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
*/
/* Public insertion methods
 * insert(Addr, MemEvent) -> insert stalled event
 * insertPointer
 * insertWriteback
 * insertInv
 */
/*bool MSHR::insert(Addr baseAddr, MemEvent* event) {
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
*/

/* Private insertion methods called by public inserts */
/*bool MSHR::insert(Addr baseAddr, mshrType entry) {
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

*/
/*void MSHR::printEntry(Addr baseAddr) {
    mshrTable::iterator it = map_.find(baseAddr);
    if (it == map_.end()) return;
    int i = 0;
    for (vector<MemEvent*>::iterator it = map_[baseAddr].begin(); it != map_[baseAddr].end(); ++it, ++i) {
        MemEvent* event = (MemEvent*)(*it);
        d_->debug(C,L5,0, "Entry %i:  Key Addr: %#016lllx, Event Addr: %#016lllx, Event Cmd = %s\n", i, baseAddr, event->getAddr(), CommandString[(int)event->getCmd()]);
    }
}*/
