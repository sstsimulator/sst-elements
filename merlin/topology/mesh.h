// -*- mode: c++ -*-

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


#ifndef COMPONENTS_MERLIN_TOPOLOGY_MESH_H
#define COMPONENTS_MERLIN_TOPOLOGY_MESH_H

#include <sst/core/event.h>
#include <sst/core/link.h>
#include <sst/core/params.h>

#include <string.h>

#include "sst/elements/merlin/router.h"

namespace SST {
namespace Merlin {

class topo_mesh_event : public internal_router_event, public SST::Core::Serialization::serializable_type<topo_mesh_event> {
public:
    int dimensions;
    int routing_dim;
    int* dest_loc;

    topo_mesh_event() {}
    topo_mesh_event(int dim) {	dimensions = dim; routing_dim = 0; dest_loc = new int[dim]; }
    virtual ~topo_mesh_event() { delete[] dest_loc; }
    virtual internal_router_event* clone(void)
    {
        topo_mesh_event* tte = new topo_mesh_event(*this);
        tte->dest_loc = new int[dimensions];
        memcpy(tte->dest_loc, dest_loc, dimensions*sizeof(int));
        return tte;
    }

    void serialize_order(SST::Core::Serialization::serializer &ser) {
        internal_router_event::serialize_order(ser);
        ser & dimensions;
        ser & routing_dim;

        if ( ser.mode() == SST::Core::Serialization::serializer::UNPACK ) {
            dest_loc = new int[dimensions];
        }

        for ( int i = 0 ; i < dimensions ; i++ ) {
            ser & dest_loc[i];
        }
    }

protected:

private:
    ImplementSerializable(topo_mesh_event)

};


class topo_mesh_init_event : public topo_mesh_event, public SST::Core::Serialization::serializable_type<topo_mesh_init_event> {
public:
    int phase;

    topo_mesh_init_event() {}
    topo_mesh_init_event(int dim) : topo_mesh_event(dim), phase(0) { }
    virtual ~topo_mesh_init_event() { }
    virtual internal_router_event* clone(void)
    {
        topo_mesh_init_event* tte = new topo_mesh_init_event(*this);
        tte->dest_loc = new int[dimensions];
        memcpy(tte->dest_loc, dest_loc, dimensions*sizeof(int));
        return tte;
    }

    void serialize_order(SST::Core::Serialization::serializer &ser) {
        topo_mesh_event::serialize_order(ser);
        ser & phase;
    }

private:
    ImplementSerializable(topo_mesh_init_event)

};


class topo_mesh: public Topology {

    int router_id;
    int* id_loc;

    int dimensions;
    int* dim_size;
    int* dim_width;

    int (* port_start)[2]; // port_start[dim][direction: 0=pos, 1=neg]

    int num_local_ports;
    int local_port_start;

public:
    topo_mesh(Component* comp, Params& params);
    ~topo_mesh();

    virtual void route(int port, int vc, internal_router_event* ev);
    virtual internal_router_event* process_input(RtrEvent* ev);

    virtual void routeInitData(int port, internal_router_event* ev, std::vector<int> &outPorts);
    virtual internal_router_event* process_InitData_input(RtrEvent* ev);

    virtual PortState getPortState(int port) const;
    virtual int computeNumVCs(int vns);
    virtual int getEndpointID(int port);

protected:
    virtual int choose_multipath(int start_port, int num_ports, int dest_dist);

private:
    void idToLocation(int id, int *location) const;
    void parseDimString(const std::string &shape, int *output) const;
    int get_dest_router(int dest_id) const;
    int get_dest_local_port(int dest_id) const;
};

}
}

#endif // COMPONENTS_MERLIN_TOPOLOGY_MESH_H
