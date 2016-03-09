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
#include "dragonfly.h"

#include <stdlib.h>


using namespace SST::Merlin;


/*
 * Port Layout:
 * [0, params.p)                    // Hosts 0 -> params.p
 * [params.p, params.p+params.a-1)  // Routers within this group
 * [params.p+params.a-1, params.k)  // Other groups
 */

topo_dragonfly::topo_dragonfly(Component* comp, Params &p) :
    Topology()
{
    params.p = (uint32_t)p.find_integer("dragonfly:hosts_per_router");
    params.a = (uint32_t)p.find_integer("dragonfly:routers_per_group");
    params.k = (uint32_t)p.find_integer("num_ports");
    params.h = (uint32_t)p.find_integer("dragonfly:intergroup_per_router");
    params.g = (uint32_t)p.find_integer("dragonfly:num_groups");

    std::string route_algo = p.find_string("dragonfly:algorithm", "minimal");

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
    output.verbose(CALL_INFO, 1, 1, "%u:%u:  ID: %u   Params:  p = %u  a = %u  k = %u  h = %u  g = %u\n",
            group_id, router_id, id, params.p, params.a, params.k, params.h, params.g);
}


topo_dragonfly::~topo_dragonfly()
{
}


void topo_dragonfly::route(int port, int vc, internal_router_event* ev)
{
    topo_dragonfly_event *td_ev = static_cast<topo_dragonfly_event*>(ev);

    if ( (uint32_t)port >= (params.p + params.a-1) ) {
        /* Came in from another group.  Increment VC */
        td_ev->setVC(vc+1);
    }


    /* Minimal Route */
    uint32_t next_port = 0;
    if ( td_ev->dest.group != group_id ) {
        if ( td_ev->dest.mid_group != group_id ) {
            next_port = port_for_group(td_ev->dest.mid_group);
        } else {
            next_port = port_for_group(td_ev->dest.group);
        }
    } else if ( td_ev->dest.router != router_id ) {
        next_port = port_for_router(td_ev->dest.router);
    } else {
        next_port = td_ev->dest.host;
    }

    output.verbose(CALL_INFO, 1, 1, "%u:%u, Recv: %d/%d  Setting Next Port/VC:  %u/%u\n", group_id, router_id, port, vc, next_port, td_ev->getVC());
    td_ev->setNextPort(next_port);
}


internal_router_event* topo_dragonfly::process_input(RtrEvent* ev)
{
    dgnflyAddr dstAddr = {0, 0, 0, 0};
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
                dstAddr.mid_group = rand() % params.g;
            } while ( dstAddr.mid_group == group_id || dstAddr.mid_group == dstAddr.group );
        }
        break;
    }

    // output.verbose(CALL_INFO, 1, 1, "Init packet from %d to %d to %u:%u:%u:%u\n", ev->request->src, ev->request->dest, dstAddr.group, dstAddr.mid_group, dstAddr.router, dstAddr.host);

    topo_dragonfly_event *td_ev = new topo_dragonfly_event(dstAddr);
    td_ev->src_group = group_id;
    td_ev->setEncapsulatedEvent(ev);
    td_ev->setVC(ev->request->vn * 3);
    
    return td_ev;
}



void topo_dragonfly::routeInitData(int port, internal_router_event* ev, std::vector<int> &outPorts)
{
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
                for ( uint32_t p = (params.p+params.a-1) ; p < params.k ; p++ ) {
                    outPorts.push_back((int)p);
                }
            }
        } else {
            /* Came in from a host
             * Send to all other hosts and routers in group, and all groups
             */
            for ( int p = 0 ; p < (int)params.k ; p++ ) {
                if ( p != port )
                    outPorts.push_back((int)p);
            }
        }
    } else {
        route(port, 0, ev);
        outPorts.push_back(ev->getNextPort());
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


void topo_dragonfly::idToLocation(int id, dgnflyAddr *location) const
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


uint32_t topo_dragonfly::router_to_group(uint32_t group) const
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
uint32_t topo_dragonfly::port_for_group(uint32_t group) const
{
    uint32_t tgt_rtr = router_to_group(group);
    if ( tgt_rtr == router_id ) {
        uint32_t port = params.p + params.a-1;
        if ( group < group_id ) {
            port += (group % params.h);
        } else {
            port += ((group-1) % params.h);
        }
        return port;
    } else {
        return port_for_router(tgt_rtr);
    }

}


uint32_t topo_dragonfly::port_for_router(uint32_t router) const
{
    uint32_t tgt = params.p + router;
    if ( router > router_id ) tgt--;
    return tgt;
}
