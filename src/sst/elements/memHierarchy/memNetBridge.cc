// Copyright 2009-2025 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2025, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// of the distribution for more information.
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

/* Debug macros */
#ifdef __SST_DEBUG_OUTPUT__ /* From sst-core, enable with --enable-debug */
#define is_debug_addr(addr) (DEBUG_ADDR.empty() || DEBUG_ADDR.find(addr) != DEBUG_ADDR.end())
#define is_debug_event(ev) (DEBUG_ADDR.empty() || ev->doDebug(DEBUG_ADDR))
#else
#define is_debug_addr(addr) false
#define is_debug_event(ev) false
#endif

MemNetBridge::MemNetBridge(SST::ComponentId_t id, SST::Params &params, Merlin::Bridge* bridge) :
    Bridge::Translator(id, params, bridge)
{
    /* Create debug output */
    dlevel = params.find<int>("debug_level", 0);
    int debugLoc = params.find<int>("debug", 0);
    dbg.init("", dlevel, 0, (Output::output_location_t)debugLoc);

    // Filter debug by address
    std::vector<uint64_t> addrArray;
    params.find_array<uint64_t>("debug_addr", addrArray);
    for (std::vector<uint64_t>::iterator it = addrArray.begin(); it != addrArray.end(); it++) {
        DEBUG_ADDR.insert(*it);
    }

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
        /* This is broadcast, intercept the name->address map and store for later translation */
        dbg.debug(_L10_, "%s received IMRE init message on interface %d: %s\n", getName().c_str(), fromNet, imre->toString().c_str());
        networks[fromNet].map[imre->info.name] = imre->info.addr;
        imre->info.addr = getAddrForNetwork(fromNet^1);
    } else if ( req->dest != SimpleNetwork::INIT_BROADCAST_ADDR ) {
        /* This is a point-to-point message, needs to be routed to the correct destination on the other network */
        MemNIC::MemRtrEvent* mre = dynamic_cast<MemNIC::MemRtrEvent*>(payload);
        if (mre) {
            dbg.debug(_L10_, "%s received init message on interface %d: %s\n", getName().c_str(), fromNet, mre->toString().c_str());
            Net_t &outNet = networks[fromNet^1];

            SimpleNetwork::nid_t tgt = getAddrFor(outNet, mre->inspectEvent()->getDst());
            req->src = getAddrForNetwork(fromNet^1);
            req->dest = tgt;
        } else {
            dbg.fatal(CALL_INFO, -1, "%s, Error: Bridge received an unexpected message type during init\n", getName().c_str());
        }
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
        tgt = getAddrFor(outNet, mre->inspectEvent()->getDst());
        dbg.debug(_L5_, "E: %-40" PRIu64 " %-21s Bridge:Tx     %d to %" PRI_NID ": (%s)\n",
                getCurrentSimCycle(), getName().c_str(), fromNet, tgt, mre->toString().c_str());
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

