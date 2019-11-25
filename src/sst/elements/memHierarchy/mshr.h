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

#ifndef _MSHR_H_
#define _MSHR_H_

#include <map>
#include <string>
#include <sstream>

#include <sst/core/event.h>
#include <sst/core/sst_types.h>
#include <sst/core/component.h>
#include <sst/core/output.h>
#include <sst/core/simulation.h>

#include "sst/elements/memHierarchy/memEvent.h"
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
        MSHREntry(MemEventBase* ev, bool stallEvict) {
            type = MSHREntryType::Event;
            evictPtrs = nullptr;
            event = ev;
            time = Simulation::getSimulation()->getCurrentSimCycle();
            inProgress = false;
            needEvict = stallEvict;
            profiled = false;
            downgrade = false;
        }

        // Writeback entry
        MSHREntry(bool downgr) {
            type = MSHREntryType::Writeback;
            evictPtrs = nullptr;
            event = nullptr;
            time = Simulation::getSimulation()->getCurrentSimCycle();
            inProgress = false;
            needEvict = false;
            profiled = false;
            event = nullptr;
            downgrade = downgr;
        }

        // Evict entry
        MSHREntry(Addr addr) {
            type = MSHREntryType::Evict;
            event = nullptr;
            evictPtrs = new std::list<Addr>;
            evictPtrs->push_back(addr);
            time = Simulation::getSimulation()->getCurrentSimCycle();
            inProgress = false;
            needEvict = false;
            profiled = false;
            event = nullptr;
            downgrade = false;
        }

        MSHREntry(const MSHREntry& entry) {
            type = entry.type;
            evictPtrs = entry.evictPtrs;
            event = entry.event;
            time = entry.time;
            inProgress = entry.inProgress;
            needEvict = entry.needEvict;
            profiled = entry.profiled;
            downgrade = entry.downgrade;
        }

        MSHREntryType getType() { return type; }

        bool getInProgress() { return inProgress; }
        void setInProgress(bool value) { inProgress = value; }

        bool getStalledForEvict() { return needEvict; }
        void setStalledForEvict(bool set) { needEvict = set; }

        void setProfiled() { profiled = true; }
        bool getProfiled() { return profiled; } 

        SimTime_t getStartTime() { return time; }

        std::list<Addr>* getPointers() {
            return evictPtrs;
        }

        MemEventBase * getEvent() {
            return event;
        }

        bool getDowngrade() {
            return downgrade;
        }

        /* Remove event at head of queue and swap with a new one (leaving entry in place) */
        MemEventBase * swapEvent(MemEventBase* newEvent) {
            if (type != MSHREntryType::Event) return nullptr;
            time = Simulation::getSimulation()->getCurrentSimCycle();
            MemEventBase* oldEvent = event;
            event = newEvent;
            return oldEvent;
        }

        std::string getString() {
            std::ostringstream str;
            str << "Time: " << std::to_string(time);
            str << " Iss: ";
            inProgress ? str << "T" : str << "F";
            str << " Evi: ";
            needEvict ? str << "T" : str << "F";
            str << " Stat: ";
            profiled ? str << "T" : str << "F";
            if (type == MSHREntryType::Event) {
                str << " Type: Event" << " (" << event->getBriefString() << ")";
            } else if (type == MSHREntryType::Evict) {
                str << " Type: Evict (";
                for (std::list<Addr>::iterator it = evictPtrs->begin(); it != evictPtrs->end(); it++) {
                    str << " 0x" << std::hex << *it;
                }
                str << ")";
            } else {
                str << " Type: Writeback";
            }
            return str.str();
        }

    private:
        MSHREntryType type;
        std::list<Addr> *evictPtrs; // Specific to Evict type
        MemEventBase* event;        // Specific to Event type
        SimTime_t time;
        bool needEvict;
        bool inProgress;
        bool profiled;
        bool downgrade;             // Specific to Writeback type
};

struct MSHRRegister {
    MSHRRegister() : acksNeeded(0), dataDirty(false) { }
    list<MSHREntry> entries;
    uint32_t acksNeeded;
    vector<uint8_t> dataBuffer;
    bool dataDirty;
};

typedef map<Addr, MSHRRegister> MSHRBlock;

/**
 *  Implements an MSHR with entries of type mshrEntry
 */
class MSHR {
public:
        
    // used externally
    MSHR(Output* dbg, int maxSize, string cacheName, std::set<Addr> debugAddr);
    
    int getMaxSize();
    int getSize();
    unsigned int getSize(Addr addr);
    bool exists(Addr addr);

    // Accessors for first event since that's most common
    MSHREntry getFront(Addr addr);
    void removeFront(Addr addr);

    MSHREntryType getFrontType(Addr addr);

    MemEventBase* getFrontEvent(Addr addr);
    std::list<Addr>* getEvictPointers(Addr addr);
    bool removeEvictPointer(Addr addr, Addr ptrAddr);
    
    // Special move accessor
    void moveEntryToFront(Addr addr, unsigned int index);

    // Generic accessors
    MSHREntry getEntry(Addr addr, size_t index);
    void removeEntry(Addr addr, size_t index);

    MSHREntryType getEntryType(Addr addr, size_t index);
    MemEventBase* getEntryEvent(Addr addr, size_t index);
   
    MemEventBase* swapFrontEvent(Addr addr, MemEventBase* event);
    
    bool pendingWriteback(Addr addr);
    bool pendingWritebackIsDowngrade(Addr addr);

    int insertEvent(Addr addr, MemEventBase* event, int position, bool fwdRequest, bool stallEvict);
    bool insertWriteback(Addr addr, bool downgrade);
    bool insertEviction(Addr evictAddr, Addr newAddr);

    void setInProgress(Addr addr, bool value = true);
    bool getInProgress(Addr addr);

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

    void setData(Addr addr, vector<uint8_t>& data, bool dirty = false);
    void clearData(Addr addr);
    vector<uint8_t>& getData(Addr addr); 
    bool hasData(Addr addr);
    bool getDataDirty(Addr addr);
    void setDataDirty(Addr addr, bool dirty);

    void printStatus(Output &out);

private:

    void printDebug(uint32_t level, std::string action, Addr addr, std::string reason);

    MSHRBlock mshr_;
    Output* d_;
    Output* d2_;
    int size_;
    int maxSize_;
    int prefetchCount_;
    string ownerName_;
    std::set<Addr> DEBUG_ADDR;
};
}}
#endif
