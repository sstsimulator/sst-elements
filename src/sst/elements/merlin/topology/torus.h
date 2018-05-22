// -*- mode: c++ -*-

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


#ifndef COMPONENTS_MERLIN_TOPOLOGY_TORUS_H
#define COMPONENTS_MERLIN_TOPOLOGY_TORUS_H

#include <sst/core/elementinfo.h>
#include <sst/core/event.h>
#include <sst/core/link.h>
#include <sst/core/params.h>

#include <string.h>

#include "sst/elements/merlin/router.h"

namespace SST {
namespace Merlin {

class topo_torus_event : public internal_router_event {
public:
    int dimensions;
    int routing_dim;
    int* dest_loc;
    
    topo_torus_event() {}
    topo_torus_event(int dim) {	dimensions = dim; routing_dim = 0; dest_loc = new int[dim]; }
    ~topo_torus_event() { delete[] dest_loc; }
    virtual internal_router_event* clone(void) override
    {
        topo_torus_event* tte = new topo_torus_event(*this);
        tte->dest_loc = new int[dimensions];
        memcpy(tte->dest_loc, dest_loc, dimensions*sizeof(int));
        return tte;
    }

    void serialize_order(SST::Core::Serialization::serializer &ser)  override {
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

private:

    ImplementSerializable(SST::Merlin::topo_torus_event)
};



class topo_torus: public Topology {

public:

    SST_ELI_REGISTER_SUBCOMPONENT(
        topo_torus,
        "merlin",
        "torus",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "Multi-dimensional torus topology object",
        "SST::Merlin::Topology")
    
    SST_ELI_DOCUMENT_PARAMS(
        {"torus:shape",        "Shape of the torus specified as the number of routers in each dimension, where each dimension is separated by an x.  For example, 4x4x2x2.  Any number of dimensions is supported."},
        {"torus:width",        "Number of links between routers in each dimension, specified in same manner as for shape.  For example, 2x2x1 denotes 2 links in the x and y dimensions and one in the z dimension."},
        {"torus:local_ports",  "Number of endpoints attached to each router."},
    )

    
private:
    int router_id;
    int* id_loc;

    int dimensions;
    int* dim_size;
    int* dim_width;

    int (* port_start)[2]; // port_start[dim][direction: 0=pos, 1=neg]

    int num_local_ports;
    int local_port_start;

public:
    topo_torus(Component* comp, Params& params);
    ~topo_torus();

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

#endif // COMPONENTS_MERLIN_TOPOLOGY_TORUS_H
