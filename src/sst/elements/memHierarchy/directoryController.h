// Copyright 2013-2018 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2013-2018, NTESS
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

#include "sst/elements/memHierarchy/memLinkBase.h"
#include "sst/elements/memHierarchy/memEvent.h"
#include "sst/elements/memHierarchy/util.h"
#include "sst/elements/memHierarchy/mshr.h"

using namespace std;

namespace SST { namespace MemHierarchy {

class DirectoryController : public Component {
public:
/* Element Library Info */
    SST_ELI_REGISTER_COMPONENT(DirectoryController, "memHierarchy", "DirectoryController", SST_ELI_ELEMENT_VERSION(1,0,0),
            "Coherence directory, MSI or MESI", COMPONENT_CATEGORY_MEMORY)

    SST_ELI_DOCUMENT_PARAMS(
            {"clock",                   "Clock rate of controller.", "1GHz"},
            {"entry_cache_size",        "Size (in # of entries) the controller will cache.", "0"},
            {"debug",                   "Where to send debug output. 0: No debugging, 1: STDOUT, 2: STDERR, 3: FILE.", "0"},
            {"debug_level",             "Debugging level: 0 to 10. Must configure sst-core with '--enable-debug'. 1=info, 2-10=debug output", "0"},
            {"debug_addr",              "(comma separated uint) Address(es) to be debugged. Leave empty for all, otherwise specify one or more, comma-separated values. Start and end string with brackets",""},
            {"verbose",                 "Output verbosity for warnings/errors. 0[fatal error only], 1[warnings], 2[full state dump on fatal error]","1"},
            {"cache_line_size",         "Size of a cache line [aka cache block] in bytes.", "64"},
            {"coherence_protocol",      "Coherence protocol.  Supported --MESI, MSI--", "MESI"},
            {"mshr_num_entries",        "Number of MSHRs. Set to -1 for almost unlimited number.", "-1"},
            {"net_memory_name",         "For directories connected to a memory over the network: name of the memory this directory owns", ""},
            {"access_latency_cycles",   "Latency of directory access in cycles", "0"},
            {"mshr_latency_cycles",     "Latency of mshr access in cycles", "0"},
            {"max_requests_per_cycle",  "Maximum number of requests to process per cycle (0 or negative is unlimited)", "0"},
            {"mem_addr_start",          "Starting memory address for the chunk of memory that this directory controller addresses.", "0"},
            {"addr_range_start",        "Lowest address handled by this directory.", "0"},
            {"addr_range_end",          "Highest address handled by this directory.", "uint64_t-1"},
            {"interleave_size",         "Size of interleaved chunks. E.g., to interleave 8B chunks among 3 directories, set size=8B, step=24B", "0B"},
            {"interleave_step",         "Distance between interleaved chunks. E.g., to interleave 8B chunks among 3 directories, set size=8B, step=24B", "0B"},
            {"node",					"Node number in multinode environment"},
            /* Old parameters - deprecated or moved */
            {"network_num_vc",          "DEPRECATED. Number of virtual channels (VCs) on the on-chip network. memHierarchy only uses one VC.", "1"}, // Remove SST 9.0
            {"network_address",         "DEPRECATD - Now auto-detected by link control", ""},   // Remove SST 9.0
            {"network_bw",                  "MOVED. Now a member of the MemNIC/MemLink subcomponent.", "80GiB/s"}, // Remove SST 9.0
            {"network_input_buffer_size",   "MOVED. Now a member of the MemNIC/MemLink subcomponent.", "1KiB"}, // Remove SST 9.0
            {"network_output_buffer_size",  "MOVED. Now a member of the MemNIC/MemLink subcomponent.", "1KiB"}) // Remove SST 9.0

    SST_ELI_DOCUMENT_PORTS(
            {"memory",      "Link to memory controller", { "memHierarchy.MemEventBase" } },
            {"network",     "Link to network; doubles as request network for split networks", { "memHierarchy.MemRtrEvent" } },
            {"network_ack", "For split networks, link to response/ack network",     { "memHierarchy.MemRtrEvent" } },
            {"network_fwd", "For split networks, link to forward request network",  { "memHierarchy.MemRtrEvent" } },
            {"network_data","For split networks, link to data network",             { "memHierarchy.MemRtrEvent" } })

    SST_ELI_DOCUMENT_STATISTICS(
            {"replacement_request_latency",     "Total latency in ns of all replacement (put*) requests handled",       "nanoseconds",  1},
            {"get_request_latency",             "Total latency in ns of all get* requests handled",                     "nanoseconds",  1},
            {"directory_cache_hits",            "Number of requests that hit in the directory cache",                   "requests",     1},
            {"mshr_hits",                       "Number of requests that hit in the MSHRs",                             "requests",     1},
            {"requests_received_GetS",          "Number of GetS (read-shared) requests received",                       "requests",     1},
            {"requests_received_GetX",          "Number of GetX (write-exclusive) requests received",                   "requests",     1},
            {"requests_received_GetSX",        "Number of GetSX (read-exclusive) requests received",                    "requests",     1},
            {"requests_received_PutS",          "Number of PutS (shared replacement) requests received",                "requests",     1},
            {"requests_received_PutE",          "Number of PutE (clean exclusive replacement) requests received",       "requests",     1},
            {"requests_received_PutM",          "Number of PutM (dirty exclusive replacement) requests received",       "requests",     1},
            {"requests_received_noncacheable",  "Number of noncacheable requests that were received and forwarded",     "requests",     1},
            {"requests_received_custom",        "Number of custom requests that were received and forwarded",           "requests",     1},
            {"responses_received_NACK",         "Number of NACK responses received",                                    "responses",    1},
            {"responses_received_FetchResp",    "Number of FetchResp responses received (response to FetchInv/Fetch)",  "responses",    1},
            {"responses_received_FetchXResp",   "Number of FetchXResp responses received (response to FetchXInv) ",     "responses",    1},
            {"responses_received_PutS",         "Number of PutS (shared replacement) requests received that raced with an Inv/Fetch* and were treated as a response to that Inv/Fetch*",   "requests",     1},
            {"responses_received_PutE",         "Number of PutE (clean exclusive replacement) requests received that raced with a Fetch* and were treated as a response to that Fetch*",   "requests",     1},
            {"responses_received_PutM",         "Number of PutM (dirty exclusive replacement) requests received that raced with a Fetch* and were treated as a response to that Fetch*",   "requests",     1},
            {"memory_requests_directory_entry_read", "Number of read requests for a directory entry sent to memory",    "requests",     1},
            {"memory_requests_directory_entry_write","Number of write requests for a directory entry sent to memory",   "requests",     1},
            {"memory_requests_data_read",       "Number of read requests for data sent to memory",                      "requests",     1},
            {"memory_requests_data_write",      "Number of write requests for data sent to memory",                     "requests",     1},
            {"requests_sent_Inv",               "Number of Inv (invalidate) requests sent to LLCs",                     "requests",     1},
            {"requests_sent_FetchInv",          "Number of FetchInv (invalidate and fetch exclusive data) requests sent to LLCs",   "requests",     1},
            {"requests_sent_FetchInvX",         "Number of FetchInvX (fetch exclusive data and downgrade) requests sent to LLCs",   "requests",     1},
            {"responses_sent_NACK",             "Number of NACK responses sent to LLCs",                                            "responses",    1},
            {"responses_sent_GetSResp",         "Number of GetSResp (data response to GetS or GetSX) responses sent to LLCs",       "responses",    1},
            {"responses_sent_GetXResp",         "Number of GetXResp (data response to GetX) responses sent to LLCs",                "responses",    1},
            {"MSHR_occupancy",                  "Number of events in MSHR each cycle",                                  "events",       1},
            {"default_stat",                    "Default statistic. If not 0 then a statistic is missing", "", 1})
            
    SST_ELI_DOCUMENT_SUBCOMPONENT_SLOTS(
            {"cpulink", "CPU-side link manager, for single-link directories, use this one only", "SST::MemHierarchy::MemLinkBase"},
            {"memlink", "Memory-side link manager", "SST::MemHierarchy::MemLinkBase"} )

/* Begin class definition */
private:
    Output out;
    Output dbg;
    std::set<Addr> DEBUG_ADDR;

    uint32_t    cacheLineSize;

    /* Range of addresses supported by this directory */
    MemRegion   region;
    Addr        memOffset; // Stack addresses if multiple DCs handle the same memory

    /* Timestamp & latencies */
    uint64_t    timestamp;
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
    Statistic<uint64_t> * stat_eventRecv[(int)Command::LAST_CMD];
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

    /* Queue of packets to work on */
    std::list<MemEvent*> eventBuffer;
    std::list<MemEvent*> retryBuffer;
    std::map<MemEvent::id_type, std::string> noncacheMemReqs;
    
    
    /* Network connections */
    MemLinkBase*    memLink;
    MemLinkBase*    cpuLink;
    string          memoryName; // if connected to mem via network, this should be the name of the memory we own - param is memory_name
    bool clockMemLink;
    bool clockCpuLink;

    bool isRequestAddressValid(Addr addr);

    void turnClockOn();

    inline void profileRequestSent(MemEvent* event);
    inline void profileResponseSent(MemEvent* event);

    void handleNoncacheableRequest(MemEventBase* ev);
    void handleNoncacheableResponse(MemEventBase* ev);

public:
    DirectoryController(ComponentId_t id, Params &params);
    ~DirectoryController();
    void setup(void);
    void init(unsigned int phase);
    void finish(void);

    /** Debug - triggered by output.fatal() or SIGUSR2 */
    virtual void printStatus(Output &out);
    virtual void emergencyShutdown();

    /** Event received from higher level caches.
        Insert to work queue so that it is process in the upcoming clock tick */
    void handlePacket(SST::Event *event);

    /** Handler that gets called by clock tick.
        Function redirects request according to their type. */
    bool processPacket(MemEvent *ev, bool replay);

    /** Clock handler */
    bool clock(SST::Cycle_t cycle);

/* Coherence portion */
public:
    bool handleGetS(MemEvent* event, bool inMSHR);
    bool handleGetX(MemEvent* event, bool inMSHR);
    bool handleGetSX(MemEvent* event, bool inMSHR);
    bool handlePutS(MemEvent* event, bool inMSHR);
    bool handlePutE(MemEvent* event, bool inMSHR);
    bool handlePutM(MemEvent* event, bool inMSHR);
    bool handlePutX(MemEvent* event, bool inMSHR);
    bool handleFlushLine(MemEvent* event, bool inMSHR);
    bool handleFlushLineInv(MemEvent* event, bool inMSHR);
    bool handleFetchInv(MemEvent* event, bool inMSHR);
    bool handleForceInv(MemEvent* event, bool inMSHR);
    bool handleGetSResp(MemEvent* event, bool inMSHR);
    bool handleGetXResp(MemEvent* event, bool inMSHR);
    bool handleFlushLineResp(MemEvent* event, bool inMSHR);
    bool handleAckPut(MemEvent* event, bool inMSHR);
    bool handleAckInv(MemEvent* event, bool inMSHR);
    bool handleFetchResp(MemEvent* event, bool inMSHR);
    bool handleFetchXResp(MemEvent* event, bool inMSHR);
    bool handleNACK(MemEvent* event, bool inMSHR);

    void sendOutgoingEvents();

    void forwardTowardsCPU(MemEventBase * ev);
    void forwardTowardsMem(MemEventBase * ev);

private:
    struct dbgin {
        SST::Event::id_type id;
        Command cmd;
        bool prefetch;
        Addr addr;
        State oldst;
        State newst;
        std::string action;
        std::string reason;
        std::string verboseline;
        
        void prefill(SST::Event::id_type i, Command c, bool p, Addr a, State o) {
            id = i;
            cmd = c;
            prefetch = p;
            addr = a;
            oldst = o;
            newst = o;
            action  = "";
            reason = "";
            verboseline = "";
        }
        void fill(State n, std::string act, std::string rea) {
            newst = n;
            action = act;
            reason = rea;
        }
    } eventDI, evictDI;
    
    struct DirEntry {
	bool                cached;         // whether block is cached or not
        Addr                addr;           // block address
        State               state;          // state
        std::list<DirEntry*>::iterator cacheIter;
	std::set<std::string> sharers;      // set of sharers for block
        std::string         owner;          // Owner of block

        DirEntry(Addr a) {
            clearEntry();
            addr = a;
            state = I;
            cached = false;
        }

        void clearEntry(){
            cached = true;
            addr = 0;
            sharers.clear();
            owner = "";
        }

        std::string getString() {
            std::ostringstream str;
            str << "State: " << StateString[state];
            str << " Sharers: [";
            bool comma = false;
            for (std::set<std::string>::iterator it = sharers.begin(); it != sharers.end(); it++) {
                if (comma)
                    str << ",";
                str << *it;
                comma = true;
            }
            str << "] Owner: " << owner;
            str << " Cached: " << (cached ? "y" : "n");
            return str.str();
        }

        bool isCached() { return cached; }

        void setCached(bool cache) { cached = cache; }

        Addr getBaseAddr() { return addr; }

        size_t getSharerCount() { return sharers.size(); }
        
        void clearSharers() { sharers.clear(); }

        void addSharer(std::string shr) { sharers.insert(shr); }

        bool isSharer(std::string shr) { return sharers.find(shr) != sharers.end(); }

        bool hasSharers() { return !(sharers.empty()); }

        std::set<std::string>* getSharers() { return &sharers; }

        void removeSharer(std::string shr) { sharers.erase(shr); }

        std::string getOwner() { return owner; }
        
        bool hasOwner() { return owner != ""; }

        void removeOwner() { owner = ""; }

        void setOwner(std::string own) { owner = own; }

        void setState(State nState) { state = nState; }

        State getState() { return state; }
    };

    int dlevel;
    void printDebugInfo();

    DirEntry* getDirEntry(Addr addr); // find entry in the master list
    bool retrieveDirEntry(DirEntry* entry, MemEvent* event, bool inMSHR); // Simulate fetching entry from memory
    
    MemEventStatus allocateMSHR(MemEvent* event, bool fwdReq, int pos = -1);

    void cleanUpAfterRequest(MemEvent* event, bool inMSHR);
    void cleanUpAfterResponse(MemEvent* event, bool inMSHR);
    
    void updateCache(DirEntry * entry);
    void sendEntryToMemory(DirEntry* entry);
    
    void issueMemoryRequest(MemEvent* event, DirEntry* entry);
    void issueFlush(MemEvent* event);
    void issueFetch(MemEvent* event, DirEntry* entry, Command cmd);
    void issueInvalidations(MemEvent* event, DirEntry* entry, Command cmd);
    void issueInvalidation(std::string dst, MemEvent* event, DirEntry* entry, Command cmd);
    void sendDataResponse(MemEvent* event, DirEntry* entry, std::vector<uint8_t>& data, Command cmd, uint32_t flags = 0);
    void sendResponse(MemEvent* event, uint32_t flags = 0, uint32_t memflags = 0);
    void writebackData(MemEvent* event);
    void writebackDataFromMSHR(Addr addr);
    void sendFetchResponse(MemEvent* event);
    void sendAckInv(MemEvent* event);
    void sendAckPut(MemEvent* event);
    void sendNACK(MemEvent* event);

    void printEntry(DirEntry * entry);

    MSHR * mshr;
    std::unordered_map<Addr, DirEntry*> directory; // Master list of all directory entries, including noncached ones

    std::multimap<uint64_t,MemEventBase*>   cpuMsgQueue;
    std::multimap<uint64_t,MemEventBase*>   memMsgQueue;

    uint64_t    entryCacheMaxSize;
    uint64_t    entryCacheSize;
    uint32_t    entrySize;
    std::list<DirEntry*> entryCache;

    uint64_t lineSize;

    uint64_t accessLatency;
    uint64_t mshrLatency;

    std::map<MemEvent::id_type, Addr> memReqs;
    std::map<Addr, std::map<std::string, MemEvent::id_type> > responses;

    CoherenceProtocol protocol;
    bool waitWBAck;
    bool sendWBAck;


};

}
}

#endif /* _MEMHIERARCHY_DIRCONTROLLER_H_ */
