// Copyright 2009-2025 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2025, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// of the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef _MSHR_H_
#define _MSHR_H_

#include <list>

#include <map>
#include <string>
#include <sstream>

#include <sst/core/event.h>
#include <sst/core/sst_types.h>
#include <sst/core/component.h>
#include <sst/core/componentExtension.h>
#include <sst/core/output.h>

#include "sst/elements/memHierarchy/memEvent.h"
#include "sst/elements/memHierarchy/memTypes.h"
#include "sst/elements/memHierarchy/util.h"

namespace SST { namespace MemHierarchy {

using namespace std;

/*
 * MSHR buffers pending and outstanding requests
 * Three types:
 *  - Writeback
 *  - Eviction
 *  - Event
 */

enum class MSHREntryType { Event, Evict, Writeback };

class MSHREntry {
    public:
        // Event entry
    MSHREntry(MemEventBase* ev, bool stallEvict, SimTime_t curr_time) {
            entry_type_ = MSHREntryType::Event;
            evict_ptrs_ = nullptr;
            event_ = ev;
            time_ = curr_time;
            in_progress_ = false;
            need_evict_ = stallEvict;
            profiled_ = false;
            downgrade_ = false;
        }

        // Writeback entry
    MSHREntry(bool downgr, SimTime_t curr_time) {
            entry_type_ = MSHREntryType::Writeback;
            evict_ptrs_ = nullptr;
            event_ = nullptr;
            time_ = curr_time;
            in_progress_ = false;
            need_evict_ = false;
            profiled_ = false;
            event_ = nullptr;
            downgrade_ = downgr;
        }

        // Evict entry
    MSHREntry(Addr addr, SimTime_t curr_time) {
            entry_type_ = MSHREntryType::Evict;
            event_ = nullptr;
            evict_ptrs_ = new std::list<Addr>;
            evict_ptrs_->push_back(addr);
            time_ = curr_time;
            in_progress_ = false;
            need_evict_ = false;
            profiled_ = false;
            event_ = nullptr;
            downgrade_ = false;
        }

    MSHREntry(const MSHREntry& entry) {
        entry_type_ = entry.entry_type_;
        evict_ptrs_ = entry.evict_ptrs_;
        event_ = entry.event_;
        time_ = entry.time_;
        in_progress_ = entry.in_progress_;
        need_evict_ = entry.need_evict_;
        profiled_ = entry.profiled_;
        downgrade_ = entry.downgrade_;
    }

        MSHREntryType getType() { return entry_type_; }

        bool getInProgress() { return in_progress_; }
        void setInProgress(bool value) { in_progress_ = value; }

        bool getStalledForEvict() { return need_evict_; }
        void setStalledForEvict(bool set) { need_evict_ = set; }

        void setProfiled() { profiled_ = true; }
        bool getProfiled() { return profiled_; }

        SimTime_t getStartTime() { return time_; }

        std::list<Addr>* getPointers() {
            return evict_ptrs_;
        }

        MemEventBase * getEvent() {
            return event_;
        }

        bool getDowngrade() {
            return downgrade_;
        }

        /* Remove event at head of queue and swap with a new one (leaving entry in place) */
    MemEventBase * swapEvent(MemEventBase* newEvent, SimTime_t curr_time) {
            if (entry_type_ != MSHREntryType::Event) return nullptr;
            time_ = curr_time;
            MemEventBase* oldEvent = event_;
            event_ = newEvent;
            return oldEvent;
        }

        std::string getString() {
            std::ostringstream str;
            str << "Time: " << std::to_string(time_);
            str << " Iss: ";
            in_progress_ ? str << "T" : str << "F";
            str << " Evi: ";
            need_evict_ ? str << "T" : str << "F";
            str << " Stat: ";
            profiled_ ? str << "T" : str << "F";
            if (entry_type_ == MSHREntryType::Event) {
                str << " Type: Event" << " (" << event_->getBriefString() << ")";
            } else if (entry_type_ == MSHREntryType::Evict) {
                str << " Type: Evict (";
                for (std::list<Addr>::iterator it = evict_ptrs_->begin(); it != evict_ptrs_->end(); it++) {
                    str << " 0x" << std::hex << *it;
                }
                str << ")";
            } else {
                str << " Type: Writeback";
            }
            return str.str();
        }

    private:
        MSHREntryType entry_type_;      // Type of entry
        std::list<Addr> *evict_ptrs_;   // Specific to Evict type - address(es) waiting for this address to be evicted from cache
        MemEventBase* event_;           // Specific to Event type
        SimTime_t time_;                // Time event was enqueued in the MSHR - used for latency stats and detecting timeout
        bool need_evict_;               // Whether event is stalled for an eviction
        bool in_progress_;              // Whether event is currently being handled; prevents early retries
        bool profiled_;                 // Whether event has been profiled/added to stats already
        bool downgrade_;                // Specific to Writeback type
};

struct MSHRRegister {
    MSHRRegister() : acks_needed_(0), data_dirty_(false), pending_retries_(0) { }
    list<MSHREntry> entries_;
    uint32_t acks_needed_;
    vector<uint8_t> data_buffer_;
    bool data_dirty_;
    uint32_t pending_retries_;

    uint32_t getPendingRetries() { return pending_retries_; }
    void addPendingRetry() { pending_retries_++; }
    void removePendingRetry() { pending_retries_--; }
};

typedef map<Addr, MSHRRegister> MSHRBlock;

/**
 *  Implements an MSHR with entries of type mshrEntry
 */
class MSHR : public ComponentExtension {
public:

    /* Construct a new MSHR */
    MSHR(ComponentId_t cid, Output* dbg, int maxSize, string cacheName, std::set<Addr> debugAddr);

    /* Return maxSize_ */
    int getMaxSize();

    /* Return size_ */
    int getSize();

    /* Return size of MSHR list for a particular address */
    unsigned int getSize(Addr addr);

    /* Return the size of the flush queue */
    int getFlushSize();

    /* Return whether an address exists in the MSHR (i.e., current outstanding events for that address) */
    bool exists(Addr addr);

// Functions to manage the head entry in an address's list.
    /* Accessor for first entry in the list for a particular address since that's most common */
    MSHREntry getFront(Addr addr);

    /* Remove first entry in the list for a particular address */
    void removeFront(Addr addr);

    /* Return what type the first entry in the list for a particular address is */
    MSHREntryType getFrontType(Addr addr);

    /* Return the event at the front of the list for an address IF it is an event */
    MemEventBase* getFrontEvent(Addr addr);

    /* Return the list of addresses which have events waiting for 'addr' to be evicted */
    std::list<Addr>* getEvictPointers(Addr addr);

    /* Remove the eviction pointer from ptrAddr to addr */
    bool removeEvictPointer(Addr addr, Addr ptrAddr);

// Functions to manage entries other than the head of the list for an address
    /* Sometimes need to rearrange MSHR entries to handle races.
     * Move the entry at index 'index' for 'addr' to the front of the list.
     */
    void moveEntryToFront(Addr addr, unsigned int index);

    /* Get the entry at index 'index' in 'addr's list. */
    MSHREntry getEntry(Addr addr, size_t index);

    /* Remove the entry at index 'index' in 'addr's list. */
    void removeEntry(Addr addr, size_t index);

    /* Return the type of the entry at index 'index' in address 'addr's list */
    MSHREntryType getEntryType(Addr addr, size_t index);

    /* Return the event mapping to entry at index 'index' in address 'addr's list IF it is an event */
    MemEventBase* getEntryEvent(Addr addr, size_t index);

    /* Swap the event at the head of 'addr's list for 'event' and return the original one */
    MemEventBase* swapFrontEvent(Addr addr, MemEventBase* event);

    /* Return whether the entry at the head of 'addr's list is a writeback (including downgrade) */
    bool pendingWriteback(Addr addr);

    /* Return whether the entry at the head of 'addr's list is a downgrade */
    bool pendingWritebackIsDowngrade(Addr addr);

// Functions to manage Flush list
    MemEventBase* getFlush();
    void removeFlush();
    void incrementFlushCount(int count = 1);
    void decrementFlushCount();
    int getFlushCount();

// Functions to manage events and retries
    int insertEvent(Addr addr, MemEventBase* event, int position, bool fwdRequest, bool stallEvict);
    int insertEventIfConflict(Addr addr, MemEventBase* event);
    bool insertWriteback(Addr addr, bool downgrade);
    bool insertEviction(Addr evictAddr, Addr newAddr);
    MemEventStatus insertFlush(MemEventBase* event, bool forward, bool check_ok_to_forward = false);

    void setInProgress(Addr addr, bool value = true);
    bool getInProgress(Addr addr);

    void addPendingRetry(Addr addr);
    void removePendingRetry(Addr addr);
    uint32_t getPendingRetries(Addr addr);

    void setStalledForEvict(Addr addr, bool set);
    bool getStalledForEvict(Addr addr);

    void setProfiled(Addr addr);
    bool getProfiled(Addr addr);

    void setProfiled(Addr addr, SST::Event::id_type id);
    bool getProfiled(Addr addr, SST::Event::id_type id);

    MemEventBase* getFirstEventEntry(Addr addr, Command cmd);
    MSHREntry* getOldestEntry();

    void incrementAcksNeeded(Addr addr);
    bool decrementAcksNeeded(Addr addr);
    uint32_t getAcksNeeded(Addr addr);

// Functions to manage temporary data storage for an address
    void setData(Addr addr, vector<uint8_t>& data, bool dirty = false);
    void clearData(Addr addr);
    vector<uint8_t>& getData(Addr addr);
    bool hasData(Addr addr);
    bool getDataDirty(Addr addr);
    void setDataDirty(Addr addr, bool dirty);

    /* Print contents of MSHR to out*/
    void printStatus(Output &out);

private:

    void printDebug(uint32_t level, std::string action, Addr addr, std::string reason);

    MSHRBlock mshr_;                                    // MSHR maps each address to a list of events/evictions/etc
    std::list<MemEventBase*> flushes_;                  // Flushes are not linked to a particular address so are stored outside the mshr_ structure
    int flush_all_in_mshr_count_;                       // Number of FlushAll (vs ForwardFlush) in the flushes_ list
    int flush_acks_needed_;                             // Number of things that need to complete before flush can retry
    Output* dbg_;                                       // Debug output acquired from owning component
    int size_;                                          // Current entries in mshr_ + flushes_
    int max_size_;                                      // Size limit for mshr_ + flushes_
    int prefetch_count_;                                // Number of prefetches in mshr_
    string owner_name_;                                 // Name of owning component
    std::set<Addr> DEBUG_ADDR;                          // Which addresses to print debug info for (empty = all)
};
}}
#endif
