// Copyright 2009-2018 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2018, NTESS
// All rights reserved.
// 
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.
#include <sst_config.h>
#include "torus.h"

#include <algorithm>
#include <stdlib.h>



using namespace SST::Merlin;


topo_torus::topo_torus(Component* comp, Params& params) :
    Topology(comp)
{

    // Get the various parameters
    router_id = params.find<int>("id",-1);
    if ( router_id == -1 ) {
    }

    std::string shape;
    shape = params.find<std::string>("torus:shape");
    if ( !shape.compare("") ) {
    }

    // Need to parse the shape string to get the number of dimensions
    // and the size of each dimension
    dimensions = std::count(shape.begin(),shape.end(),'x') + 1;

    dim_size = new int[dimensions];
    dim_width = new int[dimensions];
    port_start = new int[dimensions][2];

    parseDimString(shape, dim_size);

    std::string width = params.find<std::string>("torus:width", "");
    if ( width.compare("") == 0 ) {
        for ( int i = 0 ; i < dimensions ; i++ )
            dim_width[i] = 1;
    } else {
        parseDimString(width, dim_width);
    }

    int next_port = 0;
    for ( int d = 0 ; d < dimensions ; d++ ) {
        for ( int i = 0 ; i < 2 ; i++ ) {
            port_start[d][i] = next_port;
            next_port += dim_width[d];
        }
    }

    num_local_ports = params.find<int>("torus:local_ports", 1);

    // int n_vc = params.find<int>("num_vcs");
    // if ( n_vc < 2 || (n_vc & 1) ) {
    //     output.fatal(CALL_INFO, -1, "Number of VC's must be a multiple of two for a torus\n");
    // }

    int n_ports = params.find<int>("num_ports",-1);
    if ( n_ports == -1 )
        output.fatal(CALL_INFO, -1, "Router must have 'num_ports' parameter set\n");

    int needed_ports = 0;
    for ( int i = 0 ; i < dimensions ; i++ ) {
        needed_ports += 2 * dim_width[i];
    }


    if ( n_ports < (needed_ports+num_local_ports) ) {
        output.fatal(CALL_INFO, -1, "Number of ports should be %d for this configuration\n", needed_ports+num_local_ports);
    }

    local_port_start = needed_ports;// Local delivery is on the last ports

    id_loc = new int[dimensions];
    idToLocation(router_id, id_loc);
}

topo_torus::~topo_torus()
{
    delete [] id_loc;
    delete [] dim_size;
    delete [] dim_width;
    delete [] port_start;
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


                output.verbose(CALL_INFO, 1, 1, " %d to %d:  Dist Neg: %d, Dist Pos: %d\n",
                        id_loc[dim], tt_ev->dest_loc[dim], dist_neg, dist_pos);

                int p = choose_multipath(
                        port_start[dim][(go_pos) ? 0 : 1],
                        dim_width[dim],
                        (go_pos)? dist_pos : dist_neg);

                tt_ev->setNextPort(p);

                if ( id_loc[dim] == 0 && port < local_port_start ) { // Crossing dateline
                    int new_vc = vc ^ 1;
                    tt_ev->setVC(new_vc); // Toggle VC
                    output.verbose(CALL_INFO, 1, 1, "Crossing dateline.  Changing from VC %d to %d\n", vc, new_vc);
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
    tt_ev->setVC(ev->request->vn * 2);
    
    // Need to figure out what the torus address is for easier
    // routing.
    int run_id = get_dest_router(tt_ev->getDest());
    idToLocation(run_id, tt_ev->dest_loc);

	return tt_ev;
}


void topo_torus::routeInitData(int port, internal_router_event* ev, std::vector<int> &outPorts)
{
    if ( ev->getDest() == INIT_BROADCAST_ADDR ) {
        /* For broadcast, use dest_loc as src_loc */
        topo_torus_event *tt_ev = static_cast<topo_torus_event*>(ev);
        /*
         * Find dimension came in on
         * Send in positive direction in all dimensions that level and higher (unless at end)
         */
        int inc_dim = 0;
        for ( ; inc_dim < dimensions ; inc_dim++ ) {
            if ( port == port_start[inc_dim][1] ) {
                break;
            }
        }
        if (inc_dim >= dimensions) inc_dim = 0; // A new message

        for ( int dim = inc_dim ; dim < dimensions ; dim++ ) {
            if ( ((id_loc[dim] + 1) % dim_size[dim]) != tt_ev->dest_loc[dim] ) {
                outPorts.push_back(port_start[dim][0]);
            }
        }

        // Also, send to hosts
        for ( int p = 0 ; p < num_local_ports ; p++ ) {
            if ( (local_port_start + p) != port ) {
                outPorts.push_back(local_port_start +p);
            }
        }


    } else {
        route(port, 0, ev);
        outPorts.push_back(ev->getNextPort());
    }
}


internal_router_event* topo_torus::process_InitData_input(RtrEvent* ev)
{
    topo_torus_event* tt_ev = new topo_torus_event(dimensions);
    tt_ev->setEncapsulatedEvent(ev);
    tt_ev->setVC(ev->request->vn * 2);
    if ( tt_ev->getDest() == INIT_BROADCAST_ADDR ) {
        /* For broadcast, use dest_loc as src_loc */
        for ( int i = 0 ; i < dimensions ; i++ ) {
            tt_ev->dest_loc[i] = id_loc[i];
        }
    } else {
        int rtr_id = get_dest_router(tt_ev->getDest());
        idToLocation(rtr_id, tt_ev->dest_loc);
    }
    return tt_ev;
}


Topology::PortState
topo_torus::getPortState(int port) const
{
    if (port >= local_port_start) {
        if ( port < (local_port_start + num_local_ports) )
            return R2N;
        return UNCONNECTED;
    }
    return R2R;
}


void
topo_torus::idToLocation(int run_id, int *location) const
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
topo_torus::parseDimString(const std::string &shape, int *output) const
{
    size_t start = 0;
    size_t end = 0;
    for ( int i = 0; i < dimensions; i++ ) {
        end = shape.find('x',start);
        size_t length = end - start;
        std::string sub = shape.substr(start,length);
        output[i] = strtol(sub.c_str(), NULL, 0);
        start = end + 1;
    }
}


int
topo_torus::get_dest_router(int dest_id) const
{
    return dest_id / num_local_ports;
}

int
topo_torus::get_dest_local_port(int dest_id) const
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

int
topo_torus::computeNumVCs(int vns)
{
    return 2*vns;
}

int
topo_torus::getEndpointID(int port)
{
    if ( !isHostPort(port) ) return -1;
    return (router_id * num_local_ports) + (port - local_port_start);
}


