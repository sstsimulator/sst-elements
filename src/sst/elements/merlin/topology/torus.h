// -*- mode: c++ -*-

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


#ifndef COMPONENTS_MERLIN_TOPOLOGY_TORUS_H
#define COMPONENTS_MERLIN_TOPOLOGY_TORUS_H

#include <sst/core/event.h>
#include <sst/core/link.h>
#include <sst/core/params.h>

#include <string.h>

#include "sst/elements/merlin/router.h"

namespace SST {
namespace Merlin {

class topo_torus_event : public internal_router_event {
public:
    int dimensions = 0;
    int routing_dim = 0;
    int* dest_loc = nullptr;

    topo_torus_event() = default;
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
        SST_SER(dimensions);
        SST_SER(routing_dim);

        SST_SER(SST::Core::Serialization::array(dest_loc, dimensions));
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
        SST::Merlin::Topology
    )

    SST_ELI_DOCUMENT_PARAMS(
        // Parameters needed for use with old merlin python module
        {"torus.shape", "Shape of the torus specified as the number of routers in each dimension, where each "
                        "dimension is separated by an x.  For example, 4x4x2x2.  Any number of dimensions is supported."},
        {"torus.width", "Number of links between routers in each dimension, specified in same manner as for shape.  "
                        "For example, 2x2x1 denotes 2 links in the x and y dimensions and one in the z dimension."},
        {"torus.local_ports", "Number of endpoints attached to each router."},


        {"shape", "Shape of the torus specified as the number of routers in each dimension, where each dimension is "
                  "separated by an x.  For example, 4x4x2x2.  Any number of dimensions is supported."},
        {"width", "Number of links between routers in each dimension, specified in same manner as for shape.  For "
                  "example, 2x2x1 denotes 2 links in the x and y dimensions and one in the z dimension."},
        {"local_ports", "Number of endpoints attached to each router."},
    )


private:
    int router_id = -1;
    int* id_loc = nullptr;

    int dimensions = 0;
    int* dim_size = nullptr;
    int* dim_width = nullptr;

    int (* port_start)[2]; // port_start[dim][direction: 0=pos, 1=neg]

    int num_local_ports = 0;
    int local_port_start = 0;

    int num_vns = 0;

public:
    topo_torus(ComponentId_t cid, Params& params, int num_ports, int rtr_id, int num_vns);
    ~topo_torus();

    topo_torus() = default;

    void serialize_order(SST::Core::Serialization::serializer& ser) override
    {
        Topology::serialize_order(ser);
        SST_SER(router_id);
        SST_SER(dimensions);
        SST_SER(num_local_ports);
        SST_SER(local_port_start);
        SST_SER(num_vns);

        SST_SER(SST::Core::Serialization::array(id_loc, dimensions));
        SST_SER(SST::Core::Serialization::array(dim_size, dimensions));
        SST_SER(SST::Core::Serialization::array(dim_width, dimensions));
        SST_SER(SST::Core::Serialization::array(port_start, dimensions));
    }

    ImplementSerializable(SST::Merlin::topo_torus)

    virtual void route_packet(int port, int vc, internal_router_event* ev) override;
    virtual internal_router_event* process_input(RtrEvent* ev) override;

    virtual void routeUntimedData(int port, internal_router_event* ev, std::vector<int> &outPorts) override;
    virtual internal_router_event* process_UntimedData_input(RtrEvent* ev) override;

    virtual PortState getPortState(int port) const override;
    virtual int getEndpointID(int port) override;

    virtual void getVCsPerVN(std::vector<int>& vcs_per_vn) override
    {
        for ( int i = 0; i < num_vns; ++i ) {
            vcs_per_vn[i] = 2;
        }
    }

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
