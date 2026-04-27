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


#ifndef COMPONENTS_MERLIN_TOPOLOGY_HYPERX_H
#define COMPONENTS_MERLIN_TOPOLOGY_HYPERX_H

#include <sst/core/event.h>
#include <sst/core/link.h>
#include <sst/core/params.h>
#include <sst/core/rng/rng.h>

#include <string.h>
#include <vector>

#include "sst/elements/merlin/router.h"

namespace SST {
namespace Merlin {

class topo_hyperx_event : public internal_router_event {
public:
    int dimensions = 0;
    // First non aligned dimension
    int last_routing_dim = -1;
    int* dest_loc = nullptr;
    bool val_route_dest = false;
    int* val_loc = nullptr;

    id_type id;
    bool rerouted = false;

    topo_hyperx_event() = default;

    topo_hyperx_event(int dim) :
        internal_router_event(),
        dimensions(dim)
    {
        dest_loc = new int[dim];
        val_loc = new int[dim];
        id = generateUniqueId();
    }
    virtual ~topo_hyperx_event() { delete[] dest_loc; delete[] val_loc; }
    virtual internal_router_event* clone(void) override
    {
        topo_hyperx_event* tte = new topo_hyperx_event(*this);
        tte->dest_loc = new int[dimensions];
        memcpy(tte->dest_loc, dest_loc, dimensions*sizeof(int));
        return tte;
    }

    void getUnalignedDimensions(int* curr_loc, std::vector<int>& dims) {
        for (int i = 0; i < dimensions; ++i ) {
            if ( dest_loc[i] != curr_loc[i] ) dims.push_back(i);
        }
    }

    void serialize_order(SST::Core::Serialization::serializer &ser)  override {
        internal_router_event::serialize_order(ser);
        SST_SER(dimensions);
        SST_SER(last_routing_dim);

        SST_SER(SST::Core::Serialization::array(dest_loc, dimensions));
        SST_SER(SST::Core::Serialization::array(val_loc, dimensions));

        SST_SER(val_route_dest);
        SST_SER(id);
        SST_SER(rerouted);
    }

protected:

private:
    ImplementSerializable(SST::Merlin::topo_hyperx_event)

};


class topo_hyperx_init_event : public topo_hyperx_event {
public:
    int phase = 0;

    topo_hyperx_init_event() = default;
    topo_hyperx_init_event(int dim) : topo_hyperx_event(dim), phase(0) { }
    virtual ~topo_hyperx_init_event() { }
    virtual internal_router_event* clone(void) override
    {
        topo_hyperx_init_event* tte = new topo_hyperx_init_event(*this);
        tte->dest_loc = new int[dimensions];
        tte->val_loc = new int[dimensions];
        memcpy(tte->dest_loc, dest_loc, dimensions*sizeof(int));
        return tte;
    }

    void serialize_order(SST::Core::Serialization::serializer &ser)  override {
        topo_hyperx_event::serialize_order(ser);
        SST_SER(phase);
    }

private:
    ImplementSerializable(SST::Merlin::topo_hyperx_init_event)

};


class RNGFunc {
    RNG::Random* rng = nullptr;

public:
    RNGFunc() = default;
    RNGFunc(RNG::Random* rng) : rng(rng) {}

    int operator() (int i) {
        return rng->generateNextUInt32() % i;
    }

    void serialize_order(SST::Core::Serialization::serializer& ser)
    {
        SST_SER(rng);
    }
};

class topo_hyperx: public Topology {

public:

    SST_ELI_REGISTER_SUBCOMPONENT(
        topo_hyperx,
        "merlin",
        "hyperx",
        SST_ELI_ELEMENT_VERSION(0,1,0),
        "Multi-dimensional hyperx topology object",
        SST::Merlin::Topology
    )

    SST_ELI_DOCUMENT_PARAMS(
        // Parameters needed for use with old merlin python module
        {"hyperx.shape", "Shape of the mesh specified as the number of routers in each dimension, where each dimension "
                         "is separated by a colon.  For example, 4x4x2x2.  Any number of dimensions is supported."},
        {"hyperx.width", "Number of links between routers in each dimension, specified in same manner as for shape.  "
                         "For example, 2x2x1 denotes 2 links in the x and y dimensions and one in the z dimension."},
        {"hyperx.local_ports",  "Number of endpoints attached to each router."},
        {"hyperx.algorithm",    "Routing algorithm to use.", "DOR"},


        {"shape", "Shape of the mesh specified as the number of routers in each dimension, where each dimension "
                  "is separated by a colon.  For example, 4x4x2x2.  Any number of dimensions is supported."},
        {"width", "Number of links between routers in each dimension, specified in same manner as for shape.  "
                  "For example, 2x2x1 denotes 2 links in the x and y dimensions and one in the z dimension."},
        {"local_ports", "Number of endpoints attached to each router."},
        {"algorithm", "Routing algorithm to use.", "DOR"}
    )

    enum RouteAlgo {
        DOR,
        DORND,
        MINA,
        VALIANT,
        DOAL,
        VDAL
    };

private:
    int router_id = -1;
    int* id_loc = nullptr;

    int dimensions = 0;
    int* dim_size = nullptr;
    int* dim_width = nullptr;
    int total_routers = 0;

    int* port_start = nullptr; // where does each dimension start

    int num_local_ports = 0;
    int local_port_start = 0;

    int const* output_credits;
    int const* output_queue_lengths;
    int num_vcs = 0;
    int num_vns = 0;

    RNG::Random* rng = nullptr;
    RNGFunc* rng_func = nullptr;

    struct vn_info
    {
        int start_vc;
        int num_vcs;
        RouteAlgo algorithm = DOR;

        void serialize_order(SST::Core::Serialization::serializer& ser)
        {
            SST_SER(start_vc);
            SST_SER(num_vcs);
            SST_SER(algorithm);
        }

    };

    vn_info* vns = nullptr;


public:
    topo_hyperx(ComponentId_t cid, Params& p, int num_ports, int rtr_id, int num_vns);
    ~topo_hyperx();

    topo_hyperx() : Topology() {}

    void serialize_order(SST::Core::Serialization::serializer& ser) override;

    ImplementSerializable(SST::Merlin::topo_hyperx)

    virtual void route_packet(int port, int vc, internal_router_event* ev) override;
    virtual internal_router_event* process_input(RtrEvent* ev) override;

    virtual void routeUntimedData(int port, internal_router_event* ev, std::vector<int> &outPorts) override;
    virtual internal_router_event* process_UntimedData_input(RtrEvent* ev) override;

    virtual PortState getPortState(int port) const override;
    virtual int getEndpointID(int port) override;

    virtual void setOutputBufferCreditArray(int const* array, int vcs) override;
    virtual void setOutputQueueLengthsArray(int const* array, int vcs) override;

    virtual void getVCsPerVN(std::vector<int>& vcs_per_vn)  override
    {
        for ( int i = 0; i < num_vns; ++i ) {
            vcs_per_vn[i] = vns[i].num_vcs;
        }
    }

protected:
    virtual int choose_multipath(int start_port, int num_ports);

private:
    void idToLocation(int id, int *location) const;
    void parseDimString(const std::string &shape, int *output) const;
    int get_dest_router(int dest_id) const;
    int get_dest_local_port(int dest_id) const;

    std::pair<int,int> routeDORBase(int* dest_loc);
    void routeDOR(int port, int vc, topo_hyperx_event* ev);
    void routeDORND(int port, int vc, topo_hyperx_event* ev);
    void routeMINA(int port, int vc, topo_hyperx_event* ev);
    void routeDOAL(int port, int vc, topo_hyperx_event* ev);
    void routeVDAL(int port, int vc, topo_hyperx_event* ev);
    void routeValiant(int port, int vc, topo_hyperx_event* ev);
};

}
}

#endif // COMPONENTS_MERLIN_TOPOLOGY_MESH_H
