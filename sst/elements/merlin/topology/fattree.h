// -*- mode: c++ -*-

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


#ifndef COMPONENTS_MERLIN_TOPOLOGY_FATTREE_H
#define COMPONENTS_MERLIN_TOPOLOGY_FATTREE_H

#include <sst/core/event.h>
#include <sst/core/link.h>
#include <sst/core/params.h>

#include "sst/elements/merlin/router.h"

using namespace SST;



class topo_fattree: public Topology {

    typedef union {
        uint8_t  x[4];
        int32_t  s;
        uint32_t u;
    } Addr;

    struct RouteTableEntry {
        Addr mask;
        Addr value;
    };


    class topo_fattree_event : public internal_router_event {

    public:
        Addr destaddr;

        topo_fattree_event(RtrEvent *ev)
        {
            setEncapsulatedEvent(ev);
            destaddr.s = getDest();
        }
        ~topo_fattree_event() { }
        Addr getDestAddr() { return destaddr; }
    };


    int router_id;
    int rtr_level;
    int edge_loading;
    Addr my_address;
    int num_ports;

    RouteTableEntry *table;


    void buildRouteTable(void);
    void printRouteTable(FILE *fp);

public:
    topo_fattree(Params& params);
    ~topo_fattree();

    virtual void route(int port, int vc, internal_router_event* ev);
    virtual internal_router_event* process_input(RtrEvent* ev);
    virtual bool isHostPort(int port);


private:
};

#endif // COMPONENTS_MERLIN_TOPOLOGY_FATTREE_H
