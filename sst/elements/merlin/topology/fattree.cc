// Copyright 2009-2014 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2014, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.
#include <sst_config.h>
#include "sst/core/serialization.h"
#include "fattree.h"

#include <algorithm>
#include <stdlib.h>

#include <sst/core/debug.h>

using namespace SST::Merlin;

#define DPRINTF( fmt, args...) __DBG( DBG_NETWORK, topo_fattree, fmt, ## args )


topo_fattree::topo_fattree(Component* comp, Params& params) :
    Topology()
{
    num_ports = params.find_integer("num_ports");
    my_address.s = params.find_integer("fattree:addr");
    rtr_level = params.find_integer("fattree:level");
    edge_loading = params.find_integer("fattree:loading", -1);
    if ( edge_loading < 0 ) edge_loading = (num_ports/2);
    
    buildRouteTable();
}


topo_fattree::~topo_fattree()
{
    delete [] table;
}


void topo_fattree::buildRouteTable(void)
{
    table = new RouteTableEntry[num_ports];

    switch (rtr_level) {
    case 1:
        /* Downstream */
        for ( int port = 0 ; port < edge_loading ; port++ ) {
            table[port].mask.u = 0xffffffff;
            //{10,my_address.x[1],my_address.x[2],port+2};
            table[port].value = my_address;
            table[port].value.x[3] = port+2;
        }

        /* Invalid ports */
        for ( int port = edge_loading ; port < num_ports/2 ; port++ ) {
            table[port].mask.u = 0xffffffff;
            table[port].value.u = 0;
        }

        /* Upstream */
        for ( int port = num_ports/2, value = 2 ; port < num_ports ; port++, value++ ) {
            table[port].mask.u = 0xff000000;
            table[port].value.u = 0;
            table[port].value.x[3] = value;
        }
        break;
    case 2:
        /* Downstream */
        for ( int port = 0 ; port < num_ports/2 ; port++ ) {
            table[port].mask.u = 0x00ffffff;
            // {10,my_address.x[1],port, 0};
            table[port].value = my_address;
            table[port].value.x[2] = port;
            table[port].value.x[3] = 0;
        }

        /* Upstream */
        for ( int port = num_ports/2, value = 2 ; port < num_ports ; port++, value++ ) {
            table[port].mask.u = 0xff000000;
            table[port].value.u = 0;
            table[port].value.x[3] = value;
        }
        break;
    case 3:
        /* Downstream */
        for ( int port = 0 ; port < num_ports ; port++ ) {
            table[port].mask.u = 0x0000ffff;
            // {10,port,0,0};
            table[port].value.x[0] = 10;
            table[port].value.x[1] = port;
            table[port].value.x[2] = 0;
            table[port].value.x[3] = 0;
        }
        break;
    default:
        _abort(topt_fattree, "Bad level %d\n", rtr_level);

    }

}


void topo_fattree::printRouteTable(FILE *fp) const
{
    fprintf(fp, "Route Table:\n");
    fprintf(fp, "%4s  %15s  %15s\n", "Port", "Mask", "Value");
    for ( int i = 0 ; i < num_ports ; i++ ) {
        fprintf(fp, "% 4d  %03u.%03u.%03u.%03u  %03u.%03u.%03u.%03u\n",
                i,
                table[i].mask.x[0], table[i].mask.x[1], table[i].mask.x[2], table[i].mask.x[3],
                table[i].value.x[0], table[i].value.x[1], table[i].value.x[2], table[i].value.x[3]);
    }
}



void topo_fattree::route(int port, int vc, internal_router_event* ev)
{
    Addr dest;
    dest.s = ev->getDest();

    /* Going in order means that downstream matches (ports 0 - radix/2) will
     * have priority over upstream.
     */
    for ( int i = 0 ; i < num_ports ; i++ ) {
        if ( (dest.u & table[i].mask.u) == table[i].value.u ) {
            ev->setNextPort(i);
            return;
        }
    }
    /* Should only make it here if nothing matched.  That's not a good condition */
    printf("Router %u.%u.%u.%u Unable to match address %u.%u.%u.%u!\n",
            my_address.x[0], my_address.x[1], my_address.x[2], my_address.x[3],
            dest.x[0], dest.x[1], dest.x[2], dest.x[3]);
    printRouteTable(stdout);
    _abort(topo_fattree, "Aborting!\n");

}



internal_router_event* topo_fattree::process_input(RtrEvent* ev)
{
    internal_router_event* ire = new internal_router_event(ev);
    ire->setVC(ev->vn);
    return ire;
}

void topo_fattree::routeInitData(int inPort, internal_router_event* ev, std::vector<int> &outPorts)
{
    if ( ev->getDest() == INIT_BROADCAST_ADDR ) {
        switch (rtr_level) {
        case 1:
            /* Downstream  - send to hosts*/
            for ( int port = 0 ; port < edge_loading ; port++ ) {
                if ( port != inPort )
                    outPorts.push_back(port);
            }

            if ( inPort < num_ports/2 ) {
                /* Upstream  - send to 1 upper-level router*/
                outPorts.push_back(num_ports/2);
            }
            break;
        case 2:
            if ( inPort < (num_ports/2) ) {
                /* came from Downstream, send to 1 upper-level router */
                outPorts.push_back(num_ports/2);
                for ( int port = 0 ; port < num_ports/2 ; port++ ) {
                    /* also, send downstream withing this router */
                    if ( port != inPort )
                        outPorts.push_back(port);
                }
            } else {
                /* came from Upstream */
                for ( int port = 0 ; port < num_ports/2 ; port++ ) {
                    outPorts.push_back(port);
                }
            }
            break;
        case 3:
            /* Send to all Downstream (except from incoming port) */
            for ( int port = 0 ; port < num_ports ; port++ ) {
                if ( port != inPort )
                    outPorts.push_back(port);
            }
            break;
        default:
            _abort(topt_fattree, "Bad level %d\n", rtr_level);
        }
    } else {
        route(inPort, 0, ev);
        outPorts.push_back(ev->getNextPort());
    }
}


internal_router_event* topo_fattree::process_InitData_input(RtrEvent* ev)
{
    return new internal_router_event(ev);
}

int
topo_fattree::getEndpointID(int port)
{
    if ( rtr_level > 1 ) return -2;
    if ( port >= edge_loading ) return -3;
    return table[port].value.s;
}

Topology::PortState topo_fattree::getPortState(int port) const
{
        if ( rtr_level == 1 ) {
            if ( port < edge_loading ) return R2N;
            else if ( port >= (num_ports/2) ) return R2R;
            else return UNCONNECTED;
        } else {
            return R2R;
        }
}
