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
#include "offeredload/offered_load.h"

#include <sst/core/params.h>
#include <sst/core/simulation.h>
#include <sst/core/timeLord.h>

using namespace SST::Merlin;
using namespace SST::Interfaces;

OfferedLoad::OfferedLoad(ComponentId_t cid, Params& params) :
    Component(cid),
    next_time(0),
    generation(0),
    id(-1)
{
    out.init(getName() + ": ", 0, 0, Output::STDOUT);

    try {
        params.find_array<double>("offered_load",offered_load);
    }
    catch ( std::invalid_argument e ) {
        bool found = false;
        double ol = params.find<double>("offered_load",found);
        if ( found ) {
            offered_load.push_back(ol);
        }
    }

    if ( offered_load.empty() ) {
        out.fatal(CALL_INFO, -1, "offered_load must be set!\n");
    }

    num_peers = params.find<int>("num_peers",-1);
    if ( num_peers == -1 ) {
        out.fatal(CALL_INFO, -1, "num_peers must be set!\n");
    }

    bool found = false;
    UnitAlgebra link_bw = params.find<UnitAlgebra>("link_bw",found);
    if ( !found ) {
        out.fatal(CALL_INFO, -1, "link_bw must be set!\n");
    }
    if ( link_bw.hasUnits("B/s") ) link_bw *= UnitAlgebra("8b/B");

    UnitAlgebra pkt_size = params.find<UnitAlgebra>("message_size","64b");
    if ( pkt_size.hasUnits("B") ) pkt_size  *= UnitAlgebra("8b/B");
    packet_size = pkt_size.getRoundedValue();

    // Compute the send interval.  We do this by computing time to
    // serialize one packet and dividing by the offered_load
    serialization_time = ((pkt_size / link_bw) / UnitAlgebra("1ps"));
    UnitAlgebra interval = serialization_time / offered_load[0];
    send_interval = interval.getRoundedValue();

    // Load the specified SimpleNetwork object

    // First see if it is defined in the python
    link_if = loadUserSubComponent<SST::Interfaces::SimpleNetwork>
        ("networkIF", ComponentInfo::SHARE_NONE, 1 /* vns */);

    if ( !link_if ) {
        // Not in python, just load the default
        Params if_params;

        if_params.insert("link_bw",params.find<std::string>("link_bw"));
        if_params.insert("input_buf_size",params.find<std::string>("buffer_size"));
        if_params.insert("output_buf_size",params.find<std::string>("buffer_size"));
        if_params.insert("port_name","rtr");

        link_if = loadAnonymousSubComponent<SST::Interfaces::SimpleNetwork>
            ("merlin.linkcontrol", "networkIF", 0,
             ComponentInfo::SHARE_PORTS | ComponentInfo::INSERT_STATS, if_params, 1 /* vns */);
    }


    // Register functors for the SimpleNetwork IF
    send_notify_functor = new SST::Interfaces::SimpleNetwork::Handler<OfferedLoad>(this, &OfferedLoad::send_notify);
    recv_notify_functor = new SST::Interfaces::SimpleNetwork::Handler<OfferedLoad>(this, &OfferedLoad::handle_receives);

    // link_if->setNotifyOnSend(send_notify_functor);
    link_if->setNotifyOnReceive(recv_notify_functor);


    // Set up the communication pattern generator
    std::string pattern = params.find<std::string>("pattern",found);
    if ( !found ) {
        out.fatal(CALL_INFO, -1, "pattern must be set!\n");
    }

    pattern_params = new Params();
    // packetDestGen = static_cast<TargetGenerator*>(loadSubComponent(pattern, this, params));
    pattern_params->insert(params.get_scoped_params("pattern"));
    pattern_params->insert("pattern_gen",pattern);

    UnitAlgebra warmup_time_ua = params.find<UnitAlgebra>("warmup_time","5us");
    if ( !warmup_time_ua.hasUnits("s") ) {
        out.fatal(CALL_INFO,-1,"warmup_time must specified in seconds");
    }
    warmup_time = (warmup_time_ua / UnitAlgebra("1ps")).getRoundedValue();
    start_time = warmup_time;


    UnitAlgebra collect_time_ua = params.find<UnitAlgebra>("collect_time","20us");
    if ( !collect_time_ua.hasUnits("s") ) {
        out.fatal(CALL_INFO,-1,"collect_time must specified in seconds");
    }
    collect_time = (collect_time_ua / UnitAlgebra("1ps")).getRoundedValue();
    end_time = start_time + collect_time;


    UnitAlgebra drain_time_ua = params.find<UnitAlgebra>("drain_time","50us");
    if ( !collect_time_ua.hasUnits("s") ) {
        out.fatal(CALL_INFO,-1,"drain_time must specified in seconds");
    }
    drain_time = (drain_time_ua / UnitAlgebra("1ps")).getRoundedValue();

    registerAsPrimaryComponent();
    primaryComponentDoNotEndSim();
    // clock_functor = new Clock::Handler<TrafficGen>(this,&TrafficGen::clock_handler);
    // clock_tc = registerClock( params.find<std::string>("message_rate", "1GHz"), clock_functor, false);

    base_tc = registerTimeBase("1ps",false);
    timing_link = configureSelfLink("timing_link", base_tc, new Event::Handler<OfferedLoad>(this, &OfferedLoad::output_timing));

    end_link = configureSelfLink("end_link", base_tc, new Event::Handler<OfferedLoad>(this, &OfferedLoad::end_handler));

    complete_event.push_back(new offered_load_complete_event(generation));

    // out.output("send_interval = %llu\n",send_interval);
    // out.output("start_time = %llu\n",start_time);
    // out.output("end_time = %llu\n",end_time);
}


OfferedLoad::~OfferedLoad()
{
    delete link_if;
}



void OfferedLoad::finish()
{
    link_if->finish();

    if ( id == 0 ) {
        // for ( auto ev : complete_event ) {

        //     out.output("Latency Statistics for offered load = %f\n",offered_load[ev->generation]);
        //     out.output("  sum = %llu\n",ev->sum);
        //     out.output("  sum_of_squares = %llu\n",ev->sum_of_squares);
        //     out.output("  min = %llu\n",ev->min);
        //     out.output("  max = %llu\n",ev->max);
        //     out.output("  count = %llu\n\n",ev->count);
        //     UnitAlgebra backup = UnitAlgebra("1ps") * (ev->backup / num_peers);
        //     out.output("  backup = %s\n",backup.toStringBestSI().c_str());
        //     UnitAlgebra average = UnitAlgebra("1ps") * ev->sum / ev->count;
        //     out.output("  average = %s\n\n",average.toStringBestSI().c_str());
        // }


        // Now, write out a summary table with just the latencies

        out.output("%9s %9s\n","Offered","Average");
        out.output("%9s %9s\n","Load ","Latency");
        for ( auto ev : complete_event ) {
            UnitAlgebra average = UnitAlgebra("1ps") * ev->sum / ev->count;
            out.output("%9.2f %15s",offered_load[ev->generation],average.toStringBestSI().c_str());
            if ( ev->backup > 0 ) out.output("*\n");
            else out.output("\n");
        }
        out.output("\n");

    }
}

void OfferedLoad::setup()
{
    link_if->setup();

    // Set up everyting to compute delays.  We'll convert bandwidth to
    // ps/bit so we can just multiply by bits sent to get number of ps
    // delay for packet.  b/s -> b in 1ps -> ps for one bit to
    // transfer.
    // link_bw = (link_bw * UnitAlgebra("1ps")).invert();

    // kick things off
    timing_link->send(0,NULL);
    end_link->send(end_time,NULL);
}

void
OfferedLoad::init(unsigned int phase) {
    link_if->init(phase);
    if ( id == -1 && link_if->isNetworkInitialized() ) {
        id = link_if->getEndpointID();
        std::string pattern = pattern_params->find<std::string>("pattern_gen");
        packetDestGen = loadAnonymousSubComponent<TargetGenerator>(pattern, "pattern_gen", 0, ComponentInfo::SHARE_NONE, *pattern_params, id, num_peers);
        delete pattern_params;
    }

}

void
OfferedLoad::complete(unsigned int phase) {
    link_if->complete(phase);

    if ( id == 0 ) {
        SimpleNetwork::Request* req = link_if->recvUntimedData();
        while ( req != NULL ) {
            offered_load_complete_event* ev = static_cast<offered_load_complete_event*>(req->takePayload());
            int generation = ev->generation;
            complete_event[generation]->sum += ev->sum;
            complete_event[generation]->sum_of_squares += ev->sum_of_squares;
            complete_event[generation]->min = ev->min < complete_event[generation]->min ? ev->min : complete_event[generation]->min;
            complete_event[generation]->max = ev->max > complete_event[generation]->max ? ev->max : complete_event[generation]->max;
            complete_event[generation]->count += ev->count;
            complete_event[generation]->backup += ev->backup;

            req = link_if->recvUntimedData();
        }
    }
    else {
        if ( phase == 0 ) {
            for ( auto ev : complete_event ) {
                link_if->sendUntimedData(new SimpleNetwork::Request(0,id,0,true,true,ev));
            }
        }
    }
}


bool
OfferedLoad::handle_receives(int vn)
{
    SimpleNetwork::Request* req = link_if->recv(vn);
    if ( req->dest != id ) {
        out.fatal(CALL_INFO,-1,"Endpoint %d received a packet intended for %lld\n",id,req->dest);
    }
    if ( req != NULL ) {
        SimTime_t current_time = getCurrentSimTime(base_tc);
        // Don't start counting until after warmup.  This is stored in
        // start_time.
        if ( start_time <= current_time ) {

            // Get the latency and add it to the complete_event)
            SimTime_t latency = current_time - ((offered_load_event*)req->inspectPayload())->start_time;

            complete_event[generation]->sum += latency;
            complete_event[generation]->sum_of_squares += (latency * latency);
            complete_event[generation]->min = latency < complete_event[generation]->min ? latency : complete_event[generation]->min;
            complete_event[generation]->max = latency > complete_event[generation]->max ? latency : complete_event[generation]->max;
            complete_event[generation]->count++;
        }
        delete req;
    }
    return true;
}


bool
OfferedLoad::send_notify(int vn)
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
OfferedLoad::output_timing(Event* ev)
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
OfferedLoad::progress_messages(SimTime_t current_time) {
    while ( (next_time <= current_time) && link_if->spaceToSend(0,packet_size) ) {
        offered_load_event* ev = new offered_load_event(next_time);
        SimpleNetwork::Request* req = new SimpleNetwork::Request(packetDestGen->getNextValue(), id, packet_size, true, true, ev);
        link_if->send(req,0);

        next_time += send_interval;
    }
}

void
OfferedLoad::end_handler(Event* ev) {

    // Compute backup metric and put it in event
    SimTime_t current_time = getCurrentSimTime(base_tc);

    if ( current_time <= next_time ) {
        complete_event[generation]->backup = 0;
    }
    else {
        complete_event[generation]->backup = current_time - next_time;
    }

    // See if we are done
    if ( complete_event.size() == offered_load.size() ) {
        primaryComponentOKToEndSim();
    }
    else {

        // Need to set things up for the next iteration
        SimTime_t current_time = getCurrentSimTime(base_tc);

        // Add a new complete_event entry and increment generation
        // count
        complete_event.push_back(new offered_load_complete_event(++generation));

        // Compute the new send interval based on the new
        // offered_load.  We do this by computing time to serialize
        // one packet and dividing by the offered_load
        UnitAlgebra interval = serialization_time / offered_load[generation];
        send_interval = interval.getRoundedValue();

        // Compute the next time to send a packet.  We'll wait for
        // the drain_time so the network is empty.
        next_time = current_time + drain_time;

        // Compute the new start_time for recording values (after the
        // warm up period)
        start_time = next_time + warmup_time;

        // Need to send the next event to end this round.  The total
        // time to the next ending is drain_time + warmup_time +
        // collect_time
        end_link->send(drain_time + warmup_time + collect_time, NULL);
    }

    // if ( id == 0 ) {
    //     out.output("END (%llu): start\n",current_time);
    //     out.output("  drain_time = %llu\n",drain_time);
    //     out.output("  next_time = %llu\n",next_time);
    //     out.output("  start_time = %llu\n",start_time);
    //     out.output("  send_interval = %llu\n",send_interval);
    //     out.output("  next end is %llu from now\n",drain_time+warmup_time+collect_time);
    // }



}
