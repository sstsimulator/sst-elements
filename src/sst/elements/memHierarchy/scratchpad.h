// Copyright 2009-2017 Sandia Corporation. Under the terms
// of Contract DE-NA0003525 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2017, Sandia Corporation
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef _SCRATCHPAD_H
#define _SCRATCHPAD_H


#include <sst/core/event.h>
#include <sst/core/sst_types.h>
#include <sst/core/component.h>
#include <sst/core/link.h>
#include <sst/core/output.h>
#include <map>
#include <list>

#include "sst/elements/memHierarchy/membackend/backing.h"
#include "moveEvent.h"
#include "memEvent.h"
#include "memNIC.h"

namespace SST {
namespace MemHierarchy {

class ScratchBackendConvertor;

class Scratchpad : public SST::Component {
public:
    Scratchpad(ComponentId_t id, Params &params);
    void init(unsigned int);

    // Output for debug info
    Output dbg;

    // Handler for backend responses
    void handleScratchResponse(SST::Event::id_type id);

private:

    ~Scratchpad() {}

    // Parameters - scratchpad
    uint64_t scratchSize_;      // Size of the total scratchpad in bytes - any address above this is assumed to address remote memory
    uint64_t scratchLineSize_;  // Size of each line in the scratchpad in bytes
    bool doNotBack_;            // For very large scratchpads where data doesn't matter
    
    // Parameters - memory
    uint64_t remoteAddrOffset_;   // Offset for remote addresses, defaults to scratchSize (i.e., CPU addr scratchSize = mem addr 0)
    uint64_t remoteLineSize_;

    // Backend
    ScratchBackendConvertor * scratch_;

    // Backing store for scratchpad
    Backend::Backing * backing_;

    // Local variables
    uint64_t timestamp_;

    // Event handling
    bool clock(SST::Cycle_t cycle);

    void processIncomingCPUEvent(SST::Event* event);
    void processIncomingRemoteEvent(SST::Event* event);

    void handleRead(MemEventBase * event);
    void handleWrite(MemEventBase * event);

    void handleScratchRead(MemEvent * event);
    void handleScratchWrite(MemEvent * event);
    void handleRemoteRead(MemEvent * event);
    void handleRemoteWrite(MemEvent * event);
    void handleScratchGet(MemEventBase * event);
    void handleScratchPut(MemEventBase * event);
    void handleAckInv(MemEventBase * event);
    void handleFetchResp(MemEventBase * event);

    void handleRemoteGetResponse(MemEvent * response, SST::Event::id_type id);
    void handleRemoteReadResponse(MemEvent * response, SST::Event::id_type id);

    // Helper methods
    void updateMSHR(Addr baseAddr);

    std::vector<uint8_t> doScratchRead(MemEvent * read);
    void doScratchWrite(MemEvent * write);
    void sendResponse(MemEventBase * event);

    bool startGet(Addr baseAddr, MoveEvent * get);
    bool startPut(Addr baseAddr, MoveEvent * put);

    void updateGet(SST::Event::id_type id);
    void updatePut(SST::Event::id_type id);
    void finishRequest(SST::Event::id_type id);

    uint32_t deriveSize(Addr addr, Addr baseAddr, Addr requestAddr, uint32_t requestSize);

    // Links
    SST::Link* linkUp_;     // To cache/cpu
    SST::Link* linkDown_;   // To memory
    MemNIC* linkNet_;       // To memory over network
    
    class OutstandingEvent {
        public:
            MemEventBase * request;     // Request (outstanding event)
            MemEventBase * response;    // Sent to processor when complete
            MemEvent * remoteWrite;     // For Put requests, collect scratch read responses here
            uint32_t count;             // Number of lines we are waiting on - when 0, the request is complete
                                        // i.e., for a read or write, just 1, for a get or put, the size/lineSize
            
            OutstandingEvent(MemEventBase * request, MemEventBase * response) : request(request), response(response), remoteWrite(nullptr), count(0) { }
            OutstandingEvent(MemEventBase * request, MemEventBase * response, MemEvent * write) : request(request), response(response), remoteWrite(write), count(0) { } 

            uint32_t decrementCount() { return count--; }
            void incrementCount() { count++; }
    };
    // MSHR entry
    // MSHR consists of a base address and queue of these
    class MSHREntry {
        public:
            SST::Event::id_type id; // Event id that we are mapped to
            MemEvent * scratch;     // If we are waiting to send an event to scratch, the event to send
            Command cmd;            // Event command
            bool started;           // Whether we've started processing this event
            bool needDataAndAck;    // Waiting for both data and ack or just one of them?

            // For GetS/GetX
            MSHREntry(SST::Event::id_type id, Command cmd, bool started) : id(id), scratch(nullptr), cmd(cmd), started(started), needDataAndAck(false) { }
            
            // For events not yet started that need to send to scratch
            MSHREntry(SST::Event::id_type id, Command cmd, MemEvent * scratch) : id (id), scratch(scratch), cmd(cmd), started(false), needDataAndAck(false) { }

            // For events that may need data and ack and are started
            MSHREntry(SST::Event::id_type id, Command cmd, bool started, bool needDataAndAck) : id(id), scratch(nullptr), cmd(cmd), started(started), needDataAndAck(needDataAndAck) { }

            // For debug
            std::string getString() {
                std::string cmdStr(CommandString[(int)cmd]);
                ostringstream str;
                str <<"ID: <" << id.first << "," << id.second << ">";
                str << " Cmd: " << cmdStr;
                str << " Start: " << (started ? "T" : "F");
                str << " Ack&Data: " << (needDataAndAck ? "T" : "F");
                return str.str();
            }
    };
    

    std::map<SST::Event::id_type,SST::Event::id_type> responseIDMap_;   // Map a forwarded request ID to a original request ID
    std::map<SST::Event::id_type,Addr> responseIDAddrMap_;              // Map an outstanding scratch request ID to the request's baseAddr
    std::map<SST::Event::id_type,OutstandingEvent> outstandingEventList_; // List of all outstanding events
    std::map<Addr,std::list<MSHREntry> > mshr_; // MSHR for scratch accesses


    // Outgoing message queues - map send timestamp to event
    std::multimap<uint64_t, MemEventBase*> procMsgQueue_;
    std::multimap<uint64_t, MemEvent*> memMsgQueue_;
    
    // Throughput limits
    uint32_t responsesPerCycle_;
    
    // Caching information
    bool caching_;  // Whether or not caching is possible
    std::vector<bool> cacheStatus_; // One entry per scratchpad line, whether line may be cached
    std::map<SST::Event::id_type, uint64_t> cacheCounters_; // Map of a Get or Put ID to the number of cache acks/data responses we are waiting for

    // Statistics
    Statistic<uint64_t>* stat_ScratchReadReceived;
    Statistic<uint64_t>* stat_ScratchWriteReceived;
    Statistic<uint64_t>* stat_RemoteReadReceived;
    Statistic<uint64_t>* stat_RemoteWriteReceived;
    Statistic<uint64_t>* stat_ScratchGetReceived;
    Statistic<uint64_t>* stat_ScratchPutReceived;
    Statistic<uint64_t>* stat_ScratchReadIssued;
    Statistic<uint64_t>* stat_ScratchWriteIssued;
};

}}

#endif /* _SCRATCHPAD_H */
