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

#include <stdlib.h>

#include <algorithm>

#include "torus.h"

topo_torus::topo_torus(Params& params) :
    Topology()
{

    // Get the various parameters
    id = params.find_integer("id");
    if ( id == -1 ) {
    }

    std::string shape;
    shape = params.find_string("torus:shape");
    if ( !shape.compare("") ) {
    }

    // Need to parse the shape string to get the number of dimensions
    // and the size of each dimension
    dimensions = std::count(shape.begin(),shape.end(),'x') + 1;

    std::cout << "  dimensions: " << dimensions << std::endl;

    dim_size = new int[dimensions];

    size_t start = 0;
    size_t end = 0;
    for ( int i = 0; i < dimensions; i++ ) {
	end = shape.find('x',start);
	size_t length = end - start;
	std::string sub = shape.substr(start,length);
	std::cout << "Substring: " << sub << std::endl;
	dim_size[i] = strtol(sub.c_str(), NULL, 0);
	start = end + 1;
	std::cout << "    dim_size[" << i << "] = " << dim_size[i] << std::endl;
    }
}

topo_torus::~topo_torus()
{
    delete[] dim_size;
}


void
topo_torus::route(int port, int vc, internal_router_event* ev)
{
    // Just for a quick test.
    if ( ev->getDest() == id ) ev->setNextPort(2);
    else ev->setNextPort(0);
}

internal_router_event*
topo_torus::process_input(RtrEvent* ev)
{
    topo_torus_event* tt_ev = new topo_torus_event(dimensions);
    tt_ev->setEncapsulatedEvent(ev);
    
    int run_id = 53;
    // Need to figure out what the torus address is for easier
    // routing.
    for ( int i = dimensions - 1; i > 0; i-- ) {
	int div = 1;
	for ( int j = 0; j < i; j++ ) {
	    div *= dim_size[j];
	}
	int value = (run_id / div);
	tt_ev->dest_loc[i] = value;
	std::cout << "dim[" << i << "] = " << value << std::endl;
	run_id -= (value * div);
    }
    tt_ev->dest_loc[0] = run_id;
    // std::cout << "dim[0] = " << run_id << std::endl;
    
    return tt_ev;
}


bool
topo_torus::isHostPort(int port)
{
    if (port == 2*dimensions) return true;
    return false;
}
