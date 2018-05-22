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


#ifndef COMPONENTS_KINGSLEY_NOC_MESH_H
#define COMPONENTS_KINGSLEY_NOC_MESH_H

#include <sst/core/clock.h>
#include <sst/core/component.h>
#include <sst/core/elementinfo.h>
#include <sst/core/event.h>
#include <sst/core/link.h>
#include <sst/core/output.h>
#include <sst/core/timeConverter.h>

#include <sst/core/statapi/stataccumulator.h>

#include <queue>

#include "sst/elements/kingsley/nocEvents.h"
#include "sst/elements/kingsley/lru_unit.h"

using namespace SST;

namespace SST {
namespace Kingsley {

class noc_mesh_event;

class noc_mesh : public Component {

public:

    SST_ELI_REGISTER_COMPONENT(
        noc_mesh,
        "kingsley",
        "noc_mesh",
        SST_ELI_ELEMENT_VERSION(0,1,0),
        "2-D mesh NOC router",
        COMPONENT_CATEGORY_NETWORK)

    SST_ELI_DOCUMENT_PARAMS(
        {"local_ports",        "Number of ports that are dedicated to endpoints.","1"},
        // {"frequency",          "Frequency of the router in Hz (can include SI prefix."},
        {"link_bw",            "Bandwidth of the links specified in either b/s or B/s (can include SI prefix)."},
        {"flit_size",          "Flit size specified in either b or B (can include SI prefix)."},
        {"input_buf_size",     "Size of input buffers in either b or B (can use SI prefix).  Default is 2*flit_size."},
        {"port_priority_equal","Set to true to have all port have equal priority (usually endpoint ports have higher priority).","false"},
        {"route_y_first",      "Set to true to rout Y-dimension first.","false"},
        {"use_dense_map",      "Set to true to have a dense network id map instead of the sparse map normally used.","false"},
        // {"network_inspectors", "Comma separated list of network inspectors to put on output ports.", ""},
    )

    SST_ELI_DOCUMENT_PORTS(
        { "north", "North port", {} },
        { "south", "South port", {} },
        { "east",  "East port",  {} },
        { "west",  "West port",  {} },
        {"local%(local_ports)d",  "Ports which connect to endpoints.", { } }
    )

    SST_ELI_DOCUMENT_STATISTICS(
        { "send_bit_count",     "Count number of bits sent on link", "bits", 1},
        // { "send_packet_count",  "Count number of packets sent on link", "packets", 1},
        { "output_port_stalls", "Time output port is stalled (in units of core timebase)", "time in stalls", 1},
        { "xbar_stalls",        "Count number of cycles the xbar is stalled", "cycles", 1},
        // { "idle_time",          "Amount of time spent idle for a given port", "units of core timebase", 1},
    )

    static const int north_port = 0;
    static const int south_port = 1;
    static const int east_port = 2;
    static const int west_port = 3;
    static const int local_port_start = 4;
    
    static const int north_mask = 1 << north_port;
    static const int south_mask = 1 << south_port;
    static const int east_mask = 1 << east_port;
    static const int west_mask = 1 << west_port;

private:

    int init_state;
    int init_count;
    int endpoint_start;
    int total_endpoints;
    unsigned int edge_status;
    unsigned int endpoint_locations;
    
    int flit_size;
    int input_buf_size;

    int x_size;
    int y_size;

    int my_x;
    int my_y;

    bool route_y_first;
    
    
    typedef std::queue<noc_mesh_event*> port_queue_t;

    Clock::Handler<noc_mesh>* my_clock_handler;
    TimeConverter* clock_tc;
    void clock_wakeup();
    bool clock_is_off;
    Cycle_t last_time = 0;

    Link** ports;
    port_queue_t* port_queues;
    int* port_busy;
    int* port_credits;
    int local_ports;
    bool use_dense_map;
    bool port_priority_equal;
    const int* dense_map;

    std::vector< lru_unit<int> > lru_units;
    // lru_unit<int> local_lru;
    // lru_unit<int> mesh_lru;
    
    bool clock_handler(Cycle_t cycle);
    // Statistic<uint64_t>** xbar_stalls;

    Output& output;

    noc_mesh_event* wrap_incoming_packet(NocPacket* packet);
    void handle_input_r2r(Event* ev, int port);
    void handle_input_ep2r(Event* ev, int port);

    void route(noc_mesh_event* event);
    

    Statistic<uint64_t>** send_bit_count;
    Statistic<uint64_t>** output_port_stalls;
    Statistic<uint64_t>** xbar_stalls;
    // Statistic<uint64_t>** xbar_stalls_prioirty;
    // Statistic<uint64_t>** xbar_stalls_normal;
    // Statistic<uint64_t>** output_idle;
    
    
public:
    noc_mesh(ComponentId_t cid, Params& params);
    ~noc_mesh();
    
    void init(unsigned int phase);
    void complete(unsigned int phase);
    void setup();
    void finish();

    // void sendTopologyEvent(int port, TopologyEvent* ev);
    // void recvTopologyEvent(int port, TopologyEvent* ev);
    
    // void dumpState(std::ostream& stream);
    void printStatus(Output& out);


};

class noc_mesh_event : public BaseNocEvent {

public:

    std::pair<int,int> dest_mesh_loc;
    int egress_port;

    int next_port;
    NocPacket* encap_ev;

    noc_mesh_event() :
        BaseNocEvent(BaseNocEvent::INTERNAL)
    {
        encap_ev = NULL;
    }

    noc_mesh_event(NocPacket* ev) :
        BaseNocEvent(BaseNocEvent::INTERNAL)
    {encap_ev = ev;}

    virtual ~noc_mesh_event() {
        if ( encap_ev != NULL ) delete encap_ev;
    }

    virtual noc_mesh_event* clone(void) override {
        noc_mesh_event* ret = new noc_mesh_event(*this);
        ret->dest_mesh_loc = dest_mesh_loc;
        ret->egress_port = egress_port;
        ret->next_port = next_port;
        ret->encap_ev = encap_ev->clone();
        return ret;
    }
            
    void serialize_order(SST::Core::Serialization::serializer &ser)  override {
        BaseNocEvent::serialize_order(ser);
        ser & dest_mesh_loc;
        ser & egress_port;
        ser & next_port;
        ser & encap_ev;
    }
    
private:
    ImplementSerializable(SST::Kingsley::noc_mesh_event)
        
};

    
}
}

#endif // COMPONENTS_KINGSLEY_NOC_MESH_H
