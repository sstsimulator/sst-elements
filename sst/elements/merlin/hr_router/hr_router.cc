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
    
    // Get the topology
    std::string topology = params.find_string("topology");
    std::cout << "Topology: " << topology << std::endl;

    if ( !topology.compare("torus") ) {
	std::cout << "Creating new topology" << std::endl;
	topo = new topo_torus(params);
    }

    // Get the Xbar arbitration
    arb = new xbar_arb_rr(num_ports,num_vcs);
    

    // Parse all the timing parameters
    std::string link_bw = params.find_string("link_bw");
    if ( link_bw == "" ) {
    }
    std::cout << "link_bw: " << link_bw << std::endl;

    TimeConverter* tc = Simulation::getSimulation()->getTimeLord()->getTimeConverter(link_bw);    
    
    // std::string link_bw = params.find_string("link_bw");
    // if ( link_bw == "" ) {
    // }
    // std::cout << "link_bw: " << link_bw << std::endl;

    // TimeConverter* tc = Simulation::getSimulation()->getTimeLord()->getTimeConverter(link_bw);    
    
    // Create all the PortControl blocks
    ports = new PortControl*[num_ports];

    
    int in_buf_sizes[num_vcs];
    int out_buf_sizes[num_vcs];
    
    for ( int i = 0; i < num_vcs; i++ ) {
	in_buf_sizes[i] = 100;
	out_buf_sizes[i] = 100;
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

	ports[i] = new PortControl(this, id, port_name.str(), i, tc, topo, num_vcs, in_buf_sizes, out_buf_sizes);
	
	std::cout << port_name.str() << std::endl;
	// links[i] = configureLink(port_name.str(), "1ns", new Event::Handler<hr_router,int>(this,&hr_router::port_handler,i));
    }

    registerClock( "1GHz", new Clock::Handler<hr_router>(this,&hr_router::clock_handler), false);
}

void
hr_router::dumpState(std::ostream& stream)
{
    stream << "Router id: " << id << std::endl;
    for ( int i = 0; i < num_ports; i++ ) {
	ports[i]->dumpState(stream);
	stream << "  Output_busy: " << out_port_busy[i] << std::endl;
	stream << "  Input_Busy: " <<  in_port_busy[i] << std::endl;
    }
    
}


bool
hr_router::clock_handler(Cycle_t cycle)
{
    // All we need to do is arbitrate the crossbar
    arb->arbitrate(ports,in_port_busy,out_port_busy,progress_vcs);

    // Move the events and decrement the busy values
    for ( int i = 0; i < num_ports; i++ ) {
	if ( progress_vcs[i] != -1 ) {
	    internal_router_event* ev = ports[i]->recv(progress_vcs[i]);
	    ports[ev->getNextPort()]->send(ev,ev->getVC());
	    // std::cout << "" << id << ": " << "Moving VC " << progress_vcs[i] <<
	    // 	" for port " << i << " to port " << ev->getNextPort() << std::endl;

	    if ( ev->getTraceType() == RtrEvent::FULL ) {
		std::cout << "TRACE(" << ev->getTraceID() << "): " << getCurrentSimTimeNano()
			  << " ns: Copying event (src = " << ev->getSrc() << ","
			  << " dest = "<< ev->getDest() << ") over crossbar in router " << id
			  << " (" << getName() << ")"
			  << " from port " << i << ", VC " << progress_vcs[i] 
			  << " to port " << ev->getNextPort() << ", VC " << ev->getVC()
			  << "." << std::endl;
	    }
	}
	
	// Should stop at zero, need to find a clean way to do this
	// with no branch.  For now it should work.
	in_port_busy[i]--;
	out_port_busy[i]--;
    }

    return false;
}

int
hr_router::Setup()
{
    for ( int i = 0; i < num_ports; i++ ) {
	ports[i]->Setup();
    }
    return 0;
}

