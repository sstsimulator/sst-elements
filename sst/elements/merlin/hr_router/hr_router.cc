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
#include <sst_config.h>
#include "sst/core/serialization/element.h"

#include <sst/core/element.h>
#include <sst/core/simulation.h>
#include <sst/core/timeLord.h>

#include <sstream>

#include "hr_router.h"
#include "portControl.h"
#include "sst/elements/merlin/topology/torus.h"
#include "xbar_arb_rr.h"

hr_router::~hr_router()
{
}

hr_router::hr_router(ComponentId_t cid, Params& params) :
    Component(cid)
{
    // Get the options for the router
    id = params.find_integer("id");
    if ( id == -1 ) {
    }
    std::cout << "id: " << id << std::endl;

    num_ports = params.find_integer("num_ports");
    if ( num_ports == -1 ) {
    }
    std::cout << "num_ports: " << num_ports << std::endl;

    num_vcs = params.find_integer("num_vcs");
    if ( num_vcs == -1 ) {
    }
    std::cout << "num_vcs: " << num_vcs << std::endl;

    std::string link_bw = params.find_string("link_bw");
    if ( link_bw == "" ) {
    }
    std::cout << "link_bw: " << link_bw << std::endl;

    TimeConverter* tc = Simulation::getSimulation()->getTimeLord()->getTimeConverter(link_bw);    
    
    // Get the topology
    std::string topology = params.find_string("topology");
    std::cout << "Topology: " << topology << std::endl;

    if ( !topology.compare("torus") ) {
	std::cout << "Creating new topology" << std::endl;
	topo = new topo_torus(params);
    }

    // Get the Xbar arbitration
    arb = new xbar_arb_rr(num_ports,num_vcs);
    
    // Create all the PortControl blocks
    ports = new PortControl*[num_ports];

    
    int in_buf_sizes[num_vcs];
    int out_buf_sizes[num_vcs];
    
    for ( int i = 0; i < num_vcs; i++ ) {
	in_buf_sizes[i] = 10;
	out_buf_sizes[i] = 10;
    }
    
    // Naming convention is from point of view of the xbar.  So,
    // in_port_busy is >0 if someone is writing to that xbar port and
    // out_port_busy is >0 if that xbar port being read.
    in_port_busy = new int[num_ports];
    out_port_busy = new int[num_ports];

    progress_vcs = new int[num_ports];

    for ( int i = 0; i < num_ports; i++ ) {
	in_port_busy[i] = 0;
	out_port_busy[i] = 0;

	std::stringstream port_name;
	port_name << "port";
	port_name << i;
	std::cout << port_name.str() << std::endl;

	ports[i] = new PortControl(this, port_name.str(), i, tc, topo, num_vcs, in_buf_sizes, out_buf_sizes);
	
	std::cout << port_name.str() << std::endl;
	// links[i] = configureLink(port_name.str(), "1ns", new Event::Handler<hr_router,int>(this,&hr_router::port_handler,i));
    }

    registerClock( "1GHz", new Clock::Handler<hr_router>(this,&hr_router::clock_handler), false);
}


bool
hr_router::clock_handler(Cycle_t cycle)
{
    /*
    internal_router_event** heads = new internal_router_event*[num_vcs];
// Progress the input ports.  For now, no arbitration or
    // contention is accounted for
    for ( int i = rr_port, j = 0; j < num_ports; i = (i + 1) % num_ports, j++ ) {
	// Nothing to do if port into crossbar is busy
	if ( port_in_busy[i] > 0 ) {
	    port_in_busy[i]--;
	    continue;
	}

	// See what we should progress
	ports[i]->getVCHeads(heads);
	for ( int k = rr_vcs[i], l = 0; l < num_vcs; k = (k+1) % num_vcs, l++ ) {
	    
	}
    }
    */
    return false;
}

int
hr_router::Setup()
{
    /*
    for ( int i = rr_port; i < rr_port + num_ports; i++ ) {
	ports[i]->Setup();
    }
    */
    return 0;
}

