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
#include "memNetBridge.h"

#include "memNIC.h"

using namespace SST::MemHierarchy;
using SST::Merlin::Bridge;
using SST::Interfaces::SimpleNetwork;



MemNetBridge::MemNetBridge(SST::Component *comp, SST::Params &params) :
    Bridge::Translator(comp, params)
{
    int debugLevel = params.find<int>("debug_level", 0);
    dbg.init("@t:Bridge::@p():@l " + comp->getName() + ": ",
            debugLevel, 0, (Output::output_location_t)params.find<int>("debug", 0));
}


MemNetBridge::~MemNetBridge()
{
}


void MemNetBridge::init(unsigned int phase)
{
}

void MemNetBridge::setup(void)
{
    for ( int i = 0 ; i < 2 ; i++ ) {
        Net_t &net = networks[i];
        dbg.debug(CALL_INFO, 2, 0, "Address Map on Interface %u:\n", i);
        for ( auto a : net.map ) {
            dbg.debug(CALL_INFO, 2, 0, "\t%s -> %" PRIu64 "\n", a.first.c_str(), a.second);
        }
    }
}


void MemNetBridge::finish(void)
{
}


SimpleNetwork::Request* MemNetBridge::initTranslate(SimpleNetwork::Request *req, uint8_t fromNet)
{
    dbg.debug(CALL_INFO, 2, 0, "Received init phase event on interface %d\n", fromNet);
    Event *payload = req->inspectPayload();
    MemNIC::InitMemRtrEvent *imre = dynamic_cast<MemNIC::InitMemRtrEvent*>(payload);
    if ( imre ) {
        networks[fromNet].map[imre->info.name] = imre->info.addr;
        imre->info.addr = getAddrForNetwork(fromNet^1);
    } else if ( req->dest != SimpleNetwork::INIT_BROADCAST_ADDR ) {
        /* TODO */
        dbg.fatal(CALL_INFO, 1, "I should't crash here.   This is a TODO\n");
    }
    req->vn = 0;
    return req;

}



SimpleNetwork::Request* MemNetBridge::translate(SimpleNetwork::Request *req, uint8_t fromNet)
{
    Net_t &inNet = networks[fromNet];
    Net_t &outNet = networks[fromNet^1];

    MemNIC::MemRtrEvent *mre = static_cast<MemNIC::MemRtrEvent*>(req->inspectPayload());

    SimpleNetwork::nid_t tgt;
    if ( mre->hasClientData() ) {
        tgt = getAddrFor(outNet, mre->event->getDst());
    } else {
        MemNIC::InitMemRtrEvent *imre = static_cast<MemNIC::InitMemRtrEvent*>(mre);
        imre->info.addr = getAddrForNetwork(fromNet^1);
        /* IMRE's don't have a specific destination - They are broadcast. */
        tgt = (outNet.imreMap[imre->info.name]++) % outNet.map.size();
    }

    req->src  = getAddrForNetwork(fromNet^1);
    req->dest = tgt;
    req->vn = 0;

    return req;
}

SimpleNetwork::nid_t MemNetBridge::getAddrFor(Net_t &net, const std::string &tgt)
{
    auto i = net.map.find(tgt);
    if ( i == net.map.end() ) {
        dbg.fatal(CALL_INFO, 1, "Unable to find mapping to %s\n", tgt.c_str());
    }
    return i->second;
}

