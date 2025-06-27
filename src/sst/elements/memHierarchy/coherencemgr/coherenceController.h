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

#ifndef MEMHIERARCHY_COHERENCECONTROLLER_H
#define MEMHIERARCHY_COHERENCECONTROLLER_H

#include <array>

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
    virtual ~CoherenceController() { }

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
    virtual bool handleFlushAll(MemEvent * event, bool inMSHR);
    virtual bool handleForwardFlush(MemEvent * event, bool inMSHR);
    virtual bool handleFetch(MemEvent * event, bool inMSHR);
    virtual bool handleInv(MemEvent * event, bool inMSHR);
    virtual bool handleFetchInvX(MemEvent * event, bool inMSHR);
    virtual bool handleFetchInv(MemEvent * event, bool inMSHR);
    virtual bool handleForceInv(MemEvent * event, bool inMSHR);
    virtual bool handleGetSResp(MemEvent * event, bool inMSHR);
    virtual bool handleGetXResp(MemEvent * event, bool inMSHR);
    virtual bool handleWriteResp(MemEvent * event, bool inMSHR);
    virtual bool handleFlushLineResp(MemEvent * event, bool inMSHR);
    virtual bool handleFlushAllResp(MemEvent * event, bool inMSHR);
    virtual bool handleAckPut(MemEvent * event, bool inMSHR);
    virtual bool handleAckInv(MemEvent * event, bool inMSHR);
    virtual bool handleAckFlush(MemEvent * event, bool inMSHR);
    virtual bool handleUnblockFlush(MemEvent * event, bool inMSHR);
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

    /* Broadcast an event to a group of components */
    int broadcastMemEventToSources(Command cmd, MemEvent* metadata, Cycle_t ts);
    int broadcastMemEventToPeers(Command cmd, MemEvent* metadata, Cycle_t ts);

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

    /* Setup function from BaseComponent API */
    virtual void setup() override;

    /*
     * Get the InitCoherenceEvent
     * Event contains information about the protocol configuration and so is generated by coherence managers
     */
    virtual MemEventInitCoherence * getInitCoherenceEvent() = 0;

    /* Parse an incoming InitCoherence event */
    virtual void processInitCoherenceEvent(MemEventInitCoherence * event, bool source);

    /* Prepare for complete stage - called at complete phase=0 */
    virtual void beginCompleteStage() {}

    /* Parse an incoming Complete (quiesce, checkpoint, etc. ) event */
    virtual void processCompleteEvent(MemEventInit* event, MemLinkBase* highlink, MemLinkBase* lowlink);

    /* Some managers care, others don't */
    virtual void hasUpperLevelCacheName(std::string cachename) {}

    /* Setup array of cache listeners */
    void setCacheListener(std::vector<CacheListener*> &ptr, size_t dropPrefetchLevel, size_t maxOutPrefetches) {
        listeners_ = ptr;
        drop_prefetch_level_ = dropPrefetchLevel;
        max_outstanding_prefetch_ = maxOutPrefetches;
    }

    /* Set MSHR */
    void setMSHR(MSHR* ptr) { mshr_ = ptr; }

    /* Set link managers */
    void setLinks(MemLinkBase * link_up, MemLinkBase * link_down) {
        link_up_ = link_up;
        link_down_ = link_down;
    }

    /* Prefetch drop statistic is used by both controller and coherence managers */
    void setStatistics(Statistic<uint64_t>* prefetchdrop) { statPrefetchDrop = prefetchdrop; }

    /* Controller records received events, but valid types are determined by coherence manager. Share those here */
    virtual std::set<Command> getValidReceiveEvents() = 0;

    /* Call through to cache array to configure banking/slicing */
    virtual void setSliceAware(uint64_t interleaveSize, uint64_t interleaveStep) = 0;

    /* Register callback to enable the cache's clock if needed */
    void registerClockEnableFunction(std::function<void()> fcn) { reenableClock_ = fcn; }

    /* Setup debug info (cache-wide) */
    void setDebug(std::set<Addr> debug_addr) { debug_addr_filter_ = debug_addr; }

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
    virtual void printStatus(Output &out) override;

    // Serialization
    CoherenceController() : SubComponent() {}
    void serialize_order(SST::Core::Serialization::serializer& ser) override;
    ImplementVirtualSerializable(SST::MemHierarchy::CoherenceController)

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

    /* Insight into link status */
    bool isPeer(std::string name);

    /* Statistics */
    virtual void recordLatencyType(SST::Event::id_type id, int latencytype);
    virtual void recordPrefetchLatency(SST::Event::id_type, int latencytype);

    /* Debug */

    struct dbgin {
        SST::Event::id_type id;
        uint32_t thr;
        bool hasThr;
        Command cmd;
        std::string mod; // Command modifier
        Addr addr;
        State oldst;
        State newst;
        std::string action;
        std::string reason;
        std::string verboseline;

        void prefill(SST::Event::id_type i, Command c, std::string m, Addr a, State o) {
            prefill(i, 0, c, m, a, o);
            hasThr = false;
        }

        void prefill(SST::Event::id_type i, uint32_t t, Command c, std::string m, Addr a, State o) {
            id = i;
            thr = t;
            hasThr = true;
            cmd = c;
            mod = m;
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

        void serialize_order(SST::Core::Serialization::serializer& ser) {
            SST_SER(id);
            SST_SER(thr);
            SST_SER(hasThr);
            SST_SER(cmd);
            SST_SER(mod);
            SST_SER(addr);
            SST_SER(oldst);
            SST_SER(newst);
            SST_SER(action);
            SST_SER(reason);
            SST_SER(verboseline);
        }
    };

    /* These track information about coherence events in case a debug log is being generated */
    dbgin event_debuginfo_, evict_debuginfo_;

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
    size_t max_outstanding_prefetch_;
    size_t drop_prefetch_level_;
    size_t outstanding_prefetch_count_;

    /* Cache name - used for identifying where events came from/are going to */
    std::string cachename_;

    /* Output & debug */
    Output* output_ = nullptr;   // Output stream for warnings, notices, fatal, etc.
    Output* debug_ = nullptr;    // Output stream for debug -> SST must be compiled with --enable-debug
    std::set<Addr> debug_addr_filter_;  // Addresses to print debug info for (all if empty)
    uint32_t debug_level_;            // Debug level -> used to determine output format/amount of output

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
    bool flush_manager_;        // Whether this cache will manage (order) FlushAll events - one per system, may be a directory
    bool flush_helper_;         // Whether this cache will locally manage (order) FlushAll events - one per shared group of caches
    std::string flush_dest_;    // Destination for FlushAll requests

    /* Response structure - used for outgoing event queues */
    struct Response {
        MemEventBase* event;    // Event to send
        uint64_t deliveryTime;  // Time this event can be sent
        uint64_t size;          // Size of event (for bandwidth accounting)

        void serialize_order(SST::Core::Serialization::serializer& ser) {
            SST_SER(event);
            SST_SER(deliveryTime);
            SST_SER(size);
        }
    };

    /* Retry buffer - filled by coherence manangers and drained by parent */
    std::vector<MemEventBase*> retryBuffer_;

    /* Statistics - some variables used by all are declared here, but they are maintained by coherence protocols */
    std::array<Statistic<uint64_t>*, (int)Command::LAST_CMD> stat_eventSent;  // Count events sent
    std::array<Statistic<uint64_t>*, (int)LAST_STATE> stat_evict;             // Count how many evictions happened in a given state
    std::array<std::array<Statistic<uint64_t>*, LAST_STATE>, (int)Command::LAST_CMD> stat_eventstate_;

    struct LatencyStat{
        uint64_t time;
        Command cmd;
        int missType;
        LatencyStat(uint64_t t, Command c, int m) : time(t), cmd(c), missType(m) { }

        LatencyStat() {}

        void serialize_order(SST::Core::Serialization::serializer& ser) {
            SST_SER(time);
            SST_SER(cmd);
            SST_SER(missType);
        }
    };

    std::map<SST::Event::id_type, LatencyStat> startTimes_;

    /* When internally monitoring a timeout period, a coherence controller may need to re-enable the cache's clock */
    std::function<void()> reenableClock_;

    /* Add a new event to the outgoing command queue towards memory */
    virtual void addToOutgoingQueue(Response& resp);

    /* Add a new event to the outgoing command queue towards the CPU */
    virtual void addToOutgoingQueueUp(Response& resp);

    virtual uint64_t sendResponseUp(MemEvent * event, vector<uint8_t>* data, bool replay, uint64_t baseTime, bool success = true);
    virtual uint64_t sendResponseUp(MemEvent * event, Command cmd, vector<uint8_t>* data, bool replay, uint64_t baseTime, bool success = true);
    virtual uint64_t sendResponseUp(MemEvent * event, Command cmd, vector<uint8_t>* data, bool dirty, bool replay, uint64_t baseTime, bool success = true);

    std::string getSrc();

    std::set<std::string> cpus; // If connected to CPUs or other endpoints (e.g., accelerator), list of CPU names in case we need to broadcast something

    void sendUntimedDataUp(MemEventInit* event);
    void sendUnitmedDataDown(MemEventInit* event, bool broadcast);

private:
    /* Outgoing event queues - events are stalled here to account for access latencies */
    list<Response> outgoing_event_queue_down_;
    list<Response> outgoing_event_queue_up_;

    MemLinkBase * link_up_ = nullptr;
    MemLinkBase * link_down_ = nullptr;

    /**************** Haven't determined if we need the rest yet ! ************************************/
protected:

    /* Resend an event after a NACK */
    void resendEvent(MemEvent * event, bool towardsCPU);

    /* Throughput control TODO move these to a port manager */
    uint64_t maxBytesUp;
    uint64_t maxBytesDown;
    uint64_t packetHeaderBytes;

    /* Prefetch statistics */
    Statistic<uint64_t>* statPrefetchEvict = nullptr;
    Statistic<uint64_t>* statPrefetchInv = nullptr;
    Statistic<uint64_t>* statPrefetchRedundant = nullptr;
    Statistic<uint64_t>* statPrefetchUpgradeMiss = nullptr;
    Statistic<uint64_t>* statPrefetchHit = nullptr;
    Statistic<uint64_t>* statPrefetchDrop = nullptr;
};

}}

#endif	/* COHERENCECONTROLLER_H */
