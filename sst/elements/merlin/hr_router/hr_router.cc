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

#include <sstream>

#include "hr_router.h"
#include "sst/elements/merlin/topology/torus.h"

port_buffer::port_buffer(int num_vcs)
{
    this->num_vcs = num_vcs;
    buffers = new port_queue_t[num_vcs];
    tokens = new int[num_vcs];
}

void port_buffer::setTokens(int vc, int num_tokens)
{
    if ( vc < 0 || vc >= num_vcs ) {
	std::cout << "ERROR: Invalid VC specified in call to port_buffer::setTokens()" << std::endl;
	abort();
    }
    tokens[vc] = num_tokens;
}

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

    // Create all the buffers
    input_bufs = new port_buffer*[num_ports];
    output_bufs = new port_buffer*[num_ports];

    for ( int i = 0; i < num_ports; i++ ) {
	input_bufs[i] = new port_buffer(num_vcs);
	output_bufs[i] = new port_buffer(num_vcs);

	// For now, just give all the buffers a depth of 10
	for ( int j = 0; j < num_vcs; j++ ) {
	    input_bufs[i]->setTokens(j,10);
	    output_bufs[i]->setTokens(j,10);
	}
    }


    // Register all the links to the ports
    links = new Link*[num_ports];
    
    for ( int i = 0; i < num_ports; i++ ) {
	std::stringstream port_name;
	port_name << "port";
	port_name << i;
	std::cout << port_name.str() << std::endl;
	links[i] = configureLink(port_name.str(), "1ns", new Event::Handler<hr_router,int>(this,&hr_router::port_handler,i));
    }

    // Get the topology
    std::string topology = params.find_string("topology");
    std::cout << "Topology: " << topology << std::endl;

    if ( !topology.compare("torus") ) {
	std::cout << "Creating new topology" << std::endl;
	topo = new topo_torus(params);
    }
    
}

void
hr_router::port_handler(Event* ev, int port)
{
}
