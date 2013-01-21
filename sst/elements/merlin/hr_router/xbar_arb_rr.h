// Copyright 2009-2010 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2010, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#ifndef COMPONENTS_HR_ROUTER_XBAR_ARB_RR_H
#define COMPONENTS_HR_ROUTER_XBAR_ARB_RR_H

#include <sst/core/component.h>
#include <sst/core/event.h>
#include <sst/core/link.h>
#include <sst/core/timeConverter.h>

#include <queue>

#include "sst/elements/merlin/router.h"
#include "sst/elements/merlin/portControl.h"

using namespace SST;

class xbar_arb_rr : public XbarArbitration {

private:
    int num_ports;
    int num_vcs;

    int *rr_vcs;
    int rr_port;

    internal_router_event** vc_heads;
public:
    xbar_arb_rr(int num_ports, int num_vcs) :
	XbarArbitration(),
	num_ports(num_ports),
	num_vcs(num_vcs)
    {
	rr_vcs = new int[num_ports];
	for ( int i = 0; i < num_ports; i++ ) {
	    rr_vcs[i] = 0;
	}
	
	rr_port = 0;

	vc_heads = new internal_router_event*[num_vcs];
    }
    ~xbar_arb_rr() {
	delete[] vc_heads;
    }

    // Naming convention is from point of view of the xbar.  So,
    // in_port_busy is >0 if someone is writing to that xbar port and
    // out_port_busy is >0 if that xbar port being read.
    void arbitrate(PortControl** ports, int* in_port_busy, int* out_port_busy, int* progress_vc) {
	// Run through each of the ports, giving first pick in a round robin fashion
	for ( int port = rr_port, pcount = 0; pcount < num_ports; port = (port+1) % num_ports, pcount++ ) {
	    // Overwrite old data
	    progress_vc[port] = -1;
	    // if the output of this port is busy, nothing to do.
	    if ( in_port_busy[port] > 0 ) continue;

	    ports[port]->getVCHeads(vc_heads);
	    
	    // See what we should progress for this port
	    for ( int vc = rr_vcs[port], vcount = 0; vcount < num_vcs; vc = (vc+1) % num_vcs, vcount++ ) {

		// If there is no event, move to next VC
		internal_router_event* src_event = vc_heads[vc];
		if ( src_event == NULL ) continue;
		
		// Have an event, see if it can be progressed
		int next_port = src_event->getNextPort();

		// We can progress if the next port's input is not
		// busy and there are enough credits.
		if ( out_port_busy[next_port] > 0 ) continue;

		// Need to see if the VC has enough credits
		int next_vc = src_event->getVC();

		// See if there is enough space
		if ( !ports[next_port]->spaceToSend(next_vc, src_event->getFlitCount()) ) continue;

		// Tell the router what to move
		progress_vc[port] = vc;

		// Need to set the busy values
		in_port_busy[port] = src_event->getFlitCount();
		out_port_busy[next_port] = src_event->getFlitCount();
		break;  // Go to next port;
	    }
	    // Increemnt rr_vcs for next time
	    rr_vcs[port] = (rr_vcs[port] + 1) % num_vcs;
	}
	rr_port = (rr_port + 1) % num_ports;
    }

    void dumpState(std::ostream& stream) {
	int *rr_vcs;
	int rr_port;

	stream << "Current round robin port: " << rr_port << std::endl;
	stream << "  Current round robin VC by port:" << std::endl;
	for ( int i = 0; i < num_ports; i++ ) {
	    stream << i << ": " << rr_vcs[i] << std::endl;
	}
    }

};

#endif // COMPONENTS_HR_ROUTER_XBAR_ARB_RR_H
