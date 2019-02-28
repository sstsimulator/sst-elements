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
#include "sst/elements/memHierarchy/memNICMulti.h"

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
MemNICMulti::MemNICMulti(Component * parent, Params &params) : MemNICBase(parent, params) {

    /* MemNIC init will set up the control network */
    /* We set up data network */
    bool found;

    // Get network parameters and create link control(s)
    std::string linkName = params.find<std::string>("data.port", "");
    if (linkName == "")
        dbg.fatal(CALL_INFO, -1, "Param not specified(%s): data.port - the name of the port that the MemNIC's data network is attached to. This should be set internally by components creating the memNIC.\n",
                getName().c_str());

    // Error checking for much of this is done by the link control
    std::string linkBandwidth = params.find<std::string>("data.network_bw", "80GiB/s");
    int num_vcs = 1; // MemNIC does not use VCs
    std::string linkInbufSize = params.find<std::string>("data.network_input_buffer_size", "1KiB");
    std::string linkOutbufSize = params.find<std::string>("data.network_output_buffer_size", "1KiB");

    data_link_control = (SimpleNetwork*)parent->loadSubComponent(params.find<std::string>("data.linkcontrol", "merlin.linkcontrol"), parent, params);
    // But link control doesn't use params so manually initialize
    data_link_control->initialize(linkName, UnitAlgebra(linkBandwidth), num_vcs, UnitAlgebra(linkInbufSize), UnitAlgebra(linkOutbufSize));

    // Packet size
    UnitAlgebra packetSize = UnitAlgebra(params.find<std::string>("data.min_packet_size", "8B"));
    if (!packetSize.hasUnits("B"))
        dbg.fatal(CALL_INFO, -1, "Invalid param(%s): data.min_packet_size - must have units of bytes (B). SI units OK. You specified '%s'\n.",
                getName().c_str(), packetSize.toString().c_str());

    dataPacketHeaderBytes = packetSize.getRoundedValue();

    // Set link control to call recvNotify on event receive
    data_link_control->setNotifyOnReceive(new SimpleNetwork::Handler<MemNICMulti>(this, &MemNICMulti::recvNotifyData));
    link_control->setNotifyOnReceive(new SimpleNetwork::Handler<MemNICMulti>(this, &MemNICMulti::recvNotifyCtrl));

    // Are we splitting data packets across data/ctrl or letting the coherence protocol take care of ordering?
    dupCtrl = params.find<bool>("dupCtrl", true);

    nextTag = 1;
}

void MemNICMulti::init(unsigned int phase) {
    data_link_control->init(phase);

    MemNICBase::init(phase);

}

/*
 * Called by parent on a clock
 * Returns whether anything sent this cycle
 */
bool MemNICMulti::clock() {
    if (sendQueue.empty() && dataSendQueue.empty()) return true;

    // Attempt send on the control network
    while (!sendQueue.empty()) {
        SimpleNetwork::Request *head = sendQueue.front();
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
        if (link_control->spaceToSend(0, head->size_in_bits) && link_control->send(head, 0)) {
#ifdef __SST_DEBUG_OUTPUT__
            if (!debugEvStr.empty() && doDebug) {
                dbg.debug(_L9_, "%s (memNIC), Sending message %s to dst addr %" PRIu64 "\n",
                        getName().c_str(), debugEvStr.c_str(), dst);
            }
#endif
            sendQueue.pop();
        } else {
            break;
        }
    }

    while (!dataSendQueue.empty()) {
        SimpleNetwork::Request *head = dataSendQueue.front();
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
        if (data_link_control->spaceToSend(0, head->size_in_bits) && data_link_control->send(head, 0)) {
#ifdef __SST_DEBUG_OUTPUT__
            if (!debugEvStr.empty() && doDebug) {
                dbg.debug(_L9_, "%s (memNIC), Sending message %s to dst addr %" PRIu64 "\n",
                        getName().c_str(), debugEvStr.c_str(), dst);
            }
#endif
            dataSendQueue.pop();
        } else {
            break;
        }
    }

    return false;
}

/* Send event to memNIC */
void MemNICMulti::send(MemEventBase *ev) {
    bool ctrlmsg = ev->getPayloadSize() == 0;

    SimpleNetwork::Request * req = new SimpleNetwork::Request();
    req->vn = 0;
    req->src = info.addr;
    req->dest = lookupNetworkAddress(ev->getDst());

    if (dupCtrl && !ctrlmsg) {
        SimpleNetwork::Request * dreq = new SimpleNetwork::Request();
        dreq->vn = 0;
        dreq->src = req->src;
        dreq->dest = req->dest;

        SplitMemRtrEvent * cmre = new SplitMemRtrEvent(ev, nextTag);    // ev always travels with control
        SplitMemRtrEvent * dmre = new SplitMemRtrEvent(nextTag);        // placeholder to force ordering

        nextTag++;

        req->size_in_bits = 8 * packetHeaderBytes;
        dreq->size_in_bits = getSizeInBits(ev);

        req->givePayload(cmre);
        dreq->givePayload(dmre);

        if (is_debug_event(ev)) {
                dbg.debug(_L9_, "%s, memNIC adding to control send queue: dst: %" PRIu64 ", bits: %zu, cmd: %s\n",
                        getName().c_str(), req->dest, req->size_in_bits, CommandString[(int)ev->getCmd()]);

            dbg.debug(_L9_, "%s, memNIC adding to data send queue: dst: %" PRIu64 ", bits: %zu, cmd: %s\n",
                        getName().c_str(), dreq->dest, dreq->size_in_bits, CommandString[(int)ev->getCmd()]);
        }

        sendQueue.push(req);
        dataSendQueue.push(dreq);

    } else {
        SplitMemRtrEvent * mre = new SplitMemRtrEvent(ev);
        req->givePayload(mre);

        if (ctrlmsg) {
            req->size_in_bits = getSizeInBits(ev);
            if (is_debug_event(ev)) {
                dbg.debug(_L9_, "%s, memNIC adding to control send queue: dst: %" PRIu64 ", bits: %zu, cmd: %s\n",
                        getName().c_str(), req->dest, req->size_in_bits, CommandString[(int)ev->getCmd()]);
            }
            sendQueue.push(req);
        } else {
            req->size_in_bits = getSizeInBits(ev);
            if (is_debug_event(ev)) {
                dbg.debug(_L9_, "%s, memNIC adding to data send queue: dst: %" PRIu64 ", bits: %zu, cmd: %s\n",
                        getName().c_str(), req->dest, req->size_in_bits, CommandString[(int)ev->getCmd()]);
            }
            dataSendQueue.push(req);
        }
    }
}


/*
 * Event handler called by link control on event receive
 * Return whether event can be received
 */
bool MemNICMulti::recvNotifyCtrl(int) {
    SimpleNetwork::Request * req = link_control->recv(0);
    uint64_t src = req->src;
    SplitMemRtrEvent * mre = processRecv(req); // Return the splitmemrtrevent if we have one

    dbg.debug(_L9_, "%s, memNIC received a control message: <%" PRIu64 ", %u>\n",
            getName().c_str(), src, mre->tag);

    if (mre != nullptr) {
        // If we need to match tags, check if data has arrived
        // If we matched tags or we don't need to match tags, return if no queue, otherwise put in queue
        // If we can't match tags yet, put in queue

        if (mre->tag != 0) {
            if (!matchDataTags(src, mre->tag)) {
                recvMsgQueue[src].push(std::make_pair(false,mre));
                dbg.debug(_L9_, "   No matching data yet\n");

                return true;
            }
        }
        // Either matched tags or we don't need to
        if (recvMsgQueue[src].empty()) {
            dbg.debug(_L9_, "   Matched/queue empty: proceeding\n");
            recvNotify(mre);
        } else {
            recvMsgQueue[src].push(std::make_pair(true,mre));
            dbg.debug(_L9_, "   Matched/queue not empty: stalling\n");
        }
    }
    return true;
}

bool MemNICMulti::recvNotifyData(int) {
    /* Either we haven't received the control yet, or this matches the top waiting control message */
    SimpleNetwork::Request * req = data_link_control->recv(0);
    uint64_t src = req->src;
    SplitMemRtrEvent * dmre = processRecv(req);
    dbg.debug(_L9_, "%s, memNIC received a data message: <%" PRIu64 ", %u>\n",
            getName().c_str(), src, dmre->tag);


    if (dmre != nullptr) {
        if (matchCtrlTags(src, dmre->tag)) {
            SplitMemRtrEvent * cmre = recvMsgQueue[src].front().second;
            recvMsgQueue[src].pop();
            delete dmre;
            dbg.debug(_L9_, "   Matched data to waiting control: proceeding\n");
            recvNotify(cmre); // We matched
            while (!recvMsgQueue[src].empty() && recvMsgQueue[src].front().first == true) {
                cmre = recvMsgQueue[src].front().second;
                recvMsgQueue[src].pop();
                dbg.debug(_L9_, "   Clearing waiting message: %u\n", cmre->tag);
                recvNotify(cmre); // Waiting control message
            }
        } else {
            delete dmre;
        }
    }
    return true;
}

void MemNICMulti::recvNotify(SplitMemRtrEvent* mre) {
    MemEventBase * me = static_cast<MemEventBase*>(mre->event);
    delete mre;

    if (!me) return;

    if (is_debug_event(me)) {
        dbg.debug(_L9_, "%s, memNIC ctrl recv: src: %s. cmd: %s\n",
                getName().c_str(), me->getSrc().c_str(), CommandString[(int)me->getCmd()]);
    }

    // Call parent's handler
    (*recvHandler)(me);
}

MemNICMulti::SplitMemRtrEvent* MemNICMulti::processRecv(SimpleNetwork::Request * req) {
    if (req != nullptr) {
        MemRtrEvent * mre = static_cast<MemRtrEvent*>(req->takePayload());
        delete req;

        if (mre->hasClientData()) {
            SplitMemRtrEvent * smre = static_cast<SplitMemRtrEvent*>(mre);
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

bool MemNICMulti::matchDataTags(uint64_t src, unsigned int tag) {
    if (dataTags[src].find(tag) != dataTags[src].end()) {
        dataTags[src].erase(tag);
        return true; // Found!
    } else {
        ctrlTags[src].insert(tag);
        return false;
    }
}

bool MemNICMulti::matchCtrlTags(uint64_t src, unsigned int tag) {
    if (ctrlTags[src].find(tag) != ctrlTags[src].end()) {
        ctrlTags[src].erase(tag);
        return true;
    } else {
        dataTags[src].insert(tag);
        return false;
    }
}

/** Helper functions **/
/* Calculate size in bits of an event */
size_t MemNICMulti::getSizeInBits(MemEventBase *ev) {
    return 8 * (dataPacketHeaderBytes + ev->getPayloadSize());
}
