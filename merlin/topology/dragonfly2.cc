// Copyright 2009-2015 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2015, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.
//

#include <sst_config.h>
#include <sst/core/serialization.h>
#include <sst/core/sharedRegion.h>
#include "sst/core/rng/xorshift.h"

#include "dragonfly2.h"

#include <stdlib.h>
#include <sstream>

using namespace SST::Merlin;


/*
 * Port Layout:
 * [0, params.p)                    // Hosts 0 -> params.p
 * [params.p, params.p+params.a-1)  // Routers within this group
 * [params.p+params.a-1, params.k)  // Other groups
 */
topo_dragonfly2::topo_dragonfly2(Component* comp, Params &p) :
    Topology()
{
    params.p = (uint32_t)p.find_integer("dragonfly:hosts_per_router");
    params.a = (uint32_t)p.find_integer("dragonfly:routers_per_group");
    params.k = (uint32_t)p.find_integer("num_ports");
    params.h = (uint32_t)p.find_integer("dragonfly:intergroup_per_router");
    params.g = (uint32_t)p.find_integer("dragonfly:num_groups");
    params.n = (uint32_t)p.find_integer("dragonfly:intergroup_links");

    
    std::string route_algo = p.find_string("dragonfly:algorithm", "minimal");

    // Get the global link map
    std::vector<int64_t> global_link_map;
    // p.find_integer_array("dragonfly:global_link_map",global_link_map);

    // For now, parse array ourselves so as not to create a dependency
    // on new core features.  Once we want to use new core features,
    // delete code below and uncomment line above (can also get rid of
    // #include <sstream> above).
    std::string array = p.find_string("dragonfly:global_link_map");
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
    SharedRegion* sr = Simulation::getSharedRegionManager()->getLocalSharedRegion("dragonfly:group_to_global_port",
                                                                                  (params.g-1) * params.n * sizeof(RouterPortPair));
    // Set up the RouteToGroup object
    group_to_global_port.init(sr->getRawPtr(), params.g, params.n);

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
        
        RouterPortPair& rpp = group_to_global_port.getRouterPortPair(group, route_num);
        rpp.router = router;
        rpp.port = port;
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
    } else {
        algorithm = MINIMAL;
    }

    uint32_t id = p.find_integer("id");
    group_id = id / params.a;
    router_id = id % params.a;
    
    rng = new RNG::XORShiftRNG(id+1);

    output.verbose(CALL_INFO, 1, 1, "%u:%u:  ID: %u   Params:  p = %u  a = %u  k = %u  h = %u  g = %u\n",
            group_id, router_id, id, params.p, params.a, params.k, params.h, params.g);
}


topo_dragonfly2::~topo_dragonfly2()
{
}


void topo_dragonfly2::route(int port, int vc, internal_router_event* ev)
{
    topo_dragonfly2_event *td_ev = static_cast<topo_dragonfly2_event*>(ev);

    if ( (uint32_t)port >= (params.p + params.a-1) ) {
        /* Came in from another group.  Increment VC */
        td_ev->setVC(vc+1);
    }


    /* Minimal Route */
    uint32_t next_port = 0;
    if ( td_ev->dest.group != group_id ) {
        if ( td_ev->dest.mid_group != group_id ) {
            next_port = port_for_group(td_ev->dest.mid_group, td_ev->global_slice);
        } else {
            next_port = port_for_group(td_ev->dest.group, td_ev->global_slice);
        }
    } else if ( td_ev->dest.router != router_id ) {
        next_port = port_for_router(td_ev->dest.router);
    } else {
        next_port = td_ev->dest.host;
    }

    output.verbose(CALL_INFO, 1, 1, "%u:%u, Recv: %d/%d  Setting Next Port/VC:  %u/%u\n", group_id, router_id, port, vc, next_port, td_ev->getVC());
    td_ev->setNextPort(next_port);
}


internal_router_event* topo_dragonfly2::process_input(RtrEvent* ev)
{
    dgnfly2Addr dstAddr = {0, 0, 0, 0};
    idToLocation(ev->request->dest, &dstAddr);
    
    switch (algorithm) {
    case MINIMAL:
        dstAddr.mid_group = dstAddr.group;
        break;
    case VALIANT:
        if ( dstAddr.group == group_id ) {
            // staying here.
            dstAddr.mid_group = dstAddr.group;
        } else {
            do {
                dstAddr.mid_group = rng->generateNextUInt32() % params.g;
                // dstAddr.mid_group = rand() % params.g;
            } while ( dstAddr.mid_group == group_id || dstAddr.mid_group == dstAddr.group );
        }
        break;
    }

    // output.verbose(CALL_INFO, 1, 1, "Init packet from %d to %d to %u:%u:%u:%u\n", ev->request->src, ev->request->dest, dstAddr.group, dstAddr.mid_group, dstAddr.router, dstAddr.host);

    topo_dragonfly2_event *td_ev = new topo_dragonfly2_event(dstAddr);
    td_ev->src_group = group_id;
    td_ev->setEncapsulatedEvent(ev);
    td_ev->setVC(ev->request->vn * 3);
    td_ev->global_slice = ev->request->src % params.n;
    
    return td_ev;
}



void topo_dragonfly2::routeInitData(int port, internal_router_event* ev, std::vector<int> &outPorts)
{
    bool broadcast_to_groups = false;
    topo_dragonfly2_event *td_ev = static_cast<topo_dragonfly2_event*>(ev);
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
                RouterPortPair& pair = group_to_global_port.getRouterPortPair(p,0);
                if ( pair.router == router_id ) outPorts.push_back((int)(pair.port));
            }
        }
    } else {
        route(port, 0, ev);
        outPorts.push_back(ev->getNextPort());
    }

}


internal_router_event* topo_dragonfly2::process_InitData_input(RtrEvent* ev)
{
    dgnfly2Addr dstAddr;
    idToLocation(ev->request->dest, &dstAddr);
    topo_dragonfly2_event *td_ev = new topo_dragonfly2_event(dstAddr);
    td_ev->src_group = group_id;
    td_ev->setEncapsulatedEvent(ev);

    return td_ev;
}





Topology::PortState topo_dragonfly2::getPortState(int port) const
{
    if ( (uint32_t)port < params.p ) return R2N;
    else return R2R;
}

std::string topo_dragonfly2::getPortLogicalGroup(int port) const
{
    if ( (uint32_t)port < params.p ) return "host";
    if ( (uint32_t)port >= params.p && (uint32_t)port < (params.p + params.a - 1) ) return "group";
    else return "global";
}

int
topo_dragonfly2::getEndpointID(int port)
{
    return (group_id * (params.a /*rtr_per_group*/ * params.p /*hosts_per_rtr*/)) +
        (router_id * params.p /*hosts_per_rtr*/) + port;
}


void topo_dragonfly2::idToLocation(int id, dgnfly2Addr *location)
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


uint32_t topo_dragonfly2::router_to_group(uint32_t group)
{

    /* For now, assume only 1 connection to each group */
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
uint32_t topo_dragonfly2::port_for_group(uint32_t group, uint32_t slice)
{
    // Look up global port to use
    if ( group >= group_id ) group--;
    RouterPortPair& pair = group_to_global_port.getRouterPortPair(group,slice);

    // output.output("%u, %u, %u, %u\n",group,slice,pair.router,pair.port);
    
    if ( pair.router == router_id ) {
        return pair.port;
    } else {
        return port_for_router(pair.router);
    }

}


uint32_t topo_dragonfly2::port_for_router(uint32_t router)
{
    uint32_t tgt = params.p + router;
    if ( router > router_id ) tgt--;
    return tgt;
}
