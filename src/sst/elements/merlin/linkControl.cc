// Copyright 2013-2016 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2013-2016, Sandia Corporation
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

#include "merlin.h"

namespace SST {
using namespace Interfaces;

namespace Merlin {

LinkControl::LinkControl(Component* parent, Params &params) :
    SST::Interfaces::SimpleNetwork(parent),
    rtr_link(NULL), output_timing(NULL),
    req_vns(0), total_vns(0), checker_board_factor(1), id(-1),
    rr(0), input_buf(NULL), output_buf(NULL),
    rtr_credits(NULL), in_ret_credits(NULL),
    curr_out_vn(0), waiting(true), have_packets(false), start_block(0),
    idle_start(0),
    is_idle(true),
	dynlink_start(0),
	active_past(0), idle_past(0), stalled_past(0),
	active_cur(0), idle_cur(0), stalled_cur(0),
	active_wma(0), idle_wma(0), stalled_wma(0),
	dl_bits_sent(0),
    receiveFunctor(NULL), sendFunctor(NULL),
    network_initialized(false),
    output(Simulation::getSimulation()->getSimulationOutput())
{
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
}
    
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
	input_buf_flit_count = 0;
	output_buf_flit_count = 0;

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
    // rtr_link = rif->configureLink(port_name, time_base, new Event::Handler<LinkControl>(this,&LinkControl::handle_input));
    rtr_link = configureLink(port_name, std::string("1GHz"), new Event::Handler<LinkControl>(this,&LinkControl::handle_input));
    // output_timing = rif->configureSelfLink(port_name + "_output_timing", time_base,
    //         new Event::Handler<LinkControl>(this,&LinkControl::handle_output));
    output_timing = configureSelfLink(port_name + "_output_timing", "1GHz",
            new Event::Handler<LinkControl>(this,&LinkControl::handle_output));
	
	dynlink_timing = configureSelfLink(port_name + "_dynlink_timing", "1MHz", 
			new Event::Handler<LinkControl>(this,&LinkControl::summarize_link_state));

    // Register statistics
    packet_latency = registerStatistic<uint64_t>("packet_latency");
    send_bit_count = registerStatistic<uint64_t>("send_bit_count");
    output_port_stalls = registerStatistic<uint64_t>("output_port_stalls");
    idle_time = registerStatistic<uint64_t>("idle_time");
    
    return true;
}


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

// void LinkControl::init(unsigned int phase)
// {
//     if ( phase == 0 ) {
//         // Need to send the available credits to the other side
//         for ( int i = 0; i < num_vns; i++ ) {
//             rtr_link->sendInitData(new credit_event(i,in_ret_credits[i]));
//             in_ret_credits[i] = 0;
//         }
//     }
//     // Need to recv the credits send from the other side
//     Event* ev;
//     while ( ( ev = rtr_link->recvInitData() ) != NULL ) {
//         credit_event* ce = dynamic_cast<credit_event*>(ev);
//         if ( ce != NULL ) {
//             rtr_credits[ce->vc] += ce->credits;
//             delete ev;
//         } else {
//             init_events.push_back(ev);
//         }
//     }
// }

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
            rtr_link->sendInitData(init_ev);

            // In phase zero, send the number of VNs
            RtrInitEvent* ev = new RtrInitEvent();
            ev->command = RtrInitEvent::REQUEST_VNS;
            ev->int_value = total_vns;
            rtr_link->sendInitData(ev);
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
        UnitAlgebra flit_size_ua = init_ev->ua_value;
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
        
        // std::cout << link_clock.toStringBestSI() << std::endl;
        
        TimeConverter* tc = parent->getTimeConverter(link_clock);
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
        // init_ev->print("LC: ",Simulation::getSimulation()->getSimulationOutput());
        delete ev;
        
        // Need to send available credits to other side of link
        for ( int i = 0; i < total_vns; i++ ) {
            rtr_link->sendInitData(new credit_event(i,in_ret_credits[i]));
            in_ret_credits[i] = 0;
        }
        network_initialized = true;
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
                if ( ce->vc < total_vns ) {  // Ignore credit events for VNs I don't have
                    rtr_credits[ce->vc] += ce->credits;
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


void LinkControl::finish(void)
{
    if (is_idle) {
        idle_time->addData(Simulation::getSimulation()->getCurrentSimCycle() - idle_start);
        is_idle = false;
		// Extra code for dynamic link additions
		// makes sure we don't count idle time outside of the window
		if (idle_start < dynlink_start){
			idle_cur += parent->getCurrentSimTimeNano() - dynlink_start;
		}
		else {
			idle_cur += parent->getCurrentSimTimeNano() - idle_start;
		}
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
    RtrEvent* ev = new RtrEvent(req);
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
    
    
    // printf("%d: Send message to %llu on VN: %d, which is actually VN:%d --> %llu",id,req->dest,vn,req->vn,req->dest+req->src);
    // std::cout << std::endl;
    
    output_buf[ev->request->vn].push(ev);
	output_buf_flit_count += ev->getSizeInFlits();
    if ( waiting && !have_packets ) {
        output_timing->send(1,NULL);
        waiting = false;
    }

    ev->setInjectionTime(parent->getCurrentSimTimeNano());

    if ( ev->getTraceType() != SimpleNetwork::Request::NONE ) {
        output.output("TRACE(%d): %" PRIu64 " ns: Send on LinkControl in NIC: %s\n",ev->getTraceID(),
                      parent->getCurrentSimTimeNano(), parent->getName().c_str());
        // std::cout << "TRACE(" << ev->getTraceID() << "): " << parent->getCurrentSimTimeNano()
        //           << " ns: Sent on LinkControl in NIC: "
        //           << parent->getName() << std::endl;
    }
    return true;
}


// Returns true if there is space in the output buffer and false
// otherwise.
bool LinkControl::spaceToSend(int vn, int bits) {
    if ( (outbuf_credits[vn] * flit_size) < bits) return false;
    return true;
}


// Returns the number of flits in the input buffer
int LinkControl::getInputBufSum() {
}

// Returns the number of flits in the output buffer
int LinkControl::getOutputBufSum() {
}

// Returns NULL if no event in input_buf[vn]. Otherwise, returns
// the next event.
SST::Interfaces::SimpleNetwork::Request* LinkControl::recv(int vn) {
    if ( input_buf[vn].size() == 0 ) return NULL;

    RtrEvent* event = input_buf[vn].front();
    input_buf[vn].pop();

    // Figure out how many credits to return
    int flits = event->getSizeInFlits();
    in_ret_credits[event->request->vn] += flits;

	input_buf_flit_count -= flits;

    // For now, we're just going to send the credits back to the
    // other side.  The required BW to do this will not be taken
    // into account.
    rtr_link->send(1,new credit_event(event->request->vn,in_ret_credits[event->request->vn]));
    in_ret_credits[event->request->vn] = 0;

    // printf("%d: Returning credits on VN: %d for packet from %llu",id, event->request->vn, event->request->src);
    // std::cout << std::endl;
    
    if ( event->getTraceType() != SimpleNetwork::Request::NONE ) {
        output.output("TRACE(%d): %" PRIu64 " ns: recv called on LinkControl in NIC: %s\n",event->getTraceID(),
                      parent->getCurrentSimTimeNano(), parent->getName().c_str());
        // std::cout << "TRACE(" << event->getTraceID() << "): " << parent->getCurrentSimTimeNano()
        //           << " ns: recv called on LinkControl in NIC: "
        //           << parent->getName() << std::endl;
    }

    SST::Interfaces::SimpleNetwork::Request* ret = event->request;
    ret->vn = ret->vn / checker_board_factor;
    event->request = NULL;
    delete event;
    return ret;
}

void LinkControl::sendInitData(SST::Interfaces::SimpleNetwork::Request* req)
{
    rtr_link->sendInitData(new RtrEvent(req));
}

SST::Interfaces::SimpleNetwork::Request* LinkControl::recvInitData()
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



void LinkControl::handle_input(Event* ev)
{
    // Check to see if this is a credit or data packet
    // credit_event* ce = dynamic_cast<credit_event*>(ev);
    // if ( ce != NULL ) {
    BaseRtrEvent* base_event = static_cast<BaseRtrEvent*>(ev);
    if ( base_event->getType() == BaseRtrEvent::CREDIT ) {
    	credit_event* ce = static_cast<credit_event*>(ev);
        rtr_credits[ce->vc] += ce->credits;
        // std::cout << "Got " << ce->credits << " credits for VN: " << ce->vc << ".  Current credits: " << rtr_credits[ce->vc] << std::endl;
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
        // std::cout << "Enter handle_input" << std::endl;
        // std::cout << "LinkControl received an event" << std::endl;
        RtrEvent* event = static_cast<RtrEvent*>(ev);
        // Simply put the event into the right virtual network queue
        int actual_vn = event->request->vn / checker_board_factor;
        // std::cout << event->request->vn << ", " << actual_vn << std::endl;

        // printf("%d: Received event from %llu on VN: %d, which is actually %d\n", id, event->request->src, event->request->vn, actual_vn);
        // std::cout << std::endl;

        input_buf[actual_vn].push(event);
		input_buf_flit_count += event->getSizeInFlits();
        if (is_idle) {
            idle_time->addData(Simulation::getSimulation()->getCurrentSimCycle() - idle_start);
            is_idle = false;
			// Extra code for dynamic link additions
			// makes sure we don't count idle time outside of the window
			if (idle_start < dynlink_start){
				idle_cur += parent->getCurrentSimTimeNano() - dynlink_start;
			}
			else {
				idle_cur += parent->getCurrentSimTimeNano() - idle_start;
			}
        }
        if ( event->request->getTraceType() == SimpleNetwork::Request::FULL ) {
            output.output("TRACE(%d): %" PRIu64 " ns: Received and event on LinkControl in NIC: %s"
                          " on VN %d from src %" PRIu64 "\n",
                          event->request->getTraceID(),
                          parent->getCurrentSimTimeNano(),
                          parent->getName().c_str(),
                          event->request->vn,
                          event->request->src);

            // std::cout << "TRACE(" << event->request->getTraceID() << "): " << parent->getCurrentSimTimeNano()
            //           << " ns: Received an event on LinkControl in NIC: "
            //           << parent->getName() << " on VN " << event->request->vn << " from src " << event->request->src
            //           << "." << std::endl;
        }
        SimTime_t lat = parent->getCurrentSimTimeNano() - event->getInjectionTime();
        packet_latency->addData(lat);
        // stats.insertPacketLatency(lat);
        // std::cout << "Exit handle_input" << std::endl;
        if ( receiveFunctor != NULL ) {
            bool keep = (*receiveFunctor)(actual_vn);
            if ( !keep) receiveFunctor = NULL;
        }
    }
}

// This function is here to periodically collect measurements of the link
// so that we can summarize performance for some time interval.
// These summaries can be fed into other algorithms for dynamically setting link properties
// e.g. link width, frequency, etc
//This code should probably be abstracted away 
//so that other methods can be plugged in for predicting link behvaior.
//But this first bit will use a weighted moving average.
void LinkControl::summarize_link_state(Event* ev)
{
	uint64_t giga = 1000*1000*1000;
	double micro = 1/(1000*1000);
	double nano = 1/(1000*1000*1000);
	double window = micro;
	float weight = 4;
	
	//Need to know the frequency this is called to
	//determine percents below

	//We need a measure of percent time active
	int64_t bw_val = link_bw.getRoundedValue();
	//I'm hardcoding this in to start but this should automatically adjust
	//using the UnitAlgebra methods and micro shoudl be adjustable by the user.
	active_cur = (dl_bits_sent / (bw_val*8*giga)) / window;
	
	// If the idle interval extends over more than one window, 
	// we only capture the amount of time idle in this window.
	if (is_idle){
		if (idle_start < dynlink_start){
			idle_cur = parent->getCurrentSimTimeNano() - dynlink_start;
		}
		else {
			idle_cur += parent->getCurrentSimTimeNano() - idle_start;
		}
	}
	//We need a measure of percent time idle
	//idle_cur is collected in nano seconds
	//so divide by 1000 if we are reporting 1 micsecond windows
	idle_cur = idle_cur / (window/nano);	
	
	//We need a measure of percent time stalled
	stalled_cur = 1 - (idle_cur + active_cur);
	
	active_wma = (weight * active_cur + active_past)/(weight + 1);
	idle_wma = (weight * idle_cur + idle_past)/(weight + 1);
	stalled_wma = (weight * stalled_cur + stalled_past)/(weight + 1);

	fprintf(stderr, "Current active:%f, idle:%f stalled:%f\n",
		active_cur, idle_cur, stalled_cur);

	//The present becomes the past
	active_past = active_cur;
	idle_past = idle_cur;
	stalled_past = stalled_cur;

	//Reset current parameters to 0
	active_cur = 0;
	idle_cur = 0;
	stalled_cur = 0;
	dl_bits_sent = 0;
	dynlink_start = parent->getCurrentSimTimeNano();

	//Insert code to adjust links
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
		output_buf_flit_count -= send_event->getSizeInFlits();
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
			output_buf_flit_count -= send_event->getSizeInFlits();
            found = true;
            break;
        }
    }


    // If we found an event to send, go ahead and send it
    if ( found ) {
        // Send the output to the network.
        // First set the virtual channel.
        send_event->request->vn = vn_to_send;

        // Need to return credits to the output buffer
        int size = send_event->getSizeInFlits();
        // outbuf_credits[vn_to_send] += size;
        outbuf_credits[vn_to_send / checker_board_factor] += size;

        // Send an event to wake up again after this packet is sent.
        output_timing->send(size,NULL);
        
        curr_out_vn = vn_to_send + 1;
        if ( curr_out_vn == total_vns ) curr_out_vn = 0;

        // Add in inject time so we can track latencies
        send_event->setInjectionTime(parent->getCurrentSimTimeNano());
        
        // Subtract credits
        rtr_credits[vn_to_send] -= size;

		if (is_idle){
            idle_time->addData(Simulation::getSimulation()->getCurrentSimCycle() - idle_start);
            is_idle = false;
			// Extra code for dynamic link additions
			// makes sure we don't count idle time outside of the window
			if (idle_start < dynlink_start){
				idle_cur += parent->getCurrentSimTimeNano() - dynlink_start;
			}
			else {
				idle_cur += parent->getCurrentSimTimeNano() - idle_start;
			}
        }
        // printf("%d: Sending packet to %llu on VN: %d",id, send_event->request->dest, send_event->request->vn);
        // std::cout << std::endl;

        rtr_link->send(send_event);
        // std::cout << "Sent packet on vn " << vn_to_send << ", credits remaining: " << rtr_credits[vn_to_send] << std::endl;
        
        if ( send_event->getTraceType() == SimpleNetwork::Request::FULL ) {
            output.output("TRACE(%d): %" PRIu64 " ns: Sent an event to router from LinkControl"
                          " in NIC: %s on VN %d to dest %" PRIu64 ".\n",
                          send_event->getTraceID(),
                          parent->getCurrentSimTimeNano(),
                          parent->getName().c_str(),
                          send_event->request->vn,
                          send_event->request->dest);

            // std::cout << "TRACE(" << send_event->getTraceID() << "): " << parent->getCurrentSimTimeNano()
            //           << " ns: Sent an event to router from LinkControl in NIC: "
            //           << parent->getName() << " on VN " << send_event->request->vn << " to dest " << send_event->request->dest
            //           << "." << std::endl;
        }
        send_bit_count->addData(send_event->request->size_in_bits);
		dl_bits_sent += send_event->request->size_in_bits;
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
        // std::cout << "Waiting ..." << std::endl;
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
			// Extra code for dynamic link additions
			// makes sure we don't count idle time outside of the window
			if (idle_start < dynlink_start){
				idle_cur += parent->getCurrentSimTimeNano() - dynlink_start;
			}
			else {
				idle_cur += parent->getCurrentSimTimeNano() - idle_start;
			}
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
