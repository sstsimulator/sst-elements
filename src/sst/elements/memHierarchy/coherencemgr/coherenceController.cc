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


#include <sst_config.h>

#include "coherencemgr/coherenceController.h"

using namespace SST;
using namespace SST::MemHierarchy;


CoherenceController::CoherenceController(ComponentId_t id, Params &params, Params& ownerParams, bool prefetch) : SubComponent(id) {
    params.insert(ownerParams); // Combine params

    /* Output stream */
    output = new Output("", 1, 0, SST::Output::STDOUT);

    /* Debug stream */
    debug = new Output("", params.find<int>("debug_level", 1), 0, (Output::output_location_t)params.find<int>("debug", SST::Output::NONE));

    dlevel = params.find<int>("debug_level", 1);
    if (params.find<int>("debug", SST::Output::NONE) == SST::Output::NONE)
        dlevel = 0;

    bool found;

    /* Get latency parameters */
    accessLatency_ = params.find<uint64_t>("access_latency_cycles", 1, found);
    if (!found) {
        output->fatal(CALL_INFO, -1, "%s, Param not specified: access_latency_cycles - this is the access time in cycles for the cache; if tag_latency is also specified, this is the data array access time\n",
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
        output->fatal(CALL_INFO, -1, "%s, Invalid param: min_packet_size - must have units of bytes (B), SI units OK. Ex: '8B'. You specified '%s'\n", getName().c_str(), packetSize.toString().c_str());
    if (!downLinkBW.hasUnits("B"))
        output->fatal(CALL_INFO, -1, "%s, Invalid param: request_link_width - must have units of bytes (B), SI units OK. Ex: '64B'. You specified '%s'\n", getName().c_str(), downLinkBW.toString().c_str());
    if (!upLinkBW.hasUnits("B"))
        output->fatal(CALL_INFO, -1, "%s, Invalid param: response_link_width - must have units of bytes (B), SI units OK. Ex: '64B'. You specified '%s'\n", getName().c_str(), upLinkBW.toString().c_str());

    maxBytesUp = upLinkBW.getRoundedValue();
    maxBytesDown = downLinkBW.getRoundedValue();
    packetHeaderBytes = packetSize.getRoundedValue();

    /* Initialize variables */
    timestamp_ = 0;
    outstandingPrefetches_ = 0;

    /* Default values for cache parameters */
    // May be updated during init()
    lastLevel_ = true;
    silentEvictClean_ = true;
    writebackCleanBlocks_ = false;
    recvWritebackAck_ = false;
    sendWritebackAck_ = false;

    // The following cache parameters are set by the cache controller in its constructor
    // Just in case, we give an initial value here
    dropPrefetchLevel_ = ((size_t) - 1);
    maxOutstandingPrefetch_ = ((size_t) - 2);

    // Get parent component's name
    cachename_ = getParentComponentName();

    // Register statistics - only those that are common across all coherence managers
    // Give  all array entries a default statistic so we don't end up with segfaults during execution
    Statistic<uint64_t> * defStat = registerStatistic<uint64_t>("default_stat");
    for (int i = 0; i < (int)Command::LAST_CMD; i++) {
        stat_eventSent[i] = defStat;
        for (int j = 0; j < LAST_STATE; j++) {
            stat_eventState[i][j] = defStat;

            if (i == 0) {
                stat_evict[j] = defStat;
            }
        }
    }

    // Initialize event debug info (eventDI/evictDI)
    evictDI.id.first = 0;
    evictDI.id.second = 0;
    evictDI.cmd = Command::NULLCMD;
    evictDI.prefetch = false;
    evictDI.oldst = I;
    evictDI.newst = I;
    evictDI.action = "";
    evictDI.reason = "";
    evictDI.verboseline = "";
    eventDI.id.first = 0;
    eventDI.id.second = 0;
    eventDI.cmd = Command::NULLCMD;
    eventDI.prefetch = false;
    eventDI.oldst = I;
    eventDI.newst = I;
    eventDI.action = "";
    eventDI.reason = "";
    eventDI.verboseline = "";
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

    debug->fatal(CALL_INFO, -1, "%s, Invalid param: replacement_policy - supported policies are 'lru', 'lfu', 'random', 'mru', and 'nmru'. You specified '%s'.\n", getName().c_str(), policy.c_str());
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
    debug->fatal(CALL_INFO, -1, "%s, Error: GetS events are not handled by this coherence manager. Event: %s. Time: %" PRIu64 "ns.\n",
            getName().c_str(), event->getVerboseString().c_str(), getCurrentSimTimeNano());
    return false;
}

bool CoherenceController::handleGetX(MemEvent* event, bool inMSHR) {
    debug->fatal(CALL_INFO, -1, "%s, Error: GetX events are not handled by this coherence manager. Event: %s. Time: %" PRIu64 "ns.\n",
            getName().c_str(), event->getVerboseString().c_str(), getCurrentSimTimeNano());
    return false;
}

bool CoherenceController::handleGetSX(MemEvent* event, bool inMSHR) {
    debug->fatal(CALL_INFO, -1, "%s, Error: GetSX events are not handled by this coherence manager. Event: %s. Time: %" PRIu64 "ns.\n",
            getName().c_str(), event->getVerboseString().c_str(), getCurrentSimTimeNano());
    return false;
}

bool CoherenceController::handleFlushLine(MemEvent* event, bool inMSHR) {
    debug->fatal(CALL_INFO, -1, "%s, Error: FlushLine events are not handled by this coherence manager. Event: %s. Time: %" PRIu64 "ns.\n",
            getName().c_str(), event->getVerboseString().c_str(), getCurrentSimTimeNano());
    return false;
}

bool CoherenceController::handleFlushLineInv(MemEvent* event, bool inMSHR) {
    debug->fatal(CALL_INFO, -1, "%s, Error: FlushLineInv events are not handled by this coherence manager. Event: %s. Time: %" PRIu64 "ns.\n",
            getName().c_str(), event->getVerboseString().c_str(), getCurrentSimTimeNano());
    return false;
}

bool CoherenceController::handlePutS(MemEvent* event, bool inMSHR) {
    debug->fatal(CALL_INFO, -1, "%s, Error: PutS events are not handled by this coherence manager. Event: %s. Time: %" PRIu64 "ns.\n",
            getName().c_str(), event->getVerboseString().c_str(), getCurrentSimTimeNano());
    return false;
}

bool CoherenceController::handlePutX(MemEvent* event, bool inMSHR) {
    debug->fatal(CALL_INFO, -1, "%s, Error: PutX events are not handled by this coherence manager. Event: %s. Time: %" PRIu64 "ns.\n",
            getName().c_str(), event->getVerboseString().c_str(), getCurrentSimTimeNano());
    return false;
}

bool CoherenceController::handlePutE(MemEvent* event, bool inMSHR) {
    debug->fatal(CALL_INFO, -1, "%s, Error: PutE events are not handled by this coherence manager. Event: %s. Time: %" PRIu64 "ns.\n",
            getName().c_str(), event->getVerboseString().c_str(), getCurrentSimTimeNano());
    return false;
}

bool CoherenceController::handlePutM(MemEvent* event, bool inMSHR) {
    debug->fatal(CALL_INFO, -1, "%s, Error: PutM events are not handled by this coherence manager. Event: %s. Time: %" PRIu64 "ns.\n",
            getName().c_str(), event->getVerboseString().c_str(), getCurrentSimTimeNano());
    return false;
}

bool CoherenceController::handleGetSResp(MemEvent* event, bool inMSHR) {
    debug->fatal(CALL_INFO, -1, "%s, Error: GetSResp events are not handled by this coherence manager. Event: %s. Time: %" PRIu64 "ns.\n",
            getName().c_str(), event->getVerboseString().c_str(), getCurrentSimTimeNano());
    return false;
}

bool CoherenceController::handleGetXResp(MemEvent* event, bool inMSHR) {
    debug->fatal(CALL_INFO, -1, "%s, Error: GetXResp events are not handled by this coherence manager. Event: %s. Time: %" PRIu64 "ns.\n",
            getName().c_str(), event->getVerboseString().c_str(), getCurrentSimTimeNano());
    return false;
}

bool CoherenceController::handleFlushLineResp(MemEvent* event, bool inMSHR) {
    debug->fatal(CALL_INFO, -1, "%s, Error: FlushLineResp events are not handled by this coherence manager. Event: %s. Time: %" PRIu64 "ns.\n",
            getName().c_str(), event->getVerboseString().c_str(), getCurrentSimTimeNano());
    return false;
}

bool CoherenceController::handleAckPut(MemEvent* event, bool inMSHR) {
    debug->fatal(CALL_INFO, -1, "%s, Error: AckPut events are not handled by this coherence manager. Event: %s. Time: %" PRIu64 "ns.\n",
            getName().c_str(), event->getVerboseString().c_str(), getCurrentSimTimeNano());
    return false;
}

bool CoherenceController::handleFetch(MemEvent* event, bool inMSHR) {
    debug->fatal(CALL_INFO, -1, "%s, Error: Fetch events are not handled by this coherence manager. Event: %s. Time: %" PRIu64 "ns.\n",
            getName().c_str(), event->getVerboseString().c_str(), getCurrentSimTimeNano());
    return false;
}

bool CoherenceController::handleFetchInv(MemEvent* event, bool inMSHR) {
    debug->fatal(CALL_INFO, -1, "%s, Error: FetchInv events are not handled by this coherence manager. Event: %s. Time: %" PRIu64 "ns.\n",
            getName().c_str(), event->getVerboseString().c_str(), getCurrentSimTimeNano());
    return false;
}

bool CoherenceController::handleFetchInvX(MemEvent* event, bool inMSHR) {
    debug->fatal(CALL_INFO, -1, "%s, Error: FetchInvX events are not handled by this coherence manager. Event: %s. Time: %" PRIu64 "ns.\n",
            getName().c_str(), event->getVerboseString().c_str(), getCurrentSimTimeNano());
    return false;
}

bool CoherenceController::handleForceInv(MemEvent* event, bool inMSHR) {
    debug->fatal(CALL_INFO, -1, "%s, Error: ForceInv events are not handled by this coherence manager. Event: %s. Time: %" PRIu64 "ns.\n",
            getName().c_str(), event->getVerboseString().c_str(), getCurrentSimTimeNano());
    return false;
}

bool CoherenceController::handleInv(MemEvent* event, bool inMSHR) {
    debug->fatal(CALL_INFO, -1, "%s, Error: Inv events are not handled by this coherence manager. Event: %s. Time: %" PRIu64 "ns.\n",
            getName().c_str(), event->getVerboseString().c_str(), getCurrentSimTimeNano());
    return false;
}

bool CoherenceController::handleFetchResp(MemEvent* event, bool inMSHR) {
    debug->fatal(CALL_INFO, -1, "%s, Error: FetchResp events are not handled by this coherence manager. Event: %s. Time: %" PRIu64 "ns.\n",
            getName().c_str(), event->getVerboseString().c_str(), getCurrentSimTimeNano());
    return false;
}

bool CoherenceController::handleFetchXResp(MemEvent* event, bool inMSHR) {
    debug->fatal(CALL_INFO, -1, "%s, Error: FetchXResp events are not handled by this coherence manager. Event: %s. Time: %" PRIu64 "ns.\n",
            getName().c_str(), event->getVerboseString().c_str(), getCurrentSimTimeNano());
    return false;
}

bool CoherenceController::handleAckInv(MemEvent* event, bool inMSHR) {
    debug->fatal(CALL_INFO, -1, "%s, Error: AckInv events are not handled by this coherence manager. Event: %s\n. Time: %" PRIu64 "ns.",
            getName().c_str(), event->getVerboseString().c_str(), getCurrentSimTimeNano());
    return false;
}

bool CoherenceController::handleNULLCMD(MemEvent* event, bool inMSHR) {
    debug->fatal(CALL_INFO, -1, "%s, Error: NULLCMD events are not handled by this coherence manager. Event: %s. Time: %" PRIu64 "ns.\n",
            getName().c_str(), event->getVerboseString().c_str(), getCurrentSimTimeNano());
    return false;
}

bool CoherenceController::handleNACK(MemEvent* event, bool inMSHR) {
    debug->fatal(CALL_INFO, -1, "%s, Error: NACK events are not handled by this coherence manager. Event: %s. Time: %" PRIu64 "ns.\n",
            getName().c_str(), event->getVerboseString().c_str(), getCurrentSimTimeNano());
    return false;
}


/*******************************************************************************
 * Send events
 *******************************************************************************/

/* Send commands when their timestampe expires. Return whether queue is empty or not. */
bool CoherenceController::sendOutgoingEvents() {
    // Update timestamp
    timestamp_++;

    // Check for ready events in outgoing 'down' queue
    uint64_t bytesLeft = maxBytesDown;
    while (!outgoingEventQueue_.empty() && outgoingEventQueue_.front().deliveryTime <= timestamp_) {
        MemEventBase *outgoingEvent = outgoingEventQueue_.front().event;
        if (maxBytesDown != 0) {
            if (bytesLeft == 0) break;
            if (bytesLeft >= outgoingEventQueue_.front().size) {
                bytesLeft -= outgoingEventQueue_.front().size;  // Send this many bytes
            } else {
                outgoingEventQueue_.front().size -= bytesLeft;
                break;
            }
        }

        outgoingEvent->setDst(linkDown_->findTargetDestination(outgoingEvent->getRoutingAddress()));

        if (is_debug_event(outgoingEvent)) {
            debug->debug(_L4_, "E: %-20" PRIu64 " %-20" PRIu64 " %-20s Event:Send    (%s)\n",
                    Simulation::getSimulation()->getCurrentSimCycle(), timestamp_, cachename_.c_str(), outgoingEvent->getBriefString().c_str());
        }

        linkDown_->send(outgoingEvent);
        outgoingEventQueue_.pop_front();

    }

    // Check for ready events in outgoing 'up' queue
    bytesLeft = maxBytesUp;
    while (!outgoingEventQueueUp_.empty() && outgoingEventQueueUp_.front().deliveryTime <= timestamp_) {
        MemEventBase * outgoingEvent = outgoingEventQueueUp_.front().event;
        if (maxBytesUp != 0) {
            if (bytesLeft == 0) break;
            if (bytesLeft >= outgoingEventQueueUp_.front().size) {
                bytesLeft -= outgoingEventQueueUp_.front().size;
            } else {
                outgoingEventQueueUp_.front().size -= bytesLeft;
                break;
            }
        }

        if (is_debug_event(outgoingEvent)) {
            debug->debug(_L4_, "E: %-20" PRIu64 " %-20" PRIu64 " %-20s Event:Send    (%s)\n",
                    Simulation::getSimulation()->getCurrentSimCycle(), timestamp_, cachename_.c_str(), outgoingEvent->getBriefString().c_str());
        }

        if (startTimes_.find(outgoingEvent->getResponseToID()) != startTimes_.end()) {
            LatencyStat stat = startTimes_.find(outgoingEvent->getResponseToID())->second;
            recordLatency(stat.cmd, stat.missType, timestamp_ - stat.time);
            startTimes_.erase(outgoingEvent->getResponseToID());
        }

        linkUp_->send(outgoingEvent);
        outgoingEventQueueUp_.pop_front();
    }

    // Return whether it's ok for the cache to turn off the clock - we need it on to be able to send waiting events
    return outgoingEventQueue_.empty() && outgoingEventQueueUp_.empty();
}

bool CoherenceController::checkIdle() {
    return outgoingEventQueue_.empty() && outgoingEventQueueUp_.empty();
}


/* Forward an events toward memory. Return expected send time. */
uint64_t CoherenceController::forwardTowardsMem(MemEventBase * event) {
    event->setSrc(cachename_);
    event->setDst(linkDown_->findTargetDestination(event->getRoutingAddress()));

    Response fwdReq = {event, timestamp_ + 1, packetHeaderBytes + event->getPayloadSize()};
    addToOutgoingQueue(fwdReq);
    return timestamp_ + 1;
}

/* Forward an event towards processor. Return expected send time. */
uint64_t CoherenceController::forwardTowardsCPU(MemEventBase * event, std::string dst) {
    event->setSrc(cachename_);
    event->setDst(dst);

    Response fwdReq = {event, timestamp_ + 1, packetHeaderBytes + event->getPayloadSize()};
    addToOutgoingQueueUp(fwdReq);
    return timestamp_ + 1;
}


/*******************************************************************************
 * Initialization/finish functions used by parent
 *******************************************************************************/

/* Parse an init coherence vent. Coherence managers can override this */
void CoherenceController::processInitCoherenceEvent(MemEventInitCoherence* event, bool source) {
    // If something besides memory is below us, we are not the last level doing coherence
    if (event->getType() != Endpoint::Memory)
        lastLevel_ = false;

    // If the component below us tracks whether a block is present above, then we can't silently evict (per current protocols)
    if (event->getTracksPresence() || lastLevel_)
        silentEvictClean_ = false;

    // The component below us is noninclusive, therefore we need to write back data when evicting clean blocks
    if (!event->getInclusive())
        writebackCleanBlocks_ = true;

    // The component below us will send writeback acks, we should wait for them
    if (!source && event->getSendWBAck())
        recvWritebackAck_ = true;

    if (source && event->getRecvWBAck())
        sendWritebackAck_ = true;

    debug->debug(_L3_, "%s processInitCoherenceEvent. Result is: LL ? %s, silentEvict ? %s, WBClean ? %s, sendWBAck ? %s, recvWBAck ? %s\n",
            cachename_.c_str(),
            lastLevel_ ? "Y" : "N",
            silentEvictClean_ ? "Y" : "N",
            writebackCleanBlocks_ ? "Y" : "N",
            sendWritebackAck_ ? "Y" : "N",
            recvWritebackAck_ ? "Y" : "N");
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
uint64_t CoherenceController::forwardMessage(MemEvent * event, unsigned int requestSize, uint64_t baseTime, vector<uint8_t>* data) {
    /* Create event to be forwarded */
    MemEvent* forwardEvent;
    forwardEvent = new MemEvent(*event);

    if (data == nullptr) forwardEvent->setPayload(0, nullptr);

    forwardEvent->setSrc(cachename_);
    forwardEvent->setDst(linkDown_->findTargetDestination(event->getRoutingAddress()));
    forwardEvent->setSize(requestSize);

    if (data != nullptr) forwardEvent->setPayload(*data);

    /* Determine latency in cycles */
    uint64_t deliveryTime;
    if (baseTime < timestamp_) baseTime = timestamp_;
    if (event->queryFlag(MemEvent::F_NONCACHEABLE)) {
        forwardEvent->setFlag(MemEvent::F_NONCACHEABLE);
        deliveryTime = timestamp_ + mshrLatency_;
    } else deliveryTime = baseTime + tagLatency_;

    Response fwdReq = {forwardEvent, deliveryTime, packetHeaderBytes + forwardEvent->getPayloadSize() };
    addToOutgoingQueue(fwdReq);

    if (is_debug_event(event))
        eventDI.action = "Forward";

    return deliveryTime;
}

/* Send a NACK event */
void CoherenceController::sendNACK(MemEvent * event) {
    MemEvent * NACKevent = event->makeNACKResponse(event);

    uint64_t deliveryTime = timestamp_ + tagLatency_; // Probably had to lookup and see that we couldn't handle this request and/or MSHR was full
    Response resp = {NACKevent, deliveryTime, packetHeaderBytes};

    if (event->isCPUSideEvent())
        addToOutgoingQueueUp(resp);
    else
        addToOutgoingQueue(resp);

    if (is_debug_event(event))
        eventDI.action = "NACK";
}


/* Resend an event after a NACK */
void CoherenceController::resendEvent(MemEvent * event, bool towardsCPU) {
    // Calculate backoff - avoids flooding links
    int retries = event->getRetries();
    if (retries > 10) retries = 10;
    uint64_t backoff = ( 0x1 << retries);
    event->incrementRetries();

    uint64_t deliveryTime =  timestamp_ + mshrLatency_ + backoff;
    Response resp = {event, deliveryTime, packetHeaderBytes + event->getPayloadSize() };
    if (!towardsCPU)
        addToOutgoingQueue(resp);
    else
        addToOutgoingQueueUp(resp);

    if (is_debug_event(event)) {
        eventDI.action = "Resend";
        eventDI.reason = event->getBriefString();
    }
}


/* Send response up (towards CPU). L1s need to implement their own to split out the requested block */
uint64_t CoherenceController::sendResponseUp(MemEvent * event, vector<uint8_t>* data, bool replay, uint64_t baseTime, bool atomic) {
    return sendResponseUp(event, CommandResponse[(int)event->getCmd()], data, false, replay, baseTime, atomic);
}


/* Send response up (towards CPU). L1s need to implement their own to split out the requested block */
uint64_t CoherenceController::sendResponseUp(MemEvent * event, Command cmd, vector<uint8_t>* data, bool replay, uint64_t baseTime, bool atomic) {
    return sendResponseUp(event, cmd, data, false, replay, baseTime, atomic);
}


/* Send response towards the CPU. L1s need to implement their own to split out the requested block */
uint64_t CoherenceController::sendResponseUp(MemEvent * event, Command cmd, vector<uint8_t>* data, bool dirty, bool replay, uint64_t baseTime, bool atomic) {
    MemEvent * responseEvent = event->makeResponse(cmd);
    responseEvent->setDst(event->getSrc());
    responseEvent->setSize(event->getSize());
    if (data != nullptr) responseEvent->setPayload(*data);
    responseEvent->setDirty(dirty);

    if (baseTime < timestamp_) baseTime = timestamp_;
    uint64_t deliveryTime = baseTime + (replay ? mshrLatency_ : accessLatency_);
    Response resp = {responseEvent, deliveryTime, packetHeaderBytes + responseEvent->getPayloadSize() };
    addToOutgoingQueueUp(resp);

    return deliveryTime;
}

/* Return the name of the source for this cache */
std::string CoherenceController::getSrc() {
    return linkUp_->getSources()->begin()->name;
}

/* Allocate an MSHR entry for an event */
MemEventStatus CoherenceController::allocateMSHR(MemEvent * event, bool fwdReq, int pos, bool stallEvict) {
    // Screen prefetches first to ensure limits are not exceeeded:
    //      - Maximum number of outstanding prefetches
    //      - MSHR too full to accept prefetches
    if (event->isPrefetch() && event->getRqstr() == cachename_) {
        if (dropPrefetchLevel_ <= mshr_->getSize()) {
            eventDI.action = "Reject";
            eventDI.reason = "Prefetch drop level";
            return MemEventStatus::Reject;
        }
        if (maxOutstandingPrefetch_ <= outstandingPrefetches_) {
            eventDI.action = "Reject";
            eventDI.reason = "Max outstanding prefetches";
            return MemEventStatus::Reject;
        }
    }

    int end_pos = mshr_->insertEvent(event->getBaseAddr(), event, pos, fwdReq, stallEvict);
    if (end_pos == -1) {
        if (is_debug_event(event)) {
            eventDI.action = "Reject";
            eventDI.reason = "MSHR full";
        }
        return MemEventStatus::Reject; // MSHR is full
    } else if (end_pos != 0) {
        if (is_debug_event(event)) {
            eventDI.action = "Stall";
            eventDI.reason = "MSHR conflict";
        }
        if (event->isPrefetch())
            outstandingPrefetches_++;
        return MemEventStatus::Stall;
    }

    if (event->isPrefetch())
        outstandingPrefetches_++;
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
    printDebugInfo(&eventDI);
}

void CoherenceController::printDebugInfo(dbgin * diStruct) {
    if (dlevel < 5)
        return;

    std::string cmd = CommandString[(int)diStruct->cmd];
    if (diStruct->prefetch)
        cmd += "-pref";

    std::stringstream id;
    id << "<" << diStruct->id.first << "," << diStruct->id.second << ">";

    stringstream reas;
    reas << "(" << diStruct->reason << ")";

    debug->debug(_L5_, "C: %-20" PRIu64 " %-20" PRIu64 " %-20s %-13s 0x%-16" PRIx64 " %-15s %-6s %-6s %-10s %-15s",
            Simulation::getSimulation()->getCurrentSimCycle(), timestamp_, cachename_.c_str(), cmd.c_str(), diStruct->addr,
            id.str().c_str(), StateString[diStruct->oldst], StateString[diStruct->newst], diStruct->action.c_str(), reas.str().c_str());

    debug->debug(_L6_, " %s", diStruct->verboseline.c_str());
    debug->debug(_L5_, "\n");
}

void CoherenceController::printDebugAlloc(bool alloc, Addr addr, std::string note) {
    if (dlevel < 5)
        return;

    std::string action = alloc ? "Alloc" : "Dealloc";

    debug->debug(_L5_, "C: %-20" PRIu64 " %-20" PRIu64 " %-20s %-13s 0x%-16" PRIx64 "",
            Simulation::getSimulation()->getCurrentSimCycle(), timestamp_, cachename_.c_str(), action.c_str(), addr);

    if (note != "")
        debug->debug(_L5_, " %s\n", note.c_str());
    else
        debug->debug(_L5_, "\n");
}

void CoherenceController::printStatus(Output& out) {
    out.output("  Begin MemHierarchy::CoherenceController %s\n", getName().c_str());

    out.output("    Events waiting in outgoingEventQueue: %zu\n", outgoingEventQueue_.size());
    for (list<Response>::iterator it = outgoingEventQueue_.begin(); it!= outgoingEventQueue_.end(); it++) {
        out.output("      Time: %" PRIu64 ", Event: %s\n", (*it).deliveryTime, (*it).event->getVerboseString().c_str());
    }

    out.output("    Events waiting in outgoingEventQueueUp_: %zu\n", outgoingEventQueueUp_.size());
    for (list<Response>::iterator it = outgoingEventQueueUp_.begin(); it!= outgoingEventQueueUp_.end(); it++) {
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
    for (rit = outgoingEventQueue_.rbegin(); rit!= outgoingEventQueue_.rend(); rit++) {
        if (resp.deliveryTime >= (*rit).deliveryTime) break;
        if (resp.event->getRoutingAddress() == (*rit).event->getRoutingAddress()) break;
    }
    outgoingEventQueue_.insert(rit.base(), resp);
}

/* Add a new event to the outgoing queue up (towards memory)
 * Again, to do not reorder events to the same address
 */
void CoherenceController::addToOutgoingQueueUp(Response& resp) {
    list<Response>::reverse_iterator rit;
    for (rit = outgoingEventQueueUp_.rbegin(); rit != outgoingEventQueueUp_.rend(); rit++) {
        if (resp.deliveryTime >= (*rit).deliveryTime) break;
        if (resp.event->getRoutingAddress() == (*rit).event->getRoutingAddress()) break;
    }
    outgoingEventQueueUp_.insert(rit.base(), resp);
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
    if (startTimes_.find(id) != startTimes_.end())
        startTimes_.find(id)->second.missType = type;
}

void CoherenceController::recordMiss(Event::id_type id) {
    if (startTimes_.find(id) != startTimes_.end())
        startTimes_.find(id)->second.missType = LatType::MISS;
}

void CoherenceController::recordPrefetchLatency(Event::id_type id, int type) {
    if (startTimes_.find(id) != startTimes_.end()) {
        LatencyStat stat = startTimes_.find(id)->second;
        recordLatency(stat.cmd, type, timestamp_ - stat.time);
        startTimes_.erase(id);
    }
}

