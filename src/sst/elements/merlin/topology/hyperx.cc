// Copyright 2009-2017 Sandia Corporation. Under the terms
// of Contract DE-NA0003525 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2017, Sandia Corporation
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
#include "hyperx.h"

#include <algorithm>
#include <stdlib.h>



using namespace SST::Merlin;


topo_hyperx::topo_hyperx(Component* comp, Params& params) :
    Topology(comp)
{

    // Get the various parameters
    router_id = params.find<int>("id",-1);
    if ( router_id == -1 ) {
    }

    std::string shape;
    shape = params.find<std::string>("hyperx:shape");
    if ( !shape.compare("") ) {
    }

    // Need to parse the shape string to get the number of dimensions
    // and the size of each dimension
    dimensions = std::count(shape.begin(),shape.end(),'x') + 1;

    dim_size = new int[dimensions];
    dim_width = new int[dimensions];
    port_start = new int[dimensions];

    parseDimString(shape, dim_size);

    std::string width = params.find<std::string>("hyperx:width", "");
    if ( width.compare("") == 0 ) {
        for ( int i = 0 ; i < dimensions ; i++ )
            dim_width[i] = 1;
    } else {
        parseDimString(width, dim_width);
    }

    int next_port = 0;
    for ( int d = 0 ; d < dimensions ; d++ ) {
        port_start[d] = next_port;
        next_port += dim_width[d] * (dim_size[d] - 1);
        // std::cout << next_port << std::endl;
    }
    // std::cout << std::endl;

    num_local_ports = params.find<int>("hyperx:local_ports", 1);
    local_port_start = next_port; // Local delivery is on the last ports
    output.output("Local port start = %d\n",local_port_start);
    // std::cout << local_port_start << std::endl;
    // std::cout << num_local_ports << std::endl;
    
    int n_ports = params.find<int>("num_ports",-1);
    if ( n_ports == -1 )
        output.fatal(CALL_INFO, -1, "Router must have 'num_ports' parameter set\n");
    // std::cout << n_ports << std::endl;

    int needed_ports = local_port_start + num_local_ports;
    // std::cout << needed_ports << std::endl;

    if ( n_ports < needed_ports ) {
        output.fatal(CALL_INFO, -1, "Number of ports should be at least %d for this configuration\n", needed_ports);
    }

    id_loc = new int[dimensions];
    idToLocation(router_id, id_loc);


    // Get the routing algorithm
    std::string route_algo = params.find<std::string>("hyperx:algorithm", "minimal");

    if ( !route_algo.compare("DOAL") ) {
        std::cout << "Setting algorithm to DOAL" << std::endl;
        algorithm = DOAL;
    }
    else if ( !route_algo.compare("VDAL") ) {
        algorithm = VDAL;
    }
    else {
        algorithm = MINIMAL;
    }

}

topo_hyperx::~topo_hyperx()
{
    delete [] id_loc;
    delete [] dim_size;
    delete [] dim_width;
    delete [] port_start;
}

void
topo_hyperx::route(int port, int vc, internal_router_event* ev)
{

    // Always assume minimal routing, reroute will adaptively route
    // when appropriate
    int dest_router = get_dest_router(ev->getDest());
    if ( dest_router == router_id ) {
        ev->setNextPort(get_dest_local_port(ev->getDest()));
    } else {
        topo_hyperx_event *tt_ev = static_cast<topo_hyperx_event*>(ev);

        for ( int dim = 0 ; dim < dimensions ; dim++ ) {
            if ( tt_ev->dest_loc[dim] != id_loc[dim] ) {

                // Get offset in the dimension
                int offset = tt_ev->dest_loc[dim] - ((tt_ev->dest_loc[dim] > id_loc[dim]) ? 1 : 0);
                offset *= dim_width[dim];
                
                int p = choose_multipath(
                    port_start[dim] + offset,
                    dim_width[dim]);
                
                tt_ev->setNextPort(p);

                // Set first non-aligned routing dimension for the
                // adaptive routing algorithms
                tt_ev->routing_dim = dim;
                break;
            }
        }
    }
}

void topo_hyperx::reroute(int port, int vc, internal_router_event* ev)
{
    static int count = 0;
    if ( algorithm == MINIMAL ) return;

    if ( algorithm == DOAL ) {
        // We still have to go in dimension order, but we can
        // adaptively route once in each dimension.
        int dest_router = get_dest_router(ev->getDest());
        if ( dest_router == router_id ) {
            ev->setNextPort(get_dest_local_port(ev->getDest()));
        }
        else {
            topo_hyperx_event *tt_ev = static_cast<topo_hyperx_event*>(ev);

            // output.output("(%d,%d): Input port = %d. Routing packet to router (%d,%d)\n",id_loc[0],id_loc[1],port,tt_ev->dest_loc[0],tt_ev->dest_loc[1]);
            
            for ( int dim = 0 ; dim < dimensions ; dim++ ) {
                if ( tt_ev->dest_loc[dim] != id_loc[dim] ) {
                    // output.output("  Routing in dimension: %d\n",dim);
                    // Found the dimension to route in.  See if we
                    // have already adaptively routed, if so, then we
                    // have to go direct for this dimension
                    if ( (vc & 0x1) == 1 ) {
                        // Get offset in the dimension
                        int offset = tt_ev->dest_loc[dim] - ((tt_ev->dest_loc[dim] > id_loc[dim]) ? 1 : 0);
                        offset *= dim_width[dim];
                        
                        int p = choose_multipath(
                            port_start[dim] + offset,
                            dim_width[dim]);

                        // output.output("    Just came in from adaptive route.  Routing out port: %d, vc: %d\n",p,vc-1);
                        tt_ev->setNextPort(p);
                        tt_ev->setVC(vc - 1);
                        
                        // Set first non-aligned routing dimension for the
                        // adaptive routing algorithms
                        tt_ev->routing_dim = dim;
                        break;
                    }
                    else {
                        // Just entered this dimension, we can
                        // adaptively route.  Need to find out which
                        // link is best to take.  Weight all
                        // non-minimal links by multiplying by 2 and
                        // keep the port with the lowest value
                        int min_port = 0;
                        int min_weight = 0x7fffffff;
                        int min_vc = vc;
                        // output.output("    Starting new dimension\n");
                        for ( int curr_port = port_start[dim]; curr_port < port_start[dim] + ((dim_size[dim] - 1) * dim_width[dim]); ++curr_port  ) {
                            // See if this is a minimal route
                            int offset = tt_ev->dest_loc[dim] - ((tt_ev->dest_loc[dim] > id_loc[dim]) ? 1 : 0);
                            offset = port_start[dim] + ( offset * dim_width[dim]);
                            if ( curr_port >= offset && curr_port < offset + dim_width[dim] ) {
                                // This is a minimal route.  We would
                                // use VC 0 in the VN, which is the VC
                                // the packet came in on
                                int weight = output_credits[curr_port * num_vcs + vc];
                                // output.output("      Output port %d is a minimal route and has a weight of %d\n",curr_port,weight);
                                if ( weight < min_weight ) {
                                    min_weight = weight;
                                    min_port = curr_port;
                                    // output.output("        Found a new minimum weight: min_weight = %d, min_port = %d, min_vc = %d\n",
                                    //               min_weight,min_port,min_vc);
                                }
                            }
                            else {
                                // This is a non-minimal route.  We
                                // would use VC 1 in the VN, which is
                                // one greater than the VC the packet
                                // came in on
                                int weight = 2 * output_credits[curr_port * num_vcs + vc + 1];
                                // output.output("      Output port %d is a non-minimal route and has a weight of %d\n",curr_port,weight);
                                if ( weight < min_weight ) {
                                    min_weight = weight;
                                    min_port = curr_port;
                                    min_vc = vc + 1;
                                    // output.output("        Found a new minimum weight: min_weight = %d, min_port = %d, min_pc = %d\n",
                                    //               min_weight,min_port,min_vc);
                                }
                            }
                        }
                        // Route on the minimally weighted port
                        tt_ev->setNextPort(min_port);
                        tt_ev->setVC(min_vc);
                        // output.output("    Starting new dimension.  Routing out port: %d, vc: %d\n",min_port,min_vc);
                        break;
                    }
                }
            }
        }
    }

    else if ( algorithm == VDAL ) {
    }
    
    // Look for opportunities to adaptively route

    // We will look at all the ports in unaligned dimensions and take
    // the least congested
    

    return;
}

internal_router_event*
topo_hyperx::process_input(RtrEvent* ev)
{
    topo_hyperx_event* tt_ev = new topo_hyperx_event(dimensions);
    tt_ev->setEncapsulatedEvent(ev);
    tt_ev->setVC(2*ev->request->vn);
    
    // Need to figure out what the hyperx address is for easier
    // routing.
    int run_id = get_dest_router(tt_ev->getDest());
    idToLocation(run_id, tt_ev->dest_loc);

	return tt_ev;
}


void topo_hyperx::routeInitData(int port, internal_router_event* ev, std::vector<int> &outPorts)
{
    // TraceFunction trace(CALL_INFO);
    // topo_hyperx_init_event *tt_ev = static_cast<topo_hyperx_init_event*>(ev);
    if ( ev->getDest() == INIT_BROADCAST_ADDR ) {
        // trace.getOutput().output("  broadcast\n");
        // trace.getOutput().output("    router index = %d,%d,%d\n",id_loc[0],id_loc[1],id_loc[2]);
        // trace.getOutput().output("    input port = %d\n",port);
        // Figure out what dimension it came in on.  Next to forward
        // to all higher dimensions and to hosts
        int start_dim = 0;
        if ( port < local_port_start ) {
            start_dim = dimensions;
            // Came in on port other than host port.  Figure out dimension
            for ( int i = dimensions - 1; i >= 1; --i ) {
                if ( port < port_start[i] ) start_dim = i;
                else break;
            }
        }
        // trace.getOutput().output("      start_dim = %d\n",start_dim);
        
        // Need to send in all the higher dimensions and to local
        // ports
        for ( int i = 0; i < num_local_ports; ++i ) {
            if ( port != local_port_start + i ) {
                // trace.getOutput().output("    output port = %d\n",local_port_start + i);
                outPorts.push_back(local_port_start + i);
            }
        }

        for ( int i = start_dim; i < dimensions; ++i ) {
            for ( int j = 0; j < dim_size[i] - 1; ++j ) {
                // trace.getOutput().output("    output port = %d\n",port_start[i] + (j * dim_width[i]));
                outPorts.push_back(port_start[i] + (j * dim_width[i]));
            }
        }
    }
    else {
        route(port, 0, ev);
        outPorts.push_back(ev->getNextPort());        
    }
    
    // // Also, send to hosts
    // for ( int p = 0 ; p < num_local_ports ; p++ ) {
    //     if ( (local_port_start + p) != port ) {
    //         outPorts.push_back(local_port_start +p);
    //     }
    // }
}


internal_router_event* topo_hyperx::process_InitData_input(RtrEvent* ev)
{
    topo_hyperx_init_event* tt_ev = new topo_hyperx_init_event(dimensions);
    tt_ev->setEncapsulatedEvent(ev);
    tt_ev->setVC(2*ev->request->vn);
    if ( tt_ev->getDest() != INIT_BROADCAST_ADDR ) {
        int rtr_id = get_dest_router(tt_ev->getDest());
        idToLocation(rtr_id, tt_ev->dest_loc);
    }
    return tt_ev;
}


Topology::PortState
topo_hyperx::getPortState(int port) const
{
    if (port >= local_port_start) {
        if ( port < (local_port_start + num_local_ports) )
            return R2N;
        return UNCONNECTED;
    }

    return R2R;
}


void
topo_hyperx::idToLocation(int run_id, int *location) const
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
topo_hyperx::parseDimString(const std::string &shape, int *output) const
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
topo_hyperx::get_dest_router(int dest_id) const
{
    return dest_id / num_local_ports;
}

int
topo_hyperx::get_dest_local_port(int dest_id) const
{
    return local_port_start + (dest_id % num_local_ports);
}


int
topo_hyperx::choose_multipath(int start_port, int num_ports)
{
    // For now, just use first link, ie no multipath
    return start_port;
    // if ( num_ports == 1 ) {
    //     return start_port;
    // } else {
    //     return start_port + (dest_dist % num_ports);
    // }
}

int
topo_hyperx::computeNumVCs(int vns)
{
    return 2*vns;
}

int
topo_hyperx::getEndpointID(int port)
{
    if ( !isHostPort(port) ) return -1;
    return (router_id * num_local_ports) + (port - local_port_start);
}

void
topo_hyperx::setOutputBufferCreditArray(int const* array, int vcs)
{
    output_credits = array;
    num_vcs = vcs;
}
