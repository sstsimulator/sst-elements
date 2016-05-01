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
#include "memNIC.h"

using namespace SST::MemHierarchy;


MemNetBridge::MemNetBridge(SST::ComponentId_t id, SST::Params &params) :
    SST::Component(id)
{
    int debugLevel = params.find<int>("debug_level", 0);
    dbg.init("@t:Bridge::@p():@l " + getName() + ": ",
            debugLevel, 0, (Output::output_location_t)params.find<int>("debug", 0));

    configureNIC(0, params);
    configureNIC(1, params);

    clockHandler = new Clock::Handler<MemNetBridge>(this, &MemNetBridge::clock);
    registerClock(params.find<std::string>("clock", "1GHz"), clockHandler);
    clockOn = true;
}


MemNetBridge::~MemNetBridge()
{
    for ( int i = 0 ; i < 2 ; i++ ) {
        //delete interfaces[i].stat_Send;
        //delete interfaces[i].stat_Recv;
        delete interfaces[i].nic;
    }
}


void MemNetBridge::configureNIC(uint8_t id, SST::Params &params)
{
    dbg.debug(CALL_INFO, 2, 0, "Initializing network interface %d\n", id);
    Nic_t &nic = interfaces[id];
    nic.nic = (SimpleNetwork*)loadSubComponent("merlin.linkcontrol", this, params);
    nic.nic->initialize( "network" + to_string(id),
            params.find<SST::UnitAlgebra>("network_bw", SST::UnitAlgebra("80GiB/s")),
            1, /* should be num VN */
            params.find<SST::UnitAlgebra>("network_input_buffer_size", SST::UnitAlgebra("1KiB")),
            params.find<SST::UnitAlgebra>("network_output_buffer_size", SST::UnitAlgebra("1KiB")));
    nic.nic->setNotifyOnReceive(new SimpleNetwork::Handler<MemNetBridge, uint8_t>(this, &MemNetBridge::handleIncoming, id));

    nic.stat_Recv = registerStatistic<uint64_t>("pkts_received_net" + to_string(id));
    nic.stat_Send = registerStatistic<uint64_t>("pkts_sent_net" + to_string(id));
}




void MemNetBridge::init(unsigned int phase)
{

    bool ready = true;
    for ( int i = 0 ; i < 2 ; i++ ) {
        Nic_t &nic = interfaces[i];
        nic.nic->init(phase);
        ready &= nic.nic->isNetworkInitialized();
    }

    dbg.debug(CALL_INFO, 10, 0, "Init Phase %u.  Network %sready\n", phase, ready ? "" : "NOT ");

    if ( ! ready ) return;

    for ( int i = 0 ; i < 2 ; i++ ) {
        Nic_t &nic = interfaces[i];
        Nic_t &otherNic = interfaces[i^1];


        while ( SimpleNetwork::Request *req = nic.nic->recvInitData() ) {
            dbg.debug(CALL_INFO, 2, 0, "Received init phase event on interface %d\n", i);
            Event *payload = req->inspectPayload();
            MemNIC::InitMemRtrEvent *imre = dynamic_cast<MemNIC::InitMemRtrEvent*>(payload);
            if ( imre ) {
                nic.map[imre->name] = imre->address;
                imre->address = otherNic.getAddr();
            } else if ( req->dest != SimpleNetwork::INIT_BROADCAST_ADDR ) {
                /* TODO */
                dbg.fatal(CALL_INFO, 1, "I should't crash here.   This is a TODO\n");
            }
            req->vn = 0;
            otherNic.nic->sendInitData(req);
        }
    }

}


void MemNetBridge::setup(void)
{
    interfaces[0].nic->setup();
    interfaces[1].nic->setup();

    for ( int i = 0 ; i < 2 ; i++ ) {
        Nic_t &nic = interfaces[i];
        dbg.debug(CALL_INFO, 2, 0, "Address Map on Interface %u:\n", i);
        for ( auto a : nic.map ) {
            dbg.debug(CALL_INFO, 2, 0, "\t%s -> %" PRIu64 "\n", a.first.c_str(), a.second);
        }
    }
}


void MemNetBridge::finish(void)
{
    interfaces[0].nic->finish();
    interfaces[1].nic->finish();
}




bool MemNetBridge::handleIncoming(int vn, uint8_t id)
{
    Nic_t &inNIC = interfaces[id];
    Nic_t &outNIC = interfaces[id^1];

    SimpleNetwork::Request* req = inNIC.nic->recv(vn);

    if ( NULL == req ) return false;
    inNIC.stat_Recv->addData(1);

    dbg.debug(CALL_INFO, 5, 0, "Received event on interface %u\n", id);

    MemNIC::MemRtrEvent *mre = static_cast<MemNIC::MemRtrEvent*>(req->inspectPayload());

    SimpleNetwork::nid_t tgt;
    if ( mre->hasClientData() ) {
        tgt = getAddrFor(outNIC, mre->event->getDst());
    } else {
        MemNIC::InitMemRtrEvent *imre = static_cast<MemNIC::InitMemRtrEvent*>(mre);
        imre->address = outNIC.getAddr();
        /* IMRE's don't have a specific destination - They are broadcast. */
        tgt = (outNIC.imreMap[imre->name]++) % outNIC.map.size();
    }

    req->src  = outNIC.getAddr();
    req->dest = tgt;
    req->vn = 0;

    /* TODO:  Delay? */

    /* Send req */
    if ( outNIC.nic->send(req, 0) ) {
        outNIC.stat_Send->addData(1);
    } else {
        /* We failed to send. */
        outNIC.sendQueue.push_back(req);
        /* Turn on clock */
        if ( !clockOn ) {
            clockOn = true;
            reregisterClock(defaultTimeBase, clockHandler);
        }
    }
    return true;
}

bool MemNetBridge::clock(SST::Cycle_t cycle)
{
    bool empty = interfaces[0].sendQueue.empty() && interfaces[1].sendQueue.empty();
    if ( !empty) {
        for ( int i = 0 ; i < 2 ; i++ ) {
            Nic_t &nic = interfaces[i];
            while ( ! nic.sendQueue.empty() ) {
                if ( nic.nic->send(nic.sendQueue.front(), 0) ) {
                    nic.sendQueue.pop_front();
                    nic.stat_Send->addData(1);
                } else {
                    break;
                }
            }
        }
    }

    if ( empty ) {
        clockOn = false;
        return true;
    }
    return false;
}


SimpleNetwork::nid_t MemNetBridge::getAddrFor(Nic_t &nic, const std::string &tgt)
{
    auto i = nic.map.find(tgt);
    if ( i == nic.map.end() ) {
        dbg.fatal(CALL_INFO, 1, "Unable to find mapping to %s\n", tgt.c_str());
    }
    return i->second;
}

