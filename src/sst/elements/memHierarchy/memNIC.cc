// Copyright 2013-2021 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2013-2021, NTESS
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

MemNIC::MemNIC(ComponentId_t id, Params &params, TimeConverter* tc) : MemNICBase(id, params, tc) {
    
    link_control = loadUserSubComponent<SimpleNetwork>("linkcontrol", ComponentInfo::SHARE_NONE, 1); // 1 is the num virtual networks
    if (!link_control) {
        Params netparams;
        netparams.insert("port_name", params.find<std::string>("port", "port"));
        netparams.insert("in_buf_size", params.find<std::string>("network_input_buffer_size", "1KiB"));
        netparams.insert("out_buf_size", params.find<std::string>("network_output_buffer_size", "1KiB"));
        netparams.insert("link_bw", params.find<std::string>("network_bw", "80GiB/s"));
        std::string link_control_class = params.find<std::string>("network_link_control", "merlin.linkcontrol");

        if (link_control_class != "merlin.linkcontrol")
            dbg.output("%s, Warning: use of the 'network_link_control' parameter is deprecated in favor of specifying a named 'linkcontrol' subcomponent in the input configuration.\n",
                    getName().c_str());

        link_control = loadAnonymousSubComponent<SimpleNetwork>(link_control_class, "linkcontrol", 0, ComponentInfo::SHARE_PORTS | ComponentInfo::INSERT_STATS, netparams, 1);
    }
    link_control->setNotifyOnReceive(new SimpleNetwork::Handler<MemNIC>(this, &MemNIC::recvNotify));

    // Packet size
    packetHeaderBytes = extractPacketHeaderSize(params, "min_packet_size");

    clockHandler = new Clock::Handler<MemNIC>(this, &MemNIC::clock);
    clockTC = registerClock(tc, clockHandler);
}

void MemNIC::init(unsigned int phase) {
    link_control->init(phase);  // This MUST be called before anything else
    MemNICBase::nicInit(link_control, phase);
}

/*
 * Called by parent on a clock
 * Returns whether anything sent this cycle
 */
bool MemNIC::clock(SimTime_t cycle) {
    drainQueue(&sendQueue, link_control);
    if (sendQueue.empty()) return true; /* turn off clock */
    return false;
}

/*
 * Event handler called by link control on event receive
 * Return whether event can be received
 */
bool MemNIC::recvNotify(int) {
    MemRtrEvent * mre = doRecv(link_control);
    if (mre) {
        MemEventBase* ev = mre->event;
        delete mre;
        if (ev) {
            if (is_debug_event(ev)) {
                dbg.debug(_L5_, "E: %-40" PRIu64 "  %-20s NIC:Recv      (%s)\n", 
                    getCurrentSimCycle(), getName().c_str(), ev->getBriefString().c_str());
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
        dbg.debug(_L5_, "N: %-40" PRIu64 "  %-20s Enqueue       Dst: %lld, bits: %zu, (%s)\n", 
            getCurrentSimCycle(), getName().c_str(), req->dest, req->size_in_bits, ev->getBriefString().c_str());
    }

    req->givePayload(mre);
    sendQueue.push(req);
    //printf("%s, %" PRIu64 ", Receive %s, 0x%" PRIx64 "\n", getName().c_str(), getCurrentSimTime("1ps"), CommandString[(int)ev->getCmd()], ev->getRoutingAddress() );
    if (sendQueue.size() == 1)
        drainQueue(&sendQueue, link_control);
    if (sendQueue.size() == 1) /* Attempt again in 1 cycle */
        reregisterClock(clockTC, clockHandler);
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
        out.output("      %s\n", ev->getVerboseString(out.getVerboseLevel()).c_str());
        tmpQ.push(sendQueue.front());
        sendQueue.pop();
    }
    tmpQ.swap(sendQueue);
    out.output("    Link status: \n");
    link_control->printStatus(out);
    out.output("  End MemHierarchy::MemNIC\n");
}

void MemNIC::emergencyShutdownDebug(Output &out) {
    out.output(" Draining link control...\n");
    MemRtrEvent * mre = doRecv(link_control);
    while (mre != nullptr) {
        MemEventBase * ev = mre->event;
        delete mre;
        if (ev) {
            out.output("      Undelivered message: %s\n", ev->getVerboseString(out.getVerboseLevel()).c_str());
        }
        mre = doRecv(link_control);
    }
}
