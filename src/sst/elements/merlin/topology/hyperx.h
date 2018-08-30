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


#ifndef COMPONENTS_MERLIN_TOPOLOGY_HYPERX_H
#define COMPONENTS_MERLIN_TOPOLOGY_HYPERX_H

#include <sst/core/elementinfo.h>
#include <sst/core/event.h>
#include <sst/core/link.h>
#include <sst/core/params.h>
#include <sst/core/rng/sstrng.h>

#include <string.h>
#include <vector>

#include "sst/elements/merlin/router.h"

namespace SST {
namespace Merlin {

class topo_hyperx_event : public internal_router_event {
public:
    int dimensions;
    // First non aligned dimension
    int last_routing_dim;
    int* dest_loc;
    bool val_route_dest;
    int* val_loc;
    
    id_type id;
    bool rerouted;
    
    topo_hyperx_event() : internal_router_event() {}
    topo_hyperx_event(int dim) :
        internal_router_event(),
        dimensions(dim),
        last_routing_dim(-1),
        val_route_dest(false)
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
        ser & dimensions;
        ser & last_routing_dim;

        if ( ser.mode() == SST::Core::Serialization::serializer::UNPACK ) {
            dest_loc = new int[dimensions];
        }

        for ( int i = 0 ; i < dimensions ; i++ ) {
            ser & dest_loc[i];
        }

        if ( ser.mode() == SST::Core::Serialization::serializer::UNPACK ) {
            val_loc = new int[dimensions];
        }

        for ( int i = 0 ; i < dimensions ; i++ ) {
            ser & val_loc[i];
        }

        ser & val_route_dest;
        ser & id;
        ser & rerouted;
    }

protected:

private:
    ImplementSerializable(SST::Merlin::topo_hyperx_event)

};


class topo_hyperx_init_event : public topo_hyperx_event {
public:
    int phase;

    topo_hyperx_init_event() : topo_hyperx_event() {}
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
        ser & phase;
    }

private:
    ImplementSerializable(SST::Merlin::topo_hyperx_init_event)

};


class RNGFunc {
    RNG::SSTRandom* rng;

public:
    RNGFunc(RNG::SSTRandom* rng) : rng(rng) {}

    int operator() (int i) {
        return rng->generateNextUInt32() % i;
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
        "SST::Merlin::Topology")
    
    SST_ELI_DOCUMENT_PARAMS(
        {"hyperx:shape",        "Shape of the mesh specified as the number of routers in each dimension, where each dimension is separated by a colon.  For example, 4x4x2x2.  Any number of dimensions is supported."},
        {"hyperx:width",        "Number of links between routers in each dimension, specified in same manner as for shape.  For example, 2x2x1 denotes 2 links in the x and y dimensions and one in the z dimension."},
        {"hyperx:local_ports",  "Number of endpoints attached to each router."},
        {"hyperx:algorithm",    "Routing algorithm to use.", "DOR"}
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
    int router_id;
    int* id_loc;

    int dimensions;
    int* dim_size;
    int* dim_width;
    int total_routers;
    
    int* port_start; // where does each dimension start

    int num_local_ports;
    int local_port_start;

    int const* output_credits;
    int const* output_queue_lengths;
    int num_vcs;
    
    RouteAlgo algorithm;
    RNG::SSTRandom* rng;
    RNGFunc* rng_func;

public:
    topo_hyperx(Component* comp, Params& params);
    ~topo_hyperx();

    virtual void route(int port, int vc, internal_router_event* ev);
    virtual void reroute(int port, int vc, internal_router_event* ev);
    virtual internal_router_event* process_input(RtrEvent* ev);

    virtual void routeInitData(int port, internal_router_event* ev, std::vector<int> &outPorts);
    virtual internal_router_event* process_InitData_input(RtrEvent* ev);

    virtual PortState getPortState(int port) const;
    virtual int computeNumVCs(int vns);
    virtual int getEndpointID(int port);

    virtual void setOutputBufferCreditArray(int const* array, int vcs);
    virtual void setOutputQueueLengthsArray(int const* array, int vcs);

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
