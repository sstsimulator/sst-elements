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


#ifndef COMPONENTS_MERLIN_RTRIF_H
#define COMPONENTS_MERLIN_RTRIF_H

#include <sst/core/component.h>
#include <sst/core/event.h>
#include <sst/core/link.h>
#include <sst/core/timeConverter.h>

#include <queue>
#include <cstring>

using namespace SST;

namespace SST::Merlin {

class RtrEvent : public Event {


public:
    typedef enum { credit, packet } msgType_t;

    msgType_t type;
    int dest;
    int src_or_cred;
    int vc;
    int size_in_flits;
};

// Whole class definition needs to be in the header file so that other
// libraries can use the class to talk with the merlin routers.


typedef std::queue<RtrEvent*> network_queue_t;

// Class to manage link between NIC and router.  A single NIC can have
// more than one link_control (and thus link to router).
class link_control {
private:
    // Link to router
    Link* rtr_link;
    // Self link for timing output.  This is how we manage bandwidth
    // usage
    Link* output_timing;

    // Number of virtual networks
    int num_vns;
    // Number of virtual channels.  Does not need to match number of
    // virtual networks.  In general, there must be at least as many
    // virtual channels as there are virtual networks.
    int num_vcs;

    // Maps virtual networks to output virtual channels
    int* vn2vc_map;
    // Mpas virtual channels to input virtual networks
    int* vc2vn_map;

    // One buffer for each virtual network.  At the NIC level, we just
    // provide a virtual network abstraction.
    network_queue_t* input_buf;
    network_queue_t* output_buf;

    // Variables to keep track of credits.  You need to keep track of
    // the credits available for your next buffer, as well as track
    // the credits you need to return to the buffer sending data to
    // you,
    int* out_credits;
    int* ret_credits;

    // Doing a round robin on the output.  Need to keep track of the
    // current virtual network.
    int curr_out_vn;

    // Vairable to tell us if we are waiting for something to happen
    // before we begin more output.  The two things we are waiting on
    // is: 1 - adding new data to output buffers, or 2 - getting
    // credits back from the router.
    bool waiting;

public:
    // time_base is a frequency which represents the bandwidth of the link in flits/second.
    link_control(Component* rif, std::string port_name, TimeConverter* time_base, int vns, int vcs, int* vn2vc, int* vc2vn, int* ret_cred) :
	num_vns(vns),
	num_vcs(vcs),
	vn2vc_map(vn2vc),
	vc2vn_map(vc2vn)
    {
	// Buffers only for the number of VNs
	input_buf = new network_queue_t[vns];
	output_buf = new network_queue_t[vns];


	// Although there are only num_vns buffers, we will manage
	// things as if there were one buffer per vc
	out_credits = new int[vcs];
	ret_credits = new int[vcs];

	// Copy the starting return tokens for the input buffers (this
	// essentially sets the size of the buffer)
	memcpy(ret_credits,ret_cred,vcs*sizeof(int));

	// The output credits are set to zero and the other side of the
	// link will send the number of tokens.
	for ( int i = 0; i < vcs; i++ ) out_credits[i] = 0;

	// Configure the links
	rtr_link = rif->configureLink(port_name, time_base, new Event::Handler<link_control>(this,&link_control::handle_input));
	output_timing = rif->configureLink(port_name + "_output_timing", time_base,
					   new Event::Handler<link_control>(this,&link_control::handle_output));

	curr_out_vn = 0;
    }

    void handle_input(Event* ev) {
	RtrEvent* event = static_cast<RtrEvent*>(ev);

	// Check to see if this is a credit or data packet
	if ( event->type == RtrEvent::credit ) {
	    // Simply return the credits
	    out_credits[event->vc] += event->src_or_cred;
	    delete event;

	    // If we're waiting, we need to send a wakeup event to the
	    // output queues
	    if ( waiting ) {
		output_timing->send(1,NULL);     // Renamed per Issue 70 - ALevine
		waiting = false;
	    }
	}
	else {
	    // Simply put the event into the right virtual network queue
	    input_buf[vc2vn_map[event->vc]].push(event);
	}
    }
    
    void handle_output(Event* ev) {
	// The event is an empty event used just for timing.

	// ***** Need to add in logic for when to return credits *****

	// We do a round robin scheduling.  If the current vn has no
	// data, find one that does.
	int vn_to_send = -1;
	bool found = false;
	RtrEvent* send_event = NULL;
	
	for ( int i = curr_out_vn; i < num_vns; i++ ) {
	    if ( output_buf[i].empty() ) continue;
	    send_event = output_buf[i].front();
	    // Check to see if the needed VC has enough space
	    if ( out_credits[vn2vc_map[i]] < send_event->size_in_flits ) continue;
	    vn_to_send = i;
	    output_buf[i].pop();
	    found = true;
	    break;
	}
	
	if (!found)  {
	    for ( int i = 0; i < curr_out_vn; i++ ) {
		if ( output_buf[i].empty() ) continue;
		send_event = output_buf[i].front();
		// Check to see if the needed VC has enough space
		if ( out_credits[vn2vc_map[i]] < send_event->size_in_flits ) continue;
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
	    send_event->vc = vn2vc_map[vn_to_send];
	    send_event->type = RtrEvent::packet;
	    rtr_link->send(send_event);   // Renamed per Issue 70 - ALevine
	    
	    // Send an event to wake up again after this packet is sent.
	    int size = send_event->size_in_flits;
	    output_timing->send(size,NULL);   // Renamed per Issue 70 - ALevine

	    curr_out_vn = vn_to_send++;
	    if ( curr_out_vn == num_vns ) curr_out_vn = 0;
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

class RtrIF : public Component {

private:
    int num_vns;
    network_queue_t* input_bufs;
    network_queue_t* output_bufs;
    
public:
    RtrIF(ComponentId_t cid, Params& params) :
	Component(cid)
    {
	// Get the parameters
	num_vns = params.find_integer("num_vns");
	if ( num_vns == -1 ) {
	}

	
    }

};

}

#endif // COMPONENTS_MERLIN_RTRIF_H
