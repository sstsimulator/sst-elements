// -*- mode: c++ -*-

// Copyright 2009-2012 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2012, Sandia Corporation
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

    // Number of virtual channels
    int num_vcs;

    Topology* topo;
    int port_number;
    bool host_port;
    
    // One buffer for each virtual network.  At the NIC level, we just
    // provide a virtual network abstraction.
    port_queue_t* input_buf;
    port_queue_t* output_buf;

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
	    output_timing->Send(1,NULL);
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
	port_link->Send(1,new credit_event(vc,port_ret_credits[vc]));
	port_ret_credits[vc] = 0;
	
	
	return event;
    }

    void getVCHeads(internal_router_event** heads) {
	for ( int i = 0; i < num_vcs; i++ ) {
	    if ( input_buf[i].size() == 0 ) heads[i] = NULL;
	    heads[i] = input_buf[i].front();
	}
    }
    
    // time_base is a frequency which represents the bandwidth of the link in flits/second.
    PortControl(Component* rif, std::string link_port_name, int port_number, TimeConverter* time_base,
		Topology *topo, int vcs, int* in_buf_size, int* out_buf_size) :
	num_vcs(vcs),
	topo(topo),
	port_number(port_number),
	waiting(true)
    {
	// Input and output buffers
	input_buf = new port_queue_t[vcs];
	output_buf = new port_queue_t[vcs];

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
	host_port = topo->isHostPort(port_number);
	if ( host_port ) {
	    port_link = rif->configureLink(link_port_name, time_base,
					   new Event::Handler<PortControl>(this,&PortControl::handle_input_n2r));
	}
	else {
	    port_link = rif->configureLink(link_port_name, time_base,
					   new Event::Handler<PortControl>(this,&PortControl::handle_input_r2r));
	}
	output_timing = rif->configureSelfLink(link_port_name + "_output_timing", time_base,
					       new Event::Handler<PortControl>(this,&PortControl::handle_output));

	curr_out_vc = 0;
    }

    int Setup() {
	// Need to send the available credits to the other side
	for ( int i = 0; i < num_vcs; i++ ) {
	    port_link->Send(1,new credit_event(i,port_ret_credits[i]));
	    port_ret_credits[i] = 0;
	}
	return 0;
    }

    
private:

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
		output_timing->Send(1,NULL);
		waiting = false;
	    }	    
	}
	else {
	    RtrEvent* event = static_cast<RtrEvent*>(ev);
	    // Simply put the event into the right virtual network queue

	    // Need to process input and do the routing
	    internal_router_event* rtr_event = topo->process_input(event);
	    topo->route(port_number, event->vc, rtr_event);
	    input_buf[event->vc].push(rtr_event);
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
		output_timing->Send(1,NULL);
		waiting = false;
	    }	    
	}
	else {
	    internal_router_event* event = static_cast<internal_router_event*>(ev);
	    // Simply put the event into the right virtual network queue

	    // Need to do the routing
	    topo->route(port_number, event->getVC(), event);
	    input_buf[event->getVC()].push(event);
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
	    output_timing->Send(size,NULL);

	    // Take care of the round variable
	    curr_out_vc = vc_to_send + 1;
	    if ( curr_out_vc == num_vcs ) curr_out_vc = 0;

	    // Subtract credits
	    port_out_credits[vc_to_send] -= size;
	    if ( host_port ) {
		// std::cout << "Found an event to send on host port " << port_number << std::endl;
		port_link->Send(send_event->getEncapsulatedEvent());
		send_event->setEncapsulatedEvent(NULL);
		delete send_event;
	    }
	    else port_link->Send(send_event);


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

#endif // COMPONENTS_MERLIN_PORTCONTROL_H
