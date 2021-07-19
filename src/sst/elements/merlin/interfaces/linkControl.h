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


#ifndef COMPONENTS_MERLIN_LINKCONTROL_H
#define COMPONENTS_MERLIN_LINKCONTROL_H

#include <sst/core/subcomponent.h>
#include <sst/core/unitAlgebra.h>

#include <sst/core/interfaces/simpleNetwork.h>

#include <sst/core/statapi/statbase.h>
#include <sst/core/shared/sharedArray.h>

#include "sst/elements/merlin/router.h"

#include <queue>

namespace SST {

class Component;

namespace Merlin {

// Whole class definition needs to be in the header file so that other
// libraries can use the class to talk with the merlin routers.

typedef std::queue<RtrEvent*> network_queue_t;

// Class to manage link between NIC and router.  A single NIC can have
// more than one link_control (and thus link to router).
class LinkControl : public SST::Interfaces::SimpleNetwork {

public:

    SST_ELI_REGISTER_SUBCOMPONENT_DERIVED(
        LinkControl,
        "merlin",
        "linkcontrol",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "Link Control module for building Merlin-enabled NICs",
        SST::Interfaces::SimpleNetwork)

    SST_ELI_DOCUMENT_PARAMS(
        {"port_name",          "Port name to connect to.  Only used when loaded anonymously",""},
        {"link_bw",            "Bandwidth of the links specified in either b/s or B/s (can include SI prefix)."},
        {"input_buf_size",     "Size of input buffers specified in b or B (can include SI prefix)."},
        {"output_buf_size",    "Size of output buffers specified in b or B (can include SI prefix)."},
        // {"network_inspectors", "Comma separated list of network inspectors to put on output ports.", ""},
        {"job_id",             "ID of the job this enpoint is part of.", "" },
        {"Job_size",           "Number of nodes in the job this endpoint is part of.",""},
        {"logical_nid",        "My logical NID", "" },
        {"use_nid_remap",      "If true, will remap logical nids in job to physical ids", "false" },
        {"nid_map_name",       "Base name of shared region where my NID map will be located.  If empty, no NID map will be used.",""},
        {"vn_remap",           "Remap VNs onto/off of the network.  If empty, no vn remapping is done", "" },

    )

    SST_ELI_DOCUMENT_STATISTICS(
        { "packet_latency",     "Histogram of latencies for received packets", "latency", 1},
        { "send_bit_count",     "Count number of bits sent on link", "bits", 1},
        { "output_port_stalls", "Time output port is stalled (in units of core timebase)", "time in stalls", 1},
        { "idle_time",          "Number of (in unites of core timebas) that port was idle", "time spent idle", 1},
        // { "recv_bit_count",     "Count number of bits received on the link", "bits", 1},
    )

    SST_ELI_DOCUMENT_PORTS(
        {"rtr_port", "Port that connects to router", { "merlin.RtrEvent", "merlin.credit_event", "" } },
    )


private:

    struct output_queue_bundle_t {
        network_queue_t queue;
        int vn;
        int credits;

        output_queue_bundle_t() :
            vn(-1),
            credits(0)
            {}
    };

    // Link to router
    Link* rtr_link;
    // Self link for timing output.  This is how we manage bandwidth
    // usage
    Link* output_timing;
    // Self link to use when waiting to send because of a congestion
    // eveng
    Link* congestion_timing;

    // Perforamne paramters
    UnitAlgebra link_bw;
    UnitAlgebra inbuf_size;
    UnitAlgebra outbuf_size;
    int flit_size; // in bits
    UnitAlgebra flit_size_ua;

    // Initialization events received from network
    std::deque<RtrEvent*> init_events;

    // Number of virtual networks
    int req_vns; // VNs requested be endpoint in constructor
    int used_vns; // VNs actually used based on VN mapping
    int total_vns; // Total VNs in the network

    int* vn_out_map;

    /******************************************************************
     * Data structures to hold output quewus/credits and mapping data
     *****************************************************************/
    // Holds the mapping from user VN to network VN.  Size is req_vns
    output_queue_bundle_t** vn_remap_out;

    // Holds the queues to output to network.  Size is used_vns
    output_queue_bundle_t* output_queues;

    // Holds the credits for the router input buffers.  Size is
    // total_vns.
    int* router_credits;

    /******************************************************************
     * Data structures to hold input quewus
     *****************************************************************/

    // Holds the credits to return to the router for my input buffers.
    // Size is total_vns.
    int* router_return_credits;

    // Input queues.  Size is req_vn
    network_queue_t* input_queues;

    SimTime_t last_time;
    SimTime_t last_recv_time;

    nid_t id;
    nid_t logical_nid;
    int job_id;
    Shared::SharedArray<nid_t> nid_map;
    bool use_nid_map;

    // Doing a round robin on the output.  Need to keep track of the
    // current virtual channel.
    int curr_out_vn;

    // Represents the start of when a port was idle
    // If the buffer was empty we instantiate this to the current time
    SimTime_t idle_start;
    bool is_idle;

    // Vairable to tell us if we are waiting for something to happen
    // before we begin more output.  The two things we are waiting on
    // is: 1 - adding new data to output buffers, or 2 - getting
    // credits back from the router.
    bool waiting;
    // Tracks whether we have packets while waiting.  If we do, that
    // means we're blocked and we need to keep track of block time
    bool have_packets;
    SimTime_t start_block;


    // Tracks congenstion state
    struct CongestionState {
        // Keeps track of how much we should backoff on sends.
        // Defaults to 1 (no backoff), will be set to a higher number
        // if congenstion management detects congestion and requests a
        // backoff.  Higher numbers will backoff more.
        int backoff;
        // If non-zero, this represents the next time data can be sent
        // to the target
        SimTime_t throttle_time;
        // Keep track of how many messages I have targeting this dest
        int count;

        // Need to be able to have an argument so we can emplace into
        // maps
        CongestionState(int backoff = 1) : backoff(backoff), throttle_time(0), count(0) {}
    };

    std::map<int,CongestionState> congestion_state;

    // Functors for notifying the parent when there is more space in
    // output queue or when a new packet arrives
    HandlerBase* receiveFunctor;
    HandlerBase* sendFunctor;

    // Component* parent;

    // PacketStats stats;
    // Statistics
    Statistic<uint64_t>* packet_latency;
    Statistic<uint64_t>* send_bit_count;
    Statistic<uint64_t>* output_port_stalls;
    Statistic<uint64_t>* idle_time;
    Statistic<uint64_t>* recv_bit_count;

    RtrInitEvent* checkInitProtocol(Event* ev, RtrInitEvent::Commands command, uint32_t line, const char* file, const char* func);

    Output& output;

public:
    LinkControl(ComponentId_t cid, Params &params, int vns);

    ~LinkControl();

    void setup();
    void init(unsigned int phase);
    void complete(unsigned int phase);
    void finish();

    // Returns true if there is space in the output buffer and false
    // otherwise.
    bool send(SST::Interfaces::SimpleNetwork::Request* req, int vn);

    // Returns true if there is space in the output buffer and false
    // otherwise.
    bool spaceToSend(int vn, int flits);

    // Returns NULL if no event in input_buf[vn]. Otherwise, returns
    // the next event.
    SST::Interfaces::SimpleNetwork::Request* recv(int vn);

    // Returns true if there is an event in the input buffer and false
    // otherwise.
    bool requestToReceive( int vn ) { return ! input_queues[vn].empty(); }

    void sendInitData(SST::Interfaces::SimpleNetwork::Request* ev);
    SST::Interfaces::SimpleNetwork::Request* recvInitData();

    void sendUntimedData(SST::Interfaces::SimpleNetwork::Request* ev);
    SST::Interfaces::SimpleNetwork::Request* recvUntimedData();

    inline void setNotifyOnReceive(HandlerBase* functor) { receiveFunctor = functor; }
    inline void setNotifyOnSend(HandlerBase* functor) { sendFunctor = functor; }

    inline bool isNetworkInitialized() const { return network_initialized; }
    // inline nid_t getEndpointID() const { return id; }
    inline nid_t getEndpointID() const {
        if ( use_nid_map ) {
            return logical_nid;
        }
        else {
            return id;
        }
    }
    inline const UnitAlgebra& getLinkBW() const { return link_bw; }


private:
    bool network_initialized;

    void handle_input(Event* ev);
    void handle_output(Event* ev);
    void handle_congestion(Event* ev);

    int sent;

    SimTime_t foo;
};

}
}

#endif // COMPONENTS_MERLIN_LINKCONTROL_H
