// Copyright 2009-2013 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2013, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.
#include <sst_config.h>
#include "sst/core/serialization/element.h"

#include <stdlib.h>

#include <algorithm>

#include "fattree.h"


#define DPRINTF( fmt, args...) __DBG( DBG_NETWORK, topo_fattree, fmt, ## args )


topo_fattree::topo_fattree(Params& params) :
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


void topo_fattree::printRouteTable(FILE *fp)
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
    topo_fattree_event *tt_ev = static_cast<topo_fattree_event*>(ev);
    Addr dest = tt_ev->getDestAddr();

    bool found = false;
    for ( int i = 0 ; i < num_ports ; i++ ) {
        if ( (dest.u & table[i].mask.u) == table[i].value.u ) {
            tt_ev->setNextPort(i);
            found = true;
            break;
        }
    }
    if ( !found ) {
        printf("Router %u.%u.%u.%u Unable to match address %u.%u.%u.%u!\n",
                my_address.x[0], my_address.x[1], my_address.x[2], my_address.x[3],
                dest.x[0], dest.x[1], dest.x[2], dest.x[3]);
        printRouteTable(stdout);
        _abort(topo_fattree, "Aborting!\n");
    }

}



internal_router_event* topo_fattree::process_input(RtrEvent* ev)
{
    topo_fattree_event* tt_ev = new topo_fattree_event(ev);

    return tt_ev;
}


Topology::PortState topo_fattree::getPortState(int port)
{
        if ( rtr_level == 1 ) {
            if ( port < edge_loading ) return R2N;
            else if ( port > (num_ports/2) ) return R2R;
            else return UNCONNECTED;
        } else {
            return R2R;
        }
}
