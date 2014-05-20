// -*- mode: c++ -*-

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


#ifndef COMPONENTS_MERLIN_TOPOLOGY_FATTREE_H
#define COMPONENTS_MERLIN_TOPOLOGY_FATTREE_H

#include <sst/core/event.h>
#include <sst/core/link.h>
#include <sst/core/params.h>

#include "sst/elements/merlin/router.h"

namespace SST {
namespace Merlin {

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


    int rtr_level;
    int edge_loading;
    Addr my_address;
    int num_ports;

    RouteTableEntry *table;


    void buildRouteTable(void);
    void printRouteTable(FILE *fp) const;

public:
    topo_fattree(Component* comp, Params& params);
    ~topo_fattree();

    virtual void route(int port, int vc, internal_router_event* ev);
    virtual internal_router_event* process_input(RtrEvent* ev);

    virtual void routeInitData(int port, internal_router_event* ev, std::vector<int> &outPorts);
    virtual internal_router_event* process_InitData_input(RtrEvent* ev);

    virtual int getEndpointID(int port);

    virtual PortState getPortState(int port) const;


private:
};

}
}

#endif // COMPONENTS_MERLIN_TOPOLOGY_FATTREE_H
