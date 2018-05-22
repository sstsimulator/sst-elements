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
//

#include <sst_config.h>
#include <sst/core/sharedRegion.h>
#include "sst/core/rng/xorshift.h"

#include "dragonfly.h"

#include <stdlib.h>
#include <sstream>

using namespace SST::Merlin;



void
RouteToGroup::init(SharedRegion* sr, size_t g, size_t r)
{
    region = sr;
    data = sr->getPtr<const RouterPortPair*>();
    groups = g;
    routes = r;
    
}

const RouterPortPair&
RouteToGroup::getRouterPortPair(int group, int route_number)
{
    // data = static_cast<RouterPortPair*>(region->getRawPtr());
    return data[group*routes + route_number];
}

void
RouteToGroup::setRouterPortPair(int group, int route_number, const RouterPortPair& pair) {
    // output.output("%d, %d, %d, %d\n",group,route_number,pair.router,pair.port);
    region->modifyArray(group*routes+route_number,pair);
}


/*
 * Port Layout:
 * [0, params.p)                    // Hosts 0 -> params.p
 * [params.p, params.p+params.a-1)  // Routers within this group
 * [params.p+params.a-1, params.k)  // Other groups
 */
topo_dragonfly::topo_dragonfly(Component* comp, Params &p) :
    Topology(comp)
{
    params.p = (uint32_t)p.find<int>("dragonfly:hosts_per_router");
    params.a = (uint32_t)p.find<int>("dragonfly:routers_per_group");
    params.k = (uint32_t)p.find<int>("num_ports");
    params.h = (uint32_t)p.find<int>("dragonfly:intergroup_per_router");
    params.g = (uint32_t)p.find<int>("dragonfly:num_groups");
    params.n = (uint32_t)p.find<int>("dragonfly:intergroup_links");

    std::string global_route_mode_s = p.find<std::string>("dragonfly:global_route_mode","absolute");
    if ( global_route_mode_s == "absolute" ) global_route_mode = ABSOLUTE;
    else if ( global_route_mode_s == "relative" ) global_route_mode = RELATIVE;
    else {
        output.fatal(CALL_INFO, -1, "Invalid dragonfly:global_route_mode specified: %s.\n",global_route_mode_s.c_str());        
    }
    
    std::string route_algo = p.find<std::string>("dragonfly:algorithm", "minimal");

    adaptive_threshold = p.find<double>("dragonfly:adaptive_threshold",2.0);
    
    // Get the global link map
    std::vector<int64_t> global_link_map;

    // For now, parse array ourselves so as not to create a dependency
    // on new core features.  Once we want to use new core features,
    // delete code below and uncomment line above (can also get rid of
    // #include <sstream> above).
    std::string array = p.find<std::string>("dragonfly:global_link_map");
    if ( array != "" ) {
        array = array.substr(1,array.size()-2);
    
        std::stringstream ss(array);
    
        while( ss.good() ) {
            std::string substr;
            getline( ss, substr, ',' );
            global_link_map.push_back(strtol(substr.c_str(), NULL, 0));
        }
    }
    // End parse array on our own
    
    // Get a shared region
    SharedRegion* sr = Simulation::getSharedRegionManager()->getGlobalSharedRegion("dragonfly:group_to_global_port",
                                                                                  ((params.g-1) * params.n) * sizeof(RouterPortPair),
                                                                                   new SharedRegionMerger());
    // Set up the RouteToGroup object
    group_to_global_port.init(sr, params.g, params.n);

    // Fill in the shared region using the RouteToGroupObject (if
    // vector for param dragonfly:global_link_map is empty, then
    // nothing will be intialized.
    for ( int i = 0; i < global_link_map.size(); i++ ) {
        // Figure out all the mappings
        int64_t value = global_link_map[i];
        if ( value == -1 ) continue;
        
        int group = value % (params.g - 1);
        int route_num = value / (params.g - 1);
        int router = i / params.h;
        int port = (i % params.h) + params.p + params.a - 1;
        
        RouterPortPair rpp;
        rpp.router = router;
        rpp.port = port;
        group_to_global_port.setRouterPortPair(group, route_num, rpp);
    }

    
    // Publish the shared region to make sure everyone has the data.
    sr->publish();
    
    if ( !route_algo.compare("valiant") ) {
        if ( params.g <= 2 ) {
            /* 2 or less groups... no point in valiant */
            algorithm = MINIMAL;
        } else {
            algorithm = VALIANT;
        }
    }
    else if ( !route_algo.compare("adaptive-local") ) {
        algorithm = ADAPTIVE_LOCAL;
    }
    else {
        algorithm = MINIMAL;
    }

    uint32_t id = p.find<int>("id");
    group_id = id / params.a;
    router_id = id % params.a;

    rng = new RNG::XORShiftRNG(id+1);

    output.verbose(CALL_INFO, 1, 1, "%u:%u:  ID: %u   Params:  p = %u  a = %u  k = %u  h = %u  g = %u\n",
            group_id, router_id, id, params.p, params.a, params.k, params.h, params.g);
}


topo_dragonfly::~topo_dragonfly()
{
}


void topo_dragonfly::route(int port, int vc, internal_router_event* ev)
{
    topo_dragonfly_event *td_ev = static_cast<topo_dragonfly_event*>(ev);

    // Break this up by port type
    uint32_t next_port = 0;
    if ( (uint32_t)port < params.p ) { 
        // Host ports

        if ( td_ev->dest.group == td_ev->src_group ) {
            // Packets stays within the group
            if ( td_ev->dest.router == router_id ) {
                // Stays within the router
                next_port = td_ev->dest.host;
            }
            else {
                // Route to the router specified by mid_group.  If
                // this is a direct route then mid_group will equal
                // router and packet will go direct.
                next_port = port_for_router(td_ev->dest.mid_group);
            }
        }
        else {
            // Packet is leaving group.  Simply route to group
            // specified by mid_group.  If this is a direct route then
            // mid_group will be set to group.
            next_port = port_for_group(td_ev->dest.mid_group, td_ev->global_slice);
        }        
    }
    else if ( (uint32_t)port < ( params.p + params.a - 1) ) {
        // Intragroup links

        if ( td_ev->dest.group == group_id ) {
            // In final group
            if ( td_ev->dest.router == router_id ) {
                // In final router, route to host port
                next_port = td_ev->dest.host;
            }
            else {
                // This is a valiantly routed packet within a group.
                // Need to increment the VC and route to correct
                // router.
                td_ev->setVC(vc+1);
                next_port = port_for_router(td_ev->dest.router);
            }
        }
        else {
            // Not in correct group, should route out one of the
            // global links
            if ( td_ev->dest.mid_group != group_id ) {
                next_port = port_for_group(td_ev->dest.mid_group, td_ev->global_slice);
            } else {
                next_port = port_for_group(td_ev->dest.group, td_ev->global_slice);
            }
        }
    }
    else { // global
        /* Came in from another group.  Increment VC */
        td_ev->setVC(vc+1);
        if ( td_ev->dest.group == group_id ) {
            if ( td_ev->dest.router == router_id ) {
                // In final router, route to host port
                next_port = td_ev->dest.host;
            }
            else {
                // Go to final router
                next_port = port_for_router(td_ev->dest.router);
            }
        }
        else {
            // Just passing through on a valiant route.  Route
            // directly to final group
            next_port = port_for_group(td_ev->dest.group, td_ev->global_slice);
        }
    }


    output.verbose(CALL_INFO, 1, 1, "%u:%u, Recv: %d/%d  Setting Next Port/VC:  %u/%u\n", group_id, router_id, port, vc, next_port, td_ev->getVC());
    td_ev->setNextPort(next_port);
}

void topo_dragonfly::reroute(int port, int vc, internal_router_event* ev)
{
    if ( algorithm != ADAPTIVE_LOCAL ) return;

    // For now, we make the adaptive routing decision only at the
    // input to the network and at the input to a group for adaptively
    // routed packets
    if ( port >= params.p && port < (params.p + params.a-1) ) return;

    
    topo_dragonfly_event *td_ev = static_cast<topo_dragonfly_event*>(ev);

    // Adaptive routing when packet stays in group
    if ( port < params.p && td_ev->dest.group == group_id ) {
        // If we're at the correct router, no adaptive needed
        if ( td_ev->dest.router == router_id) return;

        int direct_route_port = port_for_router(td_ev->dest.router);
        int direct_route_credits = output_credits[direct_route_port * num_vcs + vc];

        int valiant_route_port = port_for_router(td_ev->dest.mid_group);
        int valiant_route_credits = output_credits[valiant_route_port * num_vcs + vc];

        if ( valiant_route_credits > (int)((double)direct_route_credits * adaptive_threshold) ) {
            td_ev->setNextPort(valiant_route_port);
        }
        else {
            td_ev->setNextPort(direct_route_port);
        }
        
        return;
    }
    
    // If the dest is in the same group, no need to adaptively route
    if ( td_ev->dest.group == group_id ) return;

    
    // Based on the output queue depths, choose minimal or valiant
    // route.  We'll chose the direct route unless the remaining
    // output credits for the direct route is half of the valiant
    // route.  We'll look at two slice for each direct and indirect,
    // giving us a total of 4 routes we are looking at.  For packets
    // which came in adaptively on global links, look at two direct
    // routes and chose between the.
    int direct_slice1 = td_ev->global_slice_shadow;
    // int direct_slice2 = td_ev->global_slice;
    int direct_slice2 = (td_ev->global_slice_shadow + 1) % params.n;
    int direct_route_port1 = port_for_group(td_ev->dest.group, direct_slice1, 0 );
    int direct_route_port2 = port_for_group(td_ev->dest.group, direct_slice2, 1 );
    int direct_route_credits1 = output_credits[direct_route_port1 * num_vcs + vc];
    int direct_route_credits2 = output_credits[direct_route_port2 * num_vcs + vc];
    int direct_slice;
    int direct_route_port;
    int direct_route_credits;
    if ( direct_route_credits1 > direct_route_credits2 ) {
        direct_slice = direct_slice1;
        direct_route_port = direct_route_port1;
        direct_route_credits = direct_route_credits1;
    }
    else {
        direct_slice = direct_slice2;
        direct_route_port = direct_route_port2;
        direct_route_credits = direct_route_credits2;        
    }
    // if ( td_ev->getTraceType() != SST::Interfaces::SimpleNetwork::Request::NONE ) {
    //     output.output("TRACE(%d): reroute():"
    //                   " mid_group_shadow = %u,"
    //                   " direct_slice1 = %d,"
    //                   " direct_slice2 = %d,"
    //                   " direct_route_port1 = %d,"
    //                   " direct_route_port2 = %d",
    //                   td_ev->getTraceID(),
    //                   td_ev->dest.mid_group_shadow,
    //                   direct_slice1,
    //                   direct_slice2,
    //                   direct_route_port1,
    //                   direct_route_port2);
    // }

    int valiant_slice = 0;
    int valiant_route_port = 0;
    int valiant_route_credits = 0;

    if ( port >= (params.p + params.a-1) ) {
        // Global port, no indirect routes.  Set credits negative so
        // it will never get chosen
        valiant_route_credits = -1;
    }
    else {
        int valiant_slice1 = td_ev->global_slice;
        // int valiant_slice2 = td_ev->global_slice;
        int valiant_slice2 = (td_ev->global_slice + 1) % params.n;
        int valiant_route_port1 = port_for_group(td_ev->dest.mid_group_shadow, valiant_slice1, 2 );
        int valiant_route_port2 = port_for_group(td_ev->dest.mid_group_shadow, valiant_slice2, 3 );
        int valiant_route_credits1 = output_credits[valiant_route_port1 * num_vcs + vc];
        int valiant_route_credits2 = output_credits[valiant_route_port2 * num_vcs + vc];
        if ( valiant_route_credits1 > valiant_route_credits2 ) {
            valiant_slice = valiant_slice1;
            valiant_route_port = valiant_route_port1;
            valiant_route_credits = valiant_route_credits1;
        }
        else {
            valiant_slice = valiant_slice2;
            valiant_route_port = valiant_route_port2;
            valiant_route_credits = valiant_route_credits2;        
        }
    }

    
    if ( valiant_route_credits > (int)((double)direct_route_credits * adaptive_threshold) ) { // Use valiant route
        td_ev->dest.mid_group = td_ev->dest.mid_group_shadow;
        td_ev->setNextPort(valiant_route_port);
        // output.output("valiant_slice = %d\n", valiant_slice);
        td_ev->global_slice = valiant_slice;
    }
    else { // Use direct route
        td_ev->dest.mid_group = td_ev->dest.group;
        td_ev->setNextPort(direct_route_port);
        // output.output("direct_slice = %d\n", valiant_slice);
        td_ev->global_slice = direct_slice;
    }

    // if ( td_ev->getTraceType() != SST::Interfaces::SimpleNetwork::Request::NONE ) {
    //     output.output("TRACE(%d): reroute():"
    //                   " mid_group_shadow = %u\n",
    //                   td_ev->getTraceID(),
    //                   td_ev->dest.mid_group_shadow);
    // }
    
}


internal_router_event* topo_dragonfly::process_input(RtrEvent* ev)
{
    dgnflyAddr dstAddr = {0, 0, 0, 0};
    idToLocation(ev->request->dest, &dstAddr);
    
    switch (algorithm) {
    case MINIMAL:
        if ( dstAddr.group == group_id ) {
            dstAddr.mid_group = dstAddr.router;
        }
        else {
            dstAddr.mid_group = dstAddr.group;
        }
        break;
    case VALIANT:
    case ADAPTIVE_LOCAL:
        if ( dstAddr.group == group_id ) {
            // staying within group, set mid_group to be an intermediate router within group
            do {
                dstAddr.mid_group = rng->generateNextUInt32() % params.a;
                // dstAddr.mid_group = router_id;
            }
            while ( dstAddr.mid_group == router_id );
            // dstAddr.mid_group = dstAddr.group;
        } else {
            do {
                dstAddr.mid_group = rng->generateNextUInt32() % params.g;
                // dstAddr.mid_group = rand() % params.g;
            } while ( dstAddr.mid_group == group_id || dstAddr.mid_group == dstAddr.group );
        }
        break;
    }
    dstAddr.mid_group_shadow = dstAddr.mid_group;
    // output.verbose(CALL_INFO, 1, 1, "Init packet from %d to %d to %u:%u:%u:%u\n", ev->request->src, ev->request->dest, dstAddr.group, dstAddr.mid_group, dstAddr.router, dstAddr.host);

    topo_dragonfly_event *td_ev = new topo_dragonfly_event(dstAddr);
    td_ev->src_group = group_id;
    td_ev->setEncapsulatedEvent(ev);
    td_ev->setVC(ev->request->vn * 3);
    td_ev->global_slice = ev->request->src % params.n;
    td_ev->global_slice_shadow = ev->request->src % params.n;

    if ( td_ev->getTraceType() != SST::Interfaces::SimpleNetwork::Request::NONE ) {
        output.output("TRACE(%d): process_input():"
                      " mid_group_shadow = %u\n",
                      td_ev->getTraceID(),
                      td_ev->dest.mid_group_shadow);
    }
    return td_ev;
}



void topo_dragonfly::routeInitData(int port, internal_router_event* ev, std::vector<int> &outPorts)
{
    bool broadcast_to_groups = false;
    topo_dragonfly_event *td_ev = static_cast<topo_dragonfly_event*>(ev);
    if ( td_ev->dest.host == (uint32_t)INIT_BROADCAST_ADDR ) {
        if ( (uint32_t)port >= (params.p + params.a-1) ) {
            /* Came in from another group.
             * Send to locals, and other routers in group
             */
            for ( uint32_t p  = 0 ; p < (params.p + params.a-1) ; p++ ) {
                outPorts.push_back((int)p);
            }
        } else if ( (uint32_t)port >= params.p ) {
            /* Came in from another router in group.
             * send to hosts
             * if this is the source group, send to other groups
             */
            for ( uint32_t p = 0 ; p < params.p ; p++ ) {
                outPorts.push_back((int)p);
            }
            if ( td_ev->src_group == group_id ) {
                broadcast_to_groups = true;
                // for ( uint32_t p = (params.p+params.a-1) ; p < params.k ; p++ ) {
                //     outPorts.push_back((int)p);
                // }
            }
        } else {
            /* Came in from a host
             * Send to all other hosts and routers in group, and all groups
             */
            // for ( int p = 0 ; p < (int)params.k ; p++ ) {
            for ( int p = 0 ; p < (int)(params.p + params.a - 1) ; p++ ) {
                if ( p != port )
                    outPorts.push_back((int)p);
            }
            broadcast_to_groups = true;
        }

        if ( broadcast_to_groups ) {
            for ( int p = 0; p < (int)(params.g - 1); p++ ) {
                const RouterPortPair& pair = group_to_global_port.getRouterPortPair(p,0);
                if ( pair.router == router_id ) outPorts.push_back((int)(pair.port));
            }
        }
    } else {
        //Not all data structures used for routing during run are
        //initialized yet, so we need to just do a quick minimal
        //routing scheme for init.
        // route(port, 0, ev);

        // TraceFunction(CALL_INFO);
        // Minimal Route
        int next_port;
        if ( td_ev->dest.group != group_id ) {
            next_port = port_for_group(td_ev->dest.group, td_ev->global_slice);
        }
        else if ( td_ev->dest.router != router_id ) {
            next_port = port_for_router(td_ev->dest.router);
        }
        else {
            next_port = td_ev->dest.host;
        }
        outPorts.push_back(next_port);
    }

}


internal_router_event* topo_dragonfly::process_InitData_input(RtrEvent* ev)
{
    dgnflyAddr dstAddr;
    idToLocation(ev->request->dest, &dstAddr);
    topo_dragonfly_event *td_ev = new topo_dragonfly_event(dstAddr);
    td_ev->src_group = group_id;
    td_ev->setEncapsulatedEvent(ev);

    return td_ev;
}





Topology::PortState topo_dragonfly::getPortState(int port) const
{
    if ( (uint32_t)port < params.p ) return R2N;
    else return R2R;
}

std::string topo_dragonfly::getPortLogicalGroup(int port) const
{
    if ( (uint32_t)port < params.p ) return "host";
    if ( (uint32_t)port >= params.p && (uint32_t)port < (params.p + params.a - 1) ) return "group";
    else return "global";
}

int
topo_dragonfly::getEndpointID(int port)
{
    return (group_id * (params.a /*rtr_per_group*/ * params.p /*hosts_per_rtr*/)) +
        (router_id * params.p /*hosts_per_rtr*/) + port;
}

void
topo_dragonfly::setOutputBufferCreditArray(int const* array, int vcs)
{
    output_credits = array;
    num_vcs = vcs;
}


void topo_dragonfly::idToLocation(int id, dgnflyAddr *location)
{
    if ( id == INIT_BROADCAST_ADDR) {
        location->group = (uint32_t)INIT_BROADCAST_ADDR;
        location->mid_group = (uint32_t)INIT_BROADCAST_ADDR;
        location->router = (uint32_t)INIT_BROADCAST_ADDR;
        location->host = (uint32_t)INIT_BROADCAST_ADDR;
    } else {
        uint32_t hosts_per_group = params.p * params.a;
        location->group = id / hosts_per_group;
        location->router = (id % hosts_per_group) / params.p;
        location->host = id % params.p;
    }
}


uint32_t topo_dragonfly::router_to_group(uint32_t group)
{

    if ( group < group_id ) {
        return group / params.h;
    } else if ( group > group_id ) {
        return (group-1) / params.h;
    } else {
        output.fatal(CALL_INFO, -1, "Trying to find router to own group.\n");
        return 0;
    }
}


/* returns local router port if group can't be reached from this router */
uint32_t topo_dragonfly::port_for_group(uint32_t group, uint32_t slice, int id)
{
    // Look up global port to use
    switch ( global_route_mode ) {
    case ABSOLUTE:
        if ( group >= group_id ) group--;
        break;
    case RELATIVE:
        if ( group > group_id ) {
            group = group - group_id - 1;
        }
        else {
            group = params.g - group_id + group - 1;
        }
        break;
    default:
        break;
    }

    const RouterPortPair& pair = group_to_global_port.getRouterPortPair(group,slice);

    if ( pair.router == router_id ) {
        return pair.port;
    } else {
        return port_for_router(pair.router);
    }

}


uint32_t topo_dragonfly::port_for_router(uint32_t router)
{
    uint32_t tgt = params.p + router;
    if ( router > router_id ) tgt--;
    return tgt;
}


