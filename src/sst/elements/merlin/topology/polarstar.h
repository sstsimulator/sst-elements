// -*- mode: c++ -*-

// Copyright 2023 Intel Corporation
// SPDX-License-Identifier: BSD-3-Clause

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


    int hop_count = 0; //Count no of hops this packet has taken so far

    bool non_minimal = false;
    int valiant = 0;

    topo_polarstar_event() = default;
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

        SST_SER(hop_count);
        SST_SER(non_minimal);
        SST_SER(valiant);
    }

protected:

private:
    ImplementSerializable(SST::Merlin::topo_polarstar_event)

};


class topo_polarstar_init_event : public topo_polarstar_event {
public:
    //Insert code
    int phase = 0;

    //track what is covered
    int total_routers = 0;
    std::vector<int> covered;

    topo_polarstar_init_event() = default;
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
        SST_SER(phase);
        SST_SER(total_routers);
        SST_SER(covered);
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

    int router_id = -1;
    int d = 0;
    int pfq = 0;
    int snq = 0;
    std::string sn_type;

    int hosts_per_router = 0;
    int network_radix = 0;
    int total_radix = 0;
    int total_routers = 0;
    int total_endnodes = 0;
    RouteAlgo routing_algo = MINIMAL;

    int node_links = 0;
    std::vector<int> route_table;
    std::vector<int> neighbor_list;

    int num_vns = 0;
    int num_vcs = 0;
    RNG::Random* rng = nullptr;

    int const* output_credits;
    int const* output_queue_lengths;
    int output_buffer_size = 0;
    int adaptive_bias = 0;

    std::vector<std::vector<int>> polar;

    //For now, doing this in a very dumb way, need to figure out a right way to do it with an vector array of statistics
    Statistic<uint32_t>* hopcount1 = nullptr;
    Statistic<uint32_t>* hopcount2 = nullptr;
    Statistic<uint32_t>* hopcount3 = nullptr;
    Statistic<uint32_t>* hopcount4 = nullptr;
    Statistic<uint32_t>* hopcount5 = nullptr;
    Statistic<uint32_t>* hopcount6 = nullptr;



public:
    topo_polarstar(ComponentId_t cid, Params& params, int num_ports, int rtr_id, int num_vns);
    ~topo_polarstar();

    topo_polarstar() = default;

    void serialize_order(SST::Core::Serialization::serializer& ser) override;

    ImplementSerializable(SST::Merlin::topo_polarstar)

    virtual void route_packet(int port, int vc, internal_router_event* ev) override;
    //called at injection, add metadata about packet
    virtual internal_router_event* process_input(RtrEvent* ev) override;

    ////called during topology initilaization for sanity checks
    //virtual void routeInitData(int port, internal_router_event* ev, std::vector<int> &outPorts);
    ////process_input for initilaization packets
    //virtual internal_router_event* process_InitData_input(RtrEvent* ev);

    virtual void routeUntimedData(int port, internal_router_event* ev, std::vector<int> &outPorts) override;
    //process_input for initilaization packets
    virtual internal_router_event* process_UntimedData_input(RtrEvent* ev) override;

    virtual PortState getPortState(int port) const override;
    virtual int getEndpointID(int port) override;

    virtual void getVCsPerVN(std::vector<int>& vcs_per_vn)  override
    {
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

   void setOutputQueueLengthsArray(int const* array, int vcs) override;
   void setOutputBufferCreditArray(int const* array, int vcs) override;

};

}
}

#endif // COMPONENTS_MERLIN_TOPOLOGY_MESH_H
