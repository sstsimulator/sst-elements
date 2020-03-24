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

#include "linkControl.h"

#include <sst/core/simulation.h>
#include <sst/core/sharedRegion.h>

#include "merlin.h"

namespace SST {
using namespace Interfaces;

namespace Merlin {

LinkControl::LinkControl(ComponentId_t cid, Params &params, int vns) :
    SST::Interfaces::SimpleNetwork(cid),
    rtr_link(NULL), output_timing(NULL),
    req_vns(vns), total_vns(0), checker_board_factor(1),
    vn_remap_out(nullptr), vn_remap_in(nullptr), id(-1),
    logical_nid(-1), nid_map_shm(nullptr), nid_map(nullptr),
    rr(0), input_buf(NULL), output_buf(NULL),
    rtr_credits(NULL), in_ret_credits(NULL),
    curr_out_vn(0), waiting(true), have_packets(false), start_block(0),
    idle_start(0),
    is_idle(true),
    receiveFunctor(NULL), sendFunctor(NULL),
    network_initialized(false),
    output(Simulation::getSimulation()->getSimulationOutput())
{
    // Get the checkerboard factor
    checker_board_factor = params.find<int>("checkerboard", 1);
    std::string checkerboard_alg = params.find<std::string>("checkerboard_alg","deterministic");
    if ( checkerboard_alg == "roundrobin" ) {
        cb_alg = ROUNDROBIN;
    }
    else if ( checkerboard_alg == "deterministic" ) {
        cb_alg = DETERMINISTIC;
    }
    else {
        merlin_abort.fatal(CALL_INFO,-1,"Unknown checkerboard_alg requested: %s\n",checkerboard_alg.c_str());
    }

    total_vns = vns * checker_board_factor;

    // Get the link bandwidth
    link_bw = params.find<UnitAlgebra>("link_bw");
    if ( !link_bw.hasUnits("B/s") && !link_bw.hasUnits("b/s") ) {
        merlin_abort.fatal(CALL_INFO,1,"Error: link_bw must be specified in either B/s or b/s (SI prefix also allowed)\n");
    }

    if ( link_bw.hasUnits("B/s") ) {
        link_bw *= UnitAlgebra("8b/B");
    }

    // Input and output buffers
    input_buf = new network_queue_t[req_vns];
    output_buf = new network_queue_t[total_vns];

    // Initialize credit arrays.  Credits are in flits, and we don't
    // yet know the flit size, so can't initialize in_ret_credits and
    // outbuf_credits yet.  Will initialize them after we get the
    // flit_size
    rtr_credits = new int[total_vns];
    in_ret_credits = new int[total_vns];
    outbuf_credits = new int[req_vns];

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

    // The output credits are set to zero and the other side of the
    // link will send the number of tokens.
    for ( int i = 0; i < total_vns; i++ ) rtr_credits[i] = 0;

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


    // See if there is a vn_map set
    std::vector<int> vn_map_vec;
    params.find_array<int>("vn_remap",vn_map_vec);
    if ( vn_map_vec.size() > 0 ) {
        if ( vn_map_vec.size() != total_vns ) {
            merlin_abort.fatal(CALL_INFO,1,"LinkControl: length of vn_map (%lu) must be equal to total number of VNs (%d)\n",vn_map_vec.size(),total_vns);
        }
        vn_remap_out = new int[total_vns];
        for ( int i = 0; i < total_vns; ++i ) {
            vn_remap_out[i] = vn_map_vec[i];
        }
    }
    
    
    // Register statistics
    packet_latency = registerStatistic<uint64_t>("packet_latency");
    send_bit_count = registerStatistic<uint64_t>("send_bit_count");
    output_port_stalls = registerStatistic<uint64_t>("output_port_stalls");
    idle_time = registerStatistic<uint64_t>("idle_time");
}

#ifndef SST_ENABLE_PREVIEW_BUILD
bool
LinkControl::initialize(const std::string& port_name, const UnitAlgebra& link_bw_in,
                        int vns, const UnitAlgebra& in_buf_size,
                        const UnitAlgebra& out_buf_size)
{    
    req_vns = vns;
    total_vns = vns * checker_board_factor;
    link_bw = link_bw_in;
    if ( link_bw.hasUnits("B/s") ) {
        link_bw *= UnitAlgebra("8b/B");
    }
    // Input and output buffers
    input_buf = new network_queue_t[req_vns];
    output_buf = new network_queue_t[total_vns];


    // For now, credit arrays need to be initalized in init after we
    // get the total number.  This makes it simplier to deal with VN
    // remapping.
    
    // Initialize credit arrays.  Credits are in flits, and we don't
    // yet know the flit size, so can't initialize in_ret_credits and
    // outbuf_credits yet.  Will initialize them after we get the
    // flit_size
    rtr_credits = new int[total_vns];
    in_ret_credits = new int[total_vns];
    outbuf_credits = new int[req_vns];

    inbuf_size = in_buf_size;
    if ( !inbuf_size.hasUnits("b") && !inbuf_size.hasUnits("B") ) {
        merlin_abort.fatal(CALL_INFO,-1,"in_buf_size must be specified in either "
                           "bits or bytes: %s\n",inbuf_size.toStringBestSI().c_str());
    }
    if ( inbuf_size.hasUnits("B") ) inbuf_size *= UnitAlgebra("8b/B");


    outbuf_size = out_buf_size;
    if ( !outbuf_size.hasUnits("b") && !outbuf_size.hasUnits("B") ) {
        merlin_abort.fatal(CALL_INFO,-1,"out_buf_size must be specified in either "
                           "bits or bytes: %s\n",outbuf_size.toStringBestSI().c_str());
    }
    if ( outbuf_size.hasUnits("B") ) outbuf_size *= UnitAlgebra("8b/B");
    
    // The output credits are set to zero and the other side of the
    // link will send the number of tokens.
    for ( int i = 0; i < total_vns; i++ ) rtr_credits[i] = 0;

    // Configure the links
    // For now give it a fake timebase.  Will give it the real timebase during init
    rtr_link = configureLink(port_name, std::string("1GHz"), new Event::Handler<LinkControl>(this,&LinkControl::handle_input));
    output_timing = configureSelfLink(port_name + "_output_timing", "1GHz",
            new Event::Handler<LinkControl>(this,&LinkControl::handle_output));

    // Register statistics
    packet_latency = registerStatistic<uint64_t>("packet_latency");
    send_bit_count = registerStatistic<uint64_t>("send_bit_count");
    output_port_stalls = registerStatistic<uint64_t>("output_port_stalls");
    idle_time = registerStatistic<uint64_t>("idle_time");
    
    return true;
}
#endif

LinkControl::~LinkControl()
{
    delete [] input_buf;
    delete [] output_buf;
    delete [] rtr_credits;
    delete [] in_ret_credits;
    delete [] outbuf_credits;
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
        ev->int_value = total_vns;
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

        // Need to initialize the credit arrays
        for ( int i = 0; i < total_vns; i++ ) {
            in_ret_credits[i] = (inbuf_size / flit_size_ua).getRoundedValue();
        }
        
        // Need to initialize the credit arrays
        for ( int i = 0; i < req_vns; i++ ) {
            outbuf_credits[i] = (outbuf_size / flit_size_ua).getRoundedValue();
        }
        
        
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

        // init_ev->print("LC: ",Simulation::getSimulation()->getSimulationOutput());
        delete ev;
        
        }
        break;
    case 2:
        {
        // Will receive information about total vns used and the
        // mapping of my VNs to all of them
        ev = rtr_link->recvInitData();
        init_ev = dynamic_cast<RtrInitEvent*>(ev);
        int actual_vns = init_ev->int_value;
        delete ev;

        bool vn_map_set = false;
        if ( vn_remap_out ) {
            vn_map_set = true;
        }
        else {
            vn_remap_out = new int[total_vns];
        }
        
        vn_remap_in = new int[actual_vns];
        for ( int i = 0; i < actual_vns; ++i ) {
            vn_remap_in[i] = -1;
        }

        // Will receive a message for each of my requested vns
        for ( int i = 0; i < total_vns; ++i ) {
            ev = rtr_link->recvInitData();
            init_ev = dynamic_cast<RtrInitEvent*>(ev);
            // If map was not set yet, get values from router
            if ( !vn_map_set ) {
                int vn = init_ev->int_value;
                vn_remap_out[i] = vn;
                if ( vn >= 0 ) vn_remap_in[vn] = i;
                delete ev;
            }
            // Otherwise, use the vn_remap_out to set vn_remap_in
            else {
                int vn = vn_remap_out[i];
                if ( vn >= 0 ) vn_remap_in[vn] = i;                
            }
        }

        

        // std::cout << "ID = " << id << "\n";
        // std::cout << "vn_remap_out:\n";
        // for ( int i = 0; i < total_vns; ++i ) {
        //     std::cout << "    " << i << " : " << vn_remap_out[i] << "\n";
        // }
        // std::cout << "vn_remap_in:\n";
        // for ( int i = 0; i < actual_vns; ++i ) {
        //     std::cout << "    " << i << " : " << vn_remap_in[i] << "\n";
        // }
        // std::cout << std::endl;
        network_initialized = true;

        // Need to send available credits to other side of link
        for ( int i = 0; i < total_vns; i++ ) {
            rtr_link->sendUntimedData(new credit_event(vn_remap_out[i],in_ret_credits[i]));
            in_ret_credits[i] = 0;
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
                if ( vn_remap_in[ce->vc] != -1 ) {  // Ignore credit events for VNs I don't have
                    rtr_credits[vn_remap_in[ce->vc]] += ce->credits;
                }
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
        while ( !input_buf[i].empty() ) {
            delete input_buf[i].front();
            input_buf[i].pop();
        }
        while ( !output_buf[i].empty() ) {
            delete output_buf[i].front();
            output_buf[i].pop();
        }
    }
}


// Returns true if there is space in the output buffer and false
// otherwise.
bool LinkControl::send(SimpleNetwork::Request* req, int vn) {
    if ( vn >= req_vns ) return false;
    req->vn = vn;
    if ( nid_map ) req->dest = nid_map[req->dest];
    RtrEvent* ev = new RtrEvent(req,id);
    int flits = (ev->request->size_in_bits + (flit_size - 1)) / flit_size;
    ev->setSizeInFlits(flits);

    if ( outbuf_credits[vn] < flits ) return false;

    outbuf_credits[vn] -= flits;
    // ev->request->vn = vn;

    // Determine which actual VN to put packet into.  This is based on
    // the checker_board_factor.
    int vn_offset;
    switch ( cb_alg ) {
    case DETERMINISTIC:
        // We will add src and dest and mod by checker_board_factor to
        // get the VN offset.
        vn_offset = (req->src + req->dest) % checker_board_factor;
        break;
    case ROUNDROBIN:
        vn_offset = rr % checker_board_factor;
        rr = (rr == (checker_board_factor)) ? 0 : rr + 1;
        break;
    default:
        // Should never happen, checked in constructor
        merlin_abort.fatal(CALL_INFO,-1,"Should never happen, checked in constructor\n");
        vn_offset = 0;
    }    
    ev->request->vn = vn * checker_board_factor + vn_offset;
    
    output_buf[ev->request->vn].push(ev);
    if ( waiting && !have_packets ) {
        output_timing->send(1,NULL);
        waiting = false;
    }

    ev->setInjectionTime(getCurrentSimTimeNano());

    if ( ev->getTraceType() != SimpleNetwork::Request::NONE ) {
        output.output("TRACE(%d): %" PRIu64 " ns: Send on LinkControl in NIC: %s\n",ev->getTraceID(),
                      getCurrentSimTimeNano(), getName().c_str());
    }
    return true;
}


// Returns true if there is space in the output buffer and false
// otherwise.
bool LinkControl::spaceToSend(int vn, int bits) {
    if ( (outbuf_credits[vn] * flit_size) < bits) return false;
    return true;
}


// Returns NULL if no event in input_buf[vn]. Otherwise, returns
// the next event.
SST::Interfaces::SimpleNetwork::Request* LinkControl::recv(int vn) {
    if ( input_buf[vn].size() == 0 ) return NULL;

    RtrEvent* event = input_buf[vn].front();
    input_buf[vn].pop();

    // Figure out how many credits to return
    int flits = event->getSizeInFlits();
    in_ret_credits[vn] += flits;

    // For now, we're just going to send the credits back to the
    // other side.  The required BW to do this will not be taken
    // into account.
    // rtr_link->send(1,new credit_event(event->request->vn,in_ret_credits[event->request->vn]));
    // in_ret_credits[event->request->vn] = 0;
    rtr_link->send(1,new credit_event(event->request->vn,in_ret_credits[vn]));
    in_ret_credits[vn] = 0;

    if ( event->getTraceType() != SimpleNetwork::Request::NONE ) {
        output.output("TRACE(%d): %" PRIu64 " ns: recv called on LinkControl in NIC: %s\n",event->getTraceID(),
                      getCurrentSimTimeNano(), getName().c_str());
    }

    SST::Interfaces::SimpleNetwork::Request* ret = event->request;
    ret->vn = ret->vn / checker_board_factor;
    if ( nid_map ) ret->dest = logical_nid;
    event->request = NULL;
    delete event;
    return ret;
}

void LinkControl::sendUntimedData(SST::Interfaces::SimpleNetwork::Request* req)
{
    if ( nid_map ) {
        req->dest = nid_map[req->dest];
    }
    rtr_link->sendUntimedData(new RtrEvent(req,id));
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
        rtr_credits[vn_remap_in[ce->vc]] += ce->credits;
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
        int actual_vn = vn_remap_in[event->request->vn] / checker_board_factor;

        input_buf[actual_vn].push(event);
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
        // stats.insertPacketLatency(lat);
        if ( receiveFunctor != NULL ) {
            bool keep = (*receiveFunctor)(actual_vn);
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
    for ( int i = curr_out_vn; i < total_vns; i++ ) {
        if ( output_buf[i].empty() ) continue;
        have_packets = true;
        send_event = output_buf[i].front();
        // Check to see if the needed VN has enough space
        if ( rtr_credits[i] < send_event->getSizeInFlits() ) continue;
        vn_to_send = i;
        output_buf[i].pop();
        found = true;
        break;
    }
    
    if (!found)  {
        for ( int i = 0; i < curr_out_vn; i++ ) {
            if ( output_buf[i].empty() ) continue;
            have_packets = true;
            send_event = output_buf[i].front();
            // Check to see if the needed VN has enough space
            if ( rtr_credits[i] < send_event->getSizeInFlits() ) continue;
            vn_to_send = i;
            output_buf[i].pop();
            found = true;
            break;
        }
    }
    // If we found an event to send, go ahead and send it
    if ( found ) {
        // Send the output to the network.
        // First set the virtual channel.
        int actual_vn = vn_remap_out[vn_to_send];
        send_event->request->vn = actual_vn;

        // Need to return credits to the output buffer
        int size = send_event->getSizeInFlits();
        // outbuf_credits[vn_to_send] += size;
        outbuf_credits[vn_to_send / checker_board_factor] += size;

        // Send an event to wake up again after this packet is sent.
        output_timing->send(size,NULL);
        
        curr_out_vn = vn_to_send + 1;
        if ( curr_out_vn == total_vns ) curr_out_vn = 0;

        // Add in inject time so we can track latencies
        send_event->setInjectionTime(getCurrentSimTimeNano());
        
        // Subtract credits
        rtr_credits[vn_to_send] -= size;

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
            bool keep = (*sendFunctor)(vn_to_send / checker_board_factor);
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


// void LinkControl::PacketStats::insertPacketLatency(SimTime_t lat)
// {
//     numPkts++;
//     if ( 1 == numPkts ) {
//         minLat = lat;
//         maxLat = lat;
//         m_n = m_old = lat;
//         s_old = 0.0;
//     } else {
//         minLat = std::min(minLat, lat);
//         maxLat = std::max(maxLat, lat);
//         m_n = m_old + (lat - m_old) / numPkts;
//         s_n = s_old + (lat - m_old) * (lat - m_n);

//         m_old = m_n;
//         s_old = s_n;
//     }
// }

} // namespace Merlin
} // namespace SST
