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

#ifndef COHERENCECONTROLLER_H
#define COHERENCECONTROLLER_H

#include <sst/core/sst_config.h>
#include <sst/core/subcomponent.h>
#include <sst/core/event.h>
#include <sst/core/sst_types.h>
#include <sst/core/link.h>
#include <sst/core/timeConverter.h>
#include <sst/core/output.h>

#include "util.h"
#include "sst/elements/memHierarchy/cacheListener.h"
#include "sst/elements/memHierarchy/cacheArray.h"
#include "sst/elements/memHierarchy/mshr.h"
#include "sst/elements/memHierarchy/memLinkBase.h"

namespace SST { namespace MemHierarchy {
using namespace std;

class CoherenceController : public SST::SubComponent {

public:
    typedef CacheArray::CacheLine CacheLine;

    /***** Constructor & destructor *****/
    CoherenceController(Component * comp, Params &params);
    ~CoherenceController() {}

    /* Return whether a line access will be a miss and what kind (encoded in the int retval) */
    virtual int isCoherenceMiss(MemEvent * event, CacheLine * line) =0;

    /* Handle a cache request - GetS, GetX, etc. */
    virtual CacheAction handleRequest(MemEvent * event, CacheLine * line, bool replay) =0;

    /* Handle a cache replacement - PutS, PutM, etc. */
    virtual CacheAction handleReplacement(MemEvent * event, CacheLine * line, MemEvent * reqEvent, bool replay) =0;

    /* Handle a cache invalidation - Inv, FetchInv, etc. */
    virtual CacheAction handleInvalidationRequest(MemEvent * event, CacheLine * line, MemEvent * collisionEvent, bool replay) =0;

    /* Handle an eviction */
    virtual CacheAction handleEviction(CacheLine * line, string rqstr, bool fromDataCache=false) =0;

    /* Handle a response - AckInv, GetSResp, etc. */
    virtual CacheAction handleResponse(MemEvent * event, CacheLine * line, MemEvent * request) =0;

    /* Determine whether an event needs to be retried after a NACK */
    virtual bool isRetryNeeded(MemEvent * event, CacheLine * line) =0;

    /* Update timestamp in lockstep with parent */
    void updateTimestamp(uint64_t newTS) { timestamp_ = newTS; }


    /***** Functions for sending events *****/

    /* Send a NACK event. Used by child classes and cache controller */
    void sendNACK(MemEvent * event, bool up, SimTime_t timeInNano);

    /* Resend an event after a NACK */
    void resendEvent(MemEvent * event, bool towardsCPU);

    /* Send a response event up (towards CPU). L1s need to implement their own to split out requested bytes. */
    uint64_t sendResponseUp(MemEvent * event, vector<uint8_t>* data, bool replay, uint64_t baseTime, bool atomic=false);

    /* Send a response event up (towards CPU). L1s need to implement their own to split out requested bytes. */
    uint64_t sendResponseUp(MemEvent * event, Command cmd, vector<uint8_t>* data, bool replay, uint64_t baseTime, bool atomic=false);

    /* Send a response event up (towards CPU). L1s need to implement their own to split out requested bytes. */
    uint64_t sendResponseUp(MemEvent * event, Command cmd, vector<uint8_t>* data, bool dirty, bool replay, uint64_t baseTime, bool atomic=false);


    /* Forward a message to a lower memory level (towards memory) */
    uint64_t forwardMessage(MemEvent * event, Addr baseAddr, unsigned int requestSize, uint64_t baseTime, vector<uint8_t>* data);

    /* Forward a generic message towards memory */
    uint64_t forwardTowardsMem(MemEventBase * event);

    /* Forward a generic message towards CPU */
    uint64_t forwardTowardsCPU(MemEventBase * event, std::string dst);

    /* Return the name of the source for this cache */
    std::string getSrc();

    /* Return the destination for a given address - for sliced/distributed caches */
    std::string getDestination(Addr addr) { return linkDown_->findTargetDestination(addr); }


    /***** Manage outgoing event queuest *****/

    /* Send commands when their timestamp expires. Return whether queue is empty or not. */
    virtual bool sendOutgoingCommands(SimTime_t curTime);


    /***** Setup and initialization functions *****/

    /* Initialize variables that tell this coherence controller how to interact with the cache below it */
    void setupLowerStatus(bool silentEvict, bool isLastLevel, bool isNoninclusive, bool isDir);

    /* Setup pointers to other subcomponents/cache structures */
    void setCacheListener(CacheListener* ptr) { listener_ = ptr; }
    void setMSHR(MSHR* ptr) { mshr_ = ptr; }
    void setLinks(MemLinkBase * linkUp, MemLinkBase * linkDown) {
        linkUp_ = linkUp;
        linkDown_ = linkDown;
    }

    /* Setup debug info (this is cache-wide) */
    void setDebug(std::set<Addr> debugAddr) {
        DEBUG_ADDR = debugAddr;
    }

    /***** Statistics *****/
    virtual void recordLatency(Command cmd, State state, uint64_t latency);
    virtual void recordEventSentUp(Command cmd) =0;
    virtual void recordEventSentDown(Command cmd) =0;

    /**** Debug support *****/
    void printStatus(Output& out);

protected:
    struct Response {
        MemEventBase* event;
        uint64_t deliveryTime;
        uint64_t size;
    };

    /* Pointers to other subcomponents and cache structures */
    CacheListener*  listener_;
    MSHR *          mshr_;              // Pointer to cache's MSHR, coherence controllers are responsible for managing writeback acks

    /* Latency and timing related parameters */
    uint64_t        timestamp_;         // Local timestamp (cycles)
    uint64_t        accessLatency_;     // Cache access latency
    uint64_t        tagLatency_;        // Cache tag access latency
    uint64_t        mshrLatency_;       // MSHR lookup latency

    /* Outgoing event queues - events are stalled here to account for access latencies */
    list<Response> outgoingEventQueue_;
    list<Response> outgoingEventQueueUp_;

    /* Debug control */
    std::set<Addr>  DEBUG_ADDR;

    /* Output */
    Output*     output;
    Output*     debug;

    /* Parameters controlling how this cache interacts with the one below it */
    bool            writebackCleanBlocks_;  // Writeback clean data as opposed to just a coherence msg
    bool            silentEvictClean_;      // Silently evict clean blocks (currently ok when just mem below us)
    bool            expectWritebackAck_;    // Whether we should expect a writeback ack
    bool            lastLevel_;             // Whether we are the lowest coherence level and should not send coherence messages down

    /* General parameters and structures */
    uint64_t lineSize_;

    /* Throughput control TODO move these to a port manager */
    uint64_t maxBytesUp;
    uint64_t maxBytesDown;
    uint64_t packetHeaderBytes;

    /***** Functions used by child classes *****/

    /* Add a new event to the outgoing command queue towards memory */
    virtual void addToOutgoingQueue(Response& resp);

    /* Add a new event to the outgoing command queue towards the CPU */
    virtual void addToOutgoingQueueUp(Response& resp);

    /* Statistics */
    virtual void recordStateEventCount(Command cmd, State state);
    virtual void recordEvictionState(State state);

    /* Listener callback */
    virtual void notifyListenerOfAccess(MemEvent * event, NotifyAccessType accessT, NotifyResultType resultT);


    // Eviction statistics, count how many times we attempted to evict a block in a particular state
    Statistic<uint64_t>* stat_evict_I;
    Statistic<uint64_t>* stat_evict_E;
    Statistic<uint64_t>* stat_evict_M;
    Statistic<uint64_t>* stat_evict_IS;
    Statistic<uint64_t>* stat_evict_IM;
    Statistic<uint64_t>* stat_evict_IB;
    Statistic<uint64_t>* stat_evict_SB;

    /* Latency statistics */
    Statistic<uint64_t>* stat_latency_GetS_IS;
    Statistic<uint64_t>* stat_latency_GetS_M;
    Statistic<uint64_t>* stat_latency_GetX_IM;
    Statistic<uint64_t>* stat_latency_GetX_SM;
    Statistic<uint64_t>* stat_latency_GetX_M;
    Statistic<uint64_t>* stat_latency_GetSX_IM;
    Statistic<uint64_t>* stat_latency_GetSX_SM;
    Statistic<uint64_t>* stat_latency_GetSX_M;

    /* Prefetch statistics */
    Statistic<uint64_t>* statPrefetchEvict;
    Statistic<uint64_t>* statPrefetchInv;
    Statistic<uint64_t>* statPrefetchRedundant;
    Statistic<uint64_t>* statPrefetchUpgradeMiss;
    Statistic<uint64_t>* statPrefetchHit;
private:
    MemLinkBase * linkUp_;
    MemLinkBase * linkDown_;
};

}}

#endif	/* COHERENCECONTROLLER_H */
