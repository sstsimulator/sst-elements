// Copyright 2009-2024 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2024, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// of the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include <sst_config.h>
#include "mshr.h"

#include <algorithm>

using namespace SST;
using namespace SST::MemHierarchy;

MSHR::MSHR(ComponentId_t cid, Output* debug, int maxSize, string cacheName, std::set<Addr> debugAddr) :
    ComponentExtension(cid)
{
    dbg_ = debug;
    max_size_ = maxSize;
    size_ = 0;
    prefetch_count_ = 0;
    owner_name_ = cacheName;

    DEBUG_ADDR = debugAddr;

    flush_acks_needed_ = 0;
    flush_all_in_mshr_count_ = 0;
}

int MSHR::getMaxSize() {
    return max_size_;
}

int MSHR::getSize() {
    return size_;
}

unsigned int MSHR::getSize(Addr addr) {
    if (mshr_.find(addr) == mshr_.end())
        return 0;
    else
        return mshr_.find(addr)->second.entries_.size();
}

int MSHR::getFlushSize() {
    return (int) flushes_.size();
}

bool MSHR::exists(Addr addr) {
    return mshr_.find(addr) != mshr_.end();
}

MSHREntry MSHR::getEntry(Addr addr, size_t index) {
    if (mshr_.find(addr) == mshr_.end()) {
        dbg_->fatal(CALL_INFO, -1, "%s, Error: MSHR::getEntry(0x%" PRIx64 ", %zu). Address doesn't exist in MSHR.\n", owner_name_.c_str(), addr, index);
    }
    if (mshr_.find(addr)->second.entries_.size() <= index) {
        dbg_->fatal(CALL_INFO, -1, "%s, Error: MSHR::getEntry(0x%" PRIx64 ", %zu). Entry list size is %zu.\n", owner_name_.c_str(), addr, index, mshr_.find(addr)->second.entries_.size());
    }
    std::list<MSHREntry>::iterator it = mshr_.find(addr)->second.entries_.begin();
    std::advance(it, index);
    return *it;
}

MSHREntry MSHR::getFront(Addr addr) {
    if (mshr_.find(addr) == mshr_.end()) {
        dbg_->fatal(CALL_INFO, -1, "%s, Error: MSHR::getFront(0x%" PRIx64 "). Address doesn't exist in MSHR.\n", owner_name_.c_str(), addr);
    }

    if (mshr_.find(addr)->second.entries_.empty()) {
        dbg_->fatal(CALL_INFO, -1, "%s, Error: MSHR::getFront(0x%" PRIx64 "). Entry list is empty.\n", owner_name_.c_str(), addr);
    }
    return mshr_.find(addr)->second.entries_.front();
}

void MSHR::removeEntry(Addr addr, size_t index) {
    if (mshr_.find(addr) == mshr_.end()) {
        dbg_->fatal(CALL_INFO, -1, "%s, Error: MSHR::removeEntry(0x%" PRIx64 ", %zu). Address doesn't exist in MSHR.\n", owner_name_.c_str(), addr, index);
    }
    MSHRRegister * reg = &(mshr_.find(addr)->second);
    if (reg->entries_.size() <= index) {
        dbg_->fatal(CALL_INFO, -1, "%s, Error: MSHR::removeEntry(0x%" PRIx64 ", %zu). Entry list is shorter than requested index.\n", owner_name_.c_str(), addr, index);
    }

    std::list<MSHREntry>::iterator entry = reg->entries_.begin();
    std::advance(entry, index);

    if (entry->getType() == MSHREntryType::Event)
        size_--;

    if (is_debug_addr(addr))
        printDebug(10, "Remove", addr, (*entry).getString().c_str());

    reg->entries_.erase(entry);
    if (reg->entries_.empty()) {
        if (is_debug_addr(addr))
            printDebug(10, "Erase", addr, "");
            //dbg_->debug(_L10_, "M: %-41" PRIu64 " %-20s Erase        0x%-16" PRIx64 " %-10d\n",
            //        getCurrentSimCycle(), owner_name_.c_str(), addr, size_);
            //dbg_->debug(_L10_, "    MSHR: erasing 0x%" PRIx64 " from MSHR\n", addr);
        mshr_.erase(addr);
    }
}

void MSHR::removeFront(Addr addr) {
    if (mshr_.find(addr) == mshr_.end()) {
        dbg_->fatal(CALL_INFO, -1, "%s, Error: MSHR::removeFront(0x%" PRIx64 "). Address doesn't exist in MSHR.\n", owner_name_.c_str(), addr);
    }
    MSHRRegister * reg = &(mshr_.find(addr)->second);
    if (reg->entries_.empty()) {
        dbg_->fatal(CALL_INFO, -1, "%s, Error: MSHR::removeFront(0x%" PRIx64 "). Entry list is empty.\n", owner_name_.c_str(), addr);
    }

   // if (is_debug_addr(addr))
   //     dbg_->debug(_L10_, "    MSHR::removeFront(0x%" PRIx64 ", %s)\n", addr, reg->entries_.front().getString().c_str());

    if (getFrontType(addr) == MSHREntryType::Event)
        size_--;

    if (is_debug_addr(addr))
        printDebug(10, "RemFr", addr, (reg->entries_.front()).getString().c_str());

    reg->entries_.pop_front();
    if (reg->entries_.empty()) {
        if (is_debug_addr(addr))
            printDebug(10, "Erase", addr, "");
            //dbg_->debug(_L10_, "    MSHR: erasing 0x%" PRIx64 " from MSHR\n", addr);
        mshr_.erase(addr);
    }
}

MSHREntryType MSHR::getEntryType(Addr addr, size_t index) {
    //if (is_debug_addr(addr))
    //    dbg_->debug(_L20_, "    MSHR::getEntryType(0x%" PRIx64 ", %zu)\n", addr, index);
    if (mshr_.find(addr) == mshr_.end()) {
        dbg_->fatal(CALL_INFO, -1, "%s, Error: MSHR::getEntryType(0x%" PRIx64 ", %zu). Address doesn't exist in MSHR.\n", owner_name_.c_str(), addr, index);
    }
    if (mshr_.find(addr)->second.entries_.size() <= index) {
        dbg_->fatal(CALL_INFO, -1, "%s, Error: MSHR::getEntryType(0x%" PRIx64 ", %zu). Entry list is shoerter than index.\n", owner_name_.c_str(), addr, index);
    }
    std::list<MSHREntry>::iterator it = mshr_.find(addr)->second.entries_.begin();
    std::advance(it, index);
    return it->getType();
}

MSHREntryType MSHR::getFrontType(Addr addr) {
    //if (is_debug_addr(addr))
    //    dbg_->debug(_L20_, "    MSHR::getFrontType(0x%" PRIx64 ")\n", addr);
    if (mshr_.find(addr) == mshr_.end()) {
        dbg_->fatal(CALL_INFO, -1, "%s, Error: MSHR::getFrontType(0x%" PRIx64 "). Address doesn't exist in MSHR.\n", owner_name_.c_str(), addr);
    }
    if (mshr_.find(addr)->second.entries_.empty()) {
        dbg_->fatal(CALL_INFO, -1, "%s, Error: MSHR::getFrontType(0x%" PRIx64 "). Entry list is empty.\n", owner_name_.c_str(), addr);
    }
    return mshr_.find(addr)->second.entries_.front().getType();
}

MemEventBase* MSHR::getEntryEvent(Addr addr, size_t index) {
    //if (is_debug_addr(addr))
    //    dbg_->debug(_L20_, "    MSHR::getEntryEvent(0x%" PRIx64 ", %zu)\n", addr, index);

    if (mshr_.find(addr) == mshr_.end() || mshr_.find(addr)->second.entries_.size() <= index)
        return nullptr;

    std::list<MSHREntry>::iterator it = mshr_.find(addr)->second.entries_.begin();
    std::advance(it, index);
    if (it->getType() != MSHREntryType::Event)
        return nullptr;
    return it->getEvent();
}


MemEventBase* MSHR::getFrontEvent(Addr addr) {
    //if (is_debug_addr(addr))
    //    dbg_->debug(_L20_, "    MSHR::getFrontEvent(0x%" PRIx64 ")\n", addr);
    if (getFrontType(addr) != MSHREntryType::Event) {
        return nullptr;
    }
    return mshr_.find(addr)->second.entries_.front().getEvent();
}

MemEventBase* MSHR::getFirstEventEntry(Addr addr, Command cmd) {
//    if (is_debug_addr(addr))
//        dbg_->debug(_L20_, "    MSHR::getFirstEventEntry(0x%" PRIx64 ", %s)\n", addr, CommandString[(int)cmd]);

    if (mshr_.find(addr) == mshr_.end())
        return nullptr;

    for (std::list<MSHREntry>::iterator it = mshr_.find(addr)->second.entries_.begin(); it != mshr_.find(addr)->second.entries_.end(); it++) {
        if (it->getType() == MSHREntryType::Event && it->getEvent()->getCmd() == cmd)
            return it->getEvent();
    }
    return nullptr;
}

MemEventBase* MSHR::getFlush() {
    if (flushes_.empty()) return nullptr;
    return flushes_.front();
}

void MSHR::removeFlush() {
    if (flushes_.empty())
        dbg_->fatal(CALL_INFO, -1, "%s, Error: removeFlush. Flush queue is empty.\n", owner_name_.c_str());
    size_--;
    if (flushes_.front()->getCmd() == Command::FlushAll)
        flush_all_in_mshr_count_--;
    printDebug(10, "RemFlush", 0, flushes_.front()->toString().c_str());

    flushes_.pop_front();
}

std::list<Addr>* MSHR::getEvictPointers(Addr addr) {
    if (getFrontType(addr) != MSHREntryType::Evict)
        dbg_->fatal(CALL_INFO, -1, "%s, Error: MSHR::getEvictPointers(0x%" PRIx64 "). Entry type is not Evict.\n", owner_name_.c_str(), addr);

    return mshr_.find(addr)->second.entries_.front().getPointers();
}

// Return whether we should retry a new event or not
bool MSHR::removeEvictPointer(Addr addr, Addr addrPtr) {
    if (getFrontType(addr) == MSHREntryType::Event)
        dbg_->fatal(CALL_INFO, -1, "%s, Error: MSHR::removeEvictPointer(0x%" PRIx64 ", 0x%" PRIx64 "). Front entry type is not Evict or Writeback.\n", owner_name_.c_str(), addr, addrPtr);

    if (is_debug_addr(addr) || is_debug_addr(addrPtr)) {
        stringstream reason;
        reason << "to 0x" << std::hex << addrPtr;
        printDebug(10, "RemPtr", addr, reason.str());
    }

    // Sometimes we insert a WB before the Evict & then remove the Evict pointer, othertimes the Evict is front
    if (getFrontType(addr) == MSHREntryType::Evict) {
        MSHREntry * entry = &(mshr_.find(addr)->second.entries_.front());
        entry->getPointers()->remove(addrPtr);
        if (entry->getPointers()->empty()) {
            removeFront(addr);
            return true;
        }
    } else {
        std::list<MSHREntry>::iterator it = mshr_.find(addr)->second.entries_.begin();
        it++;
        if (it->getType() != MSHREntryType::Evict)
            dbg_->fatal(CALL_INFO, -1, "%s, Error: MSHR::removeEvictPointer(0x%" PRIx64 ", 0x%" PRIx64 "). Entry type is not Evict.\n", owner_name_.c_str(), addr, addrPtr);
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
        return mshr_.find(addr)->second.entries_.front().getDowngrade();
    return false;
}

int MSHR::insertEvent(Addr addr, MemEventBase* event, int pos, bool fwdRequest, bool stallEvict) {
    if ((size_ == max_size_) || (!fwdRequest && (size_ == max_size_-1))) {
        if (is_debug_addr(addr)) {
            stringstream reason;
            reason << "<" << event->getID().first << "," << event->getID().second << "> FAILED " << (fwdRequest ? "fwd, " : "") << "maxsz: " << max_size_;
            printDebug(10, "InsEv", addr, reason.str());
        }
        return -1;
    }

    // Success
    size_++;

    if (mshr_.find(addr) == mshr_.end()) {
        MSHRRegister reg;
        reg.entries_.push_back(MSHREntry(event, stallEvict, getCurrentSimCycle()));

        mshr_.insert(std::make_pair(addr, reg));
        if (is_debug_addr(addr)) {
            stringstream reason;
            reason << "<" << event->getID().first << "," << event->getID().second << ">, pos=0";
            printDebug(10, "InsEv", addr, reason.str());

        }

        return 0;
    } else {
        if (pos == -1 || pos > mshr_.find(addr)->second.entries_.size()) {
            mshr_.find(addr)->second.entries_.push_back(MSHREntry(event, stallEvict, getCurrentSimCycle()));
            if (is_debug_addr(addr)) {
                stringstream reason;
                reason << "<" << event->getID().first << "," << event->getID().second << ">, pos=" << (mshr_.find(addr)->second.entries_.size() - 1);
                printDebug(10, "InsEv", addr, reason.str());
            }
            return (mshr_.find(addr)->second.entries_.size() - 1);
        } else {
            std::list<MSHREntry>::iterator it = mshr_.find(addr)->second.entries_.begin();
            std::advance(it, pos);
            mshr_.find(addr)->second.entries_.insert(it, MSHREntry(event, stallEvict, getCurrentSimCycle()));
            if (is_debug_addr(addr)) {
                stringstream reason;
                reason << "<" << event->getID().first << "," << event->getID().second << ">, pos=" << pos;
                printDebug(10, "InsEv", addr, reason.str());
            }
            return pos;
        }
    }
}

/*
 *  Attempt to insert the event if a conflict exists
 *  Return:
 *      0 = no conflict, not inserted
 *      >=1 = conflict, inserted (returns position in which event was inserted)
 *      -1 = conflict, not inserted
 */
int MSHR::insertEventIfConflict(Addr addr, MemEventBase* event) {
    if (mshr_.find(addr) == mshr_.end())
        return 0;
    
    if (size_ == max_size_-1) { /* Assuming fwdEvent == false */
        if (is_debug_addr(addr)) {
            stringstream reason;
            reason << "<" << event->getID().first << "," << event->getID().second << "> FAILED " << "maxsz: " << max_size_;
            printDebug(10, "InsEv", addr, reason.str());
        }
        return -1;
    }
    size_++;
    mshr_.find(addr)->second.entries_.push_back(MSHREntry(event, false, getCurrentSimCycle()));
    if (is_debug_addr(addr)) {
        stringstream reason;
        reason << "<" << event->getID().first << "," << event->getID().second << ">, pos=" << (mshr_.find(addr)->second.entries_.size() - 1);
        printDebug(10, "InsEv", addr, reason.str());
    }
    return (mshr_.find(addr)->second.entries_.size() - 1);
}

MemEventBase* MSHR::swapFrontEvent(Addr addr, MemEventBase* event) {
    if (is_debug_addr(addr))
        printDebug(10, "SwpEv", addr, "");

    if (mshr_.find(addr)->second.entries_.empty())
        return nullptr;

    return  mshr_.find(addr)->second.entries_.front().swapEvent(event, getCurrentSimCycle());
}

void MSHR::moveEntryToFront(Addr addr, unsigned int index) {
    if (mshr_.find(addr) == mshr_.end()) {
        dbg_->fatal(CALL_INFO, -1, "%s, Error: MSHR::moveEntryToFront(0x%" PRIx64 ", %u). Address doesn't exist in MSHR.\n", owner_name_.c_str(), addr, index);
    }
    MSHRRegister * reg = &(mshr_.find(addr)->second);
    if (reg->entries_.size() <= index) {
        dbg_->fatal(CALL_INFO, -1, "%s, Error: MSHR::moveEntryToFront(0x%" PRIx64 ", %u). Entry list is shorter than requested index.\n", owner_name_.c_str(), addr, index);
    }

    std::list<MSHREntry>::iterator entry = reg->entries_.begin();
    std::advance(entry, index);

    MSHREntry tmpEntry = *entry;

    if (is_debug_addr(addr))
        printDebug(10, "MvEnt", addr, entry->getString());
    reg->entries_.erase(entry);
    reg->entries_.push_front(tmpEntry);
}

bool MSHR::insertWriteback(Addr addr, bool downgrade) {
//    if (is_debug_addr(addr))
//        dbg_->debug(_L10_, "    MSHR::insertWriteback(0x%" PRIx64 ")\n", addr);

    if (is_debug_addr(addr)) {
        stringstream reason;
        reason << "Downgrade: " << (downgrade ? "T" : "F");
        printDebug(10, "InsWB", addr, reason.str());
    }

    if (mshr_.find(addr) == mshr_.end()) {
        MSHRRegister reg;
        reg.entries_.push_back(MSHREntry(downgrade, getCurrentSimCycle()));
        mshr_.insert(std::make_pair(addr, reg));
    } else {
        mshr_.find(addr)->second.entries_.push_front(MSHREntry(downgrade, getCurrentSimCycle()));
    }

    return true;
}


bool MSHR::insertEviction(Addr oldAddr, Addr newAddr) {
//    if (is_debug_addr(oldAddr) || is_debug_addr(newAddr))
//        dbg_->debug(_L10_, "    MSHR::insertEviction(0x%" PRIx64 ", 0x%" PRIx64 ")\n", oldAddr, newAddr);

    if (is_debug_addr(oldAddr) || is_debug_addr(newAddr)) {
        stringstream reason;
        reason << "to 0x" << std::hex << newAddr;
        printDebug(10, "InsPtr", oldAddr, reason.str());
    }

    if (mshr_.find(oldAddr) == mshr_.end()) {  // No MSHR entry for oldAddr
        MSHRRegister reg;
        reg.entries_.push_back(MSHREntry(newAddr, getCurrentSimCycle()));
        mshr_.insert(std::make_pair(oldAddr, reg));
    } else {
        list<MSHREntry>* entries = &(mshr_.find(oldAddr)->second.entries_);
        if (!entries->empty() && entries->back().getType() == MSHREntryType::Evict) { // MSHR entry for oldAddr is an Evict
            entries->back().getPointers()->push_back(newAddr);
        } else { // MSHR entry for oldAddr is not an Evict (or no entry exists)
            entries->push_back(MSHREntry(newAddr, getCurrentSimCycle()));
        }
    }
    return true;
}


MemEventStatus MSHR::insertFlush(MemEventBase* event, bool forward_flush, bool check_ok_to_forward) {
    if (size_ == max_size_-1 || (forward_flush && size_ == max_size_)) {
        return MemEventStatus::Reject; // MSHR is full, cannot enqueue flush
    }
    size_++;

    stringstream reason;
    reason << "<" << event->getID().first << "," << event->getID().second << ">, size=" << flushes_.size();

    printDebug(10, "InsFlush", 0, reason.str());
    MemEventStatus status;
    if (forward_flush) {
        flushes_.push_front(event);
        status = flushes_.front() == event ? MemEventStatus::OK : MemEventStatus::Stall;
    } else {
        flushes_.push_back(event);
        flush_all_in_mshr_count_++;
        if (check_ok_to_forward) {
            status = flush_all_in_mshr_count_ == 1 ? MemEventStatus::OK : MemEventStatus::Stall;
        } else {
            status = flushes_.front() == event ? MemEventStatus::OK : MemEventStatus::Stall;
        }
    }
    
    return status;
}

void MSHR::incrementFlushCount(int count) {
    flush_acks_needed_ += count;
}

void MSHR::decrementFlushCount() {
    flush_acks_needed_--;
}

int MSHR::getFlushCount() {
    return flush_acks_needed_;
}

void MSHR::addPendingRetry(Addr addr) {
    if (is_debug_addr(addr))
        printDebug(20, "IncRetry", addr, "");

    if (mshr_.find(addr) == mshr_.end()) {
        dbg_->fatal(CALL_INFO, -1, "%s, Error: MSHR::addPendingRetry(0x%" PRIx64 "). Address does not exist in MSHR.\n", owner_name_.c_str(), addr);
    }
    mshr_.find(addr)->second.addPendingRetry();
}

void MSHR::removePendingRetry(Addr addr) {
    if (is_debug_addr(addr))
        printDebug(20, "DecRetry", addr, "");

    if (mshr_.find(addr) == mshr_.end()) {
        dbg_->fatal(CALL_INFO, -1, "%s, Error: MSHR::removePendingRetry(0x%" PRIx64 "). Address does not exist in MSHR.\n", owner_name_.c_str(), addr);
    }
    mshr_.find(addr)->second.removePendingRetry();
}

uint32_t MSHR::getPendingRetries(Addr addr) {
    if (mshr_.find(addr) == mshr_.end())
        return 0;

    return mshr_.find(addr)->second.getPendingRetries();
}


void MSHR::setInProgress(Addr addr, bool value) {
//    if (is_debug_addr(addr))
//        dbg_->debug(_L10_, "    MSHR::setInProgress(0x%" PRIx64 ")\n", addr);
    if (is_debug_addr(addr))
        printDebug(20, "InProg", addr, "");

    if (mshr_.find(addr) == mshr_.end()) {
        dbg_->fatal(CALL_INFO, -1, "%s, Error: MSHR::setInProgress(0x%" PRIx64 "). Address does not exist in MSHR.\n", owner_name_.c_str(), addr);
    }
    if (mshr_.find(addr)->second.entries_.empty()) {
        dbg_->fatal(CALL_INFO, -1, "%s, Error: MSHR::setInProgress(0x%" PRIx64 "). Entry list is empty.\n", owner_name_.c_str(), addr);
    }
    mshr_.find(addr)->second.entries_.front().setInProgress(value);
}

bool MSHR::getInProgress(Addr addr) {
    if (mshr_.find(addr) == mshr_.end()) {
        return false;
    }
    if (mshr_.find(addr)->second.entries_.empty()) {
        return false;
    }
    return mshr_.find(addr)->second.entries_.front().getInProgress();
}

void MSHR::setStalledForEvict(Addr addr, bool set) {
    if (is_debug_addr(addr)) {
        if (set)
            printDebug(20, "Stall", addr, "");
        else
            printDebug(20, "Unstall", addr, "");
    }

    if (mshr_.find(addr) == mshr_.end()) {
        dbg_->fatal(CALL_INFO, -1, "%s, Error: MSHR::setStalledForEvict(0x%" PRIx64 "). Address does not exist in MSHR.\n", owner_name_.c_str(), addr);
    }
    if (mshr_.find(addr)->second.entries_.empty()) {
        dbg_->fatal(CALL_INFO, -1, "%s, Error: MSHR::setStalledForEvict(0x%" PRIx64 "). Entry list is empty.\n", owner_name_.c_str(), addr);
    }
    mshr_.find(addr)->second.entries_.front().setStalledForEvict(set);
}

bool MSHR::getStalledForEvict(Addr addr) {
    if (mshr_.find(addr) == mshr_.end()) {
        return false;
    }
    if (mshr_.find(addr)->second.entries_.empty()) {
        return false;
    }
    return mshr_.find(addr)->second.entries_.front().getStalledForEvict();
}

void MSHR::setProfiled(Addr addr) {
    if (is_debug_addr(addr))
        printDebug(20, "Profile", addr, "");

    if (mshr_.find(addr) == mshr_.end()) {
        dbg_->fatal(CALL_INFO, -1, "%s, Error: MSHR::setProfiled(0x%" PRIx64 "). Address does not exist in MSHR.\n", owner_name_.c_str(), addr);
    }
    if (mshr_.find(addr)->second.entries_.empty()) {
        dbg_->fatal(CALL_INFO, -1, "%s Error: MSHR::setProfiled(0x%" PRIx64 "). Entry list is empty.\n", owner_name_.c_str(), addr);
    }
    mshr_.find(addr)->second.entries_.front().setProfiled();
}

bool MSHR::getProfiled(Addr addr) {
//    if (is_debug_addr(addr))
//        dbg_->debug(_L20_, "    MSHR::getProfiled(0x%" PRIx64 "\n", addr);
    if (mshr_.find(addr) == mshr_.end()) {
        dbg_->fatal(CALL_INFO, -1, "%s, Error: MSHR::getProfiled(0x%" PRIx64 "). Address does not exist in MSHR.\n", owner_name_.c_str(), addr);
    }
    if (mshr_.find(addr)->second.entries_.empty()) {
        dbg_->fatal(CALL_INFO, -1, "%s, Error: MSHR::getProfiled(0x%" PRIx64 "). Entry list is empty.\n", owner_name_.c_str(), addr);
    }
    return mshr_.find(addr)->second.entries_.front().getProfiled();
}

bool MSHR::getProfiled(Addr addr, SST::Event::id_type id) {
    if (mshr_.find(addr) == mshr_.end())
        dbg_->fatal(CALL_INFO, -1, "%s, Error: MSHR::getProfiled(0x%" PRIx64 ", (%" PRIu64 ", %" PRId32 ")). Address does not exist in MSHR.\n", owner_name_.c_str(), addr, id.first, id.second);
    if (mshr_.find(addr)->second.entries_.empty())
        dbg_->fatal(CALL_INFO, -1, "%s, Error: MSHR::getProfiled(0x%" PRIx64 ", (%" PRIu64 ", %" PRId32 ")). Entry list is empty.\n", owner_name_.c_str(), addr, id.first, id.second);
    for (list<MSHREntry>::iterator jt = mshr_.find(addr)->second.entries_.begin(); jt != mshr_.find(addr)->second.entries_.end(); jt++) {
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
        dbg_->fatal(CALL_INFO, -1, "%s, Error: MSHR::setProfiled(0x%" PRIx64 ", (%" PRIu64 ", %" PRId32 ")). Address does not exist in MSHR.\n", owner_name_.c_str(), addr, id.first, id.second);
    }
    if (mshr_.find(addr)->second.entries_.empty()) {
        dbg_->fatal(CALL_INFO, -1, "%s Error: MSHR::setProfiled(0x%" PRIx64 ", (%" PRIu64 ", %" PRId32 ")). Entry list is empty.\n", owner_name_.c_str(), addr, id.first, id.second);
    }
    for (list<MSHREntry>::iterator jt = mshr_.find(addr)->second.entries_.begin(); jt != mshr_.find(addr)->second.entries_.end(); jt++) {
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
        for (list<MSHREntry>::iterator jt = it->second.entries_.begin(); jt != it->second.entries_.end(); jt++) {
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
   //     dbg_->debug(_L10_, "    MSHR::incrementAcksNeeded(0x%" PRIx64 ")\n", addr);
    if (mshr_.find(addr) == mshr_.end()) {
        MSHRRegister reg;
        mshr_.insert(std::make_pair(addr, reg));
    }
    mshr_.find(addr)->second.acks_needed_++;

    if (is_debug_addr(addr)) {
        std::stringstream reason;
        reason << mshr_.find(addr)->second.acks_needed_ << " acks";
        printDebug(10, "IncAck", addr, reason.str());
    }
}

/* Decrement acks needed and return if we're done waiting (acks_needed_ == 0) */
bool MSHR::decrementAcksNeeded(Addr addr) {
   // if (is_debug_addr(addr))
   //     dbg_->debug(_L10_, "    MSHR::decrementAcksNeeded(0x%" PRIx64 ")\n", addr);
    if (mshr_.find(addr) == mshr_.end()) {
        dbg_->fatal(CALL_INFO, -1, "%s, Error: MSHR::decrementAcksNeeded(0x%" PRIx64 "). Address does not exist in MSHR.\n", owner_name_.c_str(), addr);
    }
    if (mshr_.find(addr)->second.acks_needed_ == 0) {
        dbg_->fatal(CALL_INFO, -1, "%s, Error: MSHR::decrementAcksNeeded(0x%" PRIx64 "). AcksNeeded is already 0.\n", owner_name_.c_str(), addr);
    }
    mshr_.find(addr)->second.acks_needed_--;

    if (is_debug_addr(addr)) {
        std::stringstream reason;
        reason << mshr_.find(addr)->second.acks_needed_ << " acks";
        printDebug(10, "DecAck", addr, reason.str());
    }

    return (mshr_.find(addr)->second.acks_needed_ == 0);
}

uint32_t MSHR::getAcksNeeded(Addr addr) {
//    if (is_debug_addr(addr))
//        dbg_->debug(_L20_, "    MSHR::getAcksNeeded(0x%" PRIx64 ")\n", addr);
    if (mshr_.find(addr) == mshr_.end()) {
        return 0;
    }
    return (mshr_.find(addr)->second.acks_needed_);
}

void MSHR::setData(Addr addr, vector<uint8_t>& data, bool dirty) {
//    if (is_debug_addr(addr))
//        dbg_->debug(_L10_, "    MSHR::setData(0x%" PRIx64 ")\n", addr);
    if (mshr_.find(addr) == mshr_.end()) {
        dbg_->fatal(CALL_INFO, -1, "%s, Error: MSHR::setData(0x%" PRIx64 "). Address does not exist in MSHR.\n", owner_name_.c_str(), addr);
    }

    if (is_debug_addr(addr))
        printDebug(10, "SetData", addr, (dirty ? "Dirty" : "Clean"));

    mshr_.find(addr)->second.data_buffer_ = data;
    mshr_.find(addr)->second.data_dirty_ = dirty;
}

void MSHR::clearData(Addr addr) {
//    if (is_debug_addr(addr))
//        dbg_->debug(_L10_, "    MSHR::clearData(0x%" PRIx64 ")\n", addr);
    if (is_debug_addr(addr))
        printDebug(10, "ClrData", addr, "");

    mshr_.find(addr)->second.data_buffer_.clear();
    mshr_.find(addr)->second.data_dirty_ = false;
}

vector<uint8_t>& MSHR::getData(Addr addr) {
//    if (is_debug_addr(addr))
//        dbg_->debug(_L20_, "    MSHR::getData(0x%" PRIx64 ")\n", addr);
    if (mshr_.find(addr) == mshr_.end()) {
        dbg_->fatal(CALL_INFO, -1, "%s, Error: MSHR::getData(0x%" PRIx64 "). Address does not exist in MSHR.\n", owner_name_.c_str(), addr);
    }
    return mshr_.find(addr)->second.data_buffer_;
}

bool MSHR::hasData(Addr addr) {
    if (mshr_.find(addr) == mshr_.end())
        return false;
    return !(mshr_.find(addr)->second.data_buffer_.empty());
}

bool MSHR::getDataDirty(Addr addr) {
//    if (is_debug_addr(addr))
//        dbg_->debug(_L20_, "    MSHR::getDataDirty(0x%" PRIx64 ")\n", addr);
    if (mshr_.find(addr) == mshr_.end()) {
        dbg_->fatal(CALL_INFO, -1, "%s, Error: MSHR::getDataDirty(0x%" PRIx64 "). Address does not exist in MSHR.\n", owner_name_.c_str(), addr);
    }
    return mshr_.find(addr)->second.data_dirty_;
}

void MSHR::setDataDirty(Addr addr, bool dirty) {
//    if (is_debug_addr(addr))
//        dbg_->debug(_L10_, "    MSHR::setDataDirty(0x%" PRIx64 ")\n", addr);

    if (is_debug_addr(addr))
        printDebug(20, "SetDirt", addr, (dirty ? "Dirty" : "Clean"));

    if (mshr_.find(addr) == mshr_.end()) {
        dbg_->fatal(CALL_INFO, -1, "%s, Error: MSHR::setDataDirty(0x%" PRIx64 "). Address does not exist in MSHR.\n", owner_name_.c_str(), addr);
    }
    mshr_.find(addr)->second.data_dirty_ = dirty;

}

// Easier to adjust format if we do this in one place!
void MSHR::printDebug(uint32_t lev, std::string action, Addr addr, std::string reason) {
    if (lev == 10) {
        if (reason.empty())
            dbg_->debug(_L10_, "M: %-20" PRIu64 " -                    %-25s MSHR:%-8s 0x%-16" PRIx64 " Sz: %-6d\n",
                    getCurrentSimCycle(), owner_name_.c_str(), action.c_str(), addr, size_);
        else
            dbg_->debug(_L10_, "M: %-20" PRIu64 " -                    %-25s MSHR:%-8s 0x%-16" PRIx64 " Sz: %-6d (%s)\n",
                    getCurrentSimCycle(), owner_name_.c_str(), action.c_str(), addr, size_, reason.c_str());
    } else {
        if (reason.empty())
            dbg_->debug(_L20_, "M: %-41" PRIu64 " -                    %-25s MSHR:%-8s 0x%-16" PRIx64 " Sz: %-6d\n",
                    getCurrentSimCycle(), owner_name_.c_str(), action.c_str(), addr, size_);
        else
            dbg_->debug(_L20_, "M: %-41" PRIu64 " -                    %-25s MSHR:%-8s 0x%-16" PRIx64 " Sz: %-6d (%s)\n",
                    getCurrentSimCycle(), owner_name_.c_str(), action.c_str(), addr, size_, reason.c_str());
    }
}

// Print status. Called by cache controller on EmergencyShutdown and printStatus()
void MSHR::printStatus(Output &out) {
    out.output("    MSHR Status for %s. Size: %u. Prefetches: %u\b", owner_name_.c_str(), size_, prefetch_count_);
    for (std::map<Addr,MSHRRegister>::iterator it = mshr_.begin(); it != mshr_.end(); it++) {   // Iterate over addresses
        out.output("      Entry: Addr = 0x%" PRIx64 "\n", (it->first));
        for (std::list<MSHREntry>::iterator it2 = it->second.entries_.begin(); it2 != it->second.entries_.end(); it2++) { // Iterate over entries for each address
            out.output("        %s\n", it2->getString().c_str());
        }
    }
    out.output("    End MSHR Status for %s\n", owner_name_.c_str());
}

