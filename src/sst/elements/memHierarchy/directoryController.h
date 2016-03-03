// Copyright 2013-2015 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2013-2015, Sandia Corporation
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
    bool DEBUG_ALL;
    Addr DEBUG_ADDR;
    struct DirEntry;

    /* Total number of cache blocks we are responsible for */
    /* ie, sum of all caches we talk to */
    uint32_t    entrySize;
    uint32_t    numTargets;
    uint32_t    blocksize;
    uint32_t    targetCount;
    uint32_t    cacheLineSize;

    /* Range of addresses supported by this directory */
    Addr        addrRangeStart;
    Addr        addrRangeEnd;
    Addr        interleaveSize;
    Addr        interleaveStep;
    
    string      protocol;
    
    /* MSHRs */
    MSHR*       mshr;
    
    /* Directory cache */
    size_t      entryCacheMaxSize;
    size_t      entryCacheSize;
    
    /* Timestamp & latencies */
    uint64_t    timestamp;
    uint64_t    accessLatency;
    uint64_t    mshrLatency;

    /* Turn clocks off when idle */
    bool        clockOn;
    Clock::Handler<DirectoryController>*  clockHandler;
    TimeConverter* defaultTimeBase;
    SimTime_t   lastActiveClockCycle;

    /* Statistics counters for profiling DC */
    Statistic<uint64_t> * stat_replacementRequestLatency;   // totalReplProcessTime
    Statistic<uint64_t> * stat_getRequestLatency;           // totalGetReqProcessTime;
    Statistic<uint64_t> * stat_cacheHits;                   // numCacheHits;
    Statistic<uint64_t> * stat_mshrHits;                    // mshrHits;
    // Received events - from caches
    Statistic<uint64_t> * stat_GetXReqReceived;
    Statistic<uint64_t> * stat_GetSExReqReceived;
    Statistic<uint64_t> * stat_GetSReqReceived;
    Statistic<uint64_t> * stat_PutMReqReceived;
    Statistic<uint64_t> * stat_PutEReqReceived;
    Statistic<uint64_t> * stat_PutSReqReceived;
    Statistic<uint64_t> * stat_NACKRespReceived;
    Statistic<uint64_t> * stat_FetchRespReceived;
    Statistic<uint64_t> * stat_FetchXRespReceived;
    Statistic<uint64_t> * stat_PutMRespReceived;
    Statistic<uint64_t> * stat_PutERespReceived;
    Statistic<uint64_t> * stat_PutSRespReceived;
    // Sent events - to mem
    Statistic<uint64_t> * stat_dataReads;
    Statistic<uint64_t> * stat_dataWrites;
    Statistic<uint64_t> * stat_dirEntryReads;
    Statistic<uint64_t> * stat_dirEntryWrites;
    // Sent events - to caches
    Statistic<uint64_t> * stat_NACKRespSent;
    Statistic<uint64_t> * stat_InvSent;
    Statistic<uint64_t> * stat_FetchInvSent;
    Statistic<uint64_t> * stat_FetchInvXSent;
    Statistic<uint64_t> * stat_GetSRespSent;
    Statistic<uint64_t> * stat_GetXRespSent;
    Statistic<uint64_t> * stat_MSHROccupancy;

    /* Directory structures */
    std::list<DirEntry*>                    entryCache;
    std::map<Addr, DirEntry*>               directory;
    std::map<std::string, uint32_t>         node_lookup;
    std::vector<std::string>                nodeid_to_name;
    
    /* Queue of packets to work on */
    std::list<MemEvent*>                    workQueue;
    std::map<MemEvent::id_type, Addr>       memReqs;
    std::map<MemEvent::id_type, Addr>       dirEntryMiss;
    std::map<MemEvent::id_type, pair<Addr,Addr> > noncacheMemReqs;

    /* Network connections */
    SST::Link*  memLink;
    MemNIC*     network;
    string      memoryName; // if connected to mem via network, this should be the name of the memory we own - param is memory_name
    
    std::multimap<uint64_t,MemEvent*>   netMsgQueue;
    std::multimap<uint64_t,MemEvent*>   memMsgQueue;
    
    /** Find directory entry by base address */
    DirEntry* getDirEntry(Addr target);
	
    /** Create directory entrye */
    DirEntry* createDirEntry(Addr baseTarget, Addr target, uint32_t reqSize);

    /** Handle incoming GetS request */
    void handleGetS(MemEvent * ev);
    
    /** Handle incoming GetX/GetSEx request */
    void handleGetX(MemEvent * ev);

    /** Handle incoming PutS */
    void handlePutS(MemEvent * ev);

    /** Handle incoming PutE */
    void handlePutE(MemEvent * ev);

    /** Handle incoming PutM */
    void handlePutM(MemEvent * ev);

    /** Handle incoming FetchResp (or PutM that is treated as FetchResp) */
    void handleFetchResp(MemEvent * ev);

    /** Handle incoming FetchXResp */
    void handleFetchXResp(MemEvent * ev);

    /** Handle incoming AckInv */
    void handleAckInv(MemEvent * ev);
    
    /** Handle incoming NACK */
    void handleNACK(MemEvent * ev);

    /** Identify and issue invalidates to sharers */
    void issueInvalidates(MemEvent * ev, DirEntry * entry);
    
    void issueFetch(MemEvent * ev, DirEntry * entry, Command cmd);

    /** Send invalidate to a specific sharer */
    void sendInvalidate(int target, MemEvent * reqEv, DirEntry* entry);
    
    /** Send AckPut to a replacing cache */
    void sendAckPut(MemEvent * event);

    /** Send data request to memory */
    void issueMemoryRequest(MemEvent * ev, DirEntry * entry);

    /** Handle incoming data response from memory */
    void handleDataResponse(MemEvent * ev);

    /** Handle incoming dir entry response from memory */
    void handleDirEntryMemoryResponse(MemEvent * ev);

    /** Request dir entry from memory */
    void getDirEntryFromMemory(DirEntry * entry);

    /** NACK incoming request because there are no available MSHRs */
    void mshrNACKRequest(MemEvent * event);

    /** Find link id by name.  Create map entry if not found */
    uint32_t node_id(const std::string &name);
    
    /** Find link id by name. */
    uint32_t node_name_to_id(const std::string &name);

    /** Determines if directory controller has exceeded the max number of entries.  If so it 'deletes' entry (not really) 
        and sends the entry to main memory.  In reality the entry is always kept in DirController but this writeback 
        of entries is done to get performance stimation */
    void updateCache(DirEntry *entry);

    /** Profile request and delete it */
    void postRequestProcessing(MemEvent * ev, DirEntry * entry);

    /** Replay any waiting events */
    void replayWaitingEvents(Addr addr);

    /** Send entry to main memory. No payload is actually sent for reasons described in 'updateCacheEntry'. */
    void sendEntryToMemory(DirEntry *entry);
	
    /** Sends MemEvent to a target */
    void sendEventToCaches(MemEvent *ev, uint64_t deliveryTime);

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

    /** Profile received requests */
    inline void profileRequestRecv(MemEvent * event, DirEntry * entry);
    /** Profile sent requests */
    inline void profileRequestSent(MemEvent * event);
    /** Profile received responses */
    inline void profileResponseRecv(MemEvent * event);
    /** Profile sent responses */
    inline void profileResponseSent(MemEvent * event);

    /** */
    typedef void(DirectoryController::*ProcessFunc)(DirEntry *entry, MemEvent *new_ev);

    /** Internal struct to keep track of directory requests to main memory */
    struct DirEntry {
        static const        MemEvent::id_type NO_LAST_REQUEST;

	uint32_t            waitingAcks;    // Number of acks we are waiting for
	bool                cached;         // whether block is cached or not
        Addr                baseAddr;       // block address
        Addr                addr;
        State               state;          // state
        MemEvent::id_type   lastRequest;    // ID of message we're wanting a response to  - used to track whether a NACK needs to be retried
        std::list<DirEntry*>::iterator cacheIter;
	std::vector<bool>   sharers;        // set of sharers for block
        int                 owner;          // owner of block
        Output * dbg;
	
        DirEntry(Addr _baseAddress, Addr _address, uint32_t _bitlength, Output * d){
            clearEntry();
            baseAddr     = _baseAddress;
            addr         = _address;
            sharers.resize(_bitlength);
            dbg          = d;
            state        = I;
            cached       = false;
        }

        void clearEntry(){
            setToSteadyState();
            waitingAcks  = 0;
            cached       = true;
            baseAddr     = 0;
            addr         = 0;
            clearSharers();
            owner        = -1;
        }

        bool isCached() {
            return cached;
        }
        
        void setCached(bool cache) {
            cached = cache;
        }

        Addr getBaseAddr() {
            return baseAddr;
        }

        void setToSteadyState(){
            lastRequest = DirEntry::NO_LAST_REQUEST;
        }
        
        uint32_t getSharerCount(void) {
            uint32_t count = 0;
            for (uint32_t i = 0; i < sharers.size(); i++) {
                if (sharers[i]) count++;
            }
            return count;
        }

        void clearSharers(void){
            for (uint32_t i = 0; i < sharers.size(); i++)
                sharers[i] = false;
        }
        
        void addSharer(int _id){
            sharers[_id]= true;
        }
        
        bool isSharer(int id) {
            return sharers[id];
        }

        void removeSharer(int _id){
            if (!sharers[_id]) {
                dbg->fatal(CALL_INFO,-1,"Removing a sharer which does not exist\n");
            }
            sharers[_id]= false;
        }
        
        int getOwner(void) {
            return owner;
        }
        
        void setOwner(int id) {
            owner = id;
        }

        void clearOwner() {
            owner = -1;
        }

        void incrementWaitingAcks() {
            waitingAcks++;
        }

        void decrementWaitingAcks() {
            waitingAcks--;
        }
            
        uint32_t getWaitingAcks() {
            return waitingAcks;
        }

        void setState(State nState) {
            state = nState;
        }

        State getState() {
            return state;
        }
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
    void processPacket(MemEvent *ev);
	
    /** Clock handler */
    bool clock(SST::Cycle_t cycle);

};

}
}

#endif /* _MEMHIERARCHY_DIRCONTROLLER_H_ */
