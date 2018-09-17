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
#include "opalMemNIC.h"

using namespace SST;
using namespace SST::Opal;

/* Constructor */
OpalMemNIC::OpalMemNIC(Component * parent, Params &params) : SST::MemHierarchy::MemNICBase(parent, params) {

    info.node = params.find<uint32_t>("node", 0);
    enable = params.find<bool>("shared_memory", true);
    localMemSize = params.find<uint64_t>("local_memory_size", 0);

    /* Set up link control */
    std::string linkBandwidth = params.find<std::string>("network_bw", "80GiB/s");
    std::string linkInbufSize = params.find<std::string>("network_input_buffer_size", "1KiB");
    std::string linkOutbufSize = params.find<std::string>("network_output_buffer_size", "1KiB");

    link_control = (SST::Interfaces::SimpleNetwork*)loadSubComponent(params.find<std::string>("linkcontrol", "merlin.linkcontrol"), params);
    link_control->initialize("port", UnitAlgebra(linkBandwidth), 1, UnitAlgebra(linkInbufSize), UnitAlgebra(linkOutbufSize));
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


/* Override addDest to grab based on node IDs */
void OpalMemNIC::addDest(MemHierarchy::MemLinkBase::EndpointInfo dstInfo) {
    if (enable && (info.node == dstInfo.node || dstInfo.node == 9999)) {
        destEndpointInfo.insert(dstInfo);
    } else {
        MemNICBase::addDest(dstInfo);
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
    error << getName() + " (MemLinkBase) cannot find a destination for address " << addr << endl;
    error << "Known destination regions: " << endl;
    for (std::set<MemHierarchy::MemLinkBase::EndpointInfo>::const_iterator it = destEndpointInfo.begin(); it != destEndpointInfo.end(); it++) {
        error << it->name << " " << it->region.toString() << endl;
    }
    dbg.fatal(CALL_INFO, -1, "%s", error.str().c_str());
    return "";
}
