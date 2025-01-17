// Copyright 2009-2025 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2025, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// of the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.
#include <sst_config.h>

#include <stdlib.h>

#include <algorithm>

#include "singlerouter.h"

using namespace SST::Merlin;

#define DPRINTF( fmt, args...) __DBG( DBG_NETWORK, topo_singlerouter, fmt, ## args )

topo_singlerouter::topo_singlerouter(ComponentId_t cid, Params& params, int num_ports, int rtr_id, int num_vns) :
    Topology(cid),
    num_ports(num_ports),
    num_vns(num_vns)
{
}

topo_singlerouter::~topo_singlerouter()
{
}

void
topo_singlerouter::route_packet(int port, int vc, internal_router_event* ev)
{
    ev->setNextPort(ev->getDest());
}


internal_router_event*
topo_singlerouter::process_input(RtrEvent* ev)
{
    internal_router_event* ire = new internal_router_event(ev);
    ire->setVC(ire->getVN());
    return ire;
}


void topo_singlerouter::routeUntimedData(int port, internal_router_event* ev, std::vector<int> &outPorts)
{
    if ( ev->getDest() == UNTIMED_BROADCAST_ADDR ) {
        for ( int i = 0 ; i < num_ports ; i++ ) {
            if ( i != port )
                outPorts.push_back(i);
        }

    } else {
        route_packet(port, 0, ev);
        outPorts.push_back(ev->getNextPort());
    }
}


internal_router_event* topo_singlerouter::process_UntimedData_input(RtrEvent* ev)
{
    return new internal_router_event(ev);
}


Topology::PortState
topo_singlerouter::getPortState(int port) const
{
    if ( port < num_ports ) return R2N;
    return UNCONNECTED;
}

