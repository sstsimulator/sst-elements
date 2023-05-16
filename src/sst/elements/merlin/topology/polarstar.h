// -*- mode: c++ -*-

// Copyright 2009-2022 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2022, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// of the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

// Copyright 2023 Intel Corporation
// SPDX-License-Identifier: MIT License

// Author: Kartik Lakhotia

#ifndef COMPONENTS_MERLIN_TOPOLOGY_POLARSTAR_H
#define COMPONENTS_MERLIN_TOPOLOGY_POLARSTAR_H

#include <sst/core/event.h>
#include <sst/core/link.h>
#include <sst/core/params.h>
#include <sst/core/rng/rng.h>

#include <string.h>
#include <unistd.h>
#include <vector>
#include <algorithm>
#include <stdlib.h>
#include <iostream>
#include <fstream>
#include <sstream>

#include "sst/elements/merlin/router.h"

namespace SST {
namespace Merlin {


//For now, polarstar_event is not much different from the router event. Just added the hop_count metric to record the total number of hops this packet is taking across the network
class topo_polarstar_event : public internal_router_event {
public:


    int hop_count; //Count no of hops this packet has taken so far
     
    bool non_minimal;
    int valiant;

    topo_polarstar_event() {}
    topo_polarstar_event(int hcount) : non_minimal(false) {	
        
        hop_count = hcount;
        
     }
    virtual ~topo_polarstar_event() {}

    virtual internal_router_event* clone(void) override
    {
        return new topo_polarstar_event(*this);
         
    }

    void serialize_order(SST::Core::Serialization::serializer &ser)  override {
        internal_router_event::serialize_order(ser);
        
        ser & hop_count;
        ser & non_minimal;
        ser & valiant;
    }

protected:

private:
    ImplementSerializable(SST::Merlin::topo_polarstar_event)

};


class topo_polarstar_init_event : public topo_polarstar_event {
public:
    //Insert code
    int phase;

    //track what is covered
    int total_routers;
    std::vector<int> covered;

    topo_polarstar_init_event() {}
    topo_polarstar_init_event(int p, int N) : topo_polarstar_event(0), phase(p), total_routers(N) { covered.resize(N, 0);}
    virtual ~topo_polarstar_init_event() { }
    virtual internal_router_event* clone(void) override
    {
        topo_polarstar_init_event* tte = new topo_polarstar_init_event(*this);
        //Insert code
        return tte;
    }

    void serialize_order(SST::Core::Serialization::serializer &ser)  override {
        topo_polarstar_event::serialize_order(ser);
        ser & phase;
        ser & total_routers; 
        for (int i=0; i<total_routers; i++)
            ser & covered[i];
         
    }

private:
    ImplementSerializable(SST::Merlin::topo_polarstar_init_event)

};


class topo_polarstar: public Topology {

public:

    SST_ELI_REGISTER_SUBCOMPONENT(
        topo_polarstar,
        "merlin",
        "polarstar",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "PolarStar: Highly Scalable Diameter-3 Topology Object",
        SST::Merlin::Topology)

    SST_ELI_DOCUMENT_PARAMS(
        // Parameters needed for use with old merlin python module
        {"d", "Network Radix per router"},
        {"sn_type", "Type of supernode -- {iq, paley or max} "},
        {"pfq", "Prime power q for polarfly, pfq<0 will automatically compute pfq for optimal scale"},
        {"snq", "parameter q for supernodes; for Paley q=num nodes, for IQ q=degree"},
        {"hosts_per_router", "endpoints per router"},
        {"total_radix", "Radix of the router."},
        {"total_routers", "Number of total routers in the network."},
        {"total_endnodes", "Number of total endpoints in the network."},
    )
    SST_ELI_DOCUMENT_STATISTICS(
        { "hopcount1",     "Number of packets with 1 switch hopcount", "hops", 0},
        { "hopcount2",     "Number of packets with 2 switch hopcount", "hops", 0},
        { "hopcount3",     "Number of packets with 3 switch hopcount", "hops", 0},
        { "hopcount4",     "Number of packets with 4 switch hopcount", "hops", 0},
        { "hopcount5",     "Number of packets with 5 switch hopcount", "hops", 0},
        { "hopcount6",     "Number of packets with 6 switch hopcount", "hops", 0}
    )

    enum RouteAlgo {
        MINIMAL,
        VALIANT,
        UGAL
    };

    int router_id;
    int d;
    int pfq;
    int snq;
    std::string sn_type;

    int hosts_per_router;
    int network_radix;
    int total_radix;
    int total_routers;
    int total_endnodes;
    RouteAlgo routing_algo;

    int node_links;
    std::vector<int> route_table;
    std::vector<int> neighbor_list;

    int num_vns;
    int num_vcs;
    RNG::Random* rng;

    int const* output_credits;
    int const* output_queue_lengths;
    int output_buffer_size;
    int adaptive_bias;

    std::vector<std::vector<int>> polar;

    //For now, doing this in a very dumb way, need to figure out a right way to do it with an vector array of statistics
    Statistic<uint32_t>* hopcount1;
    Statistic<uint32_t>* hopcount2;
    Statistic<uint32_t>* hopcount3;
    Statistic<uint32_t>* hopcount4;
    Statistic<uint32_t>* hopcount5;
    Statistic<uint32_t>* hopcount6;


    
public:
    topo_polarstar(ComponentId_t cid, Params& params, int num_ports, int rtr_id, int num_vns);
    ~topo_polarstar();

    virtual void route_packet(int port, int vc, internal_router_event* ev);
    //called at injection, add metadata about packet
    virtual internal_router_event* process_input(RtrEvent* ev);

    ////called during topology initilaization for sanity checks
    //virtual void routeInitData(int port, internal_router_event* ev, std::vector<int> &outPorts);
    ////process_input for initilaization packets
    //virtual internal_router_event* process_InitData_input(RtrEvent* ev);

    virtual void routeUntimedData(int port, internal_router_event* ev, std::vector<int> &outPorts);
    //process_input for initilaization packets
    virtual internal_router_event* process_UntimedData_input(RtrEvent* ev);

    virtual PortState getPortState(int port) const;
    virtual int getEndpointID(int port);

    virtual void getVCsPerVN(std::vector<int>& vcs_per_vn) {
        for ( int i = 0; i < num_vns; ++i ) {
            vcs_per_vn[i] = num_vcs;
        }
    }
    
protected:
    //virtual int choose_multipath(int start_port, int num_ports, int dest_dist);

private:

   void routeMinimal(int port, int vc, internal_router_event* ev);
   void routeValiant(int port, int vc, internal_router_event* ev);
   void routeUgal(int port, int vc, internal_router_event* ev);

   void initPolarGraph();
   void initRouteTable();

   int getRouterID(int endpoint);
   int getDestLocalPort(int node);
   void dumpHopCount(topo_polarstar_event* ev);

   void setOutputQueueLengthsArray(int const* array, int vcs);
   void setOutputBufferCreditArray(int const* array, int vcs); 

};

}
}

#endif // COMPONENTS_MERLIN_TOPOLOGY_MESH_H
