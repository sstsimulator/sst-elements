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

#include <sst_config.h>
#include "sst/elements/memHierarchy/memNIC.h"

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


/******************************************************************/
/*** MemNIC implementation ************************************/
/******************************************************************/

/* Constructor */
MemNIC::MemNIC(Component * parent, Params &params) : MemNICBase(parent, params) {
    
    // Get network parameters and create link control
    std::string linkName = params.find<std::string>("port", "port");

    // Error checking for much of this is done by the link control
    bool found;
    std::string linkBandwidth = params.find<std::string>("network_bw", "80GiB/s");
    int num_vcs = 1; // MemNIC does not use VCs
    std::string linkInbufSize = params.find<std::string>("network_input_buffer_size", "1KiB");
    std::string linkOutbufSize = params.find<std::string>("network_output_buffer_size", "1KiB");

    link_control = (SimpleNetwork*)loadSubComponent("merlin.linkcontrol", params); // But link control doesn't use params so manually initialize
    link_control->initialize(linkName, UnitAlgebra(linkBandwidth), num_vcs, UnitAlgebra(linkInbufSize), UnitAlgebra(linkOutbufSize));

    // Packet size
    UnitAlgebra packetSize = UnitAlgebra(params.find<std::string>("min_packet_size", "8B"));
    if (!packetSize.hasUnits("B"))
        dbg.fatal(CALL_INFO, -1, "Invalid param(%s): min_packet_size - must have units of bytes (B). SI units OK. You specified '%s'\n.",
                getName().c_str(), packetSize.toString().c_str());

    packetHeaderBytes = packetSize.getRoundedValue();

    // Set link control to call recvNotify on event receive
    link_control->setNotifyOnReceive(new SimpleNetwork::Handler<MemNIC>(this, &MemNIC::recvNotify));
}

void MemNIC::init(unsigned int phase) {
    link_control->init(phase);  // This MUST be called before anything else
    MemNICBase::nicInit(link_control, phase);
}

/* 
 * Called by parent on a clock 
 * Returns whether anything sent this cycle
 */
bool MemNIC::clock() {
    if (sendQueue.empty()) return true;
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
    return false;
}

/* 
 * Event handler called by link control on event receive 
 * Return whether event can be received
 */
bool MemNIC::recvNotify(int) {
    MemEventBase * me = recv();
    if (me) {
        if (is_debug_event(me)) {
            dbg.debug(_L9_, "%s, memNIC recv: src: %s. cmd: %s\n", 
                    getName().c_str(), me->getSrc().c_str(), CommandString[(int)me->getCmd()]);
        }

        // Call parent's handler
        (*recvHandler)(me);
    }
    return true;
}


/* Send event to memNIC */
void MemNIC::send(MemEventBase *ev) {
    SimpleNetwork::Request *req = new SimpleNetwork::Request();
    MemRtrEvent * mre = new MemRtrEvent(ev);
    req->src = info.addr;
    req->dest = lookupNetworkAddress(ev->getDst());
    req->size_in_bits = getSizeInBits(ev);
    req->vn = 0;
    
    if (is_debug_event(ev)) {
        dbg.debug(_L9_, "%s, memNIC adding to send queue: dst: %s, bits: %zu, cmd: %s\n",
                getName().c_str(), ev->getDst().c_str(), req->size_in_bits, CommandString[(int)ev->getCmd()]);
    }

    req->givePayload(mre);
    sendQueue.push(req);
}


/** Helper functions **/

/* Calculate size in bits of an event */
size_t MemNIC::getSizeInBits(MemEventBase *ev) {
    return 8 * (packetHeaderBytes + ev->getPayloadSize());
}


/* 
 * Polling-based receive function
 */
MemEventBase* MemNIC::recv() {
    SimpleNetwork::Request *req = link_control->recv(0);
    if (req != nullptr) {
        MemRtrEvent *mre = static_cast<MemRtrEvent*>(req->takePayload());
        delete req;
        
        if (mre->hasClientData()) {
            MemEventBase * ev = mre->event;
            ev->setDeliveryLink(mre->getLinkId(), NULL);
            delete mre;
            return ev;
        } else { /* InitMemRtrEvent - someone updated their info */
            InitMemRtrEvent *imre = static_cast<InitMemRtrEvent*>(mre);
            if (networkAddressMap.find(imre->info.name) == networkAddressMap.end()) {
                dbg.fatal(CALL_INFO, -1, "%s (MemNIC), received information about previously unknown endpoint. This case is not handled. Endpoint name: %s\n",
                        getName().c_str(), imre->info.name.c_str());
            }
            if (sourceIDs.find(imre->info.id) != sourceIDs.end()) {
                //sourceEndpointInfo.insert(imre->info);
            	addSource(imre->info);
            } else if (destIDs.find(imre->info.id) != destIDs.end()) {
                //destEndpointInfo.insert(imre->info);
            	addDest(imre->info);
            }
            delete imre;
        }
    }
    return nullptr;
}


void MemNIC::printStatus(Output &out) {
    out.output("  MemHierarchy::MemNIC\n");
    out.output("    Send queue (%zu entries):\n", sendQueue.size());
    
    // Since this is just debug/fatal we're just going to read out the queue & re-populate it
    std::queue<SST::Interfaces::SimpleNetwork::Request*> tmpQ;
    while (!sendQueue.empty()) {
        MemEventBase * ev = static_cast<MemRtrEvent*>(sendQueue.front()->inspectPayload())->event;
        out.output("      %s\n", ev->getVerboseString().c_str());
        tmpQ.push(sendQueue.front());
        sendQueue.pop();
    }
    tmpQ.swap(sendQueue);
    out.output("    Link status: \n");
    link_control->printStatus(out);
    out.output("  End MemHierarchy::MemNIC\n");
}

