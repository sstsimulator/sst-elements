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


#include <sst/core/sst_config.h>

#include "coherencemgr/coherenceController.h"

using namespace SST;
using namespace SST::MemHierarchy;

/* Debug macros included from util.h */


CoherenceController::CoherenceController(ComponentId_t id, Params &params, Params& ownerParams, bool prefetch) : SubComponent(id) {
    params.insert(ownerParams); // Combine params

    /* Output stream */
    output_= new Output("", 1, 0, SST::Output::STDOUT);

    /* Debug stream */
    debug_ = new Output("", params.find<int>("debug_level", 1), 0, (Output::output_location_t)params.find<int>("debug", SST::Output::NONE));

    debug_level_ = params.find<int>("debug_level", 1);
    if (params.find<int>("debug", SST::Output::NONE) == SST::Output::NONE)
        debug_level_ = 0;

    bool found;

    /* Get latency parameters */
    accessLatency_ = params.find<uint64_t>("access_latency_cycles", 1, found);
    if (!found) {
        output_->fatal(CALL_INFO, -1, "%s, Param not specified: access_latency_cycles - this is the access time in cycles for the cache; if tag_latency is also specified, this is the data array access time\n",
                getName().c_str());
    }

    tagLatency_ = params.find<uint64_t>("tag_access_latency_cycles", accessLatency_);
    mshrLatency_ = params.find<uint64_t>("mshr_latency_cycles", 1); /* cacheFactory is currently checking/setting this for us */

    /* Get line size - already error checked by cacheFactory */
    lineSize_ = params.find<uint64_t>("cache_line_size", 64, found);

    /* Get throughput parameters */
    UnitAlgebra packetSize = UnitAlgebra(params.find<std::string>("min_packet_size", "8B"));
    UnitAlgebra downLinkBW = UnitAlgebra(params.find<std::string>("request_link_width", "0B"));
    UnitAlgebra upLinkBW = UnitAlgebra(params.find<std::string>("response_link_width", "0B"));

    if (!packetSize.hasUnits("B"))
        output_->fatal(CALL_INFO, -1, "%s, Invalid param: min_packet_size - must have units of bytes (B), SI units OK. Ex: '8B'. You specified '%s'\n", getName().c_str(), packetSize.toString().c_str());
    if (!downLinkBW.hasUnits("B"))
        output_->fatal(CALL_INFO, -1, "%s, Invalid param: request_link_width - must have units of bytes (B), SI units OK. Ex: '64B'. You specified '%s'\n", getName().c_str(), downLinkBW.toString().c_str());
    if (!upLinkBW.hasUnits("B"))
        output_->fatal(CALL_INFO, -1, "%s, Invalid param: response_link_width - must have units of bytes (B), SI units OK. Ex: '64B'. You specified '%s'\n", getName().c_str(), upLinkBW.toString().c_str());

    maxBytesUp = upLinkBW.getRoundedValue();
    maxBytesDown = downLinkBW.getRoundedValue();
    packetHeaderBytes = packetSize.getRoundedValue();

    /* Initialize variables */
    timestamp_ = 0;
    outstanding_prefetch_count_ = 0;

    /* Default values for cache parameters */
    // May be updated during init()
    lastLevel_ = true;
    silentEvictClean_ = true;
    writebackCleanBlocks_ = false;
    recvWritebackAck_ = false;
    sendWritebackAck_ = false;

    // The following cache parameters are set by the cache controller in its constructor
    // Just in case, we give an initial value here
    drop_prefetch_level_ = ((size_t) - 1);
    max_outstanding_prefetch_ = ((size_t) - 2);

    // Get parent component's name
    cachename_ = getParentComponentName();

    // Register statistics - only those that are common across all coherence managers
    // Give  all array entries a default statistic so we don't end up with segfaults during execution
    Statistic<uint64_t> * defStat = registerStatistic<uint64_t>("default_stat");
    for (int i = 0; i < (int)Command::LAST_CMD; i++) {
        stat_eventSent[i] = defStat;
        for (int j = 0; j < LAST_STATE; j++) {
            stat_eventstate_[i][j] = defStat;

            if (i == 0) {
                stat_evict[j] = defStat;
            }
        }
    }

    // Initialize event debug info (event_debuginfo_/evict_debuginfo_)
    evict_debuginfo_.id.first = 0;
    evict_debuginfo_.id.second = 0;
    evict_debuginfo_.thr = 0;
    evict_debuginfo_.cmd = Command::NULLCMD;
    evict_debuginfo_.mod = "";
    evict_debuginfo_.oldst = I;
    evict_debuginfo_.newst = I;
    evict_debuginfo_.action = "";
    evict_debuginfo_.reason = "";
    evict_debuginfo_.verboseline = "";
    event_debuginfo_.id.first = 0;
    event_debuginfo_.id.second = 0;
    event_debuginfo_.thr = 0;
    event_debuginfo_.cmd = Command::NULLCMD;
    event_debuginfo_.mod = "";
    event_debuginfo_.oldst = I;
    event_debuginfo_.newst = I;
    event_debuginfo_.action = "";
    event_debuginfo_.reason = "";
    event_debuginfo_.verboseline = "";
}

/*******************************************************************************
 * Initialization
 *******************************************************************************/
ReplacementPolicy* CoherenceController::createReplacementPolicy(uint64_t lines, uint64_t assoc, Params& params, bool L1, int slotnum) {
    SubComponentSlotInfo* rslots = getSubComponentSlotInfo("replacement");
    if (rslots && rslots->isPopulated(slotnum))
        return rslots->create<ReplacementPolicy>(slotnum, ComponentInfo::SHARE_NONE, lines, assoc);

    // Default to the replacement policy that was used before all the recent memH changes
    Params emptyparams;
    std::string policy = params.find<std::string>("replacement_policy", "lru");
    to_lower(policy);

    if (policy == "lru") {
        if (L1) return loadAnonymousSubComponent<ReplacementPolicy>("memHierarchy.replacement.lru", "replacement", slotnum, ComponentInfo::SHARE_NONE, emptyparams, lines, assoc);
        else    return loadAnonymousSubComponent<ReplacementPolicy>("memHierarchy.replacement.lru-opt", "replacement", slotnum, ComponentInfo::SHARE_NONE, emptyparams, lines, assoc);
    }
    if (policy == "lfu") {
        if (L1) return loadAnonymousSubComponent<ReplacementPolicy>("memHierarchy.replacement.lfu", "replacement", slotnum, ComponentInfo::SHARE_NONE, emptyparams, lines, assoc);
        else    return loadAnonymousSubComponent<ReplacementPolicy>("memHierarchy.replacement.lfu-opt", "replacement", slotnum, ComponentInfo::SHARE_NONE, emptyparams, lines, assoc);
    }
    if (policy == "mru") {
        if (L1) return loadAnonymousSubComponent<ReplacementPolicy>("memHierarchy.replacement.mru", "replacement", slotnum, ComponentInfo::SHARE_NONE, emptyparams, lines, assoc);
        else    return loadAnonymousSubComponent<ReplacementPolicy>("memHierarchy.replacement.mru-opt", "replacement", slotnum, ComponentInfo::SHARE_NONE, emptyparams, lines, assoc);
    }
    if (policy == "random") return loadAnonymousSubComponent<ReplacementPolicy>("memHierarchy.replacement.random", "replacement", slotnum, ComponentInfo::SHARE_NONE, emptyparams, lines, assoc);
    if (policy == "nmru")   return loadAnonymousSubComponent<ReplacementPolicy>("memHierarchy.replacement.nmru", "replacement", slotnum, ComponentInfo::SHARE_NONE, emptyparams, lines, assoc);

    debug_->fatal(CALL_INFO, -1, "%s, Invalid param: replacement_policy - supported policies are 'lru', 'lfu', 'random', 'mru', and 'nmru'. You specified '%s'.\n", getName().c_str(), policy.c_str());
    return nullptr;
}

HashFunction* CoherenceController::createHashFunction(Params& params) {
    HashFunction * ht = loadUserSubComponent<HashFunction>("hash");
    if (ht) return ht;

    Params hparams;
    int hashFunc = params.find<int>("hash_function", 0);
    if (hashFunc == 1)  ht = loadAnonymousSubComponent<HashFunction>("memHierarchy.hash.linear", "hash", 0, ComponentInfo::SHARE_NONE, hparams);
    if (hashFunc == 2)  ht = loadAnonymousSubComponent<HashFunction>("memHierarchy.hash.xor", "hash", 0, ComponentInfo::SHARE_NONE, hparams);
    else                ht = loadAnonymousSubComponent<HashFunction>("memHierarchy.hash.none", "hash", 0, ComponentInfo::SHARE_NONE, hparams);

    return ht;
}


/*******************************************************************************
 * Event handlers - one per event type
 * Handlers return whether event was accepted (true) or rejected (false)
 *******************************************************************************/
bool CoherenceController::handleGetS(MemEvent* event, bool inMSHR) {
    debug_->fatal(CALL_INFO, -1, "%s, Error: GetS events are not handled by this coherence manager. Event: %s. Time: %" PRIu64 "ns.\n",
            getName().c_str(), event->getVerboseString().c_str(), getCurrentSimTimeNano());
    return false;
}

bool CoherenceController::handleGetX(MemEvent* event, bool inMSHR) {
    debug_->fatal(CALL_INFO, -1, "%s, Error: GetX events are not handled by this coherence manager. Event: %s. Time: %" PRIu64 "ns.\n",
            getName().c_str(), event->getVerboseString().c_str(), getCurrentSimTimeNano());
    return false;
}

bool CoherenceController::handleGetSX(MemEvent* event, bool inMSHR) {
    debug_->fatal(CALL_INFO, -1, "%s, Error: GetSX events are not handled by this coherence manager. Event: %s. Time: %" PRIu64 "ns.\n",
            getName().c_str(), event->getVerboseString().c_str(), getCurrentSimTimeNano());
    return false;
}

bool CoherenceController::handleWrite(MemEvent* event, bool inMSHR) {
    debug_->fatal(CALL_INFO, -1, "%s, Error: Write events are not handled by this coherence manager. Event: %s. Time: %" PRIu64 "ns.\n",
            getName().c_str(), event->getVerboseString().c_str(), getCurrentSimTimeNano());
    return false;
}

bool CoherenceController::handleFlushLine(MemEvent* event, bool inMSHR) {
    debug_->fatal(CALL_INFO, -1, "%s, Error: FlushLine events are not handled by this coherence manager. Event: %s. Time: %" PRIu64 "ns.\n",
            getName().c_str(), event->getVerboseString().c_str(), getCurrentSimTimeNano());
    return false;
}

bool CoherenceController::handleFlushLineInv(MemEvent* event, bool inMSHR) {
    debug_->fatal(CALL_INFO, -1, "%s, Error: FlushLineInv events are not handled by this coherence manager. Event: %s. Time: %" PRIu64 "ns.\n",
            getName().c_str(), event->getVerboseString().c_str(), getCurrentSimTimeNano());
    return false;
}

bool CoherenceController::handleFlushAll(MemEvent* event, bool inMSHR) {
    debug_->fatal(CALL_INFO, -1, "%s, Error: FlushAll events are not handled by this coherence manager. Event: %s. Time: %" PRIu64 "ns.\n",
            getName().c_str(), event->getVerboseString().c_str(), getCurrentSimTimeNano());
    return false;
}

bool CoherenceController::handleForwardFlush(MemEvent* event, bool inMSHR) {
    debug_->fatal(CALL_INFO, -1, "%s, Error: ForwardFlush events are not handled by this coherence manager. Event: %s. Time: %" PRIu64 "ns.\n",
            getName().c_str(), event->getVerboseString().c_str(), getCurrentSimTimeNano());
    return false;
}

bool CoherenceController::handlePutS(MemEvent* event, bool inMSHR) {
    debug_->fatal(CALL_INFO, -1, "%s, Error: PutS events are not handled by this coherence manager. Event: %s. Time: %" PRIu64 "ns.\n",
            getName().c_str(), event->getVerboseString().c_str(), getCurrentSimTimeNano());
    return false;
}

bool CoherenceController::handlePutX(MemEvent* event, bool inMSHR) {
    debug_->fatal(CALL_INFO, -1, "%s, Error: PutX events are not handled by this coherence manager. Event: %s. Time: %" PRIu64 "ns.\n",
            getName().c_str(), event->getVerboseString().c_str(), getCurrentSimTimeNano());
    return false;
}

bool CoherenceController::handlePutE(MemEvent* event, bool inMSHR) {
    debug_->fatal(CALL_INFO, -1, "%s, Error: PutE events are not handled by this coherence manager. Event: %s. Time: %" PRIu64 "ns.\n",
            getName().c_str(), event->getVerboseString().c_str(), getCurrentSimTimeNano());
    return false;
}

bool CoherenceController::handlePutM(MemEvent* event, bool inMSHR) {
    debug_->fatal(CALL_INFO, -1, "%s, Error: PutM events are not handled by this coherence manager. Event: %s. Time: %" PRIu64 "ns.\n",
            getName().c_str(), event->getVerboseString().c_str(), getCurrentSimTimeNano());
    return false;
}

bool CoherenceController::handleGetSResp(MemEvent* event, bool inMSHR) {
    debug_->fatal(CALL_INFO, -1, "%s, Error: GetSResp events are not handled by this coherence manager. Event: %s. Time: %" PRIu64 "ns.\n",
            getName().c_str(), event->getVerboseString().c_str(), getCurrentSimTimeNano());
    return false;
}

bool CoherenceController::handleGetXResp(MemEvent* event, bool inMSHR) {
    debug_->fatal(CALL_INFO, -1, "%s, Error: GetXResp events are not handled by this coherence manager. Event: %s. Time: %" PRIu64 "ns.\n",
            getName().c_str(), event->getVerboseString().c_str(), getCurrentSimTimeNano());
    return false;
}

bool CoherenceController::handleWriteResp(MemEvent* event, bool inMSHR) {
    debug_->fatal(CALL_INFO, -1, "%s, Error: WriteResp events are not handled by this coherence manager. Event: %s. Time: %" PRIu64 "ns.\n",
            getName().c_str(), event->getVerboseString().c_str(), getCurrentSimTimeNano());
    return false;
}

bool CoherenceController::handleFlushLineResp(MemEvent* event, bool inMSHR) {
    debug_->fatal(CALL_INFO, -1, "%s, Error: FlushLineResp events are not handled by this coherence manager. Event: %s. Time: %" PRIu64 "ns.\n",
            getName().c_str(), event->getVerboseString().c_str(), getCurrentSimTimeNano());
    return false;
}

bool CoherenceController::handleFlushAllResp(MemEvent* event, bool inMSHR) {
    debug_->fatal(CALL_INFO, -1, "%s, Error: FlushAllResp events are not handled by this coherence manager. Event: %s. Time: %" PRIu64 "ns.\n",
            getName().c_str(), event->getVerboseString().c_str(), getCurrentSimTimeNano());
    return false;
}

bool CoherenceController::handleAckFlush(MemEvent* event, bool inMSHR) {
    debug_->fatal(CALL_INFO, -1, "%s, Error: AckFlush events are not handled by this coherence manager. Event: %s. Time: %" PRIu64 "ns.\n",
            getName().c_str(), event->getVerboseString().c_str(), getCurrentSimTimeNano());
    return false;
}

bool CoherenceController::handleUnblockFlush(MemEvent* event, bool inMSHR) {
    debug_->fatal(CALL_INFO, -1, "%s, Error: UnblockFlush events are not handled by this coherence manager. Event: %s. Time: %" PRIu64 "ns.\n",
            getName().c_str(), event->getVerboseString().c_str(), getCurrentSimTimeNano());
    return false;
}

bool CoherenceController::handleAckPut(MemEvent* event, bool inMSHR) {
    debug_->fatal(CALL_INFO, -1, "%s, Error: AckPut events are not handled by this coherence manager. Event: %s. Time: %" PRIu64 "ns.\n",
            getName().c_str(), event->getVerboseString().c_str(), getCurrentSimTimeNano());
    return false;
}

bool CoherenceController::handleFetch(MemEvent* event, bool inMSHR) {
    debug_->fatal(CALL_INFO, -1, "%s, Error: Fetch events are not handled by this coherence manager. Event: %s. Time: %" PRIu64 "ns.\n",
            getName().c_str(), event->getVerboseString().c_str(), getCurrentSimTimeNano());
    return false;
}

bool CoherenceController::handleFetchInv(MemEvent* event, bool inMSHR) {
    debug_->fatal(CALL_INFO, -1, "%s, Error: FetchInv events are not handled by this coherence manager. Event: %s. Time: %" PRIu64 "ns.\n",
            getName().c_str(), event->getVerboseString().c_str(), getCurrentSimTimeNano());
    return false;
}

bool CoherenceController::handleFetchInvX(MemEvent* event, bool inMSHR) {
    debug_->fatal(CALL_INFO, -1, "%s, Error: FetchInvX events are not handled by this coherence manager. Event: %s. Time: %" PRIu64 "ns.\n",
            getName().c_str(), event->getVerboseString().c_str(), getCurrentSimTimeNano());
    return false;
}

bool CoherenceController::handleForceInv(MemEvent* event, bool inMSHR) {
    debug_->fatal(CALL_INFO, -1, "%s, Error: ForceInv events are not handled by this coherence manager. Event: %s. Time: %" PRIu64 "ns.\n",
            getName().c_str(), event->getVerboseString().c_str(), getCurrentSimTimeNano());
    return false;
}

bool CoherenceController::handleInv(MemEvent* event, bool inMSHR) {
    debug_->fatal(CALL_INFO, -1, "%s, Error: Inv events are not handled by this coherence manager. Event: %s. Time: %" PRIu64 "ns.\n",
            getName().c_str(), event->getVerboseString().c_str(), getCurrentSimTimeNano());
    return false;
}

bool CoherenceController::handleFetchResp(MemEvent* event, bool inMSHR) {
    debug_->fatal(CALL_INFO, -1, "%s, Error: FetchResp events are not handled by this coherence manager. Event: %s. Time: %" PRIu64 "ns.\n",
            getName().c_str(), event->getVerboseString().c_str(), getCurrentSimTimeNano());
    return false;
}

bool CoherenceController::handleFetchXResp(MemEvent* event, bool inMSHR) {
    debug_->fatal(CALL_INFO, -1, "%s, Error: FetchXResp events are not handled by this coherence manager. Event: %s. Time: %" PRIu64 "ns.\n",
            getName().c_str(), event->getVerboseString().c_str(), getCurrentSimTimeNano());
    return false;
}

bool CoherenceController::handleAckInv(MemEvent* event, bool inMSHR) {
    debug_->fatal(CALL_INFO, -1, "%s, Error: AckInv events are not handled by this coherence manager. Event: %s\n. Time: %" PRIu64 "ns.",
            getName().c_str(), event->getVerboseString().c_str(), getCurrentSimTimeNano());
    return false;
}

bool CoherenceController::handleNULLCMD(MemEvent* event, bool inMSHR) {
    debug_->fatal(CALL_INFO, -1, "%s, Error: NULLCMD events are not handled by this coherence manager. Event: %s. Time: %" PRIu64 "ns.\n",
            getName().c_str(), event->getVerboseString().c_str(), getCurrentSimTimeNano());
    return false;
}

bool CoherenceController::handleNACK(MemEvent* event, bool inMSHR) {
    debug_->fatal(CALL_INFO, -1, "%s, Error: NACK events are not handled by this coherence manager. Event: %s. Time: %" PRIu64 "ns.\n",
            getName().c_str(), event->getVerboseString().c_str(), getCurrentSimTimeNano());
    return false;
}


/*******************************************************************************
 * Send events
 *******************************************************************************/

/* Send commands when their timestamp expires. Return whether queue is empty or not. */
bool CoherenceController::sendOutgoingEvents() {
    // Update timestamp
    timestamp_++;

    // Check for ready events in outgoing 'down' queue
    uint64_t bytesLeft = maxBytesDown;
    while (!outgoing_event_queue_down_.empty() && outgoing_event_queue_down_.front().deliveryTime <= timestamp_) {
        MemEventBase *outgoingEvent = outgoing_event_queue_down_.front().event;
        if (maxBytesDown != 0) {
            if (bytesLeft == 0) break;
            if (bytesLeft >= outgoing_event_queue_down_.front().size) {
                bytesLeft -= outgoing_event_queue_down_.front().size;  // Send this many bytes
            } else {
                outgoing_event_queue_down_.front().size -= bytesLeft;
                break;
            }
        }


        if (mem_h_is_debug_event(outgoingEvent)) {
            debug_->debug(_L4_, "E: %-20" PRIu64 " %-20" PRIu64 " %-20s Event:Send    (%s)\n",
                    getCurrentSimCycle(), timestamp_, cachename_.c_str(), outgoingEvent->getBriefString().c_str());
        }

        link_down_->send(outgoingEvent);
        outgoing_event_queue_down_.pop_front();

    }

    // Check for ready events in outgoing 'up' queue
    bytesLeft = maxBytesUp;
    while (!outgoing_event_queue_up_.empty() && outgoing_event_queue_up_.front().deliveryTime <= timestamp_) {
        MemEventBase * outgoingEvent = outgoing_event_queue_up_.front().event;
        if (maxBytesUp != 0) {
            if (bytesLeft == 0) break;
            if (bytesLeft >= outgoing_event_queue_up_.front().size) {
                bytesLeft -= outgoing_event_queue_up_.front().size;
            } else {
                outgoing_event_queue_up_.front().size -= bytesLeft;
                break;
            }
        }

        if (mem_h_is_debug_event(outgoingEvent)) {
            debug_->debug(_L4_, "E: %-20" PRIu64 " %-20" PRIu64 " %-20s Event:Send    (%s)\n",
                    getCurrentSimCycle(), timestamp_, cachename_.c_str(), outgoingEvent->getBriefString().c_str());
        }

        if (startTimes_.find(outgoingEvent->getResponseToID()) != startTimes_.end()) {
            LatencyStat stat = startTimes_.find(outgoingEvent->getResponseToID())->second;
            recordLatency(stat.cmd, stat.missType, timestamp_ - stat.time);
            startTimes_.erase(outgoingEvent->getResponseToID());
        }

        link_up_->send(outgoingEvent);
        outgoing_event_queue_up_.pop_front();
    }

    // Return whether it's ok for the cache to turn off the clock - we need it on to be able to send waiting events
    return outgoing_event_queue_down_.empty() && outgoing_event_queue_up_.empty();
}

bool CoherenceController::checkIdle() {
    return outgoing_event_queue_down_.empty() && outgoing_event_queue_up_.empty();
}


/* Forward an event using memory address to locate a destination. */
void CoherenceController::forwardByAddress(MemEventBase * event) {
    forwardByAddress(event, timestamp_ + 1);
}

void CoherenceController::forwardByAddress(MemEventBase * event, Cycle_t ts) {
    event->setSrc(cachename_);
    std::string dst = link_down_->findTargetDestination(event->getRoutingAddress());
    if (dst != "") { /* Common case */
        event->setDst(dst);
        Response fwdReq = {event, ts, packetHeaderBytes + event->getPayloadSize()};
        addToOutgoingQueue(fwdReq);
    } else {
        dst = link_up_->findTargetDestination(event->getRoutingAddress());
        if (dst != "") {
            event->setDst(dst);
            Response fwdReq = {event, ts, packetHeaderBytes + event->getPayloadSize()};
            addToOutgoingQueueUp(fwdReq);
        } else {
            std::string availableDests = "cpulink:\n" + link_up_->getAvailableDestinationsAsString();
            if (link_up_ != link_down_) availableDests = availableDests + "memlink:\n" + link_down_->getAvailableDestinationsAsString();
            output_->fatal(CALL_INFO, -1, "%s, Error: Unable to find destination for address 0x%" PRIx64 ". Event: %s\nKnown Destinations: %s\n",
                    getName().c_str(), event->getRoutingAddress(), event->getVerboseString().c_str(), availableDests.c_str());
        }
    }

}

/* Forward an event to a specific destination */
void CoherenceController::forwardByDestination(MemEventBase * event) {
    forwardByDestination(event, timestamp_ + 1);
}

/* Forward an event to a specific destination */
void CoherenceController::forwardByDestination(MemEventBase * event, Cycle_t ts) {
    event->setSrc(cachename_);
    Response fwdReq = {event, ts, packetHeaderBytes + event->getPayloadSize()};

    if (link_up_->isReachable(event->getDst())) {
        addToOutgoingQueueUp(fwdReq);
    } else if (link_down_->isReachable(event->getDst())) {
        addToOutgoingQueue(fwdReq);
    } else {
        output_->fatal(CALL_INFO, -1, "%s, Error: Destination %s appears unreachable on both links. Event: %s\n",
                getName().c_str(), event->getDst().c_str(), event->getVerboseString().c_str());
    }
}

/* Broadcast an event to all sources */
int CoherenceController::broadcastMemEventToSources(Command cmd, MemEvent* metadata, Cycle_t ts) {
    std::set<MemLinkBase::EndpointInfo>* sources = link_up_->getSources();
    for (auto it = sources->begin(); it != sources->end(); it++) {
        MemEvent* event = new MemEvent(cachename_, cmd);
        if (metadata) event->copyMetadata(metadata);
        event->setSrc(cachename_);
        event->setDst(it->name);
        forwardByDestination(event, ts);
    }
    return sources->size();
}

int CoherenceController::broadcastMemEventToPeers(Command cmd, MemEvent* metadata, Cycle_t ts) {
    std::set<MemLinkBase::EndpointInfo>* peers = link_up_->getPeers();
    int sent = 0;
    for (auto it = peers->begin(); it != peers->end(); it++) {
        if (it->name == cachename_) continue;

        MemEvent* event = new MemEvent(cachename_, cmd);
        if (metadata) event->copyMetadata(metadata);
        event->setSrc(cachename_);
        event->setDst(it->name);
        forwardByDestination(event, ts);
        sent++;
    }
    return sent;
}

/*******************************************************************************
 * Initialization/finish functions used by parent
 *******************************************************************************/

/* Parse an init coherence vent. Coherence managers can override this */
void CoherenceController::processInitCoherenceEvent(MemEventInitCoherence* event, bool source) {
    // If something besides memory is below us, we are not the last level doing coherence
    if (!source && event->getType() != Endpoint::Memory)
        lastLevel_ = false;

    // If the component below us tracks whether a block is present above, then we can't silently evict (per current protocols)
    if (!source && (event->getTracksPresence() || lastLevel_))
        silentEvictClean_ = false;

    // The component below us does not neccessarily keep a copy of data, therefore we need to write back data when evicting clean blocks
    if ((!source && !event->getInclusive()) || (!source && event->getType() == Endpoint::Directory))
        writebackCleanBlocks_ = true;

    // The component below us will send writeback acks, we should wait for them
    if (!source && event->getSendWBAck())
        recvWritebackAck_ = true;

    if (source && event->getRecvWBAck())
        sendWritebackAck_ = true;

    // Track CPU names so we can broadcast L1 invalidation snoops if needed
    if (source && (event->getType() == Endpoint::CPU || event->getType() == Endpoint::MMIO))
        cpus.insert(event->getSrc());

    debug_->debug(_L3_, "%s processInitCoherenceEvent. Result is (source=%s): LL ? %s, silentEvict ? %s, WBClean ? %s, sendWBAck ? %s, recvWBAck ? %s\n",
            cachename_.c_str(),
            source ? "true" : "false",
            lastLevel_ ? "Y" : "N",
            silentEvictClean_ ? "Y" : "N",
            writebackCleanBlocks_ ? "Y" : "N",
            sendWritebackAck_ ? "Y" : "N",
            recvWritebackAck_ ? "Y" : "N");
}

void CoherenceController::setup() {
    /* Identify if this cache is the flush manager, and, if not, the destination for any flushes */
    flush_manager_ = lastLevel_;
    flush_helper_ = true;
    bool global_peer = lastLevel_;
    /* Identify the local flush helper in our group of peers */
    MemLinkBase::EndpointInfo min = link_up_->getEndpointInfo();
    auto peers = link_up_->getPeers();
    for (auto it = peers->begin(); it != peers->end(); it++) {
        if (*it < min) {
            min = *it;
            flush_manager_ = false;
            flush_helper_ = false;
        }
    }

    if (flush_manager_) {
        flush_dest_ = getName();
    } else if (lastLevel_) { // If true, the global flush manager is one of our peers
        flush_dest_ = min.name;
    } else {
        auto dests = link_down_->getDests();
        min = *(dests->begin());
        for (auto it = dests->begin(); it != dests->end(); it++) {
            if (*it < min) {
                min = *it;
            }
        }
        flush_dest_ = min.name;
    }
}

void CoherenceController::processCompleteEvent(MemEventInit* event, MemLinkBase* highlink, MemLinkBase* lowlink) {
    if (event->getInitCmd() == MemEventInit::InitCommand::Flush) {
        debug_->output("Complete Event (%s): %s\n", getName().c_str(), event->getVerboseString().c_str());
    }
    delete event; // Nothing for now
}

/* Retry buffer */
std::vector<MemEventBase*>* CoherenceController::getRetryBuffer() {
    return &retryBuffer_;
}

void CoherenceController::clearRetryBuffer() {
    retryBuffer_.clear();
}


/* Listener callbacks */
void CoherenceController::notifyListenerOfAccess(MemEvent * event, NotifyAccessType accessT, NotifyResultType resultT) {
    if (event->isPrefetch())
        accessT = NotifyAccessType::PREFETCH;

    CacheListenerNotification notify(event->getAddr(), event->getBaseAddr(), event->getVirtualAddress(),
            event->getInstructionPointer(), event->getSize(), accessT, resultT);

    for (int i = 0; i < listeners_.size(); i++)
        listeners_[i]->notifyAccess(notify);
}


void CoherenceController::notifyListenerOfEvict(Addr addr, uint32_t size, Addr ip) {
    CacheListenerNotification notify(addr, addr, 0, ip, size, EVICT, NA);

    for (int i = 0; i < listeners_.size(); i++) {
        listeners_[i]->notifyAccess(notify);
    }
}


/* Forward a message to a lower level (towards memory) in the hierarchy */
uint64_t CoherenceController::forwardMessage(MemEvent * event, unsigned int requestSize, uint64_t baseTime, vector<uint8_t>* data, Command fwdCmd) {
    /* Create event to be forwarded */
    MemEvent* forwardEvent;
    forwardEvent = new MemEvent(*event);

    if (fwdCmd != Command::LAST_CMD) {
        forwardEvent->setCmd(fwdCmd);
    }

    if (data == nullptr) forwardEvent->setPayload(0, nullptr);

    forwardEvent->setSize(requestSize);

    if (data != nullptr) forwardEvent->setPayload(*data);

    /* Determine latency in cycles */
    uint64_t deliveryTime;
    if (baseTime < timestamp_) baseTime = timestamp_;
    if (event->queryFlag(MemEvent::F_NONCACHEABLE)) {
        forwardEvent->setFlag(MemEvent::F_NONCACHEABLE);
        deliveryTime = timestamp_ + mshrLatency_;
    } else deliveryTime = baseTime + tagLatency_;

    forwardByAddress(forwardEvent, deliveryTime);

    if (mem_h_is_debug_event(event))
        event_debuginfo_.action = "Forward";

    return deliveryTime;
}

/* Send a NACK event */
void CoherenceController::sendNACK(MemEvent * event) {
    MemEvent * NACKevent = event->makeNACKResponse(event);

    uint64_t deliveryTime = timestamp_ + tagLatency_; // Probably had to lookup and see that we couldn't handle this request and/or MSHR was full
    forwardByDestination(NACKevent, deliveryTime);

    if (mem_h_is_debug_event(event))
        event_debuginfo_.action = "NACK";
}


/* Resend an event after a NACK */
void CoherenceController::resendEvent(MemEvent * event, bool towardsCPU) {
    // Calculate backoff - avoids flooding links
    int retries = event->getRetries();
    if (retries > 10) retries = 10;
    uint64_t backoff = ( 0x1 << retries);
    event->incrementRetries();

    uint64_t deliveryTime =  timestamp_ + mshrLatency_ + backoff;
    forwardByDestination(event, deliveryTime);

    if (mem_h_is_debug_event(event)) {
        event_debuginfo_.action = "Resend";
        event_debuginfo_.reason = event->getBriefString();
    }
}


/* Send response up (towards CPU). L1s need to implement their own to split out the requested block */
uint64_t CoherenceController::sendResponseUp(MemEvent * event, vector<uint8_t>* data, bool replay, uint64_t baseTime, bool success) {
    return sendResponseUp(event, CommandResponse[(int)event->getCmd()], data, false, replay, baseTime, success);
}


/* Send response up (towards CPU). L1s need to implement their own to split out the requested block */
uint64_t CoherenceController::sendResponseUp(MemEvent * event, Command cmd, vector<uint8_t>* data, bool replay, uint64_t baseTime, bool success) {
    return sendResponseUp(event, cmd, data, false, replay, baseTime, success);
}


/* Send response towards the CPU. L1s need to implement their own to split out the requested block */
uint64_t CoherenceController::sendResponseUp(MemEvent * event, Command cmd, vector<uint8_t>* data, bool dirty, bool replay, uint64_t baseTime, bool success) {
    MemEvent * responseEvent = event->makeResponse(cmd);
    responseEvent->setSize(event->getSize());
    if (data != nullptr) responseEvent->setPayload(*data);
    responseEvent->setDirty(dirty);

    if (!success)
        responseEvent->setFail();

    if (baseTime < timestamp_) baseTime = timestamp_;
    uint64_t deliveryTime = baseTime + (replay ? mshrLatency_ : accessLatency_);
    forwardByDestination(responseEvent, deliveryTime);

    return deliveryTime;
}

/* Return the name of the source for this cache */
std::string CoherenceController::getSrc() {
    return link_up_->getSources()->begin()->name;
}

/* Allocate an MSHR entry for an event */
MemEventStatus CoherenceController::allocateMSHR(MemEvent * event, bool fwdReq, int pos, bool stallEvict) {
    // Screen prefetches first to ensure limits are not exceeeded:
    //      - Maximum number of outstanding prefetches
    //      - MSHR too full to accept prefetches
    if (event->isPrefetch() && event->getRqstr() == cachename_) {
        if (drop_prefetch_level_ <= mshr_->getSize()) {
            event_debuginfo_.action = "Reject";
            event_debuginfo_.reason = "Prefetch drop level";
            return MemEventStatus::Reject;
        }
        if (max_outstanding_prefetch_ <= outstanding_prefetch_count_) {
            event_debuginfo_.action = "Reject";
            event_debuginfo_.reason = "Max outstanding prefetches";
            return MemEventStatus::Reject;
        }
    }

    int end_pos = mshr_->insertEvent(event->getBaseAddr(), event, pos, fwdReq, stallEvict);
    if (end_pos == -1) {
        if (mem_h_is_debug_event(event)) {
            event_debuginfo_.action = "Reject";
            event_debuginfo_.reason = "MSHR full";
        }
        return MemEventStatus::Reject; // MSHR is full
    } else if (end_pos != 0) {
        if (mem_h_is_debug_event(event)) {
            event_debuginfo_.action = "Stall";
            event_debuginfo_.reason = "MSHR conflict";
        }
        if (event->isPrefetch() && event->getRqstr() == cachename_) {
            outstanding_prefetch_count_++;
        }
        return MemEventStatus::Stall;
    }

    if (event->isPrefetch() && event->getRqstr() == cachename_) {
        outstanding_prefetch_count_++;
    }
    return MemEventStatus::OK;
}


/*******************************************************************************
 * Debug and info
 *******************************************************************************/

// Format:
// TIME     CYCLE       COMPNAME    ID  ADDR    CMD OLDSTATE    NEWSTATE    ACTION  (REASON)    VERBOSE_LINE_STATE
//
// Prints at debug level 5
// VERBOSE_LINE_STATE is only printed at debug level 6
void CoherenceController::printDebugInfo() {
    printDebugInfo(&event_debuginfo_);
}

void CoherenceController::printDebugInfo(dbgin * debuginfo) {
    if (debug_level_ < 5)
        return;

    std::string cmd = CommandString[(int)debuginfo->cmd];
    cmd += debuginfo->mod;

    std::stringstream id;
    id << "<" << debuginfo->id.first << "," << debuginfo->id.second << ">";

    stringstream reas;
    reas << "(" << debuginfo->reason << ")";

    stringstream thr;
    if (debuginfo->hasThr)
        thr << debuginfo->thr;
    else
        thr << "";

    debug_->debug(_L5_, "C: %-20" PRIu64 " %-20" PRIu64 " %-20s %-4s %-13s 0x%-16" PRIx64 " %-15s %-6s %-6s %-10s %-15s",
            getCurrentSimCycle(), timestamp_, cachename_.c_str(), thr.str().c_str(), cmd.c_str(), debuginfo->addr,
            id.str().c_str(), StateString[debuginfo->oldst], StateString[debuginfo->newst], debuginfo->action.c_str(), reas.str().c_str());

    debug_->debug(_L6_, " %s", debuginfo->verboseline.c_str());
    debug_->debug(_L5_, "\n");
}

void CoherenceController::printDebugAlloc(bool alloc, Addr addr, std::string note) {
    if (debug_level_ < 5)
        return;

    std::string action = alloc ? "Alloc" : "Dealloc";

    debug_->debug(_L5_, "C: %-20" PRIu64 " %-20" PRIu64 " %-25s %-13s 0x%-16" PRIx64 "",
            getCurrentSimCycle(), timestamp_, cachename_.c_str(), action.c_str(), addr);

    if (note != "")
        debug_->debug(_L5_, " %s\n", note.c_str());
    else
        debug_->debug(_L5_, "\n");
}

void CoherenceController::printDataValue(Addr addr, vector<uint8_t> * data, bool set) {
    if (debug_level_ < 11)
        return;

    std::string action = set ? "WRITE" : "READ";
    std::stringstream value;
    value << std::hex << std::setfill('0');
    for (unsigned int i = 0; i < data->size(); i++) {
        value << std::hex << std::setw(2) << (int)data->at(i);
    }

    debug_->debug(_L11_, "V: %-20" PRIu64 " %-20" PRIu64 " %-20s %-13s 0x%-16" PRIx64 " B: %-3zu %s\n",
            getCurrentSimCycle(), timestamp_, cachename_.c_str(), action.c_str(),
            addr, data->size(), value.str().c_str());
/*
    for (unsigned int i = 0; i < data->size(); i++) {
        printf("%02x", data->at(i));
    }
    */
}

void CoherenceController::printStatus(Output& out) {
    out.output("  Begin MemHierarchy::CoherenceController %s\n", getName().c_str());

    out.output("    Events waiting in outgoingEventQueueDown: %zu\n", outgoing_event_queue_down_.size());
    for (list<Response>::iterator it = outgoing_event_queue_down_.begin(); it!= outgoing_event_queue_down_.end(); it++) {
        out.output("      Time: %" PRIu64 ", Event: %s\n", (*it).deliveryTime, (*it).event->getVerboseString().c_str());
    }

    out.output("    Events waiting in outgoingEventQueueUp: %zu\n", outgoing_event_queue_up_.size());
    for (list<Response>::iterator it = outgoing_event_queue_up_.begin(); it!= outgoing_event_queue_up_.end(); it++) {
        out.output("      Time: %" PRIu64 ", Event: %s\n", (*it).deliveryTime, (*it).event->getVerboseString().c_str());
    }

    out.output("  End MemHierarchy::CoherenceController\n");
}

/**************************************/
/******* Manage outgoing events *******/
/**************************************/



/* Add a new event to the outgoing queue down (towards memory)
 * Add in timestamp order but do not re-order for events to the same address
 * Cache lines/banks mostly take care of this, except when we invalidate
 * a block and then re-request it, the requests can get inverted.
 */
void CoherenceController::addToOutgoingQueue(Response& resp) {
    list<Response>::reverse_iterator rit;
    for (rit = outgoing_event_queue_down_.rbegin(); rit!= outgoing_event_queue_down_.rend(); rit++) {
        if (resp.deliveryTime >= (*rit).deliveryTime) break;
        if (resp.event->getRoutingAddress() == (*rit).event->getRoutingAddress()) break;
    }
    outgoing_event_queue_down_.insert(rit.base(), resp);
}

/* Add a new event to the outgoing queue up (towards memory)
 * Again, to do not reorder events to the same address
 */
void CoherenceController::addToOutgoingQueueUp(Response& resp) {
    list<Response>::reverse_iterator rit;
    for (rit = outgoing_event_queue_up_.rbegin(); rit != outgoing_event_queue_up_.rend(); rit++) {
        if (resp.deliveryTime >= (*rit).deliveryTime) break;
        if (resp.event->getRoutingAddress() == (*rit).event->getRoutingAddress()) break;
    }
    outgoing_event_queue_up_.insert(rit.base(), resp);
}


/* Return whether the component is a peer */
bool CoherenceController::isPeer(std::string name) {
    return link_up_->isPeer(name);
}

/**************************************/
/******** Statistics handling *********/
/**************************************/

void CoherenceController::recordIncomingRequest(MemEventBase* event) {
    // Default type is -1
    LatencyStat lat(timestamp_, event->getCmd(), -1);
    startTimes_.insert(std::make_pair(event->getID(), lat));
}

void CoherenceController::removeRequestRecord(SST::Event::id_type id) {
    if (startTimes_.find(id) != startTimes_.end())
        startTimes_.erase(id);
}

void CoherenceController::recordLatencyType(Event::id_type id, int type) {
    auto it = startTimes_.find(id);
    if(it != startTimes_.end())
        it->second.missType = type;
}

void CoherenceController::recordMiss(Event::id_type id) {
    auto it = startTimes_.find(id);
    if(it != startTimes_.end())
        it->second.missType = LatType::MISS;
}

void CoherenceController::recordPrefetchLatency(Event::id_type id, int type) {
    auto it = startTimes_.find(id);
    if(it != startTimes_.end()) {
        LatencyStat stat = it->second;
        recordLatency(stat.cmd, type, timestamp_ - stat.time);
        startTimes_.erase(id);
    }
}

void CoherenceController::serialize_order(SST::Core::Serialization::serializer& ser) {
    SST::SubComponent::serialize_order(ser);

    SST_SER(event_debuginfo_);
    SST_SER(evict_debuginfo_);
    SST_SER(mshr_);
    SST_SER(listeners_);
    SST_SER(max_outstanding_prefetch_);
    SST_SER(drop_prefetch_level_);
    SST_SER(outstanding_prefetch_count_);
    SST_SER(cachename_);
    SST_SER(output_);
    SST_SER(debug_);
    SST_SER(debug_addr_filter_);
    SST_SER(debug_level_);
    SST_SER(timestamp_);
    SST_SER(accessLatency_);
    SST_SER(tagLatency_);
    SST_SER(mshrLatency_);
    SST_SER(lineSize_);
    SST_SER(writebackCleanBlocks_);
    SST_SER(silentEvictClean_);
    SST_SER(recvWritebackAck_);
    SST_SER(sendWritebackAck_);
    SST_SER(lastLevel_);
    SST_SER(flush_manager_);
    SST_SER(flush_helper_);
    SST_SER(flush_dest_);
    SST_SER(retryBuffer_);
    SST_SER(stat_eventSent);
    SST_SER(stat_evict);
    SST_SER(stat_eventstate_);
    SST_SER(startTimes_);
    SST_SER(cpus);
    SST_SER(outgoing_event_queue_down_);
    SST_SER(outgoing_event_queue_up_);
    SST_SER(link_up_);
    SST_SER(link_down_);
    SST_SER(maxBytesUp);
    SST_SER(maxBytesDown);
    SST_SER(packetHeaderBytes);
    SST_SER(statPrefetchEvict);
    SST_SER(statPrefetchInv);
    SST_SER(statPrefetchRedundant);
    SST_SER(statPrefetchUpgradeMiss);
    SST_SER(statPrefetchHit);
    SST_SER(statPrefetchDrop);

    // Rely on cache to re-register reenableClock_ in deserialization
}
