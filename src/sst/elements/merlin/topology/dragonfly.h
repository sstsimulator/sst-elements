// -*- mode: c++ -*-

// Copyright 2009-2021 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2021, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#ifndef COMPONENTS_MERLIN_TOPOLOGY_DRAGONFLY_H
#define COMPONENTS_MERLIN_TOPOLOGY_DRAGONFLY_H

#include <algorithm>

#include <sst/core/event.h>
#include <sst/core/link.h>
#include <sst/core/params.h>
#include <sst/core/rng/sstrng.h>

#include "sst/elements/merlin/router.h"



namespace SST {
class SharedRegion;

namespace Merlin {

class topo_dragonfly_event;


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
    uint32_t n;  /* # of links between groups in a pair */
};

enum global_route_mode_t { ABSOLUTE, RELATIVE };


struct RouterPortPair : public SST::Core::Serialization::serializable {
    uint16_t router;
    uint16_t port;

    RouterPortPair(int router, int port) :
        router(router),
        port(port)
        {}

    RouterPortPair() :
        router(0),
        port(0)
        {}

    bool operator==(const RouterPortPair& rhs) {
        if ( router != rhs.router || port != rhs.port ) return false;
        return true;
    }

    bool operator!=(const RouterPortPair& rhs) {
        if ( router == rhs.router && port == rhs.port ) return false;
        return true;
    }

    void serialize_order(SST::Core::Serialization::serializer &ser)  override {
        ser & router;
        ser & port;
    }

private:
    ImplementSerializable(SST::Merlin::RouterPortPair)

};


// Class to parse the failed link format.  Designed to be used with
// Paras::find_array<FailedLink>().
struct FailedLink {
    uint16_t low_group;
    uint16_t high_group;
    uint16_t slice;

    // Format for string is group1:group2:slice
    FailedLink(const std::string& format) {
        size_t start = 0;

        size_t index = format.find_first_of(":");
        uint16_t value1 = SST::Core::from_string<uint16_t>(format.substr(0,index));
        start = index + 1;

        index = format.find_first_of(":",start);
        uint16_t value2 = SST::Core::from_string<uint16_t>(format.substr(start,index-start));
        start = index + 1;

        low_group = value1 < value2 ? value1 : value2;
        high_group = value1 < value2 ? value2 : value1;

        slice = SST::Core::from_string<uint16_t>(format.substr(start,std::string::npos));
    }
};



class RouteToGroup {
private:
    Shared::SharedArray<RouterPortPair> data;
    //const RouterPortPair* data;
    Shared::SharedArray<bool> failed_links;
    // const uint8_t* link_counts;
    Shared::SharedArray<uint8_t> link_counts;
    size_t groups;  // Number of groups
    size_t routers; // Number of routers per groupt
    size_t slices;  // number of links between each pair of groups
    size_t links;   // number global links per router
    int gid;        // group id
    int global_start;  // start of global ports
    global_route_mode_t mode;   // routing mode
    bool consider_failed_links; // whether or not we are simulating failed links

public:
    RouteToGroup() {}

    // void init(SharedRegion* sr, size_t g, size_t r);
    void init_write(const std::string& basename, int group_id, global_route_mode_t route_mode,
                    const dgnflyParams& params, const std::vector<int64_t>& global_link_map,
                    bool config_failed_links, const std::vector<FailedLink>& failed_links);

    void init(const std::string& basename, int group_id, global_route_mode_t route_mode,
              const dgnflyParams& params, bool config_failed_links);

    const RouterPortPair& getRouterPortPair(int group, int route_number) const;
    const RouterPortPair& getRouterPortPairForGroup(uint32_t src_group, uint32_t dest_group, uint32_t slice) const;

    // const RouterPortPair& getRouterPortPair(int src_group, int dest_group, int route_number);
    // void setRouterPortPair(int group, int route_number, const RouterPortPair& pair);

    int getValiantGroup(int dest_group, RNG::SSTRandom* rng) const;

    inline uint8_t getLinkCount(int src_group, int dest_group) const {
        return link_counts[src_group * groups + dest_group];
    }

    inline bool isFailedPort(const RouterPortPair& rp ) const {
        return isFailedPortForGroup(gid,rp);
    }

    inline bool isFailedPortForGroup(uint32_t src_group, const RouterPortPair& rp ) const {
        if ( !consider_failed_links ) return false;
        // If the SharedArray is ready yet, then we are still in
        // construct phase and just return false for now.
        if ( failed_links.size() == 0 ) return false;
        return failed_links[(src_group * routers * links) + (rp.router * links) + rp.port - global_start];
    }
};



class topo_dragonfly: public Topology {

public:

    SST_ELI_REGISTER_SUBCOMPONENT_DERIVED(
        topo_dragonfly,
        "merlin",
        "dragonfly",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "Dragonfly topology object.  Implements a dragonfly with a single all to all pattern within the group.",
        SST::Merlin::Topology)

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

        {"hosts_per_router",      "Number of hosts connected to each router."},
        {"routers_per_group",     "Number of links used to connect to routers in same group."},
        {"intergroup_per_router", "Number of links per router connected to other groups."},
        {"intergroup_links",      "Number of links between each pair of groups."},
        {"num_groups",            "Number of groups in network."},
        {"algorithm",             "Routing algorithm to use [minmal (default) | valiant].", "minimal"},
        {"adaptive_threshold",    "Threshold to use when make adaptive routing decisions.", "2.0"},
        {"global_link_map",       "Array specifying connectivity of global links in each dragonfly group."},
        {"global_route_mode",     "Mode for intepreting global link map [absolute (default) | relative].","absolute"},
        {"config_failed_links",   "Controls whether or not failed links are considered","False"},
        {"failed_links",          "List of global links to mark as failed.  Only needs to be passed to router 0. Format is \"group1:group2:slice\"",""},
    )

    enum RouteAlgo {
        MINIMAL,
        VALIANT,
        ADAPTIVE_LOCAL,
        UGAL,
        MIN_A
    };

    RouteToGroup group_to_global_port;


    struct dgnflyParams params;
    double adaptive_threshold;
    uint32_t group_id;
    // Router id within group
    uint32_t router_id;

    // Actual id of router
    uint32_t rtr_id;

    RNG::SSTRandom* rng;

    int const* output_credits;
    int const* output_queue_lengths;
    int num_vcs;
    int num_vns;

    global_route_mode_t global_route_mode;

public:
    struct dgnflyAddr {
        uint32_t group;
        uint32_t mid_group;
        uint32_t mid_group_shadow;
        uint32_t router;
        uint32_t host;
    };

    topo_dragonfly(ComponentId_t cid, Params& p, int num_ports, int rtr_id, int num_vns);
    ~topo_dragonfly();

    virtual void route_packet(int port, int vc, internal_router_event* ev);
    virtual internal_router_event* process_input(RtrEvent* ev);

    virtual std::pair<int,int> getDeliveryPortForEndpointID(int ep_id);
    virtual int routeControlPacket(CtrlRtrEvent* ev);

    virtual PortState getPortState(int port) const;
    virtual std::string getPortLogicalGroup(int port) const;

    virtual void routeInitData(int port, internal_router_event* ev, std::vector<int> &outPorts);
    virtual internal_router_event* process_InitData_input(RtrEvent* ev);

    virtual void getVCsPerVN(std::vector<int>& vcs_per_vn) {
        for ( int i = 0; i < num_vns; ++i ) {
            vcs_per_vn[i] = vns[i].num_vcs;
        }
    }

    virtual int getEndpointID(int port);

    virtual void setOutputBufferCreditArray(int const* array, int vcs);
    virtual void setOutputQueueLengthsArray(int const* array, int vcs);

private:
    void idToLocation(int id, dgnflyAddr *location);
    int32_t router_to_group(uint32_t group);
    int32_t port_for_router(uint32_t router);
    int32_t port_for_group(uint32_t group, uint32_t global_slice, int id = -1);
    int32_t port_for_group_init(uint32_t group, uint32_t global_slice);
    int32_t hops_to_router(uint32_t group, uint32_t router, uint32_t slice);

    inline bool is_port_endpoint(uint32_t port) const { return ( port < params.p ); }
    inline bool is_port_local_group(uint32_t port) const { return (port >= params.p && port < (params.p + params.a -1 )); }
    inline bool is_port_global(uint32_t port) const { return ( port >= params.p + params.a - 1 ); }

    struct vn_info {
        int start_vc;
        int num_vcs;
        int bias;
        RouteAlgo algorithm;
    };

    vn_info* vns;

    void route_nonadaptive(int port, int vc, internal_router_event* ev);
    void route_adaptive_local(int port, int vc, internal_router_event* ev);
    void route_ugal(int port, int vc, internal_router_event* ev);
    void route_mina(int port, int vc, internal_router_event* ev);


};




class topo_dragonfly_event : public internal_router_event {

public:
    uint32_t src_group;
    topo_dragonfly::dgnflyAddr dest;
    uint16_t global_slice;
    uint16_t global_slice_shadow;

    topo_dragonfly_event() { }
    topo_dragonfly_event(const topo_dragonfly::dgnflyAddr &dest) :
        dest(dest), global_slice(0)
        {}
    ~topo_dragonfly_event() { }

    virtual internal_router_event *clone(void) override
    {
        return new topo_dragonfly_event(*this);
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
    ImplementSerializable(SST::Merlin::topo_dragonfly_event)

};

}
}

#endif // COMPONENTS_MERLIN_TOPOLOGY_DRAGONFLY_H
