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
        std::string linkName = params.find<std::string>(pref[i] + ".port", "");
        
        if (linkName == "") 
            dbg.fatal(CALL_INFO, -1, "Param not specified(%s): %s.port - the name of the port that the MemNIC's %s network is attached to. This should be set internally by components creating the memNIC.\n",
                    getName().c_str(), pref[i].c_str(), pref[i].c_str());

        // Error checking for much of this is done by the link control
        std::string linkBandwidth = params.find<std::string>(pref[i] + ".network_bw", "80GiB/s");
        int num_vcs = 1; // MemNIC does not use VCs
        std::string linkInbufSize = params.find<std::string>(pref[i] + ".network_input_buffer_size", "1KiB");
        std::string linkOutbufSize = params.find<std::string>(pref[i] + ".network_output_buffer_size", "1KiB");

        link_control[i] = (SimpleNetwork*)parent->loadSubComponent(params.find<std::string>(pref[i] + ".linkcontrol", "kingsley.linkcontrol"), parent, params); 
        // But link control doesn't use params so manually initialize
        link_control[i]->initialize(linkName, UnitAlgebra(linkBandwidth), num_vcs, UnitAlgebra(linkInbufSize), UnitAlgebra(linkOutbufSize));
    
        // Packet size
        UnitAlgebra packetSize = UnitAlgebra(params.find<std::string>(pref[i] + ".min_packet_size", "8B"));
        if (!packetSize.hasUnits("B"))
            dbg.fatal(CALL_INFO, -1, "Invalid param(%s): %s.min_packet_size - must have units of bytes (B). SI units OK. You specified '%s'\n.",
                    getName().c_str(), pref[i].c_str(), packetSize.toString().c_str());

        packetHeaderBytes[i] = packetSize.getRoundedValue();
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

    // Attempt send on the control network
    for (int i = 0; i < 4; i++) {
        while (!sendQueue[i].empty()) {
            SimpleNetwork::Request *head = sendQueue[i].front();
/* Debug info - record before we attempt send so that if send destroys anything we have it */
#ifdef __SST_DEBUG_OUTPUT__
            MemEventBase * ev = (static_cast<MemRtrEvent*>(head->inspectPayload()))->event;
            std::string debugEvStr;
            uint64_t dst = head->dest;
            bool doDebug = false;
            if (ev) { 
                debugEvStr = ev->getBriefString();
                doDebug = is_debug_event(ev);
            }
#endif
            if (link_control[i]->spaceToSend(0, head->size_in_bits) && link_control[i]->send(head, 0)) {
#ifdef __SST_DEBUG_OUTPUT__
                if (!debugEvStr.empty() && doDebug) {
                    dbg.debug(_L9_, "%s (memNIC), Sending message %s to dst addr %" PRIu64 " on link control %s\n",
                            getName().c_str(), debugEvStr.c_str(), dst, (i == REQ) ? "req" : "data");
                }
#endif
                sendQueue[i].pop();
            } else {
                break;
            }
        }
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

    OrderedMemRtrEvent * omre = new OrderedMemRtrEvent(ev, tag);
    
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
    SimpleNetwork::Request * req = link_control[REQ]->recv(0);
    doRecv(req, REQ);
    return true;
}
bool MemNICFour::recvNotifyAck(int) {
    SimpleNetwork::Request * req = link_control[ACK]->recv(0);
    doRecv(req, ACK);
    return true;
}
bool MemNICFour::recvNotifyFwd(int) {
    SimpleNetwork::Request * req = link_control[FWD]->recv(0);
    doRecv(req, FWD);
    return true;
}
bool MemNICFour::recvNotifyData(int) {
    SimpleNetwork::Request * req = link_control[DATA]->recv(0);
    doRecv(req, DATA);
    return true;
}

void MemNICFour::doRecv(SimpleNetwork::Request * req, NetType net) {
    uint64_t src = req->src;
    OrderedMemRtrEvent * mre = processRecv(req); // Return the splitmemrtrevent if we have one
                
    if (mre != nullptr) {
        dbg.debug(_L3_, "%s, memNIC received a message: <%" PRIu64 ", %u>\n",
                getName().c_str(), src, mre->tag);

        stat_oooDepthSrc->addData(orderBuffer[src].size());
        stat_oooDepth->addData(totalOOO);
        if (mre->tag == recvTags[src]) { // Got the tag we were expecting
            stat_oooEvent[net]->addData(0); // Count total number of events received
            recvTags[src]++;
            
            if (recvQueue.empty())
                recvNotify(mre);
            else {
                // Still push one message for every receive...
                recvQueue.push(mre);
                recvNotify(recvQueue.front());
                recvQueue.pop();
            }

            while (orderBuffer[src].find(recvTags[src]) != orderBuffer[src].end()) {
                totalOOO--;
                recvQueue.push(orderBuffer[src][recvTags[src]].first);
                stat_orderLatency->addData(getCurrentSimTime() - orderBuffer[src][recvTags[src]].second);
                orderBuffer[src].erase(recvTags[src]);
                recvTags[src]++;
            }
        } else {
            totalOOO++;
            stat_oooEvent[net]->addData(1); // Count number of out of order events received
            orderBuffer[src][mre->tag] = std::make_pair(mre,getCurrentSimTime());
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

MemNICFour::OrderedMemRtrEvent* MemNICFour::processRecv(SimpleNetwork::Request * req) {
    if (req != nullptr) {
        MemRtrEvent * mre = static_cast<MemRtrEvent*>(req->takePayload());
        delete req;
        
        if (mre->hasClientData()) {
            OrderedMemRtrEvent * smre = static_cast<OrderedMemRtrEvent*>(mre);
            if (smre->event != nullptr) {
                MemEventBase * ev = static_cast<MemEventBase*>(smre->event);
                ev->setDeliveryLink(smre->getLinkId(), NULL);
            }
            return smre;
        } else {
            InitMemRtrEvent *imre = static_cast<InitMemRtrEvent*>(mre);
            if (networkAddressMap.find(imre->info.name) == networkAddressMap.end()) {
                dbg.fatal(CALL_INFO, -1, "%s (MemNIC), received information about previously unknown endpoint. This case is not handled. Endpoint name: %s\n",
                        getName().c_str(), imre->info.name.c_str());
            }
            if (sourceIDs.find(imre->info.id) != sourceIDs.end()) {
                sourceEndpointInfo.insert(imre->info);
            } else if (destIDs.find(imre->info.id) != destIDs.end()) {
                destEndpointInfo.insert(imre->info);
            }
            delete imre;
        }
    } 
    return nullptr;
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
