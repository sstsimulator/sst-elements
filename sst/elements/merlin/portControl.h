// -*- mode: c++ -*-

// Copyright 2009-2013 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2013, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#ifndef COMPONENTS_MERLIN_PORTCONTROL_H
#define COMPONENTS_MERLIN_PORTCONTROL_H

#include <sst/core/component.h>
#include <sst/core/event.h>
#include <sst/core/link.h>
#include <sst/core/timeConverter.h>

#include <queue>
#include <cstring>

#include "sst/elements/merlin/router.h"

using namespace SST;

namespace SST {
namespace Merlin {

// Whole class definition needs to be in the header file so that other
// libraries can use the class to talk with the merlin routers.


typedef std::queue<internal_router_event*> port_queue_t;

// Class to manage link between NIC and router.  A single NIC can have
// more than one link_control (and thus link to router).
class PortControl {
private:
    // Link to NIC or other router
    Link* port_link;
    // Self link for timing output.  This is how we manage bandwidth
    // usage
    Link* output_timing;

    std::deque<Event*> init_events;

    int rtr_id;
    // Number of virtual channels
    int num_vcs;

    Topology* topo;
    int port_number;
    bool host_port;
    
    // One buffer for each virtual network.  At the NIC level, we just
    // provide a virtual network abstraction.
    port_queue_t* input_buf;
    port_queue_t* output_buf;

    int* input_buf_count;
    int* output_buf_count;

    // Variables to keep track of credits.  You need to keep track of
    // the credits available for your next buffer, as well as track
    // the credits you need to return to the buffer sending data to
    // you,
    int* xbar_in_credits;

    int* port_out_credits;
    int* port_ret_credits;
    
    // Doing a round robin on the output.  Need to keep track of the
    // current virtual channel.
    int curr_out_vc;

    // Vairable to tell us if we are waiting for something to happen
    // before we begin more output.  The two things we are waiting on
    // is: 1 - adding new data to output buffers, or 2 - getting
    // credits back from the router.
    bool waiting;
    Component* parent;

public:

    // Returns true if there is space in the output buffer and false
    // otherwise.
    bool send(internal_router_event* ev, int vc) {
	// std::cout << "sending event on port " << port_number << " and VC " << vc << std::endl;
	if ( xbar_in_credits[vc] < ev->getFlitCount() ) return false;

	xbar_in_credits[vc] -= ev->getFlitCount();
	ev->setVC(vc);

	output_buf[vc].push(ev);
	if ( waiting ) {
	    // std::cout << "waking up the output" << std::endl;
	    output_timing->send(1,NULL); 
	    waiting = false;
	}
	return true;
    }

    // Returns true if there is space in the output buffer and false
    // otherwise.
    bool spaceToSend(int vc, int flits) {
	if (xbar_in_credits[vc] < flits) return false;
	return true;
    }

    // Returns NULL if no event in input_buf[vc]. Otherwise, returns
    // the next event.
    internal_router_event* recv(int vc) {
	if ( input_buf[vc].size() == 0 ) return NULL;

	internal_router_event* event = input_buf[vc].front();
	input_buf[vc].pop();

	// Figure out how many credits to return
	port_ret_credits[vc] += event->getFlitCount();

	// For now, we're just going to send the credits back to the
	// other side.  The required BW to do this will not be taken
	// into account.
	port_link->send(1,new credit_event(vc,port_ret_credits[vc])); 
	port_ret_credits[vc] = 0;
	
	
	return event;
    }

    void getVCHeads(internal_router_event** heads) {
	for ( int i = 0; i < num_vcs; i++ ) {
	    if ( input_buf[i].size() == 0 ) heads[i] = NULL;
	    else heads[i] = input_buf[i].front();
	}
    }
    
    // time_base is a frequency which represents the bandwidth of the link in flits/second.
    PortControl(Component* rif, int rtr_id, std::string link_port_name, int port_number, TimeConverter* time_base,
		Topology *topo, int vcs, int* in_buf_size, int* out_buf_size,
		SimTime_t input_latency_cycles, std::string input_latency_timebase,
		SimTime_t output_latency_cycles, std::string output_latency_timebase) :
	rtr_id(rtr_id),
	num_vcs(vcs),
	topo(topo),
	port_number(port_number),
	waiting(true),
	parent(rif)
    {
	// Input and output buffers
	input_buf = new port_queue_t[vcs];
	output_buf = new port_queue_t[vcs];
	
	input_buf_count = new int[vcs];
	output_buf_count = new int[vcs];
	
	for ( int i = 0; i < num_vcs; i++ ) {
	    input_buf_count[i] = 0;
	    output_buf_count[i] = 0;
	}
	
	// Initialize credit arrays
	xbar_in_credits = new int[vcs];
	port_ret_credits = new int[vcs];
	port_out_credits = new int[vcs];

	// Copy the starting return tokens for the input buffers (this
	// essentially sets the size of the buffer)
	memcpy(port_ret_credits,in_buf_size,vcs*sizeof(int));
	memcpy(xbar_in_credits,out_buf_size,vcs*sizeof(int));
	
	// The output credits are set to zero and the other side of the
	// link will send the number of tokens.
	for ( int i = 0; i < vcs; i++ ) port_out_credits[i] = 0;

	// Configure the links
        switch ( topo->getPortState(port_number) ) {
        case Topology::R2N:
            host_port = true;
	    port_link = rif->configureLink(link_port_name, output_latency_timebase,
					   new Event::Handler<PortControl>(this,&PortControl::handle_input_n2r));
            break;
        case Topology::R2R:
            host_port = false;
	    port_link = rif->configureLink(link_port_name, output_latency_timebase,
					   new Event::Handler<PortControl>(this,&PortControl::handle_input_r2r));
            break;
        default:
            host_port = false;
            port_link = NULL;
            break;
        }


	if ( input_latency_timebase != "" ) {
	    // std::cout << "Adding extra latency" << std::endl;
	    port_link->addOutputLatency(input_latency_cycles,input_latency_timebase);
	}


	output_timing = rif->configureSelfLink(link_port_name + "_output_timing", time_base,
					       new Event::Handler<PortControl>(this,&PortControl::handle_output));

	curr_out_vc = 0;
    }

    ~PortControl() {
        delete [] input_buf;
        delete [] output_buf;
        delete [] input_buf_count;
        delete [] output_buf_count;
        delete [] xbar_in_credits;
        delete [] port_ret_credits;
        delete [] port_out_credits;
    }

    void setup() {
        while ( init_events.size() ) {
            delete init_events.front();
            init_events.pop_front();
        }
    }

    void init(unsigned int phase) {
	if ( topo->getPortState(port_number) == Topology::UNCONNECTED ) return;
        if ( phase == 0 ) {
	    // Need to send the available credits to the other side
	    for ( int i = 0; i < num_vcs; i++ ) {
		port_link->sendInitData(new credit_event(i,port_ret_credits[i]));
		port_ret_credits[i] = 0;
	    }
        }
        // Need to recv the credits send from the other side
        Event* ev;
        while ( ( ev = port_link->recvInitData() ) != NULL ) {
            credit_event* ce = dynamic_cast<credit_event*>(ev);
            if ( ce != NULL ) {
                port_out_credits[ce->vc] += ce->credits;
                delete ev;
            } else {
                init_events.push_back(ev);
            }
        }
    }

    void sendInitData(Event *ev)
    {
        port_link->sendInitData(ev);
    }

    Event* recvInitData()
    {
        if ( init_events.size() ) {
            Event *ev = init_events.front();
            init_events.pop_front();
            return ev;
        } else {
            return NULL;
        }
    }

    void dumpState(std::ostream& stream) {
	stream << "Router id: " << rtr_id << ", port " << port_number << ":" << std::endl;
	stream << "  Waiting = " << waiting << std::endl;
	stream << "  curr_out_vc = " << curr_out_vc << std::endl;
	for ( int i = 0; i < num_vcs; i++ ) {
	    stream << "  VC " << i << " Information:" << std::endl;
	    stream << "    xbar_in_credits = " << xbar_in_credits[i] << std::endl;
	    stream << "    port_out_credits = " << port_out_credits[i] << std::endl;
	    stream << "    port_ret_credits = " << port_ret_credits[i] << std::endl;
	    stream << "    Input Buffer:" << std::endl;
	    dumpQueueState(input_buf[i],stream);
	    stream << "    Output Buffer:" << std::endl;
	    dumpQueueState(output_buf[i],stream);
	}
    }

    
private:

    void dumpQueueState(port_queue_t& q, std::ostream& stream) {
	int size = q.size();
	for ( int i = 0; i < size; i++ ) {
	    internal_router_event* ev = q.front();
	    stream << "      dest = " << ev->getDest()
		   << ", size = " << ev->getFlitCount()
		   << ", vc = " << ev->getVC()
		   << ", next_port = " << ev->getNextPort()
		   << std::endl;
	    q.pop();
	    q.push(ev);
	}
    }
    
    void handle_input_n2r(Event* ev) {
	// Check to see if this is a credit or data packet
	credit_event* ce = dynamic_cast<credit_event*>(ev);
	if ( ce != NULL ) {
	    port_out_credits[ce->vc] += ce->credits;
	    delete ce;

	    // If we're waiting, we need to send a wakeup event to the
	    // output queues
	    if ( waiting ) {
		// std::cout << output_timing << std::endl;
		output_timing->send(1,NULL); 
		waiting = false;
	    }
	}
	else {
	    RtrEvent* event = static_cast<RtrEvent*>(ev);
	    // Simply put the event into the right virtual network queue

	    // Need to process input and do the routing
	    internal_router_event* rtr_event = topo->process_input(event);
	    int curr_vc = event->vc;
	    topo->route(port_number, event->vc, rtr_event);
	    input_buf[curr_vc].push(rtr_event);
	    input_buf_count[curr_vc]++;

	    if ( event->getTraceType() != RtrEvent::NONE ) {
		std::cout << "TRACE(" << event->getTraceID() << "): " << parent->getCurrentSimTimeNano()
			  << " ns: Received an event on port " << port_number
			  << " in router " << rtr_id << " ("
			  << parent->getName() << ") on VC " << curr_vc << " from src " << event->src
			  << " to dest " << event->dest << "." << std::endl;
	    }
	}
    }
    
    void handle_input_r2r(Event* ev) {
	// Check to see if this is a credit or data packet
	credit_event* ce = dynamic_cast<credit_event*>(ev);
	if ( ce != NULL ) {
	    port_out_credits[ce->vc] += ce->credits;
	    delete ce;

	    // If we're waiting, we need to send a wakeup event to the
	    // output queues
	    if ( waiting ) {
		output_timing->send(1,NULL); 
		waiting = false;
	    }	    
	}
	else {
	    internal_router_event* event = static_cast<internal_router_event*>(ev);
	    // Simply put the event into the right virtual network queue

	    // Need to do the routing
	    int curr_vc = event->getVC();
	    topo->route(port_number, event->getVC(), event);
	    input_buf[curr_vc].push(event);
	    input_buf_count[curr_vc]++;

	    if ( event->getTraceType() != RtrEvent::NONE ) {
		std::cout << "TRACE(" << event->getTraceID() << "): " << parent->getCurrentSimTimeNano()
			  << " ns: Received an event on port " << port_number
			  << " in router " << rtr_id << " ("
			  << parent->getName() << ") on VC " << curr_vc << " from src " << event->getSrc()
			  << " to dest " << event->getDest() << "." << std::endl;
	    }
	}
    }
    
    void handle_output(Event* ev) {
	// The event is an empty event used just for timing.

	// ***** Need to add in logic for when to return credits *****
	// For now just done automatically when events are pulled out
	// of the block

	// We do a round robin scheduling.  If the current vc has no
	// data, find one that does.
	int vc_to_send = -1;
	bool found = false;
	internal_router_event* send_event = NULL;
	
	for ( int i = curr_out_vc; i < num_vcs; i++ ) {
	    if ( output_buf[i].empty() ) continue;
	    send_event = output_buf[i].front();
	    // Check to see if the needed VC has enough space
	    if ( port_out_credits[i] < send_event->getFlitCount() ) continue;
	    vc_to_send = i;
	    output_buf[i].pop();
	    found = true;
	    break;
	}
	
	if (!found)  {
	    for ( int i = 0; i < curr_out_vc; i++ ) {
		if ( output_buf[i].empty() ) continue;
		send_event = output_buf[i].front();
		// Check to see if the needed VC has enough space
		if ( port_out_credits[i] < send_event->getFlitCount() ) continue;
		vc_to_send = i;
		output_buf[i].pop();
		found = true;
		break;
	    }
	}
	

	// If we found an event to send, go ahead and send it
	if ( found ) {
	    // std::cout << "Found an event to send on output port " << port_number << std::endl;
	    // Send the output to the network.
	    // First set the virtual channel.
	    send_event->setVC(vc_to_send);

	    // Need to return credits to the output buffer
	    int size = send_event->getFlitCount();
	    xbar_in_credits[vc_to_send] += size;
	    
	    // Send an event to wake up again after this packet is sent.
	    output_timing->send(size,NULL); 
	    
	    // Take care of the round variable
	    curr_out_vc = vc_to_send + 1;
	    if ( curr_out_vc == num_vcs ) curr_out_vc = 0;

	    // Subtract credits
	    port_out_credits[vc_to_send] -= size;
	    output_buf_count[vc_to_send]++;

	    if ( send_event->getTraceType() == RtrEvent::FULL ) {
		std::cout << "TRACE(" << send_event->getTraceID() << "): " << parent->getCurrentSimTimeNano()
			  << " ns: Sent an event to router from PortControl in router: " << rtr_id
			  << " (" << parent->getName() << ") on VC " << send_event->getVC()
			  << " from src " << send_event->getSrc()
			  << " to dest " << send_event->getDest()
			  << "." << std::endl;
	    }

	    if ( host_port ) {
		// std::cout << "Found an event to send on host port " << port_number << std::endl;
		port_link->send(1,send_event->getEncapsulatedEvent()); 
		send_event->setEncapsulatedEvent(NULL);
		delete send_event;
	    }
	    else {
		port_link->send(1,send_event); 
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
	    waiting = true;
	}
    }
    
};

}
}

#endif // COMPONENTS_MERLIN_PORTCONTROL_H
