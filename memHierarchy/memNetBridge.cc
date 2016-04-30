// Copyright 2009-2016 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2016, Sandia Corporation
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#include <sst_config.h>
#include "memNetBridge.h"

using namespace SST::MemHierarchy;


MemNetBridge::MemNetBridge(SST::ComponentId_t id, SST::Params &params) :
    SST::Component(id)
{
    int debugLevel = params.find<int>("debug_level", 0);
    dbg.init("", debugLevel, 0, (Output::output_location_t)params.find<int>("debug", 0));

    configureNIC(0, params);
    configureNIC(1, params);

    peerCount[0] = peerCount[1] = 0;
}


MemNetBridge::~MemNetBridge()
{
    delete interfaces[0];
    delete interfaces[1];
}


void MemNetBridge::init(unsigned int phase)
{
    interfaces[0]->init(phase);
    interfaces[1]->init(phase);

    updatePeerInfo(0);
    updatePeerInfo(1);
}


void MemNetBridge::setup(void)
{
    interfaces[0]->setup();
    interfaces[1]->setup();
}


void MemNetBridge::finish(void)
{
    interfaces[0]->finish();
    interfaces[1]->finish();
}



void MemNetBridge::configureNIC(uint8_t nic, SST::Params &params)
{
    MemNIC::ComponentInfo myInfo;
    myInfo.link_port         = "network" + to_string(nic);
    myInfo.link_bandwidth    = params.find<std::string>("network_bw");
    myInfo.num_vcs           = 1;
    myInfo.name              = getName();
    myInfo.network_addr      = params.find<uint64_t>("network" + to_string(nic) + "_addr");
    myInfo.type              = MemNIC::TypeSmartMemory; /* Can send and receive */
    myInfo.link_inbuf_size   = params.find<std::string>("network_input_buffer_size", "1KiB");
    myInfo.link_outbuf_size  = params.find<std::string>("network_output_buffer_size", "1KiB");
    interfaces[nic] = new MemNIC(this, &dbg, 0, myInfo, new Event::Handler<MemNetBridge, uint8_t>(this, &MemNetBridge::handleIncoming, nic));
}


void MemNetBridge::updatePeerInfo(uint8_t nic)
{
    MemNIC *outNic = interfaces[(nic ^ 1)];

    const std::vector<MemNIC::PeerInfo_t> &pi = interfaces[nic]->getPeerInfo();
    while ( peerCount[nic] < pi.size() ) {
        const MemNIC::ComponentInfo &ci = pi[peerCount[nic]].first;
        switch ( ci.type ) {
        case MemNIC::TypeDirectoryCtrl:
        case MemNIC::TypeNetworkDirectory:
        case MemNIC::TypeMemory:
        case MemNIC::TypeSmartMemory: {
            const MemNIC::ComponentTypeInfo &info = pi[peerCount[nic]].second;
            outNic->addTypeInfo(info);
            dbg.debug(CALL_INFO, 1, 0, "Bridge: Sending TypeInfo from nic %u to %u\n",
                    nic, (nic^1));
            dbg.debug(CALL_INFO, 2, 0, "\t%s @ %d  [%d]\t[0x%" PRIx64 " - 0x%" PRIx64 " [0x%" PRIx64 "/0x%" PRIx64 "]]\n",
                    ci.name.c_str(), ci.network_addr, ci.type,
                    info.rangeStart, info.rangeEnd, info.interleaveSize, info.interleaveStep);
            break;
        }
        default:
            break;
        }


        peerCount[nic]++;
    }
}


void MemNetBridge::handleIncoming(SST::Event *event, uint8_t nic)
{
    MemNIC *outNic = interfaces[(nic ^ 1)];

    MemEvent *ev = static_cast<MemEvent*>(event);
    ev->setDst(outNic->findTargetDestination(ev->getAddr()));
    /* TODO:  send delay? */
    outNic->send(ev);
}
