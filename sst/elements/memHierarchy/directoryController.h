// Copyright 2013-2014 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2013-2014, Sandia Corporation
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

/*
 * File:   directoryController.h
 * Author: Branden Moore / Caesar De la Paz III
 * Email:  bjmoor@sandia.gov / caesar.sst@gmail.com
 */

#ifndef _MEMHIERARCHY_DIRCONTROLLER_H_
#define _MEMHIERARCHY_DIRCONTROLLER_H_

#include <map>
#include <set>
#include <list>
#include <vector>

#include <sst/core/event.h>
#include <sst/core/sst_types.h>
#include <sst/core/component.h>
#include <sst/core/link.h>
#include <sst/core/timeConverter.h>
#include <sst/core/output.h>
#include "memEvent.h"
#include "util.h"
#include "mshr.h"

using namespace std;

namespace SST { namespace MemHierarchy {

class MemNIC;

class DirectoryController : public Component {

    Output dbg;
    struct DirEntry;
    
    /* Total number of cache blocks we are responsible for */
    /* ie, sum of all caches we talk to */
    uint32_t    entrySize;
    uint32_t    numTargets;
    uint32_t    blocksize;
    uint32_t    targetCount;
    uint32_t    cacheLineSize;

    /* Range of addresses supported by this directory */
    uint64_t    lookupBaseAddr;
    Addr        addrRangeStart;
    Addr        addrRangeEnd;
    Addr        interleaveSize;
    Addr        interleaveStep;
    
    string      protocol;
    
    /* MSHRs */
    uint64_t    mshrLatency;
    MSHR*       mshr;
    
    /* Directory cache */
    size_t      entryCacheMaxSize;
    size_t      entryCacheSize;

    /* Statistics counters */
    uint64_t    numReqsProcessed;
    uint64_t    totalReqProcessTime;
    uint64_t    numCacheHits;
    uint64_t    dataReads;
    uint64_t    dataWrites;
    uint64_t    dirEntryReads;
    uint64_t    dirEntryWrites;
    
    uint64_t    GetXReqReceived;
    uint64_t    GetSExReqReceived;
    uint64_t    GetSReqReceived;
    
    uint64_t    PutMReqReceived;
    uint64_t    PutEReqReceived;
    uint64_t    PutSReqReceived;
    
    uint64_t    mshrHits;

    /* Directory structures */
    std::list<DirEntry*>                    entryCache;
    std::map<Addr, DirEntry*>               directory;
    std::map<std::string, uint32_t>         node_lookup;
    std::vector<std::string>                nodeid_to_name;
    
    /* Queue of packets to work on */
    std::list<MemEvent*>                    workQueue;
    std::map<MemEvent::id_type, Addr>       memReqs;
    std::map<MemEvent::id_type, Addr>       dirEntryMiss;

    Output::output_location_t               printStatsLoc;
    
    /* Network connections */
    SST::Link*  memLink;
    MemNIC*     network;
    
    
    /** Find directory entry by base address */
    DirEntry* getDirEntry(Addr target);
	
    /** Create directory entrye */
    DirEntry* createDirEntry(Addr baseTarget, Addr target, uint32_t reqSize);

    /** Function processes an incoming request that has the same address as a request that is already in progress */
    pair<bool, bool> handleEntryInProgress(MemEvent *ev, DirEntry *entry, Command cmd);
    
    /** Send invalidate to a sharer */
    void sendInvalidate(int target, DirEntry* entry);

    /** Function gets called when the meta data (flags, state) for a particular request is in the directory controller*/
    void handleDataRequest(DirEntry* entry, MemEvent *new_ev);
	
    /** Fetch response received.  Respond to the event that cause the Fetch in the first place */
    void finishFetch(DirEntry* entry, MemEvent *new_ev);
	
    /** Send response to a previously received request */
    void sendResponse(DirEntry* entry, MemEvent *new_ev);
	
    /** Function changes state of the cache line to indicate the new exclusive sharer */
    void getExclusiveDataForRequest(DirEntry* entry, MemEvent *new_ev);

    /** Handles recieved PutM requests */
    void handlePutM(DirEntry *entry, MemEvent *ev);
	
    /** Handles recieved PutS requests */
    void handlePutS(MemEvent *ev);

    /** Retry original request upon receiving a NACK */
    void processIncomingNACK(MemEvent* _origReqEvent, MemEvent* _nackEvent);

    /** NACK incoming request because there are no available MSHRs */
    void mshrNACKRequest(MemEvent * event);

    /** Advances or transitions an entry to the 'next state' by calling the handler that was previously assigned */
    void advanceEntry(DirEntry *entry, MemEvent *ev = NULL);

    /** Find link id by name.  Create map entry if not found */
    uint32_t node_id(const std::string &name);
    
    /** Find link id by name. */
    uint32_t node_name_to_id(const std::string &name);

    /** Determines if directory controller has exceeded the max number of entries.  If so it 'deletes' entry (not really) 
        and sends the entry to main memory.  In reality the entry is always kept in DirController but this writeback 
        of entries is done to get performance stimation */
    void updateCacheEntry(DirEntry *entry);
	
    /** Request directory entry from main memory.  Directory entrys are always kept in this component but this 
        function is executed to get actual latency (dummy events are sent only for performance stimation) */
    void requestDirEntryFromMemory(DirEntry *entry);
    
    
    /** Requests data from Memory */
    void requestDataFromMemory(DirEntry *entry);
	
    /** Forwards data from Memory for noncacheable GetX requests */
    void forwardDataToMemory(DirEntry *entry);
    
    /** Write updated entry to memory */
    void updateEntryToMemory(DirEntry *entry);
    
    /** Send entry to main memory. No payload is actually sent for reasons described in 'updateCacheEntry'. */
    void sendEntryToMemory(DirEntry *entry);
	
    /** Called to clear "active items" from an entry */
    void resetEntry(DirEntry *entry);

    /** Sends MemEvent to a target */
    void sendResponse(MemEvent *ev);

    /** Writes data packet to Memory. Returns the MemEvent ID of the data written to memory */
    MemEvent::id_type writebackData(MemEvent *data_event);

    /** Determines if request is valid in terms of address ranges */
    bool isRequestAddressValid(MemEvent *ev);
    
    /** Convert local address of the main memory section this directory controller owns to global address */
    Addr convertAddressFromLocalAddress(Addr addr);
    
    /** Convert global address to local address of the main memory section this directory controller owns */
    Addr convertAddressToLocalAddress(Addr addr);

    /** Print directory controller status */
    const char* printDirectoryEntryStatus(Addr addr);

    /** */
    typedef void(DirectoryController::*ProcessFunc)(DirEntry *entry, MemEvent *new_ev);

    /** Internal struct to keep track of directory requests to main memory */
    struct DirEntry {
        static const        MemEvent::id_type NO_LAST_REQUEST;

        /* These items are bookkeeping for in-progress commands */
	ProcessFunc         nextFunc;
        std::string         waitingOn; // waiting to hear from this source
        Command             nextCommand;  // Command which we're waiting for
	uint32_t            waitingAcks;
        uint32_t            reqSize;
        bool                inController; // Whether this is present in the controller, or needs to be fetched
	bool                dirty;
        Addr                baseAddr;
        Addr                addr;
        MemEvent::id_type   lastRequest;  // ID of message we're wanting a response to
	MemEvent            *activeReq;
        std::list<DirEntry*>::iterator cacheIter;
	std::vector<bool>   sharers;
	
        DirEntry(Addr _baseAddress, Addr _address, uint32_t _reqSize, uint32_t _bitlength){
            clearEntry();
            baseAddr     = _baseAddress;
            addr         = _address;
            reqSize      = _reqSize;
            sharers.resize(_bitlength);
            activeReq    = NULL;

        }

        void clearEntry(){
            setToSteadyState();
            waitingAcks  = 0;
            inController = true;
            dirty        = false;
            baseAddr     = 0;
            addr         = 0;
            reqSize      = 0;
            sharers.clear();
            clearSharers();
        }
        
        void setToSteadyState(){
            activeReq   = NULL;
            nextFunc    = NULL;
            nextCommand = NULLCMD;
            lastRequest = DirEntry::NO_LAST_REQUEST;
            waitingOn   = "N/A";
        }
        
	uint32_t countRefs(void){
	    uint32_t count = 0;
            for (uint32_t i = 0; i < sharers.size(); i++)
                if (sharers[i]) count++;
            
	    return count;
	}

        void clearSharers(void){
            for (uint32_t i = 0; i < sharers.size(); i++)
                sharers[i] = false;
        }
        
        
        void setDirty(int id){
            dirty       = true;
            sharers[id] = true;
            assert(countRefs() == 1);
        }
        
        bool isDirty(){
            return dirty ? true : false;
        }
        
        void clearDirty(int id){
            dirty       = false;
            sharers[id] = false;
            assert(countRefs() == 0);
        }
        
        void addSharer(int _id){
            sharers[_id]= true;
            assert(!dirty);
        }
        
        void removeSharer(int _id){
            assert(sharers[_id]);
            assert(!dirty);
            sharers[_id]= false;
        }
        
        uint32_t findOwner(void){
            assert(dirty);
            assert(countRefs() == 1);
            for (uint32_t i = 0; i < sharers.size(); i++) {
                if (sharers[i]) return i;
            }
            assert(0);   /* Should never be here */
            return 0;
        }

	bool inProgress(void) { return activeReq != NULL; }
    };

public:
    DirectoryController(ComponentId_t id, Params &params);
    ~DirectoryController();
    void setup(void);
    void init(unsigned int phase);
    void finish(void);
    
    /** Print status of entries/work queue */
    void printStatus(Output &out);
    
    /** Function handles responses from Main Memory.  
        "Advances" the state of the directory entry */
    void handleMemoryResponse(SST::Event *event);

    /** Event received from higher level caches.  
        Insert to work queue so that it is process in the upcoming clock tick */
    void handlePacket(SST::Event *event);
	
    /** Handler that gets called by clock tick.  
        Function redirects request according to their type. */
    bool processPacket(MemEvent *ev);
	
    /** Clock handler */
    bool clock(SST::Cycle_t cycle);

};

}
}

#endif /* _MEMHIERARCHY_DIRCONTROLLER_H_ */
