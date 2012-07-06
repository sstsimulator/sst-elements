// -*- mode: c++ -*-

// Copyright 2009-2010 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2010, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#ifndef COMPONENTS_MERLIN_TOPOLOGY_TORUS_H
#define COMPONENTS_MERLIN_TOPOLOGY_TORUS_H

#include <sst/core/event.h>
#include <sst/core/link.h>
#include <sst/core/params.h>

#include "sst/elements/merlin/router.h"

using namespace SST;


class topo_torus_event : public internal_router_event {


public:
    int dimensions;
    int* dest_loc;

    topo_torus_event(int dim) {	dimensions = dim; dest_loc = new int[dim]; }
    ~topo_torus_event() { delete[] dest_loc; }
};


class topo_torus: public Topology {

    int id;
    int dimensions;
    int* dim_size;
    
public:
    topo_torus(Params& params);
    ~topo_torus();
    
    void route(int port, int vc, internal_router_event* ev);
    internal_router_event* process_input(RtrEvent* ev);
    bool isHostPort(int port);
};

#endif // COMPONENTS_MERLIN_TOPOLOGY_TORUS_H
