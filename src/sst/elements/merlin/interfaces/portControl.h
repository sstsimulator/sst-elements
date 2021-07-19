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


#ifndef COMPONENTS_MERLIN_PORTCONTROL_H
#define COMPONENTS_MERLIN_PORTCONTROL_H

#include <sst/core/component.h>
#include <sst/core/event.h>
#include <sst/core/link.h>
#include <sst/core/output.h>
#include <sst/core/timeConverter.h>
#include <sst/core/unitAlgebra.h>
#include <sst/core/shared/sharedArray.h>

#include <sst/core/statapi/stataccumulator.h>

#include <cstring>

#include "sst/elements/merlin/router.h"

using namespace SST;

namespace SST {

namespace Merlin {


// Class to manage link between NIC and router.  A single NIC can have
// more than one link_control (and thus link to router).
class PortControl : public PortInterface {
public:

    SST_ELI_REGISTER_SUBCOMPONENT_DERIVED(
        PortControl,
        "merlin",
        "portcontrol",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "Port Control module for use by hr_router",
        SST::Merlin::PortInterface)

    SST_ELI_DOCUMENT_PARAMS(
        {"port_name",          "Port name to connect to.  Only used when loaded anonymously",""},
        {"link_bw",            "Bandwidth of the links specified in either b/s or B/s (can include SI prefix)."},
        {"flit_size",          "Size of a flit in either b or B (can include SI prefix)."},
        {"input_latency",       "", "0ns"},
        {"output_latency",      "", "0ns"},
        {"input_buf_size",     "Size of input buffers specified in b or B (can include SI prefix)."},
        {"output_buf_size",    "Size of output buffers specified in b or B (can include SI prefix)."},
        {"network_inspectors", "Comma separated list of network inspectors to put on output ports.", ""},
        {"dlink_thresh",       ""},
        {"num_vns",            "Number of VNs set in router or python file (-1 if not set in the parent router)."},
        {"vn_remap_shm",       "Name of shared memory region for vn remapping.  If empty, no remapping is done", ""},
        {"vn_remap_shm_size",  "Size of shared memory region for vn remapping.  If empty, no remapping is done", "-1"},
        {"oql_track_port",     ""},
        {"oql_track_remote",   ""},
        {"output_arb",         "Arbitration unit to be used for port output", "merlin.arb.output.basic"},
        {"mtu",                "Maximum transfer unit on network in b or B (can include SI prefix).","2kB"},
        {"enable_congestion_management", "Turn on congestion management","false"},
        {"cm_outstanding_threshold", "Threshold for the amount of data outstanding to a host before congestion management can trigger","2*output_buf_size"},
        {"cm_pktsize_threshold", "Minimum size of a packet to be considered part of a stream with regards to congestion management","128B"},
        {"cm_incast_threshold", "Numbr of hosts sending to an enpoint needed to trigger congestion management","6"}
    )

    // SST_ELI_DOCUMENT_STATISTICS(
    //     { "send_bit_count",     "Count number of bits sent on link", "bits", 1},
    //     { "send_packet_count",  "Count number of packets sent on link", "packets", 1},
    //     { "output_port_stalls", "Time output port is stalled (in units of core timebase)", "time in stalls", 1},
    //     { "idle_time",          "Number of (in unites of core timebas) that port was idle", "time spent idle", 1},
    //     { "width_adj_count",    "Number of times the width of a link was adjusted to change the power on the link", "times", 1},
    // )

    // SST_ELI_DOCUMENT_PORTS(
    //     {"rtr_port", "Port that connects to router", { "merlin.RtrEvent", "merlin.credit_event", "" } },
    // )

    SST_ELI_DOCUMENT_SUBCOMPONENT_SLOTS(
        {"inspector_slot", "Network inspectors", "SST::Interfaces::SimpleNetwork::NetworkInspector" },
        {"arbitration", "Arbitration unit to use for output", "SST::Merlin::OutputArbitration" }
    )

private:
    // Link to NIC or other router
    Link* port_link;
    // Self link for timing output.  This is how we manage bandwidth
    // usage
    Link* output_timing;
    TimeConverter* flit_cycle;

	// Self link for dynamic link additions
	Link* dynlink_timing;
	// Threshold of how idle a link is before it reduces link width 0 to 1 (negative means no link adjustments).
	// i.e. if (idle > dlink_thresh) then reduce link width.
	float dlink_thresh;

	// Self link for disabling a port temporarily
	Link* disable_timing;

    std::deque<Event*> init_events;

    int rtr_id;
    // Number of virtual channels
    int num_vcs;

    int num_vns;
    std::string vn_remap_shm;
    int vn_remap_shm_size;
    Shared::SharedArray<int> vn_remap;

	int max_link_width;
	int cur_link_width;

	// Delay before a link width can be increased (limited by PLL ~400ns to 1us).
	UnitAlgebra link_width_transition_delay;

    UnitAlgebra link_bw;
    UnitAlgebra flit_size;
    UnitAlgebra input_buf_size;
    UnitAlgebra output_buf_size;

    Topology* topo;
    int port_number;
    bool host_port;
    bool remote_rdy_for_credits;

    int remote_rtr_id;
    int remote_port_number;

    // One buffer for each virtual network.  At the NIC level, we just
    // provide a virtual network abstraction.
    port_queue_t* input_buf;
    port_queue_t* output_buf;

    // Need an output queue for topology events.  Incoming topology
    // events will be directed right to the topolgy object.
    ctrl_queue_t ctrl_queue;

    // We need to avoid checking all the queues every time we clock
    // the arbitration logic, so we'll have each PortControl put the
    // head of each of its VC queues into a single array to speed
    // things up.  This is an array passed into the constructor.
    internal_router_event** vc_heads;

    int* input_buf_count;
    int* output_buf_count;

    // Keep track of how many items are in the output queues.  This
    // can be used by topology objects to make adaptive routing
    // decisions.
    int* output_queue_lengths;

    // Tracks whether queue_lengths are per VC or for the entire port.
    // If for the entire port, it will increment and decrement all VCs
    // so that they track the enire queue occupancy for the port.
    bool oql_track_port;

    // Tracks whether or not the output_queue_lengths will track only
    // the local output port, or will also track the next input queue.
    // In this mode, it only decrements the count on returned credits
    // from next router.
    bool oql_track_remote;



    // Variables to keep track of credits.  You need to keep track of
    // the credits available for your next buffer, as well as track
    // the credits you need to return to the buffer sending data to
    // you,
    int* xbar_in_credits;

    int* port_ret_credits;
    int* port_out_credits;

    // Represents the start of when a port was idle
    // If the buffer was empty we instantiate this to the current time
    SimTime_t idle_start;
	// Represents the start of a data transmit
	SimTime_t active_start;

	// Tells us whether the link was in an idle or active state.
	// Used to let us know when we need to record an idle window
	// as a statistic.
	bool is_idle;
	bool is_active;

    // Vairable to tell us if we are waiting for something to happen
    // before we begin more output.  The two things we are waiting on
    // is: 1 - adding new data to output buffers, or 2 - getting
    // credits back from the router.
    bool waiting;
    // Tracks whether we have packets while waiting.  If we do, that
    // means we're blocked and we need to keep track of block time
    bool have_packets;
    SimTime_t start_block;

    Router* parent;
    bool connected;

    // Statistics
    Statistic<uint64_t>* send_bit_count;
    Statistic<uint64_t>* send_packet_count;
    Statistic<uint64_t>* output_port_stalls;
    Statistic<uint64_t>* idle_time;
    Statistic<uint64_t>* width_adj_count;

	// SAI Metrics (S+A+I=1) corresponds to
	// sai_win_start to (sai_win_start + sai_win_length)
	double stalled;
	double active;
	double idle;

	// The length of time SAI represents
	double sai_win_length;
	uint64_t sai_win_length_nano;
	uint64_t sai_win_length_pico;
	// The start time of the window
	SimTime_t sai_win_start;
	// The amount of delay from adjusting a link width
	SimTime_t sai_adj_delay;
	// After adjusting a link the port is disable for sai_adj_delay
	bool sai_port_disabled;

    // Specifies if the active time is going past the window
	bool ongoing_transmit;
	uint64_t time_active_nano_remaining;

    RtrInitEvent* checkInitProtocol(Event* ev, RtrInitEvent::Commands command, uint32_t line, const char* file, const char* func);

    Output& output;

    PortInterface::OutputArbitration* output_arb;

    // For supporting congestion management
    struct CongestionInfo {
        const int32_t  src;
        uint32_t  count;
        uint32_t  total_count;
        uint32_t  flit_count;
        uint32_t  throttle;
        SimTime_t last_seen;
        SimTime_t expiration_time;
        bool active;
        bool reported_done;

        CongestionInfo(uint32_t src) : src(src), count(0), total_count(0), flit_count(0), throttle(0),last_seen(0), expiration_time(0), active(false), reported_done(false)  {}

    };

    class congestion_info_expiration_pq_order {
    public:
        /** Compare based off pointers */
        inline bool operator()(const CongestionInfo* lhs, const CongestionInfo* rhs) const {
            return lhs->expiration_time > rhs->expiration_time;
        }

        /** Compare based off references */
        inline bool operator()(const CongestionInfo& lhs, const CongestionInfo& rhs) const {
            return lhs.expiration_time > rhs.expiration_time;
        }
    };

    class congestion_info_src_set_order {
    public:
        /** Compare based off pointers */
        inline bool operator()(const CongestionInfo* lhs, const CongestionInfo* rhs) const {
            return lhs->src < rhs->src;
        }

        /** Compare based off references */
        inline bool operator()(const CongestionInfo& lhs, const CongestionInfo& rhs) const {
            return lhs.src < rhs.src;
        }
    };

    SimTime_t mtu_ser_time;
    SimTime_t flit_ser_time;
    bool enable_congestion_management;
    bool cm_activated;
    int cm_outstanding_threshold;
    int cm_incast_threshold;
    int cm_pktsize_threshold;
    double cm_window_factor;

    std::map<uint32_t,CongestionInfo> congestion_map;
    std::priority_queue<CongestionInfo*, std::vector<CongestionInfo* >, congestion_info_expiration_pq_order> expiration_queue;
    int current_incast;
    int total_flits_incoming;
    int total_incast_flits;
    int congestion_events;
    int congestion_count_at_last_throttle;

public:

    void recvCtrlEvent(CtrlRtrEvent* ev);
    void sendCtrlEvent(CtrlRtrEvent* ev);
    // Returns true if there is space in the output buffer and false
    // otherwise.
    void send(internal_router_event* ev, int vc);
    // Returns true if there is space in the output buffer and false
    // otherwise.
    bool spaceToSend(int vc, int flits);
    // Returns NULL if no event in input_buf[vc]. Otherwise, returns
    // the next event.
    internal_router_event* recv(int vc);
    internal_router_event** getVCHeads() {
    	return vc_heads;
    }
    virtual void reportIncomingEvent(internal_router_event* ev);

    // time_base is a frequency which represents the bandwidth of the link in flits/second.
    PortControl(ComponentId_t cid, Params& params, Router* rif, int rtr_id, int port_number, Topology *topo);

    void initVCs(int vns, int* vcs_per_vn, internal_router_event** vc_heads, int* xbar_in_credits, int* output_queue_lengths);


    ~PortControl();
    void setup();
    void finish();
    void init(unsigned int phase);
    void complete(unsigned int phase);


    void sendInitData(Event *ev);
    Event* recvInitData();
    void sendUntimedData(Event *ev);
    Event* recvUntimedData();

    void dumpState(std::ostream& stream);
    void printStatus(Output& out, int out_port_busy, int in_port_busy);

	bool decreaseLinkWidth();
	bool increaseLinkWidth();

private:

    std::vector<SST::Interfaces::SimpleNetwork::NetworkInspector*> network_inspectors;

    void dumpQueueState(port_queue_t& q, std::ostream& stream);
    void dumpQueueState(port_queue_t& q, Output& out);

    void handle_input_n2r(Event* ev);
    void handle_input_r2r(Event* ev);
    void handle_output(Event* ev);
    void handle_failed(Event* ev);
    void handleSAIWindow(Event* ev);
    void reenablePort(Event* ev);

	uint64_t increaseActive();

    void updateCongestionState(internal_router_event* send_event);
};


}  // Namespace merlin
}  // Namespace sst

#endif // COMPONENTS_MERLIN_PORTCONTROL_H
