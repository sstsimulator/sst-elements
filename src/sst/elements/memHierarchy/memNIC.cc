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

    link_control = (SimpleNetwork*)loadSubComponent(params.find<std::string>("linkcontrol", "merlin.linkcontrol"), params); // But link control doesn't use params so manually initialize
    link_control->initialize(linkName, UnitAlgebra(linkBandwidth), num_vcs, UnitAlgebra(linkInbufSize), UnitAlgebra(linkOutbufSize));
    link_control->setNotifyOnReceive(new SimpleNetwork::Handler<MemNIC>(this, &MemNIC::recvNotify));

    // Packet size
    packetHeaderBytes = extractPacketHeaderSize(params, "min_packet_size");
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

    drainQueue(&sendQueue, link_control);
    
    return false;
}

/* 
 * Event handler called by link control on event receive 
 * Return whether event can be received
 */
bool MemNIC::recvNotify(int) {
    MemRtrEvent * mre = doRecv(link_control);
    if (mre) {
        MemEventBase * ev = mre->event;
        delete mre;
        if (ev) {
            if (is_debug_event(ev)) {
                dbg.debug(_L9_, "%s, memNIC recv: src: %s. cmd: %s\n", 
                        getName().c_str(), ev->getSrc().c_str(), CommandString[(int)ev->getCmd()]);
            }
            (*recvHandler)(ev);
        }
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

