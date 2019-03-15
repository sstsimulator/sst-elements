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


#include <sst_config.h>

#include "coherencemgr/coherenceController.h"

using namespace SST;
using namespace SST::MemHierarchy;

/* Debug macros */
#ifdef __SST_DEBUG_OUTPUT__ /* From sst-core, enable with --enable-debug */
#define is_debug_addr(addr) (DEBUG_ADDR.empty() || DEBUG_ADDR.find(addr) != DEBUG_ADDR.end())
#define is_debug_event(ev) (DEBUG_ADDR.empty() || ev->doDebug(DEBUG_ADDR))
#else
#define is_debug_addr(addr) false
#define is_debug_event(ev) false
#endif


CoherenceController::CoherenceController(Component * comp, Params &params) : SubComponent(comp) {
    /* Output stream */
    output = new Output("", 1, 0, SST::Output::STDOUT);

    /* Debug stream */
    debug = new Output("--->  ", params.find<int>("debug_level", 1), 0, (Output::output_location_t)params.find<int>("debug", SST::Output::NONE));

    bool found;

    /* Get latency parameters */
    accessLatency_ = params.find<uint64_t>("access_latency_cycles", 1, found);
    if (!found) {
        output->fatal(CALL_INFO, -1, "%s, Param not specified: access_latency_cycles - this is the access time in cycles for the cache; if tag_latency is also specified, this is the data array access time\n",
                comp->getName().c_str());
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
        output->fatal(CALL_INFO, -1, "%s, Invalid param: min_packet_size - must have units of bytes (B), SI units OK. Ex: '8B'. You specified '%s'\n", parent->getName().c_str(), packetSize.toString().c_str());
    if (!downLinkBW.hasUnits("B"))
        output->fatal(CALL_INFO, -1, "%s, Invalid param: request_link_width - must have units of bytes (B), SI units OK. Ex: '64B'. You specified '%s'\n", parent->getName().c_str(), downLinkBW.toString().c_str());
    if (!upLinkBW.hasUnits("B"))
        output->fatal(CALL_INFO, -1, "%s, Invalid param: response_link_width - must have units of bytes (B), SI units OK. Ex: '64B'. You specified '%s'\n", parent->getName().c_str(), upLinkBW.toString().c_str());

    maxBytesUp = upLinkBW.getRoundedValue();
    maxBytesDown = downLinkBW.getRoundedValue();
    packetHeaderBytes = packetSize.getRoundedValue();

    /* Initialize variables */
    timestamp_ = 0;

    // Register statistics - only those that are common across all coherence managers
    // TODO move many of these to individual coherence protocol managers
    stat_evict_I =      registerStatistic<uint64_t>("evict_I");
    stat_evict_E =      registerStatistic<uint64_t>("evict_E");
    stat_evict_M =      registerStatistic<uint64_t>("evict_M");
    stat_evict_IS =     registerStatistic<uint64_t>("evict_IS");
    stat_evict_IM =     registerStatistic<uint64_t>("evict_IM");
    stat_evict_IB =     registerStatistic<uint64_t>("evict_IB");
    stat_evict_SB =     registerStatistic<uint64_t>("evict_SB");

    // TODO should these be here or part of cache controller?
    stat_latency_GetS_IS =      registerStatistic<uint64_t>("latency_GetS_IS");
    stat_latency_GetS_M =       registerStatistic<uint64_t>("latency_GetS_M");
    stat_latency_GetX_IM =      registerStatistic<uint64_t>("latency_GetX_IM");
    stat_latency_GetX_SM =      registerStatistic<uint64_t>("latency_GetX_SM");
    stat_latency_GetX_M =       registerStatistic<uint64_t>("latency_GetX_M");
    stat_latency_GetSX_IM =     registerStatistic<uint64_t>("latency_GetSX_IM");
    stat_latency_GetSX_SM =     registerStatistic<uint64_t>("latency_GetSX_SM");
    stat_latency_GetSX_M =      registerStatistic<uint64_t>("latency_GetSX_M");
}



/**************************************/
/****** Functions to send events ******/
/**************************************/

/* Send a NACK in response to a request. Could be virtual if needed. */
void CoherenceController::sendNACK(MemEvent * event, bool up, SimTime_t timeInNano) {
    MemEvent *NACKevent = event->makeNACKResponse(event, timeInNano);

    uint64_t deliveryTime = timestamp_ + tagLatency_;
    Response resp = {NACKevent, deliveryTime, packetHeaderBytes};

    if (up) addToOutgoingQueueUp(resp);
    else addToOutgoingQueue(resp);

    if (is_debug_event(event)) debug->debug(_L3_,"Sending NACK at cycle = %" PRIu64 "\n", deliveryTime);
}



/* Send response towards the CPU. L1s need to implement their own to split out the requested block */
uint64_t CoherenceController::sendResponseUp(MemEvent * event, vector<uint8_t>* data, bool replay, uint64_t baseTime, bool atomic) {
    return sendResponseUp(event, CommandResponse[(int)event->getCmd()], data, false, replay, baseTime, atomic);
}


/* Send response towards the CPU. L1s need to implement their own to split out the requested block */
uint64_t CoherenceController::sendResponseUp(MemEvent * event, Command cmd, vector<uint8_t>* data, bool dirty, bool replay, uint64_t baseTime, bool atomic) {
    MemEvent * responseEvent = event->makeResponse(cmd);
    responseEvent->setDst(event->getSrc());
    responseEvent->setSize(event->getSize());
    if (data != NULL) responseEvent->setPayload(*data);
    responseEvent->setDirty(dirty);

    if (baseTime < timestamp_) baseTime = timestamp_;
    uint64_t deliveryTime = baseTime + (replay ? mshrLatency_ : accessLatency_);
    Response resp = {responseEvent, deliveryTime, packetHeaderBytes + responseEvent->getPayloadSize() };
    addToOutgoingQueueUp(resp);

    if (is_debug_event(event)) debug->debug(_L3_,"Sending Response at cycle = %" PRIu64 ". Current Time = %" PRIu64 ", Addr = %" PRIx64 ", Dst = %s, Payload Bytes = %zu\n",
            deliveryTime, timestamp_, event->getAddr(), responseEvent->getDst().c_str(), responseEvent->getPayloadSize());

    return deliveryTime;
}

/* Send response towards the CPU. L1s need to implement their own to split out the requested block */
uint64_t CoherenceController::sendResponseUp(MemEvent * event, Command cmd, vector<uint8_t>* data, bool replay, uint64_t baseTime, bool atomic) {
    return sendResponseUp(event, cmd, data, false, replay, baseTime, atomic);
}


/* Resend an event that was NACKed
 * Add backoff latency to avoid too much traffic
 */
void CoherenceController::resendEvent(MemEvent * event, bool up) {
    // Calculate backoff
    int retries = event->getRetries();
    if (retries > 10) retries = 10;
    uint64_t backoff = ( 0x1 << retries);
    event->incrementRetries();

    uint64_t deliveryTime =  timestamp_ + mshrLatency_ + backoff;
    Response resp = {event, deliveryTime, packetHeaderBytes + event->getPayloadSize() };
    if (!up) addToOutgoingQueue(resp);
    else addToOutgoingQueueUp(resp);

    if (is_debug_event(event)) debug->debug(_L3_,"Sending request: %s\n", event->getBriefString().c_str());
}


/* Forward a message to a lower level (towards memory) in the hierarchy */
uint64_t CoherenceController::forwardMessage(MemEvent * event, Addr baseAddr, unsigned int requestSize, uint64_t baseTime, vector<uint8_t>* data) {
    /* Create event to be forwarded */
    MemEvent* forwardEvent;
    forwardEvent = new MemEvent(*event);

    if (data == NULL) forwardEvent->setPayload(0, NULL);

    forwardEvent->setSrc(parent->getName());
    forwardEvent->setDst(linkDown_->findTargetDestination(baseAddr));
    forwardEvent->setSize(requestSize);

    if (data != NULL) forwardEvent->setPayload(*data);

    /* Determine latency in cycles */
    uint64_t deliveryTime;
    if (baseTime < timestamp_) baseTime = timestamp_;
    if (event->queryFlag(MemEvent::F_NONCACHEABLE)) {
        forwardEvent->setFlag(MemEvent::F_NONCACHEABLE);
        deliveryTime = timestamp_ + mshrLatency_;
    } else deliveryTime = baseTime + tagLatency_;

    Response fwdReq = {forwardEvent, deliveryTime, packetHeaderBytes + forwardEvent->getPayloadSize() };
    addToOutgoingQueue(fwdReq);

    if (is_debug_event(event)) debug->debug(_L3_,"Forwarding request at cycle = %" PRIu64 "\n", deliveryTime);

    return deliveryTime;
}

uint64_t CoherenceController::forwardTowardsMem(MemEventBase * event) {
    event->setSrc(parent->getName());
    event->setDst(linkDown_->findTargetDestination(event->getRoutingAddress()));

    Response fwdReq = {event, timestamp_ + 1, packetHeaderBytes + event->getPayloadSize()};
    addToOutgoingQueue(fwdReq);
    return timestamp_ + 1;
}

uint64_t CoherenceController::forwardTowardsCPU(MemEventBase * event, std::string dst) {
    event->setSrc(parent->getName());
    event->setDst(dst);

    Response fwdReq = {event, timestamp_ + 1, packetHeaderBytes + event->getPayloadSize()};
    addToOutgoingQueueUp(fwdReq);
    return timestamp_ + 1;
}

std::string CoherenceController::getSrc() {
    return linkUp_->getSources()->begin()->name;
}

/**************************************/
/******* Manage outgoing events *******/
/**************************************/


/* Send outgoing commands to port manager */
bool CoherenceController::sendOutgoingCommands(SimTime_t curTime) {
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
            debug->debug(_L4_,"SEND (%s). time: (%" PRIu64 ", %" PRIu64 ") event: (%s)\n",
                    parent->getName().c_str(), timestamp_, curTime, outgoingEvent->getBriefString().c_str());
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
            debug->debug(_L4_,"SEND (%s). time: (%" PRIu64 ", %" PRIu64 ") event: (%s)\n",
                    parent->getName().c_str(), timestamp_, curTime, outgoingEvent->getBriefString().c_str());
        }

        linkUp_->send(outgoingEvent);
        outgoingEventQueueUp_.pop_front();
    }

    // Return whether it's ok for the cache to turn off the clock - we need it on to be able to send waiting events
    return outgoingEventQueue_.empty() && outgoingEventQueueUp_.empty();
}


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
/************ Miscellaneous ***********/
/**************************************/

/* Call back to listener */
void CoherenceController::notifyListenerOfAccess(MemEvent * event, NotifyAccessType accessT, NotifyResultType resultT) {
    if (!event->isPrefetch()) {
        CacheListenerNotification notify(event->getAddr(), event->getBaseAddr(), event->getVirtualAddress(),
                event->getInstructionPointer(), event->getSize(), accessT, resultT);
        listener_->notifyAccess(notify);
    }
}

/**************************************/
/******** Statistics handling *********/
/**************************************/

/* Record latency TODO should probably move to port manager */
void CoherenceController::recordLatency(Command cmd, State state, uint64_t latency) {
    switch (state) {
        case IS:
            stat_latency_GetS_IS->addData(latency);
            break;
        case IM:
            if (cmd == Command::GetX) stat_latency_GetX_IM->addData(latency);
            else stat_latency_GetSX_IM->addData(latency);
            break;
        case SM:
            if (cmd == Command::GetX) stat_latency_GetX_SM->addData(latency);
            else stat_latency_GetSX_SM->addData(latency);
            break;
        case M:
            if (cmd == Command::GetS) stat_latency_GetS_M->addData(latency);
            else if (cmd == Command::GetX) stat_latency_GetX_M->addData(latency);
            else stat_latency_GetSX_M->addData(latency);
            break;
        default:
            break;
    }
}


/* Record the number of times an event arrived in a given state */
void CoherenceController::recordStateEventCount(Command cmd, State state) { }

/* Record the state a block was in when an eviction on it was attempted */
void CoherenceController::recordEvictionState(State state) {
        switch (state) {
            case I:
                stat_evict_I->addData(1);
                break;
            case E:
                stat_evict_E->addData(1);
                break;
            case M:
                stat_evict_M->addData(1);
                break;
            case IS:
                stat_evict_IS->addData(1);
                break;
            case IM:
                stat_evict_IM->addData(1);
                break;
            case I_B:
                stat_evict_IB->addData(1);
                break;
            case S_B:
                stat_evict_SB->addData(1);
            default:
                break;
        }

    }


/**************************************/
/******** Setup related tasks *********/
/**************************************/


/* Setup variables controlling interactions with other memory levels */
void CoherenceController::setupLowerStatus(bool silentEvict, bool isLastCoherenceLevel, bool expectWritebackAck, bool lowerIsNoninclusive) {
    silentEvictClean_ = silentEvict; // Level below us doesn't track block presence so just drop clean blocks
    lastLevel_ = isLastCoherenceLevel;
    expectWritebackAck_ = expectWritebackAck; // Level below us will send writeback acks so wait for it
    writebackCleanBlocks_ = lowerIsNoninclusive; // Attach a payload to clean writebacks if the lower level might not have data

    //silentEvictClean_       = isLastCoherenceLevel; // Silently evict clean blocks if there's just a memory below us
    //expectWritebackAck_     = !isLastCoherenceLevel && (lowerIsDirectory || lowerIsNoninclusive);  // Expect writeback ack if there's a dir below us or a non-inclusive cache
    //writebackCleanBlocks_   = lowerIsNoninclusive;  // Writeback clean data if lower is non-inclusive - otherwise control message only

}

/**************************************/
/*********** Debug support ************/
/**************************************/

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


