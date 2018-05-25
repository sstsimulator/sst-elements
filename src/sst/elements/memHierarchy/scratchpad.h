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

#ifndef _SCRATCHPAD_H
#define _SCRATCHPAD_H


#include <sst/core/event.h>
#include <sst/core/sst_types.h>
#include <sst/core/component.h>
#include <sst/core/elementinfo.h>
#include <sst/core/link.h>
#include <sst/core/output.h>
#include <map>
#include <list>

#include "sst/elements/memHierarchy/membackend/backing.h"
#include "sst/elements/memHierarchy/moveEvent.h"
#include "sst/elements/memHierarchy/memEvent.h"
#include "sst/elements/memHierarchy/memLinkBase.h"

namespace SST {
namespace MemHierarchy {

class ScratchBackendConvertor;

class Scratchpad : public SST::Component {
public:
/* Element Library Info */
    SST_ELI_REGISTER_COMPONENT(Scratchpad, "memHierarchy", "Scratchpad", SST_ELI_ELEMENT_VERSION(1,0,0),
            "Scratchpad memory", COMPONENT_CATEGORY_MEMORY)

    SST_ELI_DOCUMENT_PARAMS(
            {"clock",               "(string) Clock frequency or period with units (Hz or s; SI units OK).", NULL},
            {"size",                "(string) Size of the scratchpad in bytes (B), SI units ok", NULL},
            {"scratch_line_size",   "(string) Number of bytes in a scratch line with units. 'size' must be divisible by this number.", "64B"},
            {"memory_line_size",    "(string) Number of bytes in a remote memory line with units. Used to set base addresses for routing.", "64B"},
            {"backing",             "(string) Type of backing store to use. Options: 'none' - no backing store (only use if simulation does not require correct memory values), 'malloc', or 'mmap'", "malloc"},\
            {"backing_size_unit",   "(string) For 'malloc' backing stores, malloc granularity", "1MiB"},\
            {"memory_addr_offset",  "(uint) Amount to offset remote addresses by. Default is 'size' so that remote memory addresses start at 0", "size"},
            {"response_per_cycle",  "(uint) Maximum number of responses to return to processor each cycle. 0 is unlimited", "0"},
            {"backendConvertor",    "(string) Backend convertor to use for the scratchpad", "memHierarchy.scratchpadBackendConvertor"},
            {"debug",               "(uint) Where to print debug output. Options: 0[no output], 1[stdout], 2[stderr], 3[file]", "0"},
            {"debug_level",         "(uint) Debug verbosity level. Between 0 and 10", "0"} )
            
    SST_ELI_DOCUMENT_PORTS(
            {"cpu", "Link to cpu/cache on the cpu side", {"memHierarchy.MemEventBase"}},
            {"memory", "Direct link to a memory or bus", {"memHierarchy.MemEventBase"}},
            {"network", "Network link to memory", {"memHierarchy.MemRtrEvent"}} )
                 
    SST_ELI_DOCUMENT_STATISTICS(
            {"request_received_scratch_read",   "Number of scratchpad reads received from CPU", "count", 1},
            {"request_received_scratch_write",  "Number of scratchpad writes received from CPU", "count", 1},
            {"request_received_remote_read",    "Number of remote memory reads received from CPU", "count", 1},
            {"request_received_remote_write",   "Number of remote memory writes received from CPU", "count", 1},
            {"request_received_scratch_get",    "Number of scratchpad Gets received from CPU (copy from memory to scratch)", "count", 1},
            {"request_received_scratch_put",    "Number of scratchpad Puts received from CPU (copy from scratch to memory)", "count", 1},
            {"request_issued_scratch_read",     "Number of scratchpad reads issued to scratchpad", "count", 1},
            {"request_issued_scratch_write",    "Number of scratchpad writes issued to scratchpad", "count", 1} )

/* Begin class defintion */
    Scratchpad(ComponentId_t id, Params &params);
    void init(unsigned int);
    void setup();

    // Output for debug info
    Output dbg;
    std::set<Addr> DEBUG_ADDR;

    // Output for warnings, etc.
    Output out;

    // Handler for backend responses
    void handleScratchResponse(SST::Event::id_type id);

private:

    ~Scratchpad() {}

    // Parameters - scratchpad
    uint64_t scratchSize_;      // Size of the total scratchpad in bytes - any address above this is assumed to address remote memory
    uint64_t scratchLineSize_;  // Size of each line in the scratchpad in bytes
    
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
    void processIncomingNetworkEvent(SST::Event* event);

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
    void handleNack(MemEventBase * event);

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
    MemLinkBase* linkUp_;     // To cache/cpu
    MemLinkBase* linkDown_;   // To memory
    
    class OutstandingEvent {
        public:
            MemEventBase * request;     // Request (outstanding event)
            MemEventBase * response;    // Sent to processor when complete
            MemEvent * remoteWrite;     // For Put requests, collect scratch read responses here
            uint32_t count;             // Number of lines we are waiting on - when 0, the request is complete
                                        // i.e., for a read or write, just 1, for a get or put, the size/lineSize
            
            OutstandingEvent(MemEventBase * request, MemEventBase * response) : request(request), response(response), remoteWrite(nullptr), count(0) { }
            OutstandingEvent(MemEventBase * request, MemEventBase * response, MemEvent * write) : request(request), response(response), remoteWrite(write), count(0) { } 

            uint32_t decrementCount() { count--; return count; }
            void incrementCount() { count++; }
            uint32_t getCount() { return count; }
    };
    // MSHR entry
    // MSHR consists of a base address and queue of these
    class MSHREntry {
        public:
            SST::Event::id_type id; // Event id that we are mapped to
            MemEvent * scratch;     // If we are waiting to send an event to scratch, the event to send
            Command cmd;            // Event command
            bool needData;          // Waiting on data? (from scratch or remote)
            bool needAck;           // Waiting on ack? (from cache)

            // For events not yet started
            MSHREntry(SST::Event::id_type id, Command cmd, MemEvent * scratch = nullptr) : id(id), scratch(scratch), cmd(cmd), needData(false), needAck(false) { }

            // For Get events which have not yet started but remote read has been sent
            MSHREntry(SST::Event::id_type id, Command cmd, bool needData) : id(id), scratch(nullptr), cmd(cmd), needData(needData), needAck(false) { }

            // For events that have started
            MSHREntry(SST::Event::id_type id, Command cmd, bool needData, bool needAck) : id(id), scratch(nullptr), cmd(cmd), needData(needData), needAck(needAck) { }

            // For debug
            std::string getString() {
                std::string cmdStr(CommandString[(int)cmd]);
                ostringstream str;
                str <<"ID: <" << id.first << "," << id.second << ">";
                str << " Cmd: " << cmdStr;
                str << " Data: " << (needData ? "T" : "F");
                str << " Ack: " << (needAck ? "T" : "F");
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
    bool directory_; // Whether or not a directory is managing the caches - if so we cannot assume on a writeback that the data is not cached
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
