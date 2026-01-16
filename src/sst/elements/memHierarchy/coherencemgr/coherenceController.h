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
    CoherenceController(ComponentId_t id, Params &params, Params& owner_params, bool prefetch);
    virtual ~CoherenceController() { }

    /*********************************************************************************
     * Event handlers - one per event type
     * Handlers return whether the event was accepted (true) or rejected (false)
     *********************************************************************************/
    virtual bool handleNULLCMD(MemEvent * event, bool in_mshr);
    virtual bool handleGetS(MemEvent * event, bool in_mshr);
    virtual bool handleWrite(MemEvent * event, bool in_mshr);
    virtual bool handleGetX(MemEvent * event, bool in_mshr);
    virtual bool handleGetSX(MemEvent * event, bool in_mshr);
    virtual bool handlePutS(MemEvent * event, bool in_mshr);
    virtual bool handlePutX(MemEvent * event, bool in_mshr);
    virtual bool handlePutE(MemEvent * event, bool in_mshr);
    virtual bool handlePutM(MemEvent * event, bool in_mshr);
    virtual bool handleFlushLine(MemEvent * event, bool in_mshr);
    virtual bool handleFlushLineInv(MemEvent * event, bool in_mshr);
    virtual bool handleFlushAll(MemEvent * event, bool in_mshr);
    virtual bool handleForwardFlush(MemEvent * event, bool in_mshr);
    virtual bool handleFetch(MemEvent * event, bool in_mshr);
    virtual bool handleInv(MemEvent * event, bool in_mshr);
    virtual bool handleFetchInvX(MemEvent * event, bool in_mshr);
    virtual bool handleFetchInv(MemEvent * event, bool in_mshr);
    virtual bool handleForceInv(MemEvent * event, bool in_mshr);
    virtual bool handleGetSResp(MemEvent * event, bool in_mshr);
    virtual bool handleGetXResp(MemEvent * event, bool in_mshr);
    virtual bool handleWriteResp(MemEvent * event, bool in_mshr);
    virtual bool handleFlushLineResp(MemEvent * event, bool in_mshr);
    virtual bool handleFlushAllResp(MemEvent * event, bool in_mshr);
    virtual bool handleAckPut(MemEvent * event, bool in_mshr);
    virtual bool handleAckInv(MemEvent * event, bool in_mshr);
    virtual bool handleAckFlush(MemEvent * event, bool in_mshr);
    virtual bool handleUnblockFlush(MemEvent * event, bool in_mshr);
    virtual bool handleFetchResp(MemEvent * event, bool in_mshr);
    virtual bool handleFetchXResp(MemEvent * event, bool in_mshr);
    virtual bool handleNACK(MemEvent * event, bool in_mshr);


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
    void updateTimestamp(uint64_t new_timestamp) { timestamp_ = new_timestamp; }

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
    void setCacheListener(std::vector<CacheListener*> &ptr, size_t drop_prefetch_level, size_t max_out_prefetches) {
        listeners_ = ptr;
        drop_prefetch_level_ = drop_prefetch_level;
        max_outstanding_prefetch_ = max_out_prefetches;
    }

    /* Set MSHR */
    void setMSHR(MSHR* ptr) { mshr_ = ptr; }

    /* Set link managers */
    void setLinks(MemLinkBase * link_up, MemLinkBase * link_down) {
        link_up_ = link_up;
        link_down_ = link_down;
    }

    /* Prefetch drop statistic is used by both controller and coherence managers */
    void setStatistics(Statistic<uint64_t>* prefetch_drop) { stat_prefetch_drop_ = prefetch_drop; }

    /* Controller records received events, but valid types are determined by coherence manager. Share those here */
    virtual std::set<Command> getValidReceiveEvents() = 0;

    /* Call through to cache array to configure banking/slicing */
    virtual void setSliceAware(uint64_t interleave_size, uint64_t interleave_step) = 0;

    /* Register callback to enable the cache's clock if needed */
    void registerClockEnableFunction(std::function<void()> fcn) { reenable_clock_ = fcn; }

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
    virtual void notifyListenerOfAccess(MemEvent * event, NotifyAccessType access_type, NotifyResultType result_type);
    virtual void notifyListenerOfEvict(Addr addr, uint32_t size, Addr ip, MemEventBase::id_type evId = MemEventBase::id_type(0,0));

    /* Forward a message to a lower memory level (towards memory) */
    uint64_t forwardMessage(MemEvent * event, unsigned int request_size, uint64_t base_time, vector<uint8_t>* data, Command forward_command = Command::LAST_CMD);

    /* Insert event into MSHR */
    MemEventStatus allocateMSHR(MemEvent * event, bool forward_request, int pos = -1, bool stall_for_evict = false);

    /* Insight into link status */
    bool isPeer(std::string name);

    /* Statistics */
    virtual void recordLatencyType(SST::Event::id_type id, int latency_type);
    virtual void recordPrefetchLatency(SST::Event::id_type, int latency_type);

    /* Debug */

    struct dbgin {
        SST::Event::id_type id;
        uint32_t thread;
        bool has_thread;
        Command cmd;
        std::string modifier; // Command modifier
        Addr addr;
        State old_state;
        State new_state;
        std::string action;
        std::string reason;
        std::string verbose_line;

        void prefill(SST::Event::id_type i, Command c, std::string m, Addr a, State o) {
            prefill(i, 0, c, m, a, o);
            has_thread = false;
        }

        void prefill(SST::Event::id_type i, uint32_t t, Command c, std::string m, Addr a, State o) {
            id = i;
            thread = t;
            has_thread = true;
            cmd = c;
            modifier = m;
            addr = a;
            old_state = o;
            new_state = o;
            action = "";
            reason = "";
            verbose_line = "";
        }

        void fill(State n, std::string act, std::string rea) {
            new_state = n;
            action = act;
            reason = rea;
        }

        void serialize_order(SST::Core::Serialization::serializer& ser) {
            SST_SER(id);
            SST_SER(thread);
            SST_SER(has_thread);
            SST_SER(cmd);
            SST_SER(modifier);
            SST_SER(addr);
            SST_SER(old_state);
            SST_SER(new_state);
            SST_SER(action);
            SST_SER(reason);
            SST_SER(verbose_line);
        }
    };

    /* These track information about coherence events in case a debug log is being generated */
    dbgin event_debuginfo_, evict_debuginfo_;

    virtual void printDebugInfo(dbgin * debug_struct);
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
    uint64_t access_latency_;   // Data/tag access latency
    uint64_t tag_latency_;      // Tag only access latency
    uint64_t mshr_latency_;     // MSHR lookup latency

    /* Cache parameters that are often needed by coherence managers */
    uint64_t line_size_;
    bool writeback_clean_blocks_;   // Writeback clean data as opposed to just a coherence msg
    bool silent_evict_clean_;       // Silently evict clean blocks (currently ok when just mem below us)
    bool recv_writeback_ack_;       // Whether we should expect writeback acks
    bool send_writeback_ack_;       // Whether we should send writeback acks
    bool last_level_;               // Whether we are the lowest coherence level and should not send coherence messages down
    bool flush_manager_;            // Whether this cache will manage (order) FlushAll events - one per system, may be a directory
    bool flush_helper_;             // Whether this cache will locally manage (order) FlushAll events - one per shared group of caches
    std::string flush_dest_;        // Destination for FlushAll requests

    /* Response structure - used for outgoing event queues */
    struct Response {
        MemEventBase* event;    // Event to send
        uint64_t delivery_time; // Time this event can be sent
        uint64_t size;          // Size of event (for bandwidth accounting)

        void serialize_order(SST::Core::Serialization::serializer& ser) {
            SST_SER(event);
            SST_SER(delivery_time);
            SST_SER(size);
        }
    };

    /* Retry buffer - filled by coherence managers and drained by parent */
    std::vector<MemEventBase*> retry_buffer_;

    /* Statistics - some variables used by all are declared here, but they are maintained by coherence protocols */
    std::array<Statistic<uint64_t>*, (int)Command::LAST_CMD> stat_event_sent_ = {}; // Count events sent
    std::array<Statistic<uint64_t>*, (int)LAST_STATE> stat_evict_ = {};             // Count how many evictions happened in a given state
    std::array<std::array<Statistic<uint64_t>*, LAST_STATE>, (int)Command::LAST_CMD> stat_event_state_ = {};

    struct LatencyStat{
        uint64_t time;
        Command cmd;
        int miss_type;
        LatencyStat(uint64_t t, Command c, int m) : time(t), cmd(c), miss_type(m) { }

        LatencyStat() {}

        void serialize_order(SST::Core::Serialization::serializer& ser) {
            SST_SER(time);
            SST_SER(cmd);
            SST_SER(miss_type);
        }
    };

    std::map<SST::Event::id_type, LatencyStat> start_times_;

    /* When internally monitoring a timeout period, a coherence controller may need to re-enable the cache's clock */
    std::function<void()> reenable_clock_;

    /* Add a new event to the outgoing command queue towards memory */
    virtual void addToOutgoingQueue(Response& resp);

    /* Add a new event to the outgoing command queue towards the CPU */
    virtual void addToOutgoingQueueUp(Response& resp);

    virtual uint64_t sendResponseUp(MemEvent * event, vector<uint8_t>* data, bool replay, uint64_t base_time, bool success = true);
    virtual uint64_t sendResponseUp(MemEvent * event, Command cmd, vector<uint8_t>* data, bool replay, uint64_t base_time, bool success = true);
    virtual uint64_t sendResponseUp(MemEvent * event, Command cmd, vector<uint8_t>* data, bool dirty, bool replay, uint64_t base_time, bool success = true);

    std::string getSrc();

    std::set<std::string> system_cpu_names_; // If connected to CPUs or other endpoints (e.g., accelerator), list of CPU names in case we need to broadcast something

    void sendUntimedDataUp(MemEventInit* event) {}
    void sendUnitmedDataDown(MemEventInit* event, bool broadcast) {}

private:
    /* Outgoing event queues - events are stalled here to account for access latencies */
    list<Response> outgoing_event_queue_down_;
    list<Response> outgoing_event_queue_up_;

    MemLinkBase * link_up_ = nullptr;
    MemLinkBase * link_down_ = nullptr;

    /**************** Haven't determined if we need the rest yet ! ************************************/
protected:

    /* Resend an event after a NACK */
    void resendEvent(MemEvent * event, bool towards_cpu);

    /* Throughput control TODO move these to a port manager */
    uint64_t max_bytes_up_;
    uint64_t max_bytes_down_;
    uint64_t packet_header_bytes_;

    /* Prefetch statistics */
    Statistic<uint64_t>* stat_prefetch_evict_ = nullptr;
    Statistic<uint64_t>* stat_prefetch_inv_ = nullptr;
    Statistic<uint64_t>* stat_prefetch_redundant_ = nullptr;
    Statistic<uint64_t>* stat_prefetch_upgrade_miss_ = nullptr;
    Statistic<uint64_t>* stat_prefetch_hit_ = nullptr;
    Statistic<uint64_t>* stat_prefetch_drop_ = nullptr;
};

}}

#endif	/* COHERENCECONTROLLER_H */
