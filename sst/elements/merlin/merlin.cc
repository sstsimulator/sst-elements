// Copyright 2009-2014 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2014, Sandia Corporation
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include <sst_config.h>
#include "sst/core/serialization.h"

#include <sst/core/element.h>
#include <sst/core/configGraph.h>

#include "merlin.h"
#include "linkControl.h"

#include "hr_router/hr_router.h"
#include "test/nic.h"
#include "test/pt2pt/pt2pt_test.h"
#include "test/bisection/bisection_test.h"

#include "topology/torus.h"
#include "topology/mesh.h"
#include "topology/singlerouter.h"
#include "topology/fattree.h"
#include "topology/dragonfly.h"

#include "trafficgen/trafficgen.h"

#include "pymodule.h"

#include <stdio.h>
#include <stdarg.h>

using namespace std;
using namespace SST::Merlin;

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
    {"link_bw", "Bandwidth of the links specified in either b/s or B/s (can include SI prefix)."},
    {"flit_size", "Flit size specified in either b or B (can include SI prefix)."},
    {"xbar_bw", "Bandwidth of the crossbar specified in either b/s or B/s (can include SI prefix)."},
    {"input_latency", "Latency of packets entering switch into input buffers.  Specified in s (can include SI prefix)."},
    {"output_latency", "Latency of packets exiting switch from output buffers.  Specified in s (can include SI prefix)."},
    {"input_buf_size", "Size of input buffers specified in b or B (can include SI prefix)."},
    {"output_buf_size", "Size of output buffers specified in b or B (can include SI prefix)."},
    {"debug", "Turn on debugging for router. Set to 1 for on, 0 for off.", "0"},
    {NULL,NULL,NULL}
};

static const ElementInfoPort hr_router_ports[] = {
    {"port%(num_ports)d",  "Ports which connect to endpoints or other routers.", router_events},
    {NULL, NULL, NULL}
};

static const ElementInfoStatistic hr_router_statistics[] = {
    { "port0_send_bit_count", "Count number of bits sent on link", 1},
    { "port1_send_bit_count", "Count number of bits sent on link", 1},
    { "port2_send_bit_count", "Count number of bits sent on link", 1},
    { "port3_send_bit_count", "Count number of bits sent on link", 1},
    { "port4_send_bit_count", "Count number of bits sent on link", 1},
    { "port5_send_bit_count", "Count number of bits sent on link", 1},
    { "port6_send_bit_count", "Count number of bits sent on link", 1},
    { "port7_send_bit_count", "Count number of bits sent on link", 1},
    { "port8_send_bit_count", "Count number of bits sent on link", 1},
    { "port9_send_bit_count", "Count number of bits sent on link", 1},
    { "port10_send_bit_count", "Count number of bits sent on link", 1},
    { "port11_send_bit_count", "Count number of bits sent on link", 1},
    { "port12_send_bit_count", "Count number of bits sent on link", 1},
    { "port13_send_bit_count", "Count number of bits sent on link", 1},
    { "port14_send_bit_count", "Count number of bits sent on link", 1},
    { "port15_send_bit_count", "Count number of bits sent on link", 1},
    { "port16_send_bit_count", "Count number of bits sent on link", 1},
    { "port17_send_bit_count", "Count number of bits sent on link", 1},
    { "port18_send_bit_count", "Count number of bits sent on link", 1},
    { "port19_send_bit_count", "Count number of bits sent on link", 1},
    { "port20_send_bit_count", "Count number of bits sent on link", 1},
    { "port21_send_bit_count", "Count number of bits sent on link", 1},
    { "port22_send_bit_count", "Count number of bits sent on link", 1},
    { "port23_send_bit_count", "Count number of bits sent on link", 1},
    { "port24_send_bit_count", "Count number of bits sent on link", 1},
    { "port25_send_bit_count", "Count number of bits sent on link", 1},
    { "port26_send_bit_count", "Count number of bits sent on link", 1},
    { "port27_send_bit_count", "Count number of bits sent on link", 1},
    { "port28_send_bit_count", "Count number of bits sent on link", 1},
    { "port29_send_bit_count", "Count number of bits sent on link", 1},
    { "port30_send_bit_count", "Count number of bits sent on link", 1},
    { "port31_send_bit_count", "Count number of bits sent on link", 1},
    { "port32_send_bit_count", "Count number of bits sent on link", 1},
    { "port33_send_bit_count", "Count number of bits sent on link", 1},
    { "port34_send_bit_count", "Count number of bits sent on link", 1},
    { "port35_send_bit_count", "Count number of bits sent on link", 1},
    { "port36_send_bit_count", "Count number of bits sent on link", 1},
    { "port37_send_bit_count", "Count number of bits sent on link", 1},
    { "port38_send_bit_count", "Count number of bits sent on link", 1},
    { "port39_send_bit_count", "Count number of bits sent on link", 1},
    { "port40_send_bit_count", "Count number of bits sent on link", 1},
    { "port41_send_bit_count", "Count number of bits sent on link", 1},
    { "port42_send_bit_count", "Count number of bits sent on link", 1},
    { "port43_send_bit_count", "Count number of bits sent on link", 1},
    { "port44_send_bit_count", "Count number of bits sent on link", 1},
    { "port45_send_bit_count", "Count number of bits sent on link", 1},
    { "port46_send_bit_count", "Count number of bits sent on link", 1},
    { "port47_send_bit_count", "Count number of bits sent on link", 1},
    { "port48_send_bit_count", "Count number of bits sent on link", 1},
    { "port49_send_bit_count", "Count number of bits sent on link", 1},
    { "port50_send_bit_count", "Count number of bits sent on link", 1},
    { "port51_send_bit_count", "Count number of bits sent on link", 1},
    { "port52_send_bit_count", "Count number of bits sent on link", 1},
    { "port53_send_bit_count", "Count number of bits sent on link", 1},
    { "port54_send_bit_count", "Count number of bits sent on link", 1},
    { "port55_send_bit_count", "Count number of bits sent on link", 1},
    { "port56_send_bit_count", "Count number of bits sent on link", 1},
    { "port57_send_bit_count", "Count number of bits sent on link", 1},
    { "port58_send_bit_count", "Count number of bits sent on link", 1},
    { "port59_send_bit_count", "Count number of bits sent on link", 1},
    { "port60_send_bit_count", "Count number of bits sent on link", 1},
    { "port61_send_bit_count", "Count number of bits sent on link", 1},
    { "port62_send_bit_count", "Count number of bits sent on link", 1},
    { "port63_send_bit_count", "Count number of bits sent on link", 1},

    { "port0_send_packet_count", "Count number of packets sent on link", 1},
    { "port1_send_packet_count", "Count number of packets sent on link", 1},
    { "port2_send_packet_count", "Count number of packets sent on link", 1},
    { "port3_send_packet_count", "Count number of packets sent on link", 1},
    { "port4_send_packet_count", "Count number of packets sent on link", 1},
    { "port5_send_packet_count", "Count number of packets sent on link", 1},
    { "port6_send_packet_count", "Count number of packets sent on link", 1},
    { "port7_send_packet_count", "Count number of packets sent on link", 1},
    { "port8_send_packet_count", "Count number of packets sent on link", 1},
    { "port9_send_packet_count", "Count number of packets sent on link", 1},
    { "port10_send_packet_count", "Count number of packets sent on link", 1},
    { "port11_send_packet_count", "Count number of packets sent on link", 1},
    { "port12_send_packet_count", "Count number of packets sent on link", 1},
    { "port13_send_packet_count", "Count number of packets sent on link", 1},
    { "port14_send_packet_count", "Count number of packets sent on link", 1},
    { "port15_send_packet_count", "Count number of packets sent on link", 1},
    { "port16_send_packet_count", "Count number of packets sent on link", 1},
    { "port17_send_packet_count", "Count number of packets sent on link", 1},
    { "port18_send_packet_count", "Count number of packets sent on link", 1},
    { "port19_send_packet_count", "Count number of packets sent on link", 1},
    { "port20_send_packet_count", "Count number of packets sent on link", 1},
    { "port21_send_packet_count", "Count number of packets sent on link", 1},
    { "port22_send_packet_count", "Count number of packets sent on link", 1},
    { "port23_send_packet_count", "Count number of packets sent on link", 1},
    { "port24_send_packet_count", "Count number of packets sent on link", 1},
    { "port25_send_packet_count", "Count number of packets sent on link", 1},
    { "port26_send_packet_count", "Count number of packets sent on link", 1},
    { "port27_send_packet_count", "Count number of packets sent on link", 1},
    { "port28_send_packet_count", "Count number of packets sent on link", 1},
    { "port29_send_packet_count", "Count number of packets sent on link", 1},
    { "port30_send_packet_count", "Count number of packets sent on link", 1},
    { "port31_send_packet_count", "Count number of packets sent on link", 1},
    { "port32_send_packet_count", "Count number of packets sent on link", 1},
    { "port33_send_packet_count", "Count number of packets sent on link", 1},
    { "port34_send_packet_count", "Count number of packets sent on link", 1},
    { "port35_send_packet_count", "Count number of packets sent on link", 1},
    { "port36_send_packet_count", "Count number of packets sent on link", 1},
    { "port37_send_packet_count", "Count number of packets sent on link", 1},
    { "port38_send_packet_count", "Count number of packets sent on link", 1},
    { "port39_send_packet_count", "Count number of packets sent on link", 1},
    { "port40_send_packet_count", "Count number of packets sent on link", 1},
    { "port41_send_packet_count", "Count number of packets sent on link", 1},
    { "port42_send_packet_count", "Count number of packets sent on link", 1},
    { "port43_send_packet_count", "Count number of packets sent on link", 1},
    { "port44_send_packet_count", "Count number of packets sent on link", 1},
    { "port45_send_packet_count", "Count number of packets sent on link", 1},
    { "port46_send_packet_count", "Count number of packets sent on link", 1},
    { "port47_send_packet_count", "Count number of packets sent on link", 1},
    { "port48_send_packet_count", "Count number of packets sent on link", 1},
    { "port49_send_packet_count", "Count number of packets sent on link", 1},
    { "port50_send_packet_count", "Count number of packets sent on link", 1},
    { "port51_send_packet_count", "Count number of packets sent on link", 1},
    { "port52_send_packet_count", "Count number of packets sent on link", 1},
    { "port53_send_packet_count", "Count number of packets sent on link", 1},
    { "port54_send_packet_count", "Count number of packets sent on link", 1},
    { "port55_send_packet_count", "Count number of packets sent on link", 1},
    { "port56_send_packet_count", "Count number of packets sent on link", 1},
    { "port57_send_packet_count", "Count number of packets sent on link", 1},
    { "port58_send_packet_count", "Count number of packets sent on link", 1},
    { "port59_send_packet_count", "Count number of packets sent on link", 1},
    { "port60_send_packet_count", "Count number of packets sent on link", 1},
    { "port61_send_packet_count", "Count number of packets sent on link", 1},
    { "port62_send_packet_count", "Count number of packets sent on link", 1},
    { "port63_send_packet_count", "Count number of packets sent on link", 1},

    { "port0_xbar_stalls", "Count number of cycles the xbar is stalled", 1},
    { "port1_xbar_stalls", "Count number of cycles the xbar is stalled", 1},
    { "port2_xbar_stalls", "Count number of cycles the xbar is stalled", 1},
    { "port3_xbar_stalls", "Count number of cycles the xbar is stalled", 1},
    { "port4_xbar_stalls", "Count number of cycles the xbar is stalled", 1},
    { "port5_xbar_stalls", "Count number of cycles the xbar is stalled", 1},
    { "port6_xbar_stalls", "Count number of cycles the xbar is stalled", 1},
    { "port7_xbar_stalls", "Count number of cycles the xbar is stalled", 1},
    { "port8_xbar_stalls", "Count number of cycles the xbar is stalled", 1},
    { "port9_xbar_stalls", "Count number of cycles the xbar is stalled", 1},
    { "port10_xbar_stalls", "Count number of cycles the xbar is stalled", 1},
    { "port11_xbar_stalls", "Count number of cycles the xbar is stalled", 1},
    { "port12_xbar_stalls", "Count number of cycles the xbar is stalled", 1},
    { "port13_xbar_stalls", "Count number of cycles the xbar is stalled", 1},
    { "port14_xbar_stalls", "Count number of cycles the xbar is stalled", 1},
    { "port15_xbar_stalls", "Count number of cycles the xbar is stalled", 1},
    { "port16_xbar_stalls", "Count number of cycles the xbar is stalled", 1},
    { "port17_xbar_stalls", "Count number of cycles the xbar is stalled", 1},
    { "port18_xbar_stalls", "Count number of cycles the xbar is stalled", 1},
    { "port19_xbar_stalls", "Count number of cycles the xbar is stalled", 1},
    { "port20_xbar_stalls", "Count number of cycles the xbar is stalled", 1},
    { "port21_xbar_stalls", "Count number of cycles the xbar is stalled", 1},
    { "port22_xbar_stalls", "Count number of cycles the xbar is stalled", 1},
    { "port23_xbar_stalls", "Count number of cycles the xbar is stalled", 1},
    { "port24_xbar_stalls", "Count number of cycles the xbar is stalled", 1},
    { "port25_xbar_stalls", "Count number of cycles the xbar is stalled", 1},
    { "port26_xbar_stalls", "Count number of cycles the xbar is stalled", 1},
    { "port27_xbar_stalls", "Count number of cycles the xbar is stalled", 1},
    { "port28_xbar_stalls", "Count number of cycles the xbar is stalled", 1},
    { "port29_xbar_stalls", "Count number of cycles the xbar is stalled", 1},
    { "port30_xbar_stalls", "Count number of cycles the xbar is stalled", 1},
    { "port31_xbar_stalls", "Count number of cycles the xbar is stalled", 1},
    { "port32_xbar_stalls", "Count number of cycles the xbar is stalled", 1},
    { "port33_xbar_stalls", "Count number of cycles the xbar is stalled", 1},
    { "port34_xbar_stalls", "Count number of cycles the xbar is stalled", 1},
    { "port35_xbar_stalls", "Count number of cycles the xbar is stalled", 1},
    { "port36_xbar_stalls", "Count number of cycles the xbar is stalled", 1},
    { "port37_xbar_stalls", "Count number of cycles the xbar is stalled", 1},
    { "port38_xbar_stalls", "Count number of cycles the xbar is stalled", 1},
    { "port39_xbar_stalls", "Count number of cycles the xbar is stalled", 1},
    { "port40_xbar_stalls", "Count number of cycles the xbar is stalled", 1},
    { "port41_xbar_stalls", "Count number of cycles the xbar is stalled", 1},
    { "port42_xbar_stalls", "Count number of cycles the xbar is stalled", 1},
    { "port43_xbar_stalls", "Count number of cycles the xbar is stalled", 1},
    { "port44_xbar_stalls", "Count number of cycles the xbar is stalled", 1},
    { "port45_xbar_stalls", "Count number of cycles the xbar is stalled", 1},
    { "port46_xbar_stalls", "Count number of cycles the xbar is stalled", 1},
    { "port47_xbar_stalls", "Count number of cycles the xbar is stalled", 1},
    { "port48_xbar_stalls", "Count number of cycles the xbar is stalled", 1},
    { "port49_xbar_stalls", "Count number of cycles the xbar is stalled", 1},
    { "port50_xbar_stalls", "Count number of cycles the xbar is stalled", 1},
    { "port51_xbar_stalls", "Count number of cycles the xbar is stalled", 1},
    { "port52_xbar_stalls", "Count number of cycles the xbar is stalled", 1},
    { "port53_xbar_stalls", "Count number of cycles the xbar is stalled", 1},
    { "port54_xbar_stalls", "Count number of cycles the xbar is stalled", 1},
    { "port55_xbar_stalls", "Count number of cycles the xbar is stalled", 1},
    { "port56_xbar_stalls", "Count number of cycles the xbar is stalled", 1},
    { "port57_xbar_stalls", "Count number of cycles the xbar is stalled", 1},
    { "port58_xbar_stalls", "Count number of cycles the xbar is stalled", 1},
    { "port59_xbar_stalls", "Count number of cycles the xbar is stalled", 1},
    { "port60_xbar_stalls", "Count number of cycles the xbar is stalled", 1},
    { "port61_xbar_stalls", "Count number of cycles the xbar is stalled", 1},
    { "port62_xbar_stalls", "Count number of cycles the xbar is stalled", 1},
    { "port63_xbar_stalls", "Count number of cycles the xbar is stalled", 1},
    { NULL, NULL, 0 }
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
    {NULL,NULL,NULL}
};

static const ElementInfoPort bisection_test_ports[] = {
    {"rtr",  "Port that hooks up to router.", nic_events},
    {NULL, NULL, NULL}
};

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
    {"num_vns","Number of requested virtual networks."},
    {"link_bw","Bandwidth of the router link specified in either b/s or B/s (can include SI prefix)."},
    {"topology", "Name of the topology module that should be loaded to control routing."},
    // {"packet_size","Packet size specified in either b or B (can include SI prefix)."},
    // {"packets_to_send","Number of packets to send in the test."},
    // {"buffer_size","Size of input and output buffers specified in b or B (can include SI prefix)."},
    {NULL,NULL,NULL}
};

static const ElementInfoPort test_nic_ports[] = {
    {"rtr",  "Port that hooks up to router.", nic_events},
    {NULL, NULL, NULL}
};



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
    {"PacketSize:RangeMax","Minumum size of packets.","0"},
    {"PacketSize:RangeMin","Maximum size of packets.","INT_MAX"},
    {"PacketSize:HotSpot:target", "For HotSpot, the target packet size", ""},
    {"PacketSize:HotSpot:targetProbability", "For HotSpot, with what probability is the target targeted", ""},
    {"PacketSize:Normal:Mean", "In a normal distribution, the mean", ""},
    {"PacketSize:Normal:Sigma", "In a normal distribution, the mean variance", "1.0"},
    {"PacketSize:Binomial:Mean", "In a binomial distribution, the mean", ""},
    {"PacketSize:Binomial:Sigma", "In a binomial distribution, the variance", "0.5"},
    {"PacketDelay:pattern","Address pattern to be used (Uniform, HotSpot, Normal, Binomial)",NULL},
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


static Module*
load_linkcontrol(Params& params)
{
    return new LinkControl(params);
}

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
      COMPONENT_CATEGORY_NETWORK
    },
    { "test_nic",
      "Simple NIC to test base functionality.",
      NULL,
      create_test_nic,
      test_nic_params,
      test_nic_ports,
    },
    { "trafficgen",
      "Pattern-based traffic generator.",
      NULL,
      create_traffic_generator,
      traffic_generator_params,
      traffic_generator_ports,
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
    { "linkcontrol",
      "Link Control module for building Merlin-enabled NICs",
      NULL,
      load_linkcontrol,
      NULL,
      NULL,
      "SST::Merlin::LinkControl"
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
        NULL, // partitioners,
        NULL, // generators,
        genMerlinPyModule  // Python Module Generator
    };
}

BOOST_CLASS_EXPORT(BaseRtrEvent)
BOOST_CLASS_EXPORT(RtrEvent)
BOOST_CLASS_EXPORT(RtrInitEvent)
BOOST_CLASS_EXPORT(credit_event)
BOOST_CLASS_EXPORT(internal_router_event)
BOOST_CLASS_EXPORT(TopologyEvent)

