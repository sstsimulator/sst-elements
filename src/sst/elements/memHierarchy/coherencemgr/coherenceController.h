// Copyright 2009-2021 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2021, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef MEMHIERARCHY_COHERENCECONTROLLER_H
#define MEMHIERARCHY_COHERENCECONTROLLER_H

#include <array>

#include <sst/core/sst_config.h>
#include <sst/core/subcomponent.h>
#include <sst/core/event.h>
#include <sst/core/sst_types.h>
#include <sst/core/link.h>
#include <sst/core/timeConverter.h>
#include <sst/core/output.h>

#include "util.h"
#include "sst/elements/memHierarchy/cacheListener.h"
#include "sst/elements/memHierarchy/mshr.h"
#include "sst/elements/memHierarchy/memLinkBase.h"
#include "sst/elements/memHierarchy/replacementManager.h"
#include "sst/elements/memHierarchy/hash.h"

namespace SST { namespace MemHierarchy {
using namespace std;

namespace LatType {
    enum { HIT, MISS, INV, UPGRADE };
};

class CoherenceController : public SST::SubComponent {

public:
    /* Args: Params& extraParams, bool prefetch */
    SST_ELI_REGISTER_SUBCOMPONENT_API(SST::MemHierarchy::CoherenceController, Params&, bool)

    /***** Constructor & destructor *****/
    CoherenceController(ComponentId_t id, Params &params, Params& ownerParams, bool prefetch);
    virtual ~CoherenceController() {}

    /*********************************************************************************
     * Event handlers - one per event type
     * Handlers return whether the event was accepted (true) or rejected (false)
     *********************************************************************************/
    virtual bool handleNULLCMD(MemEvent * event, bool inMSHR);
    virtual bool handleGetS(MemEvent * event, bool inMSHR);
    virtual bool handleWrite(MemEvent * event, bool inMSHR);
    virtual bool handleGetX(MemEvent * event, bool inMSHR);
    virtual bool handleGetSX(MemEvent * event, bool inMSHR);
    virtual bool handlePutS(MemEvent * event, bool inMSHR);
    virtual bool handlePutX(MemEvent * event, bool inMSHR);
    virtual bool handlePutE(MemEvent * event, bool inMSHR);
    virtual bool handlePutM(MemEvent * event, bool inMSHR);
    virtual bool handleFlushLine(MemEvent * event, bool inMSHR);
    virtual bool handleFlushLineInv(MemEvent * event, bool inMSHR);
    virtual bool handleFetch(MemEvent * event, bool inMSHR);
    virtual bool handleInv(MemEvent * event, bool inMSHR);
    virtual bool handleFetchInvX(MemEvent * event, bool inMSHR);
    virtual bool handleFetchInv(MemEvent * event, bool inMSHR);
    virtual bool handleForceInv(MemEvent * event, bool inMSHR);
    virtual bool handleGetSResp(MemEvent * event, bool inMSHR);
    virtual bool handleGetXResp(MemEvent * event, bool inMSHR);
    virtual bool handleWriteResp(MemEvent * event, bool inMSHR);
    virtual bool handleFlushLineResp(MemEvent * event, bool inMSHR);
    virtual bool handleAckPut(MemEvent * event, bool inMSHR);
    virtual bool handleAckInv(MemEvent * event, bool inMSHR);
    virtual bool handleFetchResp(MemEvent * event, bool inMSHR);
    virtual bool handleFetchXResp(MemEvent * event, bool inMSHR);
    virtual bool handleNACK(MemEvent * event, bool inMSHR);


    /*********************************************************************************
     * Send outgoing events
     *********************************************************************************/

    /* Send commands when their timestamp expires. Return whether queue is empty or not */
    virtual bool sendOutgoingEvents();

    /* Forward an event using memory address to locate a destination. */
    virtual void forwardByAddress(MemEventBase * event);                // Send time will be 1 + timestamp_
    virtual void forwardByAddress(MemEventBase * event, Cycle_t ts);    // ts specifies the send time

    /* Forward an event towards a specific destination. */
    virtual void forwardByDestination(MemEventBase * event);             // Send time will be 1 + timestamp_
    virtual void forwardByDestination(MemEventBase * event, Cycle_t ts); // ts specifies the send time
    
    /* Send a NACK event */
    void sendNACK(MemEvent * event);


    /*********************************************************************************
     * Miscellaneous functions used by parent
     *********************************************************************************/

    /* For clock handling = parent updates timestamp when the clock is re-enabled */
    void updateTimestamp(uint64_t newTS) { timestamp_ = newTS; }

    /* Check whether the event queues are empty/subcomponent is doing anything */
    bool checkIdle();

    /* Get which bank an address maps to (call through to cache array) */
    virtual Addr getBank(Addr addr) = 0;


    /*********************************************************************************
     * Initialization/finish functions used by parent
     *********************************************************************************/

    /*
     * Get the InitCoherenceEvent
     * Event contains information about the protocol configuration and so is generated by coherence managers
     */
    virtual MemEventInitCoherence * getInitCoherenceEvent() = 0;

    /* Parse an incoming InitCoherence event */
    virtual void processInitCoherenceEvent(MemEventInitCoherence * event, bool source);

    /* Some managers care, others don't */
    virtual void hasUpperLevelCacheName(std::string cachename) {}

    /* Setup array of cache listeners */
    void setCacheListener(std::vector<CacheListener*> &ptr, size_t dropPrefetchLevel, size_t maxOutPrefetches) {
        listeners_ = ptr;
        dropPrefetchLevel_ = dropPrefetchLevel;
        maxOutstandingPrefetch_ = maxOutPrefetches;
    }

    /* Set MSHR */
    void setMSHR(MSHR* ptr) { mshr_ = ptr; }

    /* Set link managers */
    void setLinks(MemLinkBase * linkUp, MemLinkBase * linkDown) {
        linkUp_ = linkUp;
        linkDown_ = linkDown;
    }

    /* Prefetch drop statistic is used by both controller and coherence managers */
    void setStatistics(Statistic<uint64_t>* prefetchdrop) { statPrefetchDrop = prefetchdrop; }

    /* Controller records received events, but valid types are determined by coherence manager. Share those here */
    virtual std::set<Command> getValidReceiveEvents() = 0;

    /* Call through to cache array to configure banking/slicing */
    virtual void setSliceAware(uint64_t interleaveSize, uint64_t interleaveStep) = 0;

    /* Setup debug info (cache-wide) */
    void setDebug(std::set<Addr> debugAddr) { DEBUG_ADDR = debugAddr; }

    /* Retry buffer - parent drains this each cycle */
    std::vector<MemEventBase*>* getRetryBuffer();
    void clearRetryBuffer();

    virtual void printDebugInfo();

    /*********************************************************************************
     * Statistics functions shared by parent
     *
     * Often statistics are recorded at the parent but the set
     * of valid statistics is determined by the specific coherence manager
     * (e.g., not all events and states are possible in all protocols)
     *********************************************************************************/
    virtual void recordLatency(Command cmd, int type, uint64_t latency) =0;

    // TODO are these needed still?
    virtual void recordIncomingRequest(MemEventBase* event);
    virtual void removeRequestRecord(SST::Event::id_type id);
    virtual void recordMiss(SST::Event::id_type id);

    // Called by owner during printStatus/emergencyShutdown
    virtual void printStatus(Output &out);

protected:

    /*********************************************************************************
     * Function members
     *********************************************************************************/

    /* Listener callbacks */
    virtual void notifyListenerOfAccess(MemEvent * event, NotifyAccessType accessT, NotifyResultType resultT);
    virtual void notifyListenerOfEvict(Addr addr, uint32_t size, uint64_t ip);

    /* Forward a message to a lower memory level (towards memory) */
    uint64_t forwardMessage(MemEvent * event, unsigned int requestSize, uint64_t baseTime, vector<uint8_t>* data, Command fwdCmd = Command::LAST_CMD);

    /* Insert event into MSHR */
    MemEventStatus allocateMSHR(MemEvent * event, bool fwdReq, int pos = -1, bool stallEvict = false);

    /* Statistics */
    virtual void recordLatencyType(SST::Event::id_type id, int latencytype);
    virtual void recordPrefetchLatency(SST::Event::id_type, int latencytype);

    /* Debug */

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
            action = "";
            reason = "";
            verboseline = "";
        }

        void fill(State n, std::string act, std::string rea) {
            newst = n;
            action = act;
            reason = rea;
        }
    } eventDI, evictDI;

    virtual void printDebugInfo(dbgin * diStruct);
    virtual void printDebugAlloc(bool alloc, Addr addr, std::string note);
    virtual void printDataValue(Addr addr, vector<uint8_t> * data, bool set);

    /* Initialization */
    ReplacementPolicy * createReplacementPolicy(uint64_t lines, uint64_t assoc, Params& params, bool L1, int slotnum = 0);
    HashFunction * createHashFunction(Params& params);

    /*********************************************************************************
     * Data members
     *********************************************************************************/

    /* Miss status handling register */
    MSHR * mshr_;

    /* Listeners: prefetchers, tracers, etc. */
    std::vector<CacheListener*> listeners_;
    size_t maxOutstandingPrefetch_;
    size_t dropPrefetchLevel_;
    size_t outstandingPrefetches_;

    /* Cache name - used for identifying where events came from/are going to */
    std::string cachename_;

    /* Output & debug */
    Output* output; // Output stream for warnings, notices, fatal, etc.
    Output* debug;  // Output stream for debug -> SST must be compiled with --enable-debug
    std::set<Addr> DEBUG_ADDR; // Addresses to print debug info for (all if empty)
    uint32_t dlevel;    // Debug level -> used to determine output format/amount of output

    /* Latencies amd timing */
    uint64_t timestamp_;        // Local timestamp (cycles)
    uint64_t accessLatency_;    // Data/tag access latency
    uint64_t tagLatency_;       // Tag only access latency
    uint64_t mshrLatency_;      // MSHR lookup latency

    /* Cache parameters that are often needed by coherence managers */
    uint64_t lineSize_;
    bool writebackCleanBlocks_; // Writeback clean data as opposed to just a coherence msg
    bool silentEvictClean_;     // Silently evict clean blocks (currently ok when just mem below us)
    bool recvWritebackAck_;     // Whether we should expect writeback acks
    bool sendWritebackAck_;     // Whether we should send writeback acks
    bool lastLevel_;            // Whether we are the lowest coherence level and should not send coherence messages down

    /* Response structure - used for outgoing event queues */
    struct Response {
        MemEventBase* event;    // Event to send
        uint64_t deliveryTime;  // Time this event can be sent
        uint64_t size;          // Size of event (for bandwidth accounting)
    };

    /* Retry buffer - filled by coherence manangers and drained by parent */
    std::vector<MemEventBase*> retryBuffer_;

    /* Statistics - some variables used by all are declared here, but they are maintained by coherence protocols */
    Statistic<uint64_t>* stat_eventSent[(int)Command::LAST_CMD];    // Count events sent
    Statistic<uint64_t>* stat_evict[LAST_STATE];                    // Count how many evictions happened in a given state
    std::array<std::array<Statistic<uint64_t>*, LAST_STATE>, (int)Command::LAST_CMD> stat_eventState;

    struct LatencyStat{
        uint64_t time;
        Command cmd;
        int missType;
        LatencyStat(uint64_t t, Command c, int m) : time(t), cmd(c), missType(m) { }
    };

    std::map<SST::Event::id_type, LatencyStat> startTimes_;


    /* Add a new event to the outgoing command queue towards memory */
    virtual void addToOutgoingQueue(Response& resp);

    /* Add a new event to the outgoing command queue towards the CPU */
    virtual void addToOutgoingQueueUp(Response& resp);

    virtual uint64_t sendResponseUp(MemEvent * event, vector<uint8_t>* data, bool replay, uint64_t baseTime, bool success = true);
    virtual uint64_t sendResponseUp(MemEvent * event, Command cmd, vector<uint8_t>* data, bool replay, uint64_t baseTime, bool success = true);
    virtual uint64_t sendResponseUp(MemEvent * event, Command cmd, vector<uint8_t>* data, bool dirty, bool replay, uint64_t baseTime, bool success = true);

    std::string getSrc();

    std::set<std::string> cpus; // If connected to CPUs or other endpoints (e.g., accelerator), list of CPU names in case we need to broadcast something

private:
    /* Outgoing event queues - events are stalled here to account for access latencies */
    list<Response> outgoingEventQueueDown_;
    list<Response> outgoingEventQueueUp_;

    MemLinkBase * linkUp_;
    MemLinkBase * linkDown_;

    /**************** Haven't determined if we need the rest yet ! ************************************/
protected:

    /* Resend an event after a NACK */
    void resendEvent(MemEvent * event, bool towardsCPU);

    /* Throughput control TODO move these to a port manager */
    uint64_t maxBytesUp;
    uint64_t maxBytesDown;
    uint64_t packetHeaderBytes;

    /* Prefetch statistics */
    Statistic<uint64_t>* statPrefetchEvict;
    Statistic<uint64_t>* statPrefetchInv;
    Statistic<uint64_t>* statPrefetchRedundant;
    Statistic<uint64_t>* statPrefetchUpgradeMiss;
    Statistic<uint64_t>* statPrefetchHit;
    Statistic<uint64_t>* statPrefetchDrop;
};

}}

#endif	/* COHERENCECONTROLLER_H */
