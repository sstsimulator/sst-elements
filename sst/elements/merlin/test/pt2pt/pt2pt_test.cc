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
#include <sst_config.h>
#include "sst/core/serialization/element.h"

#include <sst/core/element.h>
#include <sst/core/simulation.h>
#include <sst/core/timeLord.h>

#include "sst/elements/merlin/linkControl.h"
#include "sst/elements/merlin/test/pt2pt/pt2pt_test.h"

using namespace std;
using namespace SST::Merlin;

pt2pt_test::pt2pt_test(ComponentId_t cid, Params& params) :
    Component(cid),
    packets_sent(0),
    packets_recd(0)
{
    id = params.find_integer("id");
    if ( id == -1 ) {
    }
    cout << "id: " << id << endl;

    /*
    if ( id != 0 && id != 1 ) {
	cout << "pt2pt_test only works with 2 test components, aborting" << endl;
	abort();
    }
    */
    
    num_vcs = params.find_integer("num_vcs",2);
    cout << "num_vcs: " << num_vcs << endl;

    string link_bw = params.find_string("link_bw","2GHz");
    cout << "link_bw: " << link_bw << endl;
    TimeConverter* tc = Simulation::getSimulation()->getTimeLord()->getTimeConverter(link_bw);

    packet_size = params.find_integer("packet_size",16);

    packets_to_send = params.find_integer("packets_to_send",32);
    
    buffer_size = params.find_integer("buffer_size",128);
    
    // Create a LinkControl object
    int buf_size[2] = {buffer_size, buffer_size};
    link_control = (Merlin::LinkControl*)loadModule("merlin.linkcontrol", params);
    link_control->configureLink(this, "rtr", tc, num_vcs, buf_size, buf_size);

    // Register a clock
    registerClock( "1GHz", new Clock::Handler<pt2pt_test>(this,&pt2pt_test::clock_handler), false);

    if ( id == 1 ) {
	self_link = configureSelfLink("complete_link", tc,
				      new Event::Handler<pt2pt_test>(this,&pt2pt_test::handle_complete));
    }
    
    registerAsPrimaryComponent();
    primaryComponentDoNotEndSim();

}

void pt2pt_test::finish()
{
    link_control->finish();
}

void pt2pt_test::setup()
{
    link_control->setup();
}

void
pt2pt_test::init(unsigned int phase)
{
    link_control->init(phase);
}
/*
bool
pt2pt_test::clock_handler(Cycle_t cycle)
{
    if ( id == 0 ) {
	// ID 0 is the sender

	// Do a bandwidth test
	if ( packets_sent == packets_to_send ) {
      primaryComponentOKToEndSim();
	    
	    cout << "0: Done" << endl;
	    return true;  // Take myself off clock list
	}
	
	if (  link_control->spaceToSend(1,packet_size) ) {
	    pt2pt_test_event* ev = new pt2pt_test_event();
	    ev->dest = 1;
	    ev->vc = 1;
	    ev->size_in_flits = packet_size;
	    link_control->send(ev,1);
	    ++packets_sent;
	}
    }
    
    else {
	// ID 1 is the receiver
	pt2pt_test_event* rec_ev = static_cast<pt2pt_test_event*>(link_control->recv(1));
	if ( rec_ev != NULL ) {
	    if ( packets_recd == 0 ) start_time = getCurrentSimTimeNano();
	    ++packets_recd;

	    if ( packets_recd == packets_to_send ) {
		// Need to send this event to a self link to account
		// for serialization latency.  This will trigger an
		// event handler that will compute the BW.
		self_link->send(packet_size,rec_ev);
		return true;
	    }
	    else {
		delete rec_ev;
	    }
	}
    }

    return false;
}
*/
bool
pt2pt_test::clock_handler(Cycle_t cycle)
{
    if ( id == 0 ) {
	// Do a bandwidth and latency test
	if ( packets_sent == packets_to_send ) {
      primaryComponentOKToEndSim();
	    return true;  // Take myself off clock list
	}
	
	if ( link_control->spaceToSend(0,packet_size) ) {
	    pt2pt_test_event* ev = new pt2pt_test_event();
	    // if ( packets_sent == 0 ) ev->setTraceType(RtrEvent::FULL);
	    // else ev->setTraceType(RtrEvent::NONE);
	    if ( packets_sent == 0 ) ev->start_time = getCurrentSimTimeNano();
	    ev->setTraceID(packets_sent);
	    ev->dest = 1;
	    ev->src = 0;
	    ev->vc = 0;
	    ev->size_in_flits = packet_size;
	    link_control->send(ev,0);
	    ++packets_sent;
	}
    }    
    else {
	// ID 1 is the receiver
	pt2pt_test_event* rec_ev = static_cast<pt2pt_test_event*>(link_control->recv(0));
	if ( rec_ev != NULL ) {
	    // cout << "received packet at " << getCurrentSimTimeNano() << endl;
	    if ( packets_recd == 0 ) {
		start_time = getCurrentSimTimeNano();
		latency = start_time - rec_ev->start_time;
	    }
	    ++packets_recd;
	    
	    if ( packets_recd == packets_to_send ) {
		// Need to send this event to a self link to account
		// for serialization latency.  This will trigger an
		// event handler that will compute the BW.
		self_link->send(packet_size,rec_ev); 
		return true;
	    }
	    else {
		delete rec_ev;
	    }
	}

	rec_ev = static_cast<pt2pt_test_event*>(link_control->recv(1));
	if ( rec_ev != NULL ) {
	    if ( packets_recd == 0 ) {
		start_time = getCurrentSimTimeNano();
		latency = start_time - rec_ev->start_time;
	    }
	    ++packets_recd;

	    if ( packets_recd == 2*packets_to_send ) {
		// Need to send this event to a self link to account
		// for serialization latency.  This will trigger an
		// event handler that will compute the BW.
		self_link->send(packet_size,rec_ev); 
		return true;
	    }
	    else {
		delete rec_ev;
	    }
	}
    }

    return false;
}

void
pt2pt_test::handle_complete(Event* ev) {
    delete ev;

    // Compute BW
    SimTime_t end_time = getCurrentSimTimeNano();

    double total_sent = (packet_size * packets_to_send);
    double total_time = (double)end_time - double (start_time);
    double bw = total_sent / total_time;

    cout << "Latency = " << latency << " ns" << endl;
    cout << "Start time = " << start_time << endl;
    cout << "End time = " << end_time << endl;
    
    cout << "Total sent = " << total_sent << endl;
    
    cout << "BW = " << bw << " GFlits/sec" << endl;
    primaryComponentOKToEndSim();

}
