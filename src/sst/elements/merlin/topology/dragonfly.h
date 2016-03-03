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


#ifndef COMPONENTS_MERLIN_TOPOLOGY_DRAGONFLY_H
#define COMPONENTS_MERLIN_TOPOLOGY_DRAGONFLY_H

#include <sst/core/event.h>
#include <sst/core/link.h>
#include <sst/core/params.h>

#include "sst/elements/merlin/router.h"

namespace SST {
namespace Merlin {

class topo_dragonfly_event;

class topo_dragonfly: public Topology {
    /* Assumed connectivity of each router:
     * ports [0, p-1]:      Hosts
     * ports [p, p+a-2]:    Intra-group
     * ports [p+a-1, k-1]:  Inter-group
     */

    struct dgnflyParams {
        uint32_t p;  /* # of hosts / router */
        uint32_t a;  /* # of routers / group */
        uint32_t k;  /* Router Radix */
        uint32_t h;  /* # of ports / router to connect to other groups */
        uint32_t g;  /* # of Groups */
    };

    enum RouteAlgo {
        MINIMAL,
        VALIANT
    };


    struct dgnflyParams params;
    RouteAlgo algorithm;
    uint32_t group_id;
    uint32_t router_id;

public:
    struct dgnflyAddr {
        uint32_t group;
        uint32_t mid_group;
        uint32_t router;
        uint32_t host;
    };

    topo_dragonfly(Component* comp, Params& p);
    ~topo_dragonfly();

    virtual void route(int port, int vc, internal_router_event* ev);
    virtual internal_router_event* process_input(RtrEvent* ev);

    virtual PortState getPortState(int port) const;
    virtual std::string getPortLogicalGroup(int port) const;

    virtual void routeInitData(int port, internal_router_event* ev, std::vector<int> &outPorts);
    virtual internal_router_event* process_InitData_input(RtrEvent* ev);

    virtual int computeNumVCs(int vns) { return vns * 3; }
    virtual int getEndpointID(int port);

private:
    void idToLocation(int id, dgnflyAddr *location) const;
    uint32_t router_to_group(uint32_t group) const;
    uint32_t port_for_router(uint32_t router) const;
    uint32_t port_for_group(uint32_t group) const;
};




class topo_dragonfly_event : public internal_router_event {

public:
    uint32_t src_group;
    topo_dragonfly::dgnflyAddr dest;

    topo_dragonfly_event(const topo_dragonfly::dgnflyAddr &dest) : dest(dest) {}
    ~topo_dragonfly_event() { }

    virtual internal_router_event *clone(void)
    {
        return new topo_dragonfly_event(*this);
    }

private:
    topo_dragonfly_event() { }
	friend class boost::serialization::access;
	template<class Archive>
	void
	serialize(Archive & ar, const unsigned int version )
	{
		ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(SST::Merlin::internal_router_event);
		ar & BOOST_SERIALIZATION_NVP(src_group);
		ar & BOOST_SERIALIZATION_NVP(dest.group);
		ar & BOOST_SERIALIZATION_NVP(dest.mid_group);
		ar & BOOST_SERIALIZATION_NVP(dest.router);
		ar & BOOST_SERIALIZATION_NVP(dest.host);
    }
};

}
}

#endif // COMPONENTS_MERLIN_TOPOLOGY_DRAGONFLY_H
