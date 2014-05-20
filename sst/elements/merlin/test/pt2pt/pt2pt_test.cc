// Copyright 2009-2014 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2014, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.
#include <sst_config.h>
#include "sst/core/serialization.h"

#include <sst/core/element.h>
#include <sst/core/params.h>
#include <sst/core/simulation.h>
#include <sst/core/timeLord.h>
#include <sst/core/unitAlgebra.h>

#include "sst/elements/merlin/merlin.h"
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
    
    num_vns = params.find_integer("num_vns",1);
    cout << "num_vns: " << num_vns << endl;

    string link_bw_s = params.find_string("link_bw","2GB/s");
    cout << "link_bw: " << link_bw_s << endl;
    UnitAlgebra link_bw(link_bw_s);
    
    string packet_size_s = params.find_string("packet_size","512b");
    UnitAlgebra packet_size_ua(packet_size_s);
    
    if ( !packet_size_ua.hasUnits("b") && !packet_size_ua.hasUnits("B") ) {
        merlin_abort.fatal(CALL_INFO,-1,"packet_size must be specified in either "
                           "bits or bytes: %s\n",packet_size_ua.toStringBestSI().c_str());
    }
    if ( packet_size_ua.hasUnits("B") ) packet_size_ua *= UnitAlgebra("8b/B");
    packet_size = packet_size_ua.getRoundedValue();
    
    packets_to_send = params.find_integer("packets_to_send",32);
    
    string buffer_size_s = params.find_string("buffer_size","128B");
    buffer_size = UnitAlgebra(buffer_size_s);
    
    // Create a LinkControl object
    link_control = (Merlin::LinkControl*)loadModule("merlin.linkcontrol", params);
    // link_control->configureLink(this, "rtr", tc, num_vns, buf_size, buf_size);
    link_control->configureLink(this, "rtr", link_bw, num_vns, buffer_size, buffer_size);

    // Register a clock
    registerClock( "1GHz", new Clock::Handler<pt2pt_test>(this,&pt2pt_test::clock_handler), false);

    if ( id == 1 ) {
        // self_link = configureSelfLink("complete_link", link_bw_s,
		// 		      new Event::Handler<pt2pt_test>(this,&pt2pt_test::handle_complete));

        // Configure a new self link to be used to time the
        // serialization latency of the last packet.  We won't know
        // the final BW until the network is intialized, so we'll put
        // in a dummy value, then change it later.
        self_link = configureSelfLink("complete_link", "2GHz",
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
    if ( id == 1 ) {
        UnitAlgebra link_bw = link_control->getLinkBW();
        // Invert this to get a time per bit
        std::cout << link_bw.toStringBestSI() << std::endl;
        link_bw.invert();
        std::cout << link_bw.toStringBestSI() << std::endl;

        link_bw *= (UnitAlgebra("1b") *= packet_size);
        TimeConverter* tc = getTimeConverter(link_bw);
        std::cout << link_bw.toStringBestSI() << std::endl;
        self_link->setDefaultTimeBase(tc);
        std::cout << tc->getFactor() << std::endl;
    }
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
	    ev->vn = 1;
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
            if ( packets_sent == 0 ) {
                ev->start_time = getCurrentSimTimeNano();
                ev->setTraceType(RtrEvent::FULL);
            }
            ev->setTraceID(packets_sent);
            ev->dest = 1;
            ev->src = 0;
            ev->vn = 0;
            ev->size_in_bits = packet_size;
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
                std::cout << getCurrentSimTimeNano() << ", " << packet_size << std::endl;
                self_link->send(1,rec_ev); 
                return true;
            }
            else {
                delete rec_ev;
            }
        }

        // rec_ev = static_cast<pt2pt_test_event*>(link_control->recv(1));
        // if ( rec_ev != NULL ) {
        //     if ( packets_recd == 0 ) {
        //         start_time = getCurrentSimTimeNano();
        //         latency = start_time - rec_ev->start_time;
        //     }
        //     ++packets_recd;
            
        //     if ( packets_recd == 2*packets_to_send ) {
        //         // Need to send this event to a self link to account
        //         // for serialization latency.  This will trigger an
        //         // event handler that will compute the BW.
        //         self_link->send(packet_size,rec_ev); 
        //         return true;
        //     }
        //     else {
        //         delete rec_ev;
        //     }
        // }
    }
    
    return false;
}

void
pt2pt_test::handle_complete(Event* ev) {
    delete ev;

    std::cout << getCurrentSimTimeNano() << std::endl;
    // Compute BW
    SimTime_t end_time = getCurrentSimTimeNano();
    
    double total_sent = (packet_size * packets_to_send);
    double total_time = (double)end_time - double (start_time);
    double bw = total_sent / total_time;

    cout << "Latency = " << latency << " ns" << endl;
    cout << "Start time = " << start_time << endl;
    cout << "End time = " << end_time << endl;
    
    cout << "Total sent = " << total_sent << endl;
    
    cout << "BW = " << bw << " Gbits/sec" << endl;
    primaryComponentOKToEndSim();

}
