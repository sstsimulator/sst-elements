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

#include <sst_config.h>
#include "sst/core/serialization.h"

#include <sst/core/element.h>
#include <sst/core/configGraph.h>
#include <sst/core/subcomponent.h>
#include <sst/core/interfaces/simpleNetwork.h>

#include "merlin.h"
#include "linkControl.h"
#include "reorderLinkControl.h"

#include "hr_router/hr_router.h"
#include "test/nic.h"
#include "test/route_test/route_test.h"
#include "test/pt2pt/pt2pt_test.h"
#include "test/bisection/bisection_test.h"
#include "test/simple_patterns/shift.h"

#include "topology/torus.h"
#include "topology/mesh.h"
#include "topology/singlerouter.h"
#include "topology/fattree.h"
#include "topology/dragonfly.h"
#include "topology/dragonfly2.h"

#include "hr_router/xbar_arb_rr.h"
#include "hr_router/xbar_arb_lru.h"
#include "hr_router/xbar_arb_age.h"
#include "hr_router/xbar_arb_rand.h"

#include "hr_router/xbar_arb_lru_infx.h"

#include "trafficgen/trafficgen.h"

#include "inspectors/circuitCounter.h"
#include "inspectors/testInspector.h"

#include "pymodule.h"

#include <stdio.h>
#include <stdarg.h>

using namespace std;
using namespace SST::Merlin;
using namespace SST::Interfaces;

static const char * router_events[] = {"merlin.RtrEvent","merlin.internal_router_event", "merlin.topologyevent", "merlin.credit_event", NULL};
static const char * nic_events[] = {"merlin.RtrEvent", "merlin.credit_event", NULL};

// hr_router element info
static Component*
create_hr_router(SST::ComponentId_t id,
	      SST::Params& params)
{
    return new hr_router( id, params );
}

static const ElementInfoParam hr_router_params[] = {
    {"id", "ID of the router."},
    {"num_ports", "Number of ports that the router has"},
    {"num_vcs", "DEPRECATED", ""},
    {"topology", "Name of the topology module that should be loaded to control routing."},
    {"xbar_arb", "Arbitration unit to be used for crossbar.","merlin.xbar_arb_lru"},
    {"link_bw", "Bandwidth of the links specified in either b/s or B/s (can include SI prefix)."},
    {"flit_size", "Flit size specified in either b or B (can include SI prefix)."},
    {"xbar_bw", "Bandwidth of the crossbar specified in either b/s or B/s (can include SI prefix)."},
    {"input_latency", "Latency of packets entering switch into input buffers.  Specified in s (can include SI prefix)."},
    {"output_latency", "Latency of packets exiting switch from output buffers.  Specified in s (can include SI prefix)."},
    {"input_buf_size", "Size of input buffers specified in b or B (can include SI prefix)."},
    {"output_buf_size", "Size of output buffers specified in b or B (can include SI prefix)."},
    {"network_inspectors", "Comma separated list of network inspectors to put on output ports.", ""},
    {"debug", "Turn on debugging for router. Set to 1 for on, 0 for off.", "0"},
    {NULL,NULL,NULL}
};

static const ElementInfoPort hr_router_ports[] = {
    {"port%(num_ports)d",  "Ports which connect to endpoints or other routers.", router_events},
    {NULL, NULL, NULL}
};

static const ElementInfoStatistic hr_router_statistics[] = {
    { "send_bit_count", "Count number of bits sent on link", "bits", 1},
    { "send_packet_count", "Count number of packets sent on link", "packets", 1},
    { "output_port_stalls", "Time output port is stalled (in units of core timebase)", "time in stalls", 1},
    { "xbar_stalls", "Count number of cycles the xbar is stalled", "cycles", 1},
    { "idle_time", "number of nanoseconds that port was idle", "nanoseconds", 1},
    { NULL, NULL, NULL, 0 }
};

// pt2pt_test element info

static Component*
create_pt2pt_test(SST::ComponentId_t id,
		  SST::Params& params)
{
    return new pt2pt_test( id, params );
}


static const ElementInfoParam pt2pt_test_params[] = {
    {"id","Network ID of endpoint."},
    {"num_vns","Number of requested virtual networks."},
    {"link_bw","Bandwidth of the router link specified in either b/s or B/s (can include SI prefix)."},
    {"packet_size","Packet size specified in either b or B (can include SI prefix)."},
    {"packets_to_send","Number of packets to send in the test."},
    {"buffer_size","Size of input and output buffers specified in b or B (can include SI prefix)."},
    {NULL,NULL,NULL}
};

static const ElementInfoPort pt2pt_test_ports[] = {
    {"rtr",  "Port that hooks up to router.", nic_events},
    {NULL, NULL, NULL}
};

// bisection_test element info

static Component*
create_bisection_test(SST::ComponentId_t id,
		  SST::Params& params)
{
    return new bisection_test( id, params );
}


static const ElementInfoParam bisection_test_params[] = {
    //    {"num_vns","Number of requested virtual networks."},
    {"num_peers","Number of peers on the network (must be even number)"},
    {"link_bw","Bandwidth of the router link specified in either b/s or B/s (can include SI prefix).","2GB/s"},
    {"packet_size","Packet size specified in either b or B (can include SI prefix).","64B"},
    {"packets_to_send","Number of packets to send in the test.", "32"},
    {"buffer_size","Size of input and output buffers specified in b or B (can include SI prefix).", "128B"},
    {"networkIF","Network interface to use.  Must inherit from SimpleNetwork", "merlin.linkcontrol"},
    {NULL,NULL,NULL}
};

static const ElementInfoPort bisection_test_ports[] = {
    {"rtr",  "Port that hooks up to router.", nic_events},
    {NULL, NULL, NULL}
};

// static const ElementInfoStatistic bisection_test_statistics[] = {
//     // This really belongs in LinkControl, but stats in modules don't work yet
//     { "packet_latency", "Histogram of latencies for received packets", "unit", 1},
//     { "send_bit_count", "Count number of bits sent on link", "unit", 1},
//     { "output_port_stalls", "Time output port is stalled (in units of core timebase)", "unit", 1},
//    { NULL, NULL, NULL, 0 }
// };

// test_nic element info
static Component*
create_test_nic(SST::ComponentId_t id,
		SST::Params& params)
{
    return new nic( id, params );
}


static const ElementInfoParam test_nic_params[] = {
    {"id","Network ID of endpoint."},
    {"num_peers","Total number of endpoints in network."},
    {"num_messages","Total number of messages to send to each endpoint."},
    {"num_vns","Number of requested virtual networks."},
    {"link_bw","Bandwidth of the router link specified in either b/s or B/s (can include SI prefix)."},
    {"topology", "Name of the topology module that should be loaded to control routing."},
    {"remap", "Creates a logical to physical mapping shifted by remap amount.", "0"},
    // {"packet_size","Packet size specified in either b or B (can include SI prefix)."},
    // {"packets_to_send","Number of packets to send in the test."},
    // {"buffer_size","Size of input and output buffers specified in b or B (can include SI prefix)."},
    {NULL,NULL,NULL}
};

static const ElementInfoPort test_nic_ports[] = {
    {"rtr",  "Port that hooks up to router.", nic_events},
    {NULL, NULL, NULL}
};

// shift_nic element info
static Component*
create_shift_nic(SST::ComponentId_t id,
		SST::Params& params)
{
    return new shift_nic( id, params );
}


static const ElementInfoParam shift_nic_params[] = {
    {"id","Network ID of endpoint."},
    {"num_peers","Total number of endpoints in network."},
    {"shift","Number of logical network endpoints to shift to use as destination for packets."},
    {"packets_to_send","Number of packets to send in the test.","10"},
    {"packet_size","Packet size specified in either b or B (can include SI prefix).","64B"},
    {"link_bw","Bandwidth of the router link specified in either b/s or B/s (can include SI prefix)."},
    {"remap", "Creates a logical to physical mapping shifted by remap amount.", "0"},
    // {"buffer_size","Size of input and output buffers specified in b or B (can include SI prefix)."},
    {NULL,NULL,NULL}
};

static const ElementInfoPort shift_nic_ports[] = {
    {"rtr",  "Port that hooks up to router.", nic_events},
    {NULL, NULL, NULL}
};

// router_test element info
static Component*
create_route_test(SST::ComponentId_t id,
		SST::Params& params)
{
    return new route_test( id, params );
}


static const ElementInfoParam route_test_params[] = {
    {"id","Network ID of endpoint."},
    {"num_peers","Total number of endpoints in network."},
    {"link_bw","Bandwidth of the router link specified in either b/s or B/s (can include SI prefix)."},
    {NULL,NULL,NULL}
};

static const ElementInfoPort route_test_ports[] = {
    {"rtr",  "Port that hooks up to router.", nic_events},
    {NULL, NULL, NULL}
};

// static const ElementInfoStatistic test_nic_statistics[] = {
//     // This really belongs in LinkControl, but stats in modules don't work yet
//     { "packet_latency", "Histogram of latencies for received packets", "unit", 1},
//     { "send_bit_count", "Count number of bits sent on link", "unit", 1},
//     { "output_port_stalls", "Time output port is stalled (in units of core timebase)", "unit", 1},
//    { NULL, NULL, NULL, 0 }
// };



// Traffic_generator
static Component*
create_traffic_generator(SST::ComponentId_t id,
        SST::Params& params)
{
    return new TrafficGen( id, params );
}

static const ElementInfoParam traffic_generator_params[] = {
    {"id","Network ID of endpoint."},
    {"num_peers","Total number of endpoints in network."},
    {"num_vns","Number of requested virtual networks."},
    {"link_bw","Bandwidth of the router link specified in either b/s or B/s (can include SI prefix)."},
    {"topology", "Name of the topology module that should be loaded to control routing."},
    {"buffer_length", "Length of input and output buffers.","1kB"},
    {"packets_to_send","Number of packets to send in the test.","1000"},
    {"packet_size","Packet size specified in either b or B (can include SI prefix).","5"},
    {"delay_between_packets","","0"},
    {"message_rate","","1GHz"},
    {"PacketDest:pattern","Address pattern to be used (NearestNeighbor, Uniform, HotSpot, Normal, Binomial)",NULL},
    {"PacketDest:Seed", "Sets the seed of the RNG", "11" },
    {"PacketDest:RangeMax","Minumum address to send packets.","0"},
    {"PacketDest:RangeMin","Maximum address to send packets.","INT_MAX"},
    {"PacketDest:NearestNeighbor:3DSize", "For Nearest Neighbors, the 3D size \"x y z\" of the mesh", ""},
    {"PacketDest:HotSpot:target", "For HotSpot, which node is the target", ""},
    {"PacketDest:HotSpot:targetProbability", "For HotSpot, with what probability is the target targeted", ""},
    {"PacketDest:Normal:Mean", "In a normal distribution, the mean", ""},
    {"PacketDest:Normal:Sigma", "In a normal distribution, the mean variance", ""},
    {"PacketDest:Binomial:Mean", "In a binomial distribution, the mean", ""},
    {"PacketDest:Binomial:Sigma", "In a binomial distribution, the variance", ""},
    {"PacketSize:pattern","Address pattern to be used (Uniform, HotSpot, Normal, Binomial)",NULL},
    {"PacketSize:Seed", "Sets the seed of the RNG", "11" },
    {"PacketSize:RangeMax","Minumum size of packets.","0"},
    {"PacketSize:RangeMin","Maximum size of packets.","INT_MAX"},
    {"PacketSize:HotSpot:target", "For HotSpot, the target packet size", ""},
    {"PacketSize:HotSpot:targetProbability", "For HotSpot, with what probability is the target targeted", ""},
    {"PacketSize:Normal:Mean", "In a normal distribution, the mean", ""},
    {"PacketSize:Normal:Sigma", "In a normal distribution, the mean variance", "1.0"},
    {"PacketSize:Binomial:Mean", "In a binomial distribution, the mean", ""},
    {"PacketSize:Binomial:Sigma", "In a binomial distribution, the variance", "0.5"},
    {"PacketDelay:pattern","Address pattern to be used (Uniform, HotSpot, Normal, Binomial)",NULL},
    {"PacketDelay:Seed", "Sets the seed of the RNG", "11" },
    {"PacketDelay:RangeMax","Minumum delay between packets.","0"},
    {"PacketDelay:RangeMin","Maximum delay between packets.","INT_MAX"},
    {"PacketDelay:HotSpot:target", "For HotSpot, the target packet delay", ""},
    {"PacketDelay:HotSpot:targetProbability", "For HotSpot, with what probability is the target targeted", ""},
    {"PacketDelay:Normal:Mean", "In a normal distribution, the mean", ""},
    {"PacketDelay:Normal:Sigma", "In a normal distribution, the mean variance", "1.0"},
    {"PacketDelay:Binomial:Mean", "In a binomial distribution, the mean", ""},
    {"PacketDelay:Binomial:Sigma", "In a binomial distribution, the variance", "0.5"},
    {NULL,NULL,NULL}
};

static const ElementInfoPort traffic_generator_ports[] = {
    {"rtr",  "Port that hooks up to router.", nic_events},
    {NULL, NULL, NULL}
};

// static const ElementInfoStatistic traffic_generator_statistics[] = {
//     // This really belongs in LinkControl, but stats in modules don't work yet
//     { "packet_latency", "Histogram of latencies for received packets", "unit", 1},
//     { "send_bit_count", "Count number of bits sent on link", "unit", 1},
//     { "output_port_stalls", "Time output port is stalled (in units of core timebase)", "unit", 1},
//    { NULL, NULL, NULL, 0 }
// };

// MODULES

// topo_torus
static Module*
load_torus_topology(Component* comp, Params& params)
{
    return new topo_torus(comp,params);
}

static const ElementInfoParam torus_params[] = {
    {"torus:shape","Shape of the torus specified as the number of routers in each dimension, where each dimension is separated by a colon.  For example, 4x4x2x2.  Any number of dimensions is supported."},
    {"torus:width","Number of links between routers in each dimension, specified in same manner as for shape.  For example, 2x2x1 denotes 2 links in the x and y dimensions and one in the z dimension."},
    {"torus:local_ports","Number of endpoints attached to each router."},
    {NULL,NULL,NULL}
};


// topo_mesh
static Module*
load_mesh_topology(Component* comp, Params& params)
{
    return new topo_mesh(comp,params);
}

static const ElementInfoParam mesh_params[] = {
    {"mesh:shape","Shape of the mesh specified as the number of routers in each dimension, where each dimension is separated by a colon.  For example, 4x4x2x2.  Any number of dimensions is supported."},
    {"mesh:width","Number of links between routers in each dimension, specified in same manner as for shape.  For example, 2x2x1 denotes 2 links in the x and y dimensions and one in the z dimension."},
    {"mesh:local_ports","Number of endpoints attached to each router."},
    {NULL,NULL,NULL}
};


// topo tree
static Module*
load_fattree_topology(Component* comp, Params& params)
{
    return new topo_fattree(comp,params);
}

static const ElementInfoParam fattree_params[] = {
    {"fattree:shape","Shape of the fattree"},
    {"fattree:routing_alg","Routing algorithm to use. [deterministic | adaptive]","deterministic"},
    {"fattree:adaptive_threshold","Threshold used to determine if a packet will adaptively route."},
    {NULL,NULL,NULL}
};

// topo dragonfly
static Module*
load_dragonfly_topology(Component* comp, Params& params)
{
    return new topo_dragonfly(comp,params);
}

static const ElementInfoParam dragonfly_params[] = {
    {"dragonfly:hosts_per_router","Number of hosts connected to each router."},
    {"dragonfly:routers_per_group","Number of links used to connect to routers in same group."},
    {"dragonfly:intergroup_per_router","Number of links per router connected to other groups."},
    {"dragonfly:num_groups","Number of groups in network."},
    {"dragonfly:algorithm","Routing algorithm to use [minmal (default) | valiant].", "minimal"},
    {NULL,NULL,NULL}
};

// topo dragonfly2
static Module*
load_dragonfly2_topology(Component* comp, Params& params)
{
    return new topo_dragonfly2(comp,params);
}

static const ElementInfoParam dragonfly2_params[] = {
    {"dragonfly:hosts_per_router","Number of hosts connected to each router."},
    {"dragonfly:routers_per_group","Number of links used to connect to routers in same group."},
    {"dragonfly:intergroup_per_router","Number of links per router connected to other groups."},
    {"dragonfly:intergroup_links","Number of links between each pair of groups."},
    {"dragonfly:num_groups","Number of groups in network."},
    {"dragonfly:algorithm","Routing algorithm to use [minmal (default) | valiant].", "minimal"},
    {"dragonfly:adaptive_threshold","Threshold to use when make adaptive routing decisions.", "2.0"},
    {"dragonfly:global_link_map","Array specifying connectivity of global links in each dragonfly group."},
    {NULL,NULL,NULL}
};


// Crossbar arbitration units

// round_robin
static SubComponent*
load_xbar_arb_rr(Component* comp, Params& params)
{
    return new xbar_arb_rr(comp);
}

static const ElementInfoStatistic xbar_arb_rr_statistics[] = {
    { NULL, NULL, NULL, 0 }
};

static const ElementInfoParam xbar_arb_rr_params[] = {
    {NULL,NULL,NULL}
};

// least recently used
static SubComponent*
load_xbar_arb_lru(Component* comp, Params& params)
{
    return new xbar_arb_lru(comp);
}

static const ElementInfoStatistic xbar_arb_lru_statistics[] = {
    { NULL, NULL, NULL, 0 }
};

static const ElementInfoParam xbar_arb_lru_params[] = {
    {NULL,NULL,NULL}
};

// age based
static SubComponent*
load_xbar_arb_age(Component* comp, Params& params)
{
    return new xbar_arb_age(comp);
}

static const ElementInfoStatistic xbar_arb_age_statistics[] = {
    { NULL, NULL, NULL, 0 }
};

static const ElementInfoParam xbar_arb_age_params[] = {
    {NULL,NULL,NULL}
};

// random
static SubComponent*
load_xbar_arb_rand(Component* comp, Params& params)
{
    return new xbar_arb_rand(comp);
}

static const ElementInfoStatistic xbar_arb_rand_statistics[] = {
    { NULL, NULL, NULL, 0 }
};

static const ElementInfoParam xbar_arb_rand_params[] = {
    {NULL,NULL,NULL}
};


// least recently used, "infinte" crossbar
static SubComponent*
load_xbar_arb_lru_infx(Component* comp, Params& params)
{
    return new xbar_arb_lru_infx(comp);
}

static const ElementInfoStatistic xbar_arb_lru_infx_statistics[] = {
    { NULL, NULL, NULL, 0 }
};

static const ElementInfoParam xbar_arb_lru_infx_params[] = {
    {NULL,NULL,NULL}
};


static Component*
create_portals_nic(SST::ComponentId_t id,
		   SST::Params& params)
{
    // return new portals_nic( id, params );
    return NULL;
}




static Module*
load_singlerouter_topology(Component* comp, Params& params)
{
    return new topo_singlerouter(comp,params);
}


// LinkControl Info
static SubComponent*
load_linkcontrol(Component* parent, Params& params)
{
    return new LinkControl(parent, params);
}

static const ElementInfoParam linkcontrol_params[] = {
    {"checkerboard","Number of actual virtual networks to use per virtual network seen by endpoint", "1"},
    {"checkerboard_alg","Algorithm to use to spead traffic across checkerboarded VNs [deterministic | roundrobin]", "deterministic"},
    { NULL, NULL, NULL }
};

static const ElementInfoStatistic linkcontrol_statistics[] = {
    { "packet_latency", "Histogram of latencies for received packets", "latency", 1},
    { "send_bit_count", "Count number of bits sent on link", "bits", 1},
    { "output_port_stalls", "Time output port is stalled (in units of core timebase)", "time in stalls", 1},
    { NULL, NULL, NULL, 0 }
};

// ReorderLinkControlInfo
static SubComponent*
load_reorderlinkcontrol(Component* parent, Params& params)
{
    return new ReorderLinkControl(parent, params);
}

static const ElementInfoParam reorderlinkcontrol_params[] = {
    {"rlc:networkIF","SimpleNetwork subcomponent to be used for connecting to network", "merlin.linkcontrol"},
    { NULL, NULL, NULL }
};

static const ElementInfoStatistic reorderlinkcontrol_statistics[] = {
    // { "packet_latency", "Histogram of latencies for received packets", "latency", 1},
    // { "send_bit_count", "Count number of bits sent on link", "bits", 1},
    // { "output_port_stalls", "Time output port is stalled (in units of core timebase)", "time in stalls", 1},
    { NULL, NULL, NULL, 0 }
};

static SubComponent*
load_test_network_inspector(Component* parent, Params& params)
{
    return new TestNetworkInspector(parent);
}

static SubComponent*
load_circ_network_inspector(Component* parent, Params& params)
{
    return new CircNetworkInspector(parent, params);
}

static const ElementInfoStatistic test_network_inspector_statistics[] = {
    { "test_count", "Count number of packets sent on link", "packets", 1},
    { NULL, NULL, NULL, 0 }
};

static const ElementInfoComponent components[] = {
    { "portals_nic",
      "NIC with offloaded Portals4 implementation.",
      NULL,
      create_portals_nic,
    },
    { "hr_router",
      "High radix router.",
      NULL,
      create_hr_router,
      hr_router_params,
      hr_router_ports,
      COMPONENT_CATEGORY_NETWORK,
      hr_router_statistics
    },
    { "pt2pt_test",
      "Simple NIC to test basic pt2pt performance.",
      NULL,
      create_pt2pt_test,
      pt2pt_test_params,
      pt2pt_test_ports,
      COMPONENT_CATEGORY_NETWORK
    },
    { "bisection_test",
      "Simple NIC to test bisection bandwidth of a network.",
      NULL,
      create_bisection_test,
      bisection_test_params,
      bisection_test_ports,
      COMPONENT_CATEGORY_NETWORK,
      NULL
    },
    { "test_nic",
      "Simple NIC to test base functionality.",
      NULL,
      create_test_nic,
      test_nic_params,
      test_nic_ports,
      COMPONENT_CATEGORY_NETWORK,
      NULL
    },
    { "shift_nic",
      "Simple pattern NIC doing a shift pattern.",
      NULL,
      create_shift_nic,
      shift_nic_params,
      shift_nic_ports,
      COMPONENT_CATEGORY_NETWORK,
      NULL
    },
    { "route_test",
      "Simple NIC to test routing.",
      NULL,
      create_route_test,
      route_test_params,
      route_test_ports,
      COMPONENT_CATEGORY_NETWORK,
      NULL
    },
    { "trafficgen",
      "Pattern-based traffic generator.",
      NULL,
      create_traffic_generator,
      traffic_generator_params,
      traffic_generator_ports,
      COMPONENT_CATEGORY_NETWORK,
      NULL
    },
    { NULL, NULL, NULL, NULL }
};

static const ElementInfoModule modules[] = {
    { "torus",
      "Torus topology object",
      NULL,
      NULL,
      load_torus_topology,
      torus_params,
      "SST::Merlin::Topology"
    },
    { "mesh",
      "Mesh topology object",
      NULL,
      NULL,
      load_mesh_topology,
      mesh_params,
      "SST::Merlin::Topology"
    },
    { "singlerouter",
      "Simple, single-router topology object",
      NULL,
      NULL,
      load_singlerouter_topology,
      NULL,
      "SST::Merlin::Topology"
    },
    { "fattree",
      "Fattree topology object",
      NULL,
      NULL,
      load_fattree_topology,
      fattree_params,
      "SST::Merlin::Topology"
    },
    { "dragonfly",
      "Dragonfly topology object",
      NULL,
      NULL,
      load_dragonfly_topology,
      dragonfly_params,
      "SST::Merlin::Topology"
    },
    { "dragonfly2",
      "Dragonfly2 topology object",
      NULL,
      NULL,
      load_dragonfly2_topology,
      dragonfly2_params,
      "SST::Merlin::Topology"
    },
    { NULL, NULL, NULL, NULL, NULL, NULL }
};

static const ElementInfoSubComponent subcomponents[] = {
    { "linkcontrol",
      "Link Control module for building Merlin-enabled NICs",
      NULL,
      load_linkcontrol,
      linkcontrol_params,
      linkcontrol_statistics,
      "SST::Interfaces::SimpleNetwork"
    },
    { "reorderlinkcontrol",
      "Link Control module that can handle out of order packet arrival.  "
      "Events are sequenced and order is reconstructed on receive.",
      NULL,
      load_reorderlinkcontrol,
      reorderlinkcontrol_params,
      reorderlinkcontrol_statistics,
      "SST::Interfaces::SimpleNetwork"
    },
    { "test_network_inspector",
      "Used to test NetworkInspector functionality.  Duplicates send_packet_count in hr_router.",
      NULL,
      load_test_network_inspector,
      NULL,
      test_network_inspector_statistics,
      "SST::Interfaces::SimpleNetwork::NetworkInspector"
    },
    { "circuit_network_inspector",
      "Used to count the number of network circuits (as in 'circuit switched' circuits)", 
      NULL,
      load_circ_network_inspector,
      NULL,
      NULL,
      "SST::Interfaces::SimpleNetwork::NetworkInspector"
      },
    { "xbar_arb_rr",
      "Round robin arbitration unit for hr_router",
      NULL,
      load_xbar_arb_rr,
      xbar_arb_rr_params,
      xbar_arb_rr_statistics,
      "Merlin::XbarArbitration"
    },
    { "xbar_arb_lru",
      "Least recently used arbitration unit for hr_router",
      NULL,
      load_xbar_arb_lru,
      xbar_arb_lru_params,
      xbar_arb_lru_statistics,
      "Merlin::XbarArbitration"
    },
    { "xbar_arb_age",
      "Round robin arbitration unit for hr_router",
      NULL,
      load_xbar_arb_age,
      xbar_arb_age_params,
      xbar_arb_age_statistics,
      "Merlin::XbarArbitration"
    },
    { "xbar_arb_rand",
      "Random arbitration arbitration unit for hr_router",
      NULL,
      load_xbar_arb_rand,
      xbar_arb_rand_params,
      xbar_arb_rand_statistics,
      "Merlin::XbarArbitration"
    },
    { "xbar_arb_lru_infx",
      "Least recently used arbitration unit for hr_router",
      NULL,
      load_xbar_arb_lru_infx,
      xbar_arb_lru_infx_params,
      xbar_arb_lru_infx_statistics,
      "Merlin::XbarArbitration"
    },
    { NULL, NULL, NULL, NULL, NULL, NULL }
};


extern "C" {
    ElementLibraryInfo merlin_eli = {
        "merlin",
        "Flexible network components",
        components,
        NULL,   // Events
        NULL,   // Introspectors
        modules,
        subcomponents,
        NULL, // partitioners,
        genMerlinPyModule,  // Python Module Generator
        NULL // generators,
    };
}

BOOST_CLASS_EXPORT(SST::Merlin::BaseRtrEvent)
BOOST_CLASS_EXPORT(SST::Merlin::RtrEvent)
BOOST_CLASS_EXPORT(SST::Merlin::RtrInitEvent)
BOOST_CLASS_EXPORT(SST::Merlin::credit_event)
BOOST_CLASS_EXPORT(SST::Merlin::TopologyEvent)

