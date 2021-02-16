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
#include "opalMemNIC.h"

using namespace SST;
using namespace SST::Opal;

/* Constructor */

OpalMemNIC::OpalMemNIC(ComponentId_t id, Params &params) : SST::MemHierarchy::MemNICBase(id, params) {
    build(params);
}

void OpalMemNIC::build(Params &params) {

    node = params.find<uint32_t>("node", 0);
    enable = params.find<bool>("shared_memory", true);
    localMemSize = params.find<uint64_t>("local_memory_size", 0);

    /* Set up link control */
    link_control = loadUserSubComponent<SST::Interfaces::SimpleNetwork>("linkcontrol", ComponentInfo::SHARE_NONE, 1);
    if (!link_control) {
        Params lcparams;
        lcparams.insert("port_name", "port");
        lcparams.insert("link_bw", params.find<std::string>("network_bw", "80GiB/s"));
        lcparams.insert("in_buf_size", params.find<std::string>("network_input_buffer_size", "1KiB"));
        lcparams.insert("out_buf_size", params.find<std::string>("network_output_buffer_size", "1KiB"));
        std::string lcSub = params.find<std::string>("linkcontrol", "merlin.linkcontrol");
        link_control = loadAnonymousSubComponent<SST::Interfaces::SimpleNetwork>(lcSub, "linkcontrol", 0, ComponentInfo::SHARE_PORTS | ComponentInfo::INSERT_STATS, lcparams, 1);
    }
    link_control->setNotifyOnReceive(new SST::Interfaces::SimpleNetwork::Handler<OpalMemNIC>(this, &OpalMemNIC::recvNotify));

    packetHeaderBytes = extractPacketHeaderSize(params, "min_packet_size");
}

void OpalMemNIC::init(unsigned int phase) {
    link_control->init(phase);
    MemNICBase::nicInit(link_control, phase);
}

bool OpalMemNIC::clock() {
    if (sendQueue.empty()) return true;
    drainQueue(&sendQueue, link_control);
    return false;
}

bool OpalMemNIC::recvNotify(int) {
    MemRtrEvent * mre = doRecv(link_control);
    if (mre) {
        MemHierarchy::MemEventBase * me = mre->event;
        delete mre;
        if (me) {
            (*recvHandler)(me);
        }
    }
    return true;
}

void OpalMemNIC::send(MemHierarchy::MemEventBase * ev) {
    SST::Interfaces::SimpleNetwork::Request * req = new SST::Interfaces::SimpleNetwork::Request();
    MemRtrEvent * mre = new MemRtrEvent(ev);
    req->src = info.addr;
    req->dest = lookupNetworkAddress(ev->getDst());
    req->size_in_bits = 8 * (packetHeaderBytes + ev->getPayloadSize());
    req->vn = 0;
    req->givePayload(mre);
    sendQueue.push(req);
}


/* Add 'node' to InitMemRtrEvent */
MemHierarchy::MemNICBase::InitMemRtrEvent * OpalMemNIC::createInitMemRtrEvent() {
    return new OpalInitMemRtrEvent(info, node);
}

void OpalMemNIC::processInitMemRtrEvent(MemHierarchy::MemNICBase::InitMemRtrEvent * ev) {
    OpalInitMemRtrEvent* imre = static_cast<OpalInitMemRtrEvent*>(ev);
    dbg.debug(_L10_, "%s (OpalMemNIC) received imre. Name: %s, Addr: %" PRIu64 ", ID: %" PRIu32 ", start: %" PRIu64 ", end: %" PRIu64 ", size: %" PRIu64 ", step: %" PRIu64 ", node: %" PRIu32 "\n",
            getName().c_str(), imre->info.name.c_str(), imre->info.addr, imre->info.id, imre->info.region.start, imre->info.region.end, imre->info.region.interleaveSize, imre->info.region.interleaveStep, imre->node);

    if (sourceIDs.find(imre->info.id) != sourceIDs.end()) { // From one of our source groups
        dbg.debug(_L10_, "\tAdding to sourceEndpointInfo. %zu sources found\n", sourceEndpointInfo.size());
        addSource(imre->info);
    } else if (destIDs.find(imre->info.id) != destIDs.end()) {  // From one of our dest groups
        // Filter by node if sharedmem is enabled
        if (enable) {
            if (imre->node == node || imre->node == 9999) {
                dbg.debug(_L10_, "\tAdding to destEndpointInfo. %zu destinations found\n", destEndpointInfo.size());
                addDest(imre->info);
            }
        } else {
            dbg.debug(_L10_, "\tAdding to destEndpointInfo. %zu destinations found\n", destEndpointInfo.size());
            addDest(imre->info);
        }
    }
}


std::string OpalMemNIC::findTargetDestination(MemHierarchy::Addr addr) {
    for (std::set<MemHierarchy::MemLinkBase::EndpointInfo>::const_iterator it = destEndpointInfo.begin(); it != destEndpointInfo.end(); it++) {
        if (it->region.contains(addr)) return it->name;
    }

    if (enable && localMemSize) {
        MemHierarchy::Addr tempAddr = addr & (localMemSize-1);
        for (std::set<MemHierarchy::MemLinkBase::EndpointInfo>::const_iterator it = destEndpointInfo.begin(); it != destEndpointInfo.end(); it++) {
            if(it->region.contains(tempAddr)) return it->name;
        }
    }

    /* Build error string */
    stringstream error;
    error << getName() + " (OpalMemNIC) cannot find a destination for address " << addr << endl;
    error << "Known destination regions: " << endl;
    for (std::set<MemHierarchy::MemLinkBase::EndpointInfo>::const_iterator it = destEndpointInfo.begin(); it != destEndpointInfo.end(); it++) {
        error << it->name << " " << it->region.toString() << endl;
    }
    dbg.fatal(CALL_INFO, -1, "%s", error.str().c_str());
    return "";
}
