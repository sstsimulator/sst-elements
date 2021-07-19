// Copyright 2009-2021 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2021, NTESS
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
#include "background_traffic/background_traffic.h"

#include <sst/core/params.h>
#include <sst/core/simulation.h>
#include <sst/core/timeLord.h>

using namespace SST::Merlin;
using namespace SST::Interfaces;

BackgroundTraffic::BackgroundTraffic(ComponentId_t cid, Params& params) :
    Component(cid),
    next_time(0),
    id(-1)
{
    bool found = false;
    offered_load = params.find<double>("offered_load",found);
    if ( !found ) {
        Simulation::getSimulationOutput().fatal(CALL_INFO, -1, "BackgroundTraffic: offered_load must be set!\n");
    }

    num_peers = params.find<int>("num_peers",-1);
    if ( num_peers == -1 ) {
        Simulation::getSimulationOutput().fatal(CALL_INFO, -1, "BackgroundTraffic: num_peers must be set!\n");
    }

    UnitAlgebra pkt_size = params.find<UnitAlgebra>("message_size","64b");
    if ( pkt_size.hasUnits("B") ) pkt_size  *= UnitAlgebra("8b/B");
    packet_size = pkt_size.getRoundedValue();


    // UnitAlgebra link_bw = params.find<UnitAlgebra>("link_bw",found);
    // if ( !found ) {
    //     out.fatal(CALL_INFO, -1, "link_bw must be set!\n");
    // }
    // if ( link_bw.hasUnits("B/s") ) link_bw *= UnitAlgebra("8b/B");

    // // Compute the send interval.  We do this by computing time to
    // // serialize one packet and dividing by the offered_load
    // serialization_time = ((pkt_size / link_bw) / UnitAlgebra("1ps"));
    // UnitAlgebra interval = serialization_time / offered_load[0];
    // send_interval = interval.getRoundedValue();


    // For now, stash the pkt_size in serialization_time
    serialization_time = pkt_size;

    // Load the specified SimpleNetwork object

    // First see if it is defined in the python
    link_if = loadUserSubComponent<SST::Interfaces::SimpleNetwork>
        ("networkIF", ComponentInfo::SHARE_NONE, 1 /* vns */);


    // Register functors for the SimpleNetwork IF
    send_notify_functor = new SST::Interfaces::SimpleNetwork::Handler<BackgroundTraffic>(this, &BackgroundTraffic::send_notify);
    recv_notify_functor = new SST::Interfaces::SimpleNetwork::Handler<BackgroundTraffic>(this, &BackgroundTraffic::handle_receives);

    // link_if->setNotifyOnSend(send_notify_functor);
    link_if->setNotifyOnReceive(recv_notify_functor);


    // Set up the communication pattern generator
    std::string pattern = params.find<std::string>("pattern",found);
    if ( !found ) {
        Simulation::getSimulationOutput().fatal(CALL_INFO, -1, "BackgroundTraffic: pattern must be set!\n");
    }

    pattern_params = new Params();
    // packetDestGen = static_cast<TargetGenerator*>(loadSubComponent(pattern, this, params));
    pattern_params->insert(params.get_scoped_params("pattern"));
    pattern_params->insert("pattern_gen",pattern);

    registerAsPrimaryComponent();
    primaryComponentOKToEndSim();

    base_tc = registerTimeBase("1ps",false);
    timing_link = configureSelfLink("timing_link", base_tc, new Event::Handler<BackgroundTraffic>(this, &BackgroundTraffic::output_timing));


}


BackgroundTraffic::~BackgroundTraffic()
{
    delete link_if;
}



void BackgroundTraffic::finish()
{
    link_if->finish();
}

void BackgroundTraffic::setup()
{
    link_if->setup();

    // Set up everyting to compute delays.  We'll convert bandwidth to
    // ps/bit so we can just multiply by bits sent to get number of ps
    // delay for packet.  b/s -> b in 1ps -> ps for one bit to
    // transfer.
    // link_bw = (link_bw * UnitAlgebra("1ps")).invert();

    // kick things off
    timing_link->send(0,NULL);
}

void
BackgroundTraffic::init(unsigned int phase) {
    link_if->init(phase);
    if ( id == -1 && link_if->isNetworkInitialized() ) {
        id = link_if->getEndpointID();
        std::string pattern = pattern_params->find<std::string>("pattern_gen");
        // packetDestGen = static_cast<TargetGenerator*>(loadSubComponent(pattern, this, *pattern_params));
        packetDestGen = loadAnonymousSubComponent<TargetGenerator>(pattern, "pattern_gen", 0, ComponentInfo::SHARE_NONE, *pattern_params, id, num_peers);
        delete pattern_params;

        // Set up send interval based on bandwidth
        UnitAlgebra link_bw = link_if->getLinkBW();

        // Compute the send interval.  We do this by computing time to
        // serialize one packet and dividing by the offered_load.
        // pkt_size was stashed in serialization_time in the
        // constructor, so serialization_time on the right hand side
        // is actually pkt_size
        serialization_time = ((serialization_time /*pkt_size*/ / link_bw) / UnitAlgebra("1ps"));
        UnitAlgebra interval = serialization_time / offered_load;
        send_interval = interval.getRoundedValue();
    }

}

void
BackgroundTraffic::complete(unsigned int phase) {
    link_if->complete(phase);
 }


bool
BackgroundTraffic::handle_receives(int vn)
{
    SimpleNetwork::Request* req = link_if->recv(vn);
    if ( req != NULL ) {
        delete req;
    }
    return true;
}


bool
BackgroundTraffic::send_notify(int vn)
{
    // LinkControl just sent something, get current time and see if we
    // can progress and messages
    SimTime_t current_time = getCurrentSimTime(base_tc);
    progress_messages(current_time);

    // Determine if we are waiting for room in the LinkControl or not.
    // We are waiting for room if next_time is earlier than
    // current_time
    if ( next_time <= current_time ) {
        // Need to wait for more data to be sent.  Keep LinkControl
        // handler installed
        return true;
    }
    else {
        // Need to wake up again at next time to send packet
        timing_link->send(next_time - current_time, NULL);
    }

    return false;
}

void
BackgroundTraffic::output_timing(Event* ev)
{
    // TraceFunction trace(CALL_INFO);
    // Time to send next message.  Get current time and see how many
    // we can progress
    SimTime_t current_time = getCurrentSimTime(base_tc);
    progress_messages(current_time);

    // Determine if we are waiting for room in the LinkControl or not.
    // We are waiting for room if next_time is earlier than
    // current_time
    if ( next_time <= current_time ) {
        // Need to wait for more data to be sent.  Install LinkControl
        // handler
        link_if->setNotifyOnSend(send_notify_functor);
    }
    else {
        // Need to wake up again at next time to send packet
        timing_link->send(next_time - current_time, NULL);
    }
}

void
BackgroundTraffic::progress_messages(SimTime_t current_time) {
    // TraceFunction trace(CALL_INFO);
    while ( (next_time <= current_time) && link_if->spaceToSend(0,packet_size) ) {
        // trace.getOutput().output("loop start: %p, %p\n",packetDestGen, link_if);
        background_traffic_event* ev = new background_traffic_event();
        // trace.getOutput().output("  loop middle 1\n");
        SimpleNetwork::Request* req = new SimpleNetwork::Request(packetDestGen->getNextValue(), id, packet_size, true, true, ev);
        // trace.getOutput().output("sending background traffic from %d\n",id);
        link_if->send(req,0);

        next_time += send_interval;
    }
}

