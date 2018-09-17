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

#include <array>
#include <sst_config.h>
#include "sst/elements/memHierarchy/memNICFour.h"

#include <sst/core/simulation.h>

using namespace SST;
using namespace SST::MemHierarchy;
using namespace SST::Interfaces;

/* Debug macros */
#ifdef __SST_DEBUG_OUTPUT__ /* From sst-core, enable with --enable-debug */
#define is_debug_addr(addr) (DEBUG_ADDR.empty() || DEBUG_ADDR.find(addr) != DEBUG_ADDR.end())
#define is_debug_event(ev) (DEBUG_ADDR.empty() || ev->doDebug(DEBUG_ADDR))
#else
#define is_debug_addr(addr) false
#define is_debug_event(ev) false
#endif

/* Constructor */
MemNICFour::MemNICFour(Component * parent, Params &params) : MemNICBase(parent, params) {

    bool found;
    std::array<std::string,4> pref = {"req", "ack", "fwd", "data"};

    for (int i = 0; i < 4; i++) {
        // Error checking for much of this is done by the link control
        std::string linkBandwidth = params.find<std::string>(pref[i] + ".network_bw", "80GiB/s");
        int num_vcs = 1; // MemNIC does not use VCs
        std::string linkInbufSize = params.find<std::string>(pref[i] + ".network_input_buffer_size", "1KiB");
        std::string linkOutbufSize = params.find<std::string>(pref[i] + ".network_output_buffer_size", "1KiB");

        link_control[i] = (SimpleNetwork*)loadSubComponent(params.find<std::string>(pref[i] + ".linkcontrol", "kingsley.linkcontrol"), params); 
        // But link control doesn't use params so manually initialize
        link_control[i]->initialize(pref[i], UnitAlgebra(linkBandwidth), num_vcs, UnitAlgebra(linkInbufSize), UnitAlgebra(linkOutbufSize));
    
        // Packet size
        packetHeaderBytes[i] = extractPacketHeaderSize(params, pref[i] + ".min_packet_size");
    }
    
    // Set link control to call recvNotify on event receive
    link_control[DATA]->setNotifyOnReceive(new SimpleNetwork::Handler<MemNICFour>(this, &MemNICFour::recvNotifyData));
    link_control[REQ]->setNotifyOnReceive(new SimpleNetwork::Handler<MemNICFour>(this, &MemNICFour::recvNotifyReq));
    link_control[ACK]->setNotifyOnReceive(new SimpleNetwork::Handler<MemNICFour>(this, &MemNICFour::recvNotifyAck));
    link_control[FWD]->setNotifyOnReceive(new SimpleNetwork::Handler<MemNICFour>(this, &MemNICFour::recvNotifyFwd));
    
    // Register statistics
    stat_oooEvent[DATA] = registerStatistic<uint64_t>("outoforder_data_events");
    stat_oooEvent[REQ] = registerStatistic<uint64_t>("outoforder_req_events");
    stat_oooEvent[ACK] = registerStatistic<uint64_t>("outoforder_ack_events");
    stat_oooEvent[FWD] = registerStatistic<uint64_t>("outoforder_fwd_events");
    stat_oooDepth = registerStatistic<uint64_t>("outoforder_depth_at_event_receive");
    stat_oooDepthSrc = registerStatistic<uint64_t>("outoforder_depth_at_event_receive_src");
    stat_orderLatency = registerStatistic<uint64_t>("ordering_latency");
    totalOOO = 0;

    // TimeBase for statistics
    std::string timebase = params.find<std::string>("clock", "1GHz");
    setDefaultTimeBase(getTimeConverter(timebase));
}

void MemNICFour::init(unsigned int phase) {
    link_control[REQ]->init(phase);
    link_control[ACK]->init(phase);
    link_control[FWD]->init(phase);
    link_control[DATA]->init(phase);
    
    MemNICBase::nicInit(link_control[DATA], phase);
}

void MemNICFour::setup() {
    link_control[REQ]->setup();
    link_control[ACK]->setup();
    link_control[FWD]->setup();
    link_control[DATA]->setup();

    MemNICBase::setup();

    for (std::set<EndpointInfo>::iterator it = sourceEndpointInfo.begin(); it != sourceEndpointInfo.end(); it++) {
        recvTags[it->addr] = 0;
        sendTags[it->addr] = 0;
    }
    
    for (std::set<EndpointInfo>::iterator it = destEndpointInfo.begin(); it != destEndpointInfo.end(); it++) {
        recvTags[it->addr] = 0;
        sendTags[it->addr] = 0;
    }
}

/* 
 * Called by parent on a clock 
 * Returns whether anything sent this cycle
 */
bool MemNICFour::clock() {
    if (sendQueue[REQ].empty() && sendQueue[ACK].empty() && sendQueue[FWD].empty() && sendQueue[DATA].empty() && recvQueue.empty()) return true;

    // Attempt to drain network queues
    for (int i = 0; i < 4; i++) {
        drainQueue(&(sendQueue[i]), link_control[i]);
    }

    if (!recvQueue.empty()) {
        recvNotify(recvQueue.front());
        recvQueue.pop();
    }

    return false;
}

/* Send event to memNIC */
void MemNICFour::send(MemEventBase *ev) {
    NetType net = DATA;
    if (ev->getPayloadSize() == 0) {
        if (CommandClassArr[(int)ev->getCmd()] == CommandClass::Request) net = REQ;
        else if (CommandClassArr[(int)ev->getCmd()] == CommandClass::Ack) net = ACK;
        else net = FWD;
    }
            
    SimpleNetwork::Request * req = new SimpleNetwork::Request();
    req->vn = 0;
    req->src = info.addr;
    req->dest = lookupNetworkAddress(ev->getDst());

    unsigned int tag = sendTags[req->dest];
    sendTags[req->dest]++;

    OrderedMemRtrEvent * omre = new OrderedMemRtrEvent(ev, info.addr, tag);
    
    req->size_in_bits = getSizeInBits(ev, net);
    req->givePayload(omre);

    if (is_debug_event(ev)) {
        std::string netstr = "data";
        if (net == REQ) netstr = "req";
        else if (net == ACK) netstr = "ack";
        else if (net == FWD) netstr = "fwd";
        dbg.debug(_L9_, "%s, memNIC adding to %s send queue: dst: %" PRIu64 ", bits: %zu, cmd: %s\n",
                getName().c_str(), netstr.c_str(), req->dest, req->size_in_bits, CommandString[(int)ev->getCmd()]);
    }
    sendQueue[net].push(req);
}


/* 
 * Event handler called by link control on event receive 
 * Return whether event can be received
 */
bool MemNICFour::recvNotifyReq(int) {
    OrderedMemRtrEvent * omre = static_cast<OrderedMemRtrEvent*>(doRecv(link_control[REQ]));
    processRecv(omre, REQ);
    return true;
}
bool MemNICFour::recvNotifyAck(int) {
    OrderedMemRtrEvent * omre = static_cast<OrderedMemRtrEvent*>(doRecv(link_control[ACK]));
    processRecv(omre, ACK);
    return true;
}
bool MemNICFour::recvNotifyFwd(int) {
    OrderedMemRtrEvent * omre = static_cast<OrderedMemRtrEvent*>(doRecv(link_control[FWD]));
    processRecv(omre, FWD);
    return true;
}
bool MemNICFour::recvNotifyData(int) {
    OrderedMemRtrEvent * omre = static_cast<OrderedMemRtrEvent*>(doRecv(link_control[DATA]));
    processRecv(omre, DATA);
    return true;
}

void MemNICFour::processRecv(OrderedMemRtrEvent * omre, NetType net) {
    if (omre != nullptr) {
        dbg.debug(_L3_, "%s, memNIC received a message: <%" PRIu64 ", %u>\n",
                getName().c_str(), omre->src, omre->tag);

        stat_oooDepthSrc->addData(orderBuffer[omre->src].size());
        stat_oooDepth->addData(totalOOO);
        if (omre->tag == recvTags[omre->src]) { // Got the tag we were expecting
            stat_oooEvent[net]->addData(0); // Count total number of events received
            recvTags[omre->src]++;
            
            if (recvQueue.empty())
                recvNotify(omre);
            else {
                // Still push one message for every receive...
                recvQueue.push(omre);
                recvNotify(recvQueue.front());
                recvQueue.pop();
            }

            while (orderBuffer[omre->src].find(recvTags[omre->src]) != orderBuffer[omre->src].end()) {
                totalOOO--;
                recvQueue.push(orderBuffer[omre->src][recvTags[omre->src]].first);
                stat_orderLatency->addData(getCurrentSimTime() - orderBuffer[omre->src][recvTags[omre->src]].second);
                orderBuffer[omre->src].erase(recvTags[omre->src]);
                recvTags[omre->src]++;
            }
        } else {
            totalOOO++;
            stat_oooEvent[net]->addData(1); // Count number of out of order events received
            orderBuffer[omre->src][omre->tag] = std::make_pair(omre,getCurrentSimTime());
        }
    }
}

void MemNICFour::recvNotify(OrderedMemRtrEvent* mre) {
    MemEventBase * me = static_cast<MemEventBase*>(mre->event);
    delete mre;
    
    if (!me) return;

    if (is_debug_event(me)) {
        dbg.debug(_L3_, "%s, memNIC notify parent: src: %s. cmd: %s\n", 
                getName().c_str(), me->getSrc().c_str(), CommandString[(int)me->getCmd()]);
    }

    // Call parent's handler
    (*recvHandler)(me);
}


/** Helper functions **/
/* Calculate size in bits of an event */
size_t MemNICFour::getSizeInBits(MemEventBase *ev, NetType net) {
    return 8 * (packetHeaderBytes[net] + ev->getPayloadSize());
}

// If statements catch special case of fatal() during construction...
void MemNICFour::printStatus(Output& out) {
    out.output("    Begin MemHierarchy::MemNICFour %s\n", getName().c_str());
    out.output("      Data send queue size: %zu\n", sendQueue[DATA].size());
    out.output("      Req send queue size: %zu\n", sendQueue[REQ].size());
    out.output("      Ack send queue size: %zu\n", sendQueue[ACK].size());
    out.output("      Fwd send queue size: %zu\n", sendQueue[FWD].size());
    out.output("      Ordering buffer size: %" PRIu64 "\n", totalOOO);
    out.output("      Receive queue size: %zu\n", recvQueue.size());
    out.output("      Data linkcontrol status:\n");
    if (link_control[DATA]) link_control[DATA]->printStatus(out);
    out.output("      Req linkcontrol status:\n");
    if (link_control[REQ]) link_control[REQ]->printStatus(out);
    out.output("      Ack linkcontrol status:\n");
    if (link_control[ACK]) link_control[ACK]->printStatus(out);
    out.output("      Fwd linkcontrol status:\n");
    if (link_control[FWD]) link_control[FWD]->printStatus(out);
    out.output("    End MemHierarchy::MemNICFour\n");    
}
