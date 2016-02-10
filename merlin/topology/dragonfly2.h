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


#ifndef COMPONENTS_MERLIN_TOPOLOGY_DRAGONFLY2_H
#define COMPONENTS_MERLIN_TOPOLOGY_DRAGONFLY2_H

#include <sst/core/event.h>
#include <sst/core/link.h>
#include <sst/core/params.h>
#include <sst/core/rng/sstrng.h>

#include "sst/elements/merlin/router.h"



namespace SST {
class SharedRegion;

namespace Merlin {

class topo_dragonfly2_event;

struct RouterPortPair {
    uint16_t router;
    uint16_t port;

    RouterPortPair(int router, int port) :
        router(router),
        port(port)
        {}

    RouterPortPair() {}
};

class RouteToGroup {
private:
    const RouterPortPair* data;
    SharedRegion* region;
    size_t groups;
    size_t routes;

    
public:
    RouteToGroup() {}

    void init(SharedRegion* sr, size_t g, size_t r);

    const RouterPortPair& getRouterPortPair(int group, int route_number);

    void setRouterPortPair(int group, int route_number, const RouterPortPair& pair);
};

class topo_dragonfly2: public Topology {
    /* Assumed connectivity of each router:
     * ports [0, p-1]:      Hosts
     * ports [p, p+a-2]:    Intra-group
     * ports [p+a-1, k-1]:  Inter-group
     */

    struct dgnfly2Params {
        uint32_t p;  /* # of hosts / router */
        uint32_t a;  /* # of routers / group */
        uint32_t k;  /* Router Radix */
        uint32_t h;  /* # of ports / router to connect to other groups */
        uint32_t g;  /* # of Groups */
        uint32_t n;  /* # of links between groups in a pair */
    };

    enum RouteAlgo {
        MINIMAL,
        VALIANT,
        ADAPTIVE_LOCAL
    };

    RouteToGroup group_to_global_port;
    
    struct dgnfly2Params params;
    RouteAlgo algorithm;
    double adaptive_threshold;
    uint32_t group_id;
    uint32_t router_id;

    RNG::SSTRandom* rng;

    int const* output_credits;
    int num_vcs;
    
public:
    struct dgnfly2Addr {
        uint32_t group;
        uint32_t mid_group;
        uint32_t mid_group_shadow;
        uint32_t router;
        uint32_t host;
    };

    topo_dragonfly2(Component* comp, Params& p);
    ~topo_dragonfly2();

    virtual void route(int port, int vc, internal_router_event* ev);
    virtual void reroute(int port, int vc, internal_router_event* ev);
    virtual internal_router_event* process_input(RtrEvent* ev);

    virtual PortState getPortState(int port) const;
    virtual std::string getPortLogicalGroup(int port) const;

    virtual void routeInitData(int port, internal_router_event* ev, std::vector<int> &outPorts);
    virtual internal_router_event* process_InitData_input(RtrEvent* ev);

    virtual int computeNumVCs(int vns) { return vns * 3; }
    virtual int getEndpointID(int port);

    virtual void setOutputBufferCreditArray(int const* array, int vcs);
    
private:
    void idToLocation(int id, dgnfly2Addr *location);
    uint32_t router_to_group(uint32_t group);
    uint32_t port_for_router(uint32_t router);
    uint32_t port_for_group(uint32_t group, uint32_t global_slice, int id = -1);
};




class topo_dragonfly2_event : public internal_router_event {

public:
    uint32_t src_group;
    topo_dragonfly2::dgnfly2Addr dest;
    uint16_t global_slice;
    uint16_t global_slice_shadow;

    topo_dragonfly2_event(const topo_dragonfly2::dgnfly2Addr &dest) : dest(dest) {}
    ~topo_dragonfly2_event() { }

    virtual internal_router_event *clone(void)
    {
        return new topo_dragonfly2_event(*this);
    }

private:
    topo_dragonfly2_event() { }
	friend class boost::serialization::access;
	template<class Archive>
	void
	serialize(Archive & ar, const unsigned int version )
	{
		ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(SST::Merlin::internal_router_event);
		ar & BOOST_SERIALIZATION_NVP(src_group);
		ar & BOOST_SERIALIZATION_NVP(dest.group);
		ar & BOOST_SERIALIZATION_NVP(dest.mid_group);
		ar & BOOST_SERIALIZATION_NVP(dest.mid_group_shadow);
		ar & BOOST_SERIALIZATION_NVP(dest.router);
		ar & BOOST_SERIALIZATION_NVP(dest.host);
        ar & BOOST_SERIALIZATION_NVP(global_slice);
        ar & BOOST_SERIALIZATION_NVP(global_slice_shadow);
    }
};

}
}

#endif // COMPONENTS_MERLIN_TOPOLOGY_DRAGONFLY2_H
