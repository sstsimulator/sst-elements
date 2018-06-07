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
#include "hyperx.h"

#include "sst/core/rng/xorshift.h"

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
    // output.output("Local port start = %d\n",local_port_start);
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
    std::string route_algo = params.find<std::string>("hyperx:algorithm", "DOR");

    if ( !route_algo.compare("DOAL") ) {
        // std::cout << "Setting algorithm to DOAL" << std::endl;
        algorithm = DOAL;
    }
    else if ( !route_algo.compare("valiant") ) {
        algorithm = VALIANT;
    }
    else if ( !route_algo.compare("VDAL") ) {
        algorithm = VDAL;
    }
    else if ( !route_algo.compare("DOR-ND") ) {
        algorithm = DORND;
    }
    else if ( !route_algo.compare("DOR") ) {
        algorithm = DOR;
    }
    else if ( !route_algo.compare("MIN-A") ) {
        algorithm = MINA;
    }
    else {
        output.fatal(CALL_INFO,-1,"Unknown routing mode specified: %s\n",route_algo.c_str());
    }

    rng = new RNG::XORShiftRNG(router_id+1);
    rng_func = new RNGFunc(rng);
    
    total_routers = 1;
    for (int i = 0; i < dimensions; ++i ) {
        total_routers *= dim_size[i];
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

    topo_hyperx_event *tt_ev = static_cast<topo_hyperx_event*>(ev);
    tt_ev->rerouted = false;

    routeDOR(port,vc,tt_ev);    
}

void topo_hyperx::reroute(int port, int vc, internal_router_event* ev)
{

    topo_hyperx_event* tt_ev = static_cast<topo_hyperx_event*>(ev);
    if ( tt_ev->rerouted ) return;
    tt_ev->rerouted = true;
    
    if ( algorithm == DOR ) {
        return;
    }

    if ( algorithm == DORND ) {
        return routeDORND(port,vc,tt_ev);
    }

    if ( algorithm == MINA ) {
        return routeMINA(port,vc,tt_ev);
    }

    else if ( algorithm == VALIANT ) {
        return routeValiant(port,vc,tt_ev);
    }

    else if ( algorithm == DOAL ) {
        return routeDOAL(port,vc,tt_ev);
    }

    else if ( algorithm == VDAL ) {
        return routeVDAL(port,vc,tt_ev);
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
    tt_ev->setVC(num_vcs * ev->request->vn);
    if ( algorithm == VALIANT ) {
        int mid;
        do {
            mid = rng->generateNextUInt32() % total_routers;
            // idToLocation(mid, tt_ev->val_loc);
        // } while ( tt_ev->val_loc[0] == tt_ev->dest_loc[0] || tt_ev->val_loc[1] == tt_ev->dest_loc[1] );
        } while ( mid == router_id );

        idToLocation(mid, tt_ev->val_loc);
        tt_ev->val_route_dest = false;
    }
    
    // Need to figure out what the hyperx address is for easier
    // routing.
    int rtr_id = get_dest_router(tt_ev->getDest());
    idToLocation(rtr_id, tt_ev->dest_loc);

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

// rtr_id is a router id
void
topo_hyperx::idToLocation(int rtr_id, int *location) const
{
	for ( int i = dimensions - 1; i > 0; i-- ) {
		int div = 1;
		for ( int j = 0; j < i; j++ ) {
			div *= dim_size[j];
		}
		int value = (rtr_id / div);
		location[i] = value;
		rtr_id -= (value * div);
	}
	location[0] = rtr_id;
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


// dest_id is a host id
int
topo_hyperx::get_dest_router(int dest_id) const
{
    return dest_id / num_local_ports;
}

// dest_id is a host id
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
    switch ( algorithm ) {
    case VDAL:
        return 2 * dimensions * vns;
    case MINA:
        return dimensions;
    case DOR:
    case DORND:
        return 1;
    default:
        return 2; 
    }
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

void
topo_hyperx::setOutputQueueLengthsArray(int const* array, int vcs)
{
    output_queue_lengths = array;
    num_vcs = vcs;
}


// Routing algorithms

// This will return the first port for the correct next router.
// Multipath configurations will need to chose the multipath based on
// the result.  ret.first is the dimension of the port, ret.second is
// the first port as described above.  Will return -1 in ret.first if
// destination is same as router.
std::pair<int,int>
topo_hyperx::routeDORBase(int* dest_loc) {
    // Will ignore VCs and just tell you the next port to for minimal
    // dimension order routing to dest_loc

    for ( int dim = 0 ; dim < dimensions ; ++dim ) {
        // Find first unaligned dimension and route to align it
        if ( dest_loc[dim] != id_loc[dim] ) {
            // Get offset to the first unaligned dimension
            int offset = dest_loc[dim] - ((dest_loc[dim] > id_loc[dim]) ? 1 : 0);
            offset *= dim_width[dim];
            return std::make_pair(dim,port_start[dim] + offset);
        }
    }
    return std::make_pair(-1,-1);
}

void
topo_hyperx::routeDOR(int port, int vc, topo_hyperx_event* ev) {
    std::pair<int,int> next_port = routeDORBase(ev->dest_loc);

    if ( next_port.first == -1 ) {
        ev->setNextPort(get_dest_local_port(ev->getDest()));
        ev->setVC(vc);
        return;
    }
    int p = choose_multipath(next_port.second,dim_width[next_port.first]);
    ev->setNextPort(p);
    ev->setVC(vc);
    return;
    
}

void
topo_hyperx::routeDORND(int port, int vc, topo_hyperx_event* ev) {
    std::pair<int,int> next_port = routeDORBase(ev->dest_loc);

    if ( next_port.first == -1 ) {
        ev->setNextPort(get_dest_local_port(ev->getDest()));
        ev->setVC(vc);
        return;
    }

    // Choose the least loaded route to the next router
    int min = 0x7FFFFFFF;
    int min_port;

    for ( int p = next_port.second; p < next_port.second + dim_width[next_port.first]; ++p ) {
        int weight = output_queue_lengths[p * num_vcs + vc];
        if ( weight < min ) {
            min = weight;
            min_port = p;
        }
    }
    ev->setNextPort(min_port);
    ev->setVC(vc);
    return;
    
}

void
topo_hyperx::routeValiant(int port, int vc, topo_hyperx_event* ev) {
    // first we do a minimal route to the valiant router, then do
    // minimal route to dest.
    int next_vc = vc;
    if ( !ev->val_route_dest ) {
        // Still headed toward valiant mid-point
        std::pair<int,int> next_port = routeDORBase(ev->val_loc);
        if ( next_port.first == -1 ) {
            // Made it to valiant midpoint
            ev->val_route_dest = true;
            next_vc = vc + 1;
        }
        else {
            int p = choose_multipath(next_port.second,dim_width[next_port.first]);
            ev->setNextPort(p);
            ev->setVC(next_vc);
            return;
        }
    }

    // Made it to the valiant route (or the function has already
    // returned), so just route minimally to dest
    std::pair<int,int> next_port = routeDORBase(ev->dest_loc);
    if ( next_port.first == -1 ) {
        ev->setNextPort(get_dest_local_port(ev->getDest()));
        ev->setVC(vc);
        return;
    }
    int p = choose_multipath(next_port.second,dim_width[next_port.first]);
    ev->setNextPort(p);
    ev->setVC(next_vc);
    return;
}

void
topo_hyperx::routeDOAL(int port, int vc, topo_hyperx_event* ev) {
    // We still have to go in dimension order, but we can adaptively
    // route once in each dimension.
    int dest_router = get_dest_router(ev->getDest());
    if ( dest_router == router_id ) {
        ev->setNextPort(get_dest_local_port(ev->getDest()));
    }
    else {
        // topo_hyperx_event *tt_ev = static_cast<topo_hyperx_event*>(ev);
        
        for ( int dim = 0 ; dim < dimensions ; dim++ ) {
            if ( ev->dest_loc[dim] == id_loc[dim] ) continue;

            // output.output(" Routing in dimension: %d\n",dim);

            //Found the dimension to route in.  See if we have already
            // adaptively routed, if so, then we have to go direct for
            // this dimension
            if ( (vc & 0x1) == 1 ) {
                // Get offset in the dimension
                int offset = ev->dest_loc[dim] - ((ev->dest_loc[dim] > id_loc[dim]) ? 1 : 0);
                offset *= dim_width[dim];
                
                // Choose the least loaded route to the next router
                int min = 0x7FFFFFFF;
                int min_port;
                
                for ( int p = port_start[dim] + offset; p < port_start[dim] + offset + dim_width[dim]; ++p ) {
                    int weight = output_queue_lengths[p * num_vcs + vc];
                    if ( weight < min ) {
                        min = weight;
                        min_port = p;
                    }
                }

                ev->setNextPort(min_port);
                ev->setVC(vc - 1);

                ev->last_routing_dim = dim;
                break;
            }
            else {
                // Just entered this dimension, we can adaptively
                // route.  Need to find out which link is best to
                // take.  Weight all non-minimal links by multiplying
                // by 2 and keep the port with the lowest value
                int min_port = 0;
                int min_weight = 0x7fffffff;
                int min_vc = vc;
                for ( int curr_port = port_start[dim]; curr_port < port_start[dim] + ((dim_size[dim] - 1) * dim_width[dim]); ++curr_port  ) {
                    // See if this is a minimal route
                    
                    // Compute the base offset in this dimension for
                    // the minimal route (this is essentially what the
                    // offset would be if the width in this dimension
                    // is 1).
                    int offset = ev->dest_loc[dim] - ((ev->dest_loc[dim] > id_loc[dim]) ? 1 : 0);
                    
                    // Now get the actual starting port for the minimal link(s)
                    offset = port_start[dim] + ( offset * dim_width[dim]);
                    if ( curr_port >= offset && curr_port < offset + dim_width[dim] ) {
                        // This is a minimal route.  We would use VC 0
                        // in the VN, which is the VC the packet came
                        // in on
                        int weight = output_queue_lengths[curr_port * num_vcs + vc];
                        if ( weight < min_weight ) {
                            min_weight = weight;
                            min_port = curr_port;
                            min_vc = vc;
                        }
                    }
                    else {
                        // This is a non-minimal route.  We would use
                        // VC 1 in the VN, which is one greater than
                        // the VC the packet came in on
                        int weight = 2 * output_queue_lengths[curr_port * num_vcs + vc + 1] + 1;
                        if ( weight < min_weight ) {
                            min_weight = weight;
                            min_port = curr_port;
                            min_vc = vc + 1;
                        }
                    }
                }
                // Route on the minimally weighted port
                ev->setNextPort(min_port);
                ev->setVC(min_vc);
                // output.output("    Starting new dimension.  Routing out port: %d, vc: %d\n",min_port,min_vc);
                break;
            }
        }
    }
}


void
topo_hyperx::routeMINA(int port, int vc, topo_hyperx_event* ev) {

    // Check to see if we made it to the dest router
    int dest_router = get_dest_router(ev->getDest());
    if ( dest_router == router_id ) {
        ev->setNextPort(get_dest_local_port(ev->getDest()));
        return;
    }

    // We can route in any unaligned dimension, but we have to take
    // only minimal routes

    // If this is just coming into the network from and endpoint, we
    // need to set the vc to -1 in order for the logic below to work
    int start_vc = port >= local_port_start ? -1 : vc;

    int min_weight = 0x7fffffff;;
    int min_port = -1;
    for ( int dim = 0; dim < dimensions; ++dim ) {
        if ( ev->dest_loc[dim] == id_loc[dim] ) continue;

        // Find the minimum weight, minimally-routed port
        int offset = ev->dest_loc[dim] - ((ev->dest_loc[dim] > id_loc[dim]) ? 1 : 0);
        offset = port_start[dim] + (offset * dim_width[dim]);

        for ( int i = offset; i < offset + dim_width[dim]; ++i ) {
            int weight = output_queue_lengths[(i * num_vcs) + start_vc + 1];
            if ( weight < min_weight ) {
                min_port = i;
                min_weight = weight;
            }
        }
    }
    // Route on the minimally weighted port
    ev->setNextPort(min_port);
    ev->setVC(start_vc + 1);

}


void
topo_hyperx::routeVDAL(int port, int vc, topo_hyperx_event* ev) {
    // TraceFunction trace(CALL_INFO);
    // trace.getOutput().output("(%d,%d) : (%d,%d)\n",id_loc[0],id_loc[1],ev->dest_loc[0],ev->dest_loc[1]);
    // Check to see if we made it to the dest router
    int dest_router = get_dest_router(ev->getDest());
    if ( dest_router == router_id ) {
        ev->setNextPort(get_dest_local_port(ev->getDest()));
        // trace.getOutput().output("Made it to dest router\n");
        return;
    }

    // Not there yet, need to figure out what dimensions we can
    // route in (we will not route in an aligned dimension)

    // Get the unaligned dimensions
    std::vector<int> udims;
    ev->getUnalignedDimensions(id_loc,udims);


    // If this is just coming into the network from and endpoint, we
    // need to set the vc to -1 in order for the logic below to work
    int start_vc = port >= local_port_start ? -1 : vc;


    // trace.getOutput().output("%llu: udims.size = %lu, remaining_vcs = %d\n",ev->id.first,udims.size(),num_vcs - start_vc - 1 );
    // Check to see if there are extra VCs for misroutes.  If not,
    // simply fall back to MIN-A routing
    if ( udims.size() == num_vcs - start_vc - 1 ) {
        // trace.getOutput().output("Falling back to MIN-A, udims.size = %lu, remaining_vcs = %d\n",udims.size(),num_vcs - start_vc - 1 );
        return routeMINA(port,vc,ev);
    }
    
    // We'll look across all possible routes and take the minimum
    // weight route.  There are two constraints: First, we don't route
    // in an aligned dimension.  Second, we can't take two non-minimal
    // routes in the same dimension in a row

    int min_weight = 0x7fffffff;
    std::vector<int> min_ports;
    int next_vc = start_vc + 1;

    for (int dim : udims ) {
        // trace.getOutput().output("looking at dimension %d\n",dim);
        // Within each dimension, look at all the possible routes

        int offset = 0;
        for ( int router = 0; router < dim_size[dim]; ++router ) {
            // If this is my location in the dimension, skip it
            if ( router == id_loc[dim] ) continue;

            // Check to see if these are minimally-routed links
            bool minimal = router == ev->dest_loc[dim];

            // Check to see if this is the same dimension we routed in
            // last time.  If so, then we only look at minimal routes
            // and will skip non-minimal routes
            if ( !minimal && (dim == ev->last_routing_dim) ) {
                offset++;
                continue;
            }
            
            for ( int link = 0; link < dim_width[dim]; ++link ) {
                // int index = port_start[dim] + ((offset * dim_width[dim]) * num_vcs) + next_vc;
                int next_port = port_start[dim] + (offset * dim_width[dim]) + link;
                int index = next_port * num_vcs + next_vc;
                int weight;
                if ( minimal ) {
                    weight = output_queue_lengths[index];
                }
                else {
                    weight = 2 * output_queue_lengths[index] + 1;
                }

                if ( weight == min_weight ) {
                    min_ports.push_back(next_port);
                }
                else if ( weight < min_weight ) {
                    min_weight = weight;
                    min_ports.clear();
                    min_ports.push_back(next_port);
                    // ev->last_routing_dim = dim;
                }
            }
            offset++;
        }        
    }
    // Route on the minimally weighted port
    // trace.getOutput().output("min_port = %d, next_vc = %d, min_weight = %d\n",min_port,next_vc,min_weight);

    // Randomly choose from the minports
    int min_port = min_ports[rng->generateNextUInt32() % min_ports.size()];

    // Determin which dimension this is from.  Set it to last
    // dimension then look to see if it's actually one of the others
    ev->last_routing_dim = dimensions - 1;
    for ( int i = 0; i < dimensions - 1; ++i ) {
        if ( min_port < port_start[i+1] ) {
            ev->last_routing_dim = i;
            break;
        }
    }
    
    ev->setNextPort(min_port);
    ev->setVC(next_vc);
}

