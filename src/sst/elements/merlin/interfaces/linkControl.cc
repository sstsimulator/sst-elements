// Copyright 2013-2020 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2013-2020, NTESS
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

#include "linkControl.h"

#include <sst/core/simulation.h>
#include <sst/core/sharedRegion.h>

#include "merlin.h"

namespace SST {
using namespace Interfaces;

namespace Merlin {

LinkControl::LinkControl(ComponentId_t cid, Params &params, int vns) :
    SST::Interfaces::SimpleNetwork(cid),
    rtr_link(nullptr), output_timing(nullptr),
    req_vns(vns), used_vns(0), total_vns(0), vn_out_map(nullptr),
    vn_remap_out(nullptr), output_queues(nullptr), router_credits(nullptr),
    router_return_credits(nullptr), input_queues(nullptr),
    id(-1), logical_nid(-1), nid_map_shm(nullptr), nid_map(nullptr),
    curr_out_vn(0), waiting(true), have_packets(false), start_block(0),
    idle_start(0),
    is_idle(true),
    receiveFunctor(nullptr), sendFunctor(nullptr),
    network_initialized(false),
    output(Simulation::getSimulation()->getSimulationOutput())
{
    // Get the link bandwidth
    link_bw = params.find<UnitAlgebra>("link_bw");
    if ( !link_bw.hasUnits("B/s") && !link_bw.hasUnits("b/s") ) {
        merlin_abort.fatal(CALL_INFO,1,"Error: link_bw must be specified in either B/s or b/s (SI prefix also allowed)\n");
    }

    if ( link_bw.hasUnits("B/s") ) {
        link_bw *= UnitAlgebra("8b/B");
    }

    // Get the buffer sizes
    inbuf_size = params.find<UnitAlgebra>("input_buf_size","1kB");
    if ( !inbuf_size.hasUnits("b") && !inbuf_size.hasUnits("B") ) {
        merlin_abort.fatal(CALL_INFO,-1,"in_buf_size must be specified in either "
                           "bits or bytes: %s\n",inbuf_size.toStringBestSI().c_str());
    }
    if ( inbuf_size.hasUnits("B") ) inbuf_size *= UnitAlgebra("8b/B");

    outbuf_size = params.find<UnitAlgebra>("output_buf_size","1kB");
    if ( !outbuf_size.hasUnits("b") && !outbuf_size.hasUnits("B") ) {
        merlin_abort.fatal(CALL_INFO,-1,"out_buf_size must be specified in either "
                           "bits or bytes: %s\n",outbuf_size.toStringBestSI().c_str());
    }
    if ( outbuf_size.hasUnits("B") ) outbuf_size *= UnitAlgebra("8b/B");


    // Configure the links
    // For now give it a fake timebase.  Will give it the real timebase during init

    // Need to get the right port_name
    std::string port_name("rtr_port");
    if ( isAnonymous() ) {
        port_name = params.find<std::string>("port_name");
    }

    rtr_link = configureLink(port_name, std::string("1GHz"), new Event::Handler<LinkControl>(this,&LinkControl::handle_input));

    output_timing = configureSelfLink(port_name + "_output_timing", "1GHz",
            new Event::Handler<LinkControl>(this,&LinkControl::handle_output));


    // Input and output buffers.  Not all of them can be set up now.
    // Only those that are sized based on req_vns can be intialized
    // now.  Others will wait until init when we find out the rest of
    // the VN usage.
    input_queues = new network_queue_t[req_vns];

    // Need to wait to do output_queues, router_credits

    // See if there is a vn_map set
    std::vector<int> vn_map_vec;
    params.find_array<int>("vn_remap",vn_map_vec);
    if ( vn_map_vec.size() > 0 ) {
        if ( vn_map_vec.size() != req_vns ) {
            merlin_abort.fatal(CALL_INFO,1,"LinkControl: length of vn_map (%lu) must be equal to total number of VNs (%d)\n",vn_map_vec.size(),req_vns);
        }
        vn_out_map = new int[req_vns];
        for ( int i = 0; i < req_vns; ++i ) {
            vn_out_map[i] = vn_map_vec[i];
        }
    }


    // See if we need to set up a nid map
    bool found = false;
    int job_id = params.find<int>("job_id",-1,found);
    if ( found ) {
        if ( params.find<bool>("use_nid_remap",false) ) {
            std::string nid_map_name = std::string("job_") + std::to_string(job_id) + "_nid_map";

            int job_size = params.find<int>("job_size",-1);
            if ( job_size == -1 ) {
                merlin_abort.fatal(CALL_INFO,1,"LinkControl: job_size must be set\n");
            }
            logical_nid = params.find<nid_t>("logical_nid",-1);
            if ( job_size == -1 ) {
                merlin_abort.fatal(CALL_INFO,1,"LinkControl: logical_nid must be set\n");
            }
            nid_map_shm = Simulation::getSharedRegionManager()->
                getGlobalSharedRegion(nid_map_name, job_size * sizeof(nid_t), new SharedRegionMerger());
        }
    }
    else {
        std::string nid_map_name = params.find<std::string>("nid_map_name",std::string());
        if ( !nid_map_name.empty() ) {
            int job_size = params.find<int>("job_size",-1);
            if ( job_size == -1 ) {
                merlin_abort.fatal(CALL_INFO,1,"LinkControl: job_size must be set if nid_map_name is set\n");
            }
            logical_nid = params.find<nid_t>("logical_nid",-1);
            if ( job_size == -1 ) {
                merlin_abort.fatal(CALL_INFO,1,"LinkControl: logical_nid must be set if nid_map_name is set\n");
            }
            nid_map_shm = Simulation::getSharedRegionManager()->
                getGlobalSharedRegion(nid_map_name, job_size * sizeof(nid_t), new SharedRegionMerger());
        }
    }



    // Register statistics
    packet_latency = registerStatistic<uint64_t>("packet_latency");
    send_bit_count = registerStatistic<uint64_t>("send_bit_count");
    output_port_stalls = registerStatistic<uint64_t>("output_port_stalls");
    idle_time = registerStatistic<uint64_t>("idle_time");
}

LinkControl::~LinkControl()
{
    delete [] vn_remap_out;
    delete [] output_queues;
    delete [] router_credits;
    delete [] router_return_credits;
    delete [] input_queues;

    // Delete shared region manager for nid map if we're using one
    if ( nid_map_shm ) delete nid_map_shm;

}

void LinkControl::setup()
{
    while ( init_events.size() ) {
        delete init_events.front();
        init_events.pop_front();
    }
}

void LinkControl::init(unsigned int phase)
{
    Event* ev;
    RtrInitEvent* init_ev;
    switch ( phase ) {
    case 0:
    {
        // Negotiate link speed.  We will take the min of the two link speeds
        init_ev = new RtrInitEvent();
        init_ev->command = RtrInitEvent::REPORT_BW;
        init_ev->ua_value = link_bw;
        rtr_link->sendUntimedData(init_ev);

        // In phase zero, send the number of VNs
        RtrInitEvent* ev = new RtrInitEvent();
        ev->command = RtrInitEvent::REQUEST_VNS;
        ev->int_value = req_vns;
        rtr_link->sendUntimedData(ev);
    }
        break;
    case 1:
    {
        // Get the link speed from the other side.  Actual link speed
        // will be the minumum the two sides
        ev = rtr_link->recvInitData();
        init_ev = dynamic_cast<RtrInitEvent*>(ev);
        if ( link_bw > init_ev->ua_value ) link_bw = init_ev->ua_value;
        delete ev;

        // Get the flit size from the router
        ev = rtr_link->recvInitData();
        init_ev = dynamic_cast<RtrInitEvent*>(ev);
        flit_size_ua = init_ev->ua_value;
        flit_size = flit_size_ua.getRoundedValue();
        delete ev;

        // Need to reset the time base of the output link
        UnitAlgebra link_clock = link_bw / flit_size_ua;
        TimeConverter* tc = getTimeConverter(link_clock);
        output_timing->setDefaultTimeBase(tc);

        // Initialize links
        // Receive the endpoint ID from PortControl
        ev = rtr_link->recvInitData();
        if ( ev == NULL ) {
            // fail
        }
        if ( static_cast<BaseRtrEvent*>(ev)->getType() != BaseRtrEvent::INITIALIZATION ) {
            // fail
        }
        init_ev = static_cast<RtrInitEvent*>(ev);

        if ( init_ev->command != RtrInitEvent::REPORT_ID ) {
            // fail
        }

        id = init_ev->int_value;
        if ( logical_nid == -1 ) logical_nid = id;
        // If we have a nid_map, fill in my mapping
        if ( nid_map_shm ) {
            nid_map_shm->modifyArray(logical_nid,id);
            nid_map_shm->publish();
            nid_map = nid_map_shm->getPtr<const nid_t*>();
        }

        delete ev;

        }
        break;
    case 2:
        {
        // Will receive information about total vns used and the
        // mapping of my VNs to all of them
        ev = rtr_link->recvInitData();
        init_ev = dynamic_cast<RtrInitEvent*>(ev);
        total_vns = init_ev->int_value;
        delete ev;

        bool vn_map_set = false;
        if ( vn_out_map ) {
            vn_map_set = true;
        }
        else {
            vn_out_map = new int[req_vns];
        }

        // Will receive a message for each of my requested vns
        for ( int i = 0; i < req_vns; ++i ) {
            ev = rtr_link->recvInitData();
            init_ev = dynamic_cast<RtrInitEvent*>(ev);
            // If map was not set yet, get values from router
            if ( !vn_map_set ) {
                int vn = init_ev->int_value;
                vn_out_map[i] = vn;
                delete ev;
            }
        }

        // Initialize the output tracking arrays that are size
        // total_vns
        router_return_credits = new int[total_vns];
        router_credits = new int[total_vns];
        for ( int i = 0; i < total_vns; ++i ) {
            router_return_credits[i] = 0;
            router_credits[i] = 0;
        }


        int* vn_count = new int[total_vns];
        for ( int i = 0; i < total_vns; ++i ) vn_count[i] = 0;

        used_vns = 0;
        for ( int i = 0; i < req_vns; ++i ) {
            int vn = vn_out_map[i];
            if ( vn != -1 ) {
                // If this is the first time this VN was specified add
                // one to used_vns
                if ( vn_count[vn] == 0 ) {
                    used_vns++;
                    // Setup router credtis for used VNs
                    router_return_credits[vn] = (inbuf_size / flit_size_ua).getRoundedValue();
                }
                vn_count[vn]++;
            }
        }

        // Instance the output queues
        int count = 0;
        vn_remap_out = new output_queue_bundle_t*[req_vns];
        output_queues = new output_queue_bundle_t[used_vns];
        for ( int i = 0; i < total_vns; ++i ) {
            if ( vn_count[i] > 0 ) {
                // Set up the credits
                output_queues[count].credits = (outbuf_size / flit_size_ua).getRoundedValue();
                output_queues[count].vn = i;
                // Find which outputs map to this vn
                for ( int j = 0; j < req_vns; ++j ) {
                    if ( vn_out_map[j] == i ) {
                        vn_remap_out[j] = &(output_queues[count]);
                    }
                }
                count++;
            }
        }

        // Don't need this map anymore
        delete[] vn_out_map;


        network_initialized = true;

        // Need to send available credits to other side of link
        for ( int i = 0; i < total_vns; i++ ) {
            int credits = router_return_credits[i];
            if ( credits > 0 ) {
                rtr_link->sendUntimedData(new credit_event(i,router_return_credits[i]));
                router_return_credits[i] = 0;
            }
        }
        }
        break;
    default:
        // For all other phases, look for credit events, any other
        // events get passed up to containing component by adding them
        // to init_events queue
        while ( ( ev = rtr_link->recvInitData() ) != NULL ) {
            BaseRtrEvent* bev = static_cast<BaseRtrEvent*>(ev);
            switch (bev->getType()) {
            case BaseRtrEvent::CREDIT:
            {
                credit_event* ce = static_cast<credit_event*>(bev);
                router_credits[ce->vc] += ce->credits;
                delete ev;
            }
            break;
            case BaseRtrEvent::PACKET:
                init_events.push_back(static_cast<RtrEvent*>(ev));
                break;
            default:
                // This shouldn't happen.  Only RtrEvents (PACKET
                // types) should not be handled in the LinkControl
                // object.
                merlin_abort_full.fatal(CALL_INFO, 1, "Reached state where a non-RtrEvent was not handled.");
                break;
            }
        }
        break;
    }
    // Need to start the timer for links that never send data
    idle_start = Simulation::getSimulation()->getCurrentSimCycle();
    is_idle = true;
}

void LinkControl::complete(unsigned int phase)
{
    // For all other phases, look for credit events, any other
    // events get passed up to containing component by adding them
    // to init_events queue
    Event* ev;
    RtrInitEvent* init_ev;
    while ( ( ev = rtr_link->recvInitData() ) != NULL ) {
        BaseRtrEvent* bev = static_cast<BaseRtrEvent*>(ev);
        switch (bev->getType()) {
        case BaseRtrEvent::PACKET:
            init_events.push_back(static_cast<RtrEvent*>(ev));
            break;
        default:
            // This shouldn't happen.  Only RtrEvents (PACKET
            // types) should not be handled in the LinkControl
            // object.
            merlin_abort_full.fatal(CALL_INFO, 1, "Reached state where a non-RtrEvent was not handled.");
            break;
        }
    }
}


void LinkControl::finish(void)
{
    if (is_idle) {
        idle_time->addData(Simulation::getSimulation()->getCurrentSimCycle() - idle_start);
        is_idle = false;
    }
    // Clean up all the events left in the queues.  This will help
    // track down real memory leaks as all this events won't be in the
    // way.
    for ( int i = 0; i < req_vns; i++ ) {
        while ( !input_queues[i].empty() ) {
            delete input_queues[i].front();
            input_queues[i].pop();
        }
    }
    for ( int i = 0; i < used_vns; i++ ) {
        while ( !output_queues[i].queue.empty() ) {
            delete output_queues[i].queue.front();
            output_queues[i].queue.pop();
        }
    }
}


// Returns true if there is space in the output buffer and false
// otherwise.
bool LinkControl::send(SimpleNetwork::Request* req, int vn) {
    // Check to see if the VN is in range
    if ( vn >= req_vns ) return false;

    // Check to see if we need to do a nid translation
    if ( nid_map ) req->dest = nid_map[req->dest];

    // Get the output queue information for that vn
    output_queue_bundle_t& out_handle = *(vn_remap_out[vn]);

    // Do the VN remapping
    int real_vn = out_handle.vn;
    req->vn = real_vn;

    // Create a router event using id and original vn
    RtrEvent* ev = new RtrEvent(req,id,vn);
    // Fill in the number of flits
    int flits = (ev->request->size_in_bits + (flit_size - 1)) / flit_size;
    ev->setSizeInFlits(flits);

    // Check to see if there are enough credits to send
    if ( out_handle.credits < flits ) return false;

    // Update the credits
    out_handle.credits -= flits;
    // ev->request->vn = vn;

    ev->setInjectionTime(getCurrentSimTimeNano());
    out_handle.queue.push(ev);
    if ( waiting && !have_packets ) {
        output_timing->send(1,NULL);
        waiting = false;
    }

    if ( ev->getTraceType() != SimpleNetwork::Request::NONE ) {
        output.output("TRACE(%d): %" PRIu64 " ns: Send on LinkControl in NIC: %s\n",ev->getTraceID(),
                      getCurrentSimTimeNano(), getName().c_str());
    }
    return true;
}


// Returns true if there is space in the output buffer and false
// otherwise.
bool LinkControl::spaceToSend(int vn, int bits) {
    if ( vn_remap_out[vn]->credits * flit_size < bits) return false;
    return true;
}


// Returns NULL if no event in input_buf[vn]. Otherwise, returns
// the next event.
SST::Interfaces::SimpleNetwork::Request* LinkControl::recv(int vn) {
    if ( input_queues[vn].size() == 0 ) return NULL;

    RtrEvent* event = input_queues[vn].front();
    input_queues[vn].pop();

    // Figure out how many credits to return
    int flits = event->getSizeInFlits();
    router_return_credits[vn] += flits;

    // For now, we're just going to send the credits back to the
    // other side.  The required BW to do this will not be taken
    // into account.
    // rtr_link->send(1,new credit_event(event->request->vn,in_ret_credits[event->request->vn]));
    // in_ret_credits[event->request->vn] = 0;
    rtr_link->send(1,new credit_event(event->request->vn,router_return_credits[vn]));
    router_return_credits[vn] = 0;

    if ( event->getTraceType() != SimpleNetwork::Request::NONE ) {
        output.output("TRACE(%d): %" PRIu64 " ns: recv called on LinkControl in NIC: %s\n",event->getTraceID(),
                      getCurrentSimTimeNano(), getName().c_str());
    }

    SST::Interfaces::SimpleNetwork::Request* ret = event->request;
    if ( nid_map ) ret->dest = logical_nid;
    event->request = NULL;
    delete event;
;
    return ret;
}

void LinkControl::sendUntimedData(SST::Interfaces::SimpleNetwork::Request* req)
{
    if ( nid_map ) {
        req->dest = nid_map[req->dest];
    }
    rtr_link->sendUntimedData(new RtrEvent(req,id,0));
}

SST::Interfaces::SimpleNetwork::Request* LinkControl::recvUntimedData()
{
    if ( init_events.size() ) {
        RtrEvent *ev = init_events.front();
        init_events.pop_front();
        SST::Interfaces::SimpleNetwork::Request* ret = ev->request;
        ev->request = NULL;
        delete ev;
        return ret;
    } else {
        return NULL;
    }
}

void LinkControl::sendInitData(SST::Interfaces::SimpleNetwork::Request* req) {
    sendUntimedData(req);
}

SST::Interfaces::SimpleNetwork::Request* LinkControl::recvInitData() {
    return recvUntimedData();
}


void LinkControl::handle_input(Event* ev)
{
    // Check to see if this is a credit or data packet
    // credit_event* ce = dynamic_cast<credit_event*>(ev);
    // if ( ce != NULL ) {
    BaseRtrEvent* base_event = static_cast<BaseRtrEvent*>(ev);
    if ( base_event->getType() == BaseRtrEvent::CREDIT ) {
    	credit_event* ce = static_cast<credit_event*>(ev);
        router_credits[ce->vc] += ce->credits;
        delete ev;

        // If we're waiting, we need to send a wakeup event to the
        // output queues
        if ( waiting ) {
            output_timing->send(1,NULL);
            waiting = false;
            // If we were stalled waiting for credits and we had
            // packets, we need to add stall time
            if ( have_packets) {
                output_port_stalls->addData(Simulation::getSimulation()->getCurrentSimCycle() - start_block);
            }
        }
    }
    else {
        RtrEvent* event = static_cast<RtrEvent*>(ev);
        // Simply put the event into the right virtual network queue
        // int actual_vn = event->request->vn / checker_board_factor;
        // int actual_vn = vn_remap_in[event->request->vn] / checker_board_factor;
        int orig_vn = event->getOriginalVN();
        event->request->vn = orig_vn;

        input_queues[orig_vn].push(event);
        if (is_idle) {
            idle_time->addData(Simulation::getSimulation()->getCurrentSimCycle() - idle_start);
            is_idle = false;
        }
        if ( event->request->getTraceType() == SimpleNetwork::Request::FULL ) {
            output.output("TRACE(%d): %" PRIu64 " ns: Received and event on LinkControl in NIC: %s"
                          " on VN %d from src %" PRIu64 "\n",
                          event->request->getTraceID(),
                          getCurrentSimTimeNano(),
                          getName().c_str(),
                          event->request->vn,
                          event->request->src);
        }
        SimTime_t lat = getCurrentSimTimeNano() - event->getInjectionTime();
        packet_latency->addData(lat);
        if ( receiveFunctor != NULL ) {
            bool keep = (*receiveFunctor)(orig_vn);
            if ( !keep) receiveFunctor = NULL;
        }
    }
}


void LinkControl::handle_output(Event* ev)
{
    // The event is an empty event used just for timing.

    // ***** Need to add in logic for when to return credits *****
    // For now just done automatically when events are pulled out
    // of the block

    // We do a round robin scheduling.  If the current vn has no
    // data, find one that does.
    int vn_to_send = -1;
    bool found = false;
    RtrEvent* send_event = NULL;
    have_packets = false;
    for ( int i = curr_out_vn; i < used_vns; i++ ) {
        if ( output_queues[i].queue.empty() ) continue;
        have_packets = true;
        send_event = output_queues[i].queue.front();
        // Check to see if the needed VN has enough space
        if ( router_credits[output_queues[i].vn] < send_event->getSizeInFlits() ) continue;
        vn_to_send = i;
        output_queues[i].queue.pop();
        found = true;
        break;
    }

    if (!found)  {
        for ( int i = 0; i < curr_out_vn; i++ ) {
            if ( output_queues[i].queue.empty() ) continue;
            have_packets = true;
            send_event = output_queues[i].queue.front();
            // Check to see if the needed VN has enough space
            if ( router_credits[output_queues[i].vn] < send_event->getSizeInFlits() ) continue;
            vn_to_send = i;
            output_queues[i].queue.pop();
            found = true;
            break;
        }
    }
    // If we found an event to send, go ahead and send it
    if ( found ) {
        // Need to return credits to the output buffer
        int size = send_event->getSizeInFlits();
        // outbuf_credits[vn_to_send] += size;
        output_queues[vn_to_send].credits += size;

        // Send an event to wake up again after this packet is sent.
        output_timing->send(size,NULL);

        curr_out_vn = vn_to_send + 1;
        if ( curr_out_vn == used_vns ) curr_out_vn = 0;

        // Add in inject time so we can track latencies
        send_event->setInjectionTime(getCurrentSimTimeNano());

        // Subtract credits
        // rtr_credits[vn_to_send] -= size;
        router_credits[output_queues[vn_to_send].vn] -= size;

		if (is_idle){
            idle_time->addData(Simulation::getSimulation()->getCurrentSimCycle() - idle_start);
            is_idle = false;
        }

        rtr_link->send(send_event);

        if ( send_event->getTraceType() == SimpleNetwork::Request::FULL ) {
            output.output("TRACE(%d): %" PRIu64 " ns: Sent an event to router from LinkControl"
                          " in NIC: %s on VN %d to dest %" PRIu64 ".\n",
                          send_event->getTraceID(),
                          getCurrentSimTimeNano(),
                          getName().c_str(),
                          send_event->request->vn,
                          send_event->request->dest);
        }
        send_bit_count->addData(send_event->request->size_in_bits);
        if (sendFunctor != NULL ) {
            bool keep = (*sendFunctor)(send_event->getOriginalVN());
            if ( !keep ) sendFunctor = NULL;
        }
    }
    else {
        // What do we do if there's nothing to send??  It could be
        // because everything is empty or because there's not
        // enough room in the router buffers.  Either way, we
        // don't send a wakeup event.  We will send a wake up when
        // we either get something new in the output buffers or
        // receive credits back from the router.  However, we need
        // to know that we got to this state.
        start_block = Simulation::getSimulation()->getCurrentSimCycle();
        waiting = true;
        // Begin counting the amount of time this port was idle
        if (!have_packets && !is_idle) {
            idle_start = Simulation::getSimulation()->getCurrentSimCycle();
            is_idle = true;
        }
		// Should be in a stalled state rather than idle
		if (have_packets && is_idle){
            idle_time->addData(Simulation::getSimulation()->getCurrentSimCycle() - idle_start);
            is_idle = false;
        }
    }
}


} // namespace Merlin
} // namespace SST
