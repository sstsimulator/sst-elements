// Copyright 2013-2017 Sandia Corporation. Under the terms
// of Contract DE-NA0003525 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2013-2017, Sandia Corporation
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
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
#include <sst/core/timeConverter.h>
#include <sst/core/output.h>
#include "memLink.h"
#include "memEvent.h"
#include "util.h"
#include "mshr.h"

using namespace std;

namespace SST { namespace MemHierarchy {

class MemNIC;

class DirectoryController : public Component {

    Output dbg;
    std::set<Addr> DEBUG_ADDR;
    struct DirEntry;

    /* Total number of cache blocks we are responsible for */
    /* ie, sum of all caches we talk to */
    uint32_t    entrySize;
    uint32_t    numTargets;
    uint32_t    targetCount;
    uint32_t    cacheLineSize;

    /* Range of addresses supported by this directory */
    Addr        addrRangeStart;
    Addr        addrRangeEnd;
    Addr        interleaveSize;
    Addr        interleaveStep;
    Addr        memOffset; // Stack addresses if multiple DCs handle the same memory
    
    CoherenceProtocol protocol;
    bool waitWBAck;

    /* MSHRs */
    MSHR*       mshr;
    
    /* Directory cache */
    size_t      entryCacheMaxSize;
    size_t      entryCacheSize;
    
    /* Timestamp & latencies */
    uint64_t    timestamp;
    uint64_t    accessLatency;
    uint64_t    mshrLatency;
    int         maxRequestsPerCycle;

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
    Statistic<uint64_t> * stat_GetSXReqReceived;
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
    Statistic<uint64_t> * stat_NoncacheReceived;
    Statistic<uint64_t> * stat_CustomReceived;
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
    std::unordered_map<Addr,DirEntry*>      directory;
    std::map<std::string,uint32_t>          node_lookup;
    std::vector<std::string>                nodeid_to_name;
    
    /* Queue of packets to work on */
    std::list<MemEvent*>                    workQueue;
    std::map<MemEvent::id_type, Addr>       memReqs;
    std::map<MemEvent::id_type, Addr>       dirEntryMiss;
    std::map<MemEvent::id_type, std::string> noncacheMemReqs;

    /* Network connections */
    MemLink*    memLink;
    MemNIC*     network;
    string      memoryName; // if connected to mem via network, this should be the name of the memory we own - param is memory_name
    
    std::multimap<uint64_t,MemEventBase*>   netMsgQueue;
    std::multimap<uint64_t,MemEventBase*>   memMsgQueue;
    
    /** Find directory entry by base address */
    DirEntry* getDirEntry(Addr target);
	
    /** Handle incoming GetS request */
    void handleGetS(MemEvent * ev, bool replay);
    
    /** Handle incoming GetX/GetSX request */
    void handleGetX(MemEvent * ev, bool replay);

    /** Handle incoming PutS */
    void handlePutS(MemEvent * ev);

    /** Handle incoming PutE */
    void handlePutE(MemEvent * ev);

    /** Handle incoming PutM */
    void handlePutM(MemEvent * ev);

    /** Handle incoming FetchResp (or PutM or FlushLine that is treated as FetchResp) */
    void handleFetchResp(MemEvent * ev, bool keepEvent);

    /** Handle incoming FetchXResp */
    void handleFetchXResp(MemEvent * ev, bool keepEvent);

    /** Handle incoming AckInv */
    void handleAckInv(MemEvent * ev);
    
    /** Handle incoming NACK */
    void handleNACK(MemEvent * ev);

    /** Handle incoming FlushLine */
    void handleFlushLine(MemEvent * ev);
    
    /** Handle incoming FlushLineInv */
    void handleFlushLineInv(MemEvent * ev);

    /** Handle noncacheable request */
    void handleNoncacheableRequest(MemEventBase * ev);

    /** Handle noncacheable response */
    void handleNoncacheableResponse(MemEventBase * ev);

    /** Identify and issue invalidates to sharers */
    void issueInvalidates(MemEvent * ev, DirEntry * entry, Command cmd);
    
    void issueFetch(MemEvent * ev, DirEntry * entry, Command cmd);

    /** Send invalidate to a specific sharer */
    void sendInvalidate(int target, MemEvent * reqEv, DirEntry* entry, Command cmd);
    
    /** Send AckPut to a replacing cache */
    void sendAckPut(MemEvent * event);

    /** Forward flush request to memory */
    void forwardFlushRequest(MemEvent * event);

    /** Send data request to memory */
    void issueMemoryRequest(MemEvent * ev, DirEntry * entry);

    /** Handle incoming flush response from memory */
    void handleFlushLineResponse(MemEvent * event);

    /** Handle incoming data response from memory */
    void handleDataResponse(MemEvent * ev);

    /** Handle incoming dir entry response from memory */
    void handleDirEntryMemoryResponse(MemEvent * ev);

    /** Handle backwards invalidation (shootdown) from memory */
    void handleBackInv(MemEvent * ev);

    /** Request dir entry from memory */
    void getDirEntryFromMemory(DirEntry * entry);

    /** NACK incoming request because there are no available MSHRs */
    void mshrNACKRequest(MemEvent * event, bool toMem = false);

    /** Find link id by name.  Create map entry if not found */
    uint32_t node_id(const std::string &name);
    
    /** Find link id by name. */
    uint32_t node_name_to_id(const std::string &name);

    /** Determines if directory controller has exceeded the max number of entries.  If so it 'deletes' entry (not really) 
        and sends the entry to main memory.  In reality the entry is always kept in DirController but this writeback 
        of entries is done to get performance stimation */
    void updateCache(DirEntry *entry);

    /** Profile request and delete it */
    void postRequestProcessing(MemEvent * ev, DirEntry * entry, bool stable);

    /** Replay any waiting events */
    void replayWaitingEvents(Addr addr);

    /** Send entry to main memory. No payload is actually sent for reasons described in 'updateCacheEntry'. */
    void sendEntryToMemory(DirEntry *entry);
	
    /** Sends MemEvent to a target */
    void sendEventToCaches(MemEventBase *ev, uint64_t deliveryTime);
    
    /** Sends MemEventBase to a memory */
    inline void sendEventToMem(MemEventBase *ev);

    /** Writes data packet to Memory. Returns the MemEvent ID of the data written to memory */
    MemEvent::id_type writebackData(MemEvent *data_event, Command wbCmd);

    /** Determines if request is valid in terms of address ranges */
    bool isRequestAddressValid(Addr addr);
    
    /** Print directory controller status */
    const char* printDirectoryEntryStatus(Addr addr);

    /** Turn clock back on */
    void turnClockOn();

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
        State               state;          // state
        MemEvent::id_type   lastRequest;    // ID of message we're wanting a response to  - used to track whether a NACK needs to be retried
        std::list<DirEntry*>::iterator cacheIter;
	std::vector<bool>   sharers;        // set of sharers for block
        int                 owner;          // owner of block
        Output * dbg;
	
        DirEntry(Addr bsAddr, uint32_t bitlength, Output * d){
            clearEntry();
            baseAddr     = bsAddr;
            sharers.resize(bitlength);
            dbg          = d;
            state        = I;
            cached       = false;
        }

        void clearEntry(){
            setToSteadyState();
            waitingAcks  = 0;
            cached       = true;
            baseAddr     = 0;
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
        
        void addSharer(int id){
            sharers[id]= true;
        }
        
        bool isSharer(int id) {
            return sharers[id];
        }

        void removeSharer(int id){
            if (!sharers[id]) {
                dbg->fatal(CALL_INFO,-1,"Removing a sharer which does not exist\n");
            }
            sharers[id]= false;
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
    void processPacket(MemEvent *ev, bool replay);
	
    /** Clock handler */
    bool clock(SST::Cycle_t cycle);

};

}
}

#endif /* _MEMHIERARCHY_DIRCONTROLLER_H_ */
