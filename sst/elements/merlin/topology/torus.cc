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


#define DPRINTF( fmt, args...) __DBG( DBG_NETWORK, topo_torus, fmt, ## args )

topo_torus::topo_torus(Params& params) :
    Topology()
{

    // Get the various parameters
    router_id = params.find_integer("id");
    if ( router_id == -1 ) {
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
    dim_width = new int[dimensions];
    port_start = new int[dimensions][2];

    parseDimString(shape, dim_size);

    std::string width = params.find_string("torus:width", "");
    if ( width.compare("") == 0 ) {
        for ( int i = 0 ; i < dimensions ; i++ )
            dim_width[i] = 1;
    } else {
        parseDimString(width, dim_width);
    }
    for ( int i = 0 ; i < dimensions ; i++ ) {
        std::cout << "dim[" << i << "] = " << dim_size[i] << " x " << dim_width[i] << std::endl;
    }

    int next_port = 0;
    for ( int d = 0 ; d < dimensions ; d++ ) {
        for ( int i = 0 ; i < 2 ; i++ ) {
            port_start[d][i] = next_port;
            next_port += dim_width[d];
        }
    }

    num_local_ports = params.find_integer("torus:local_ports", 1);

    int n_vc = params.find_integer("num_vcs");
    if ( n_vc < 2 || (n_vc & 1) ) {
        _abort(topo_torus, "Number of VC's must be a multiple of two for a torus\n");
    }

    int n_ports = params.find_integer("num_ports");
    if ( n_ports == -1 )
        _abort(Topology, "Router must have 'num_ports' parameter set\n");

    int needed_ports = num_local_ports;
    for ( int i = 0 ; i < dimensions ; i++ ) {
        needed_ports += 2 * dim_width[i];
    }


    if ( n_ports != needed_ports ) {
        _abort(Topology, "Number of ports should be %d for this configuration\n", needed_ports);
    }

    local_port_start = n_ports-num_local_ports;// Local delivery is on the last ports

	id_loc = new int[dimensions];
	idToLocation(router_id, id_loc);
    std::cout << "Coordinates:\t";
    for ( int i = 0 ; i < dimensions ; i++ ) {
        std::cout << id_loc[i] << "\t";
    }
    std::cout << std::endl;
}

topo_torus::~topo_torus()
{
	delete[] id_loc;
    delete[] dim_size;
    delete[] dim_width;
}

void
topo_torus::route(int port, int vc, internal_router_event* ev)
{
    int dest_router = get_dest_router(ev->getDest());
    if ( dest_router == router_id ) {
        ev->setNextPort(get_dest_local_port(ev->getDest()));
    } else {
        topo_torus_event *tt_ev = static_cast<topo_torus_event*>(ev);

        for ( int dim = tt_ev->routing_dim ; dim < dimensions ; dim++ ) {
            if ( tt_ev->dest_loc[dim] != id_loc[dim] ) {

                int dist_neg = id_loc[dim] - tt_ev->dest_loc[dim];
                if ( dist_neg < 0 ) dist_neg += dim_size[dim];
                int dist_pos = tt_ev->dest_loc[dim] - id_loc[dim];
                if ( dist_pos < 0 ) dist_pos += dim_size[dim];

                int go_pos = (dist_pos <= dist_neg);


                DPRINTF(" %d to %d:  Dist Neg: %d, Dist Pos: %d\n",
                        id_loc[dim], tt_ev->dest_loc[dim], dist_neg, dist_pos);

                int p = choose_multipath(
                        port_start[dim][(go_pos) ? 0 : 1],
                        dim_width[dim],
                        (go_pos)? dist_pos : dist_neg);

                tt_ev->setNextPort(p);

                if ( id_loc[dim] == 0 && port < local_port_start ) { // Crossing dateline
                    int new_vc = vc ^ 1;
                    tt_ev->setVC(new_vc); // Toggle VC
                    DPRINTF("Crossing dateline.  Changing from VC %d to %d\n", vc, new_vc);
                }

                break;

            } else {
                // Time to change direction
                tt_ev->routing_dim++;
                tt_ev->setVC(vc & (~1)); // Reset the VC
            }
        }
    }
}



internal_router_event*
topo_torus::process_input(RtrEvent* ev)
{
    topo_torus_event* tt_ev = new topo_torus_event(dimensions);
    tt_ev->setEncapsulatedEvent(ev);

    // Need to figure out what the torus address is for easier
    // routing.
    int run_id = get_dest_router(tt_ev->getDest());
    idToLocation(run_id, tt_ev->dest_loc);

	return tt_ev;
}


bool
topo_torus::isHostPort(int port)
{
    if (port >= local_port_start) return true;
    return false;
}


void
topo_torus::idToLocation(int run_id, int *location)
{
	for ( int i = dimensions - 1; i > 0; i-- ) {
		int div = 1;
		for ( int j = 0; j < i; j++ ) {
			div *= dim_size[j];
		}
		int value = (run_id / div);
		location[i] = value;
		run_id -= (value * div);
	}
	location[0] = run_id;
}

void
topo_torus::parseDimString(std::string &shape, int *output)
{
    size_t start = 0;
    size_t end = 0;
    for ( int i = 0; i < dimensions; i++ ) {
        end = shape.find('x',start);
        size_t length = end - start;
        std::string sub = shape.substr(start,length);
        //std::cout << "Substring: " << sub << std::endl;
        output[i] = strtol(sub.c_str(), NULL, 0);
        start = end + 1;
        //std::cout << "    dim_size[" << i << "] = " << dim_size[i] << std::endl;
    }
}


int
topo_torus::get_dest_router(int dest_id)
{
    return dest_id / num_local_ports;
}

int
topo_torus::get_dest_local_port(int dest_id)
{
    return local_port_start + (dest_id % num_local_ports);
}


int
topo_torus::choose_multipath(int start_port, int num_ports, int dest_dist)
{
    if ( num_ports == 1 ) {
        return start_port;
    } else {
        return start_port + (dest_dist % num_ports);
    }
}
