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


#ifndef COMPONENTS_MERLIN_TOPOLOGY_DRAGONFLY2_H
#define COMPONENTS_MERLIN_TOPOLOGY_DRAGONFLY2_H

#include <sst/core/elementinfo.h>
#include <sst/core/event.h>
#include <sst/core/link.h>
#include <sst/core/params.h>
#include <sst/core/rng/sstrng.h>

#include "sst/elements/merlin/router.h"



namespace SST {
class SharedRegion;

namespace Merlin {

class topo_dragonfly2_event;

struct RouterPortPair2 {
    uint16_t router;
    uint16_t port;

    RouterPortPair2(int router, int port) :
        router(router),
        port(port)
        {}

    RouterPortPair2() {}
};

class RouteToGroup2 {
private:
    const RouterPortPair2* data;
    SharedRegion* region;
    size_t groups;
    size_t routes;
    
    
public:
    RouteToGroup2() {}

    void init(SharedRegion* sr, size_t g, size_t r);

    const RouterPortPair2& getRouterPortPair(int group, int route_number);

    void setRouterPortPair(int group, int route_number, const RouterPortPair2& pair);
};


class topo_dragonfly2: public Topology {

public:

    SST_ELI_REGISTER_SUBCOMPONENT(
        topo_dragonfly2,
        "merlin",
        "dragonfly2",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "Dragonfly2 topology object.  Implements a dragonfly with a single all to all pattern within the group.",
        "SST::Merlin::Topology")
    
    SST_ELI_DOCUMENT_PARAMS(
        {"dragonfly:hosts_per_router",      "Number of hosts connected to each router."},
        {"dragonfly:routers_per_group",     "Number of links used to connect to routers in same group."},
        {"dragonfly:intergroup_per_router", "Number of links per router connected to other groups."},
        {"dragonfly:intergroup_links",      "Number of links between each pair of groups."},
        {"dragonfly:num_groups",            "Number of groups in network."},
        {"dragonfly:algorithm",             "Routing algorithm to use [minmal (default) | valiant].", "minimal"},
        {"dragonfly:adaptive_threshold",    "Threshold to use when make adaptive routing decisions.", "2.0"},
        {"dragonfly:global_link_map",       "Array specifying connectivity of global links in each dragonfly group."},
        {"dragonfly:global_route_mode",     "Mode for intepreting global link map [absolute (default) | relative].","absolute"},
    )

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

    RouteToGroup2 group_to_global_port;
    
    struct dgnfly2Params params;
    RouteAlgo algorithm;
    double adaptive_threshold;
    uint32_t group_id;
    uint32_t router_id;

    RNG::SSTRandom* rng;

    int const* output_credits;
    int num_vcs;
    
    enum global_route_mode_t { ABSOLUTE, RELATIVE };
    global_route_mode_t global_route_mode;

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

    topo_dragonfly2_event() { }
    topo_dragonfly2_event(const topo_dragonfly2::dgnfly2Addr &dest) :
        dest(dest), global_slice(0)
        {}
    ~topo_dragonfly2_event() { }

    virtual internal_router_event *clone(void) override
    {
        return new topo_dragonfly2_event(*this);
    }

    void serialize_order(SST::Core::Serialization::serializer &ser)  override {
        internal_router_event::serialize_order(ser);
        ser & src_group;
        ser & dest.group;
        ser & dest.mid_group;
        ser & dest.mid_group_shadow;
        ser & dest.router;
        ser & dest.host;
        ser & global_slice;
        ser & global_slice_shadow;
    }

private:
    ImplementSerializable(SST::Merlin::topo_dragonfly2_event)

};

}
}

#endif // COMPONENTS_MERLIN_TOPOLOGY_DRAGONFLY2_H
