// -*- mode: c++ -*-

// Copyright 2009-2025 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2025, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// of the distribution for more information.
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

    SST_ELI_REGISTER_SUBCOMPONENT(
        PortControl,
        "merlin",
        "portcontrol",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "Port Control module for use by hr_router",
        SST::Merlin::PortInterface
    )

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
    Link* port_link = nullptr;
    // Self link for timing output.  This is how we manage bandwidth
    // usage
    Link* output_timing = nullptr;
    TimeConverter flit_cycle;

	// Self link for dynamic link additions
	Link* dynlink_timing = nullptr;
	// Threshold of how idle a link is before it reduces link width 0 to 1 (negative means no link adjustments).
	// i.e. if (idle > dlink_thresh) then reduce link width.
	float dlink_thresh = 0.0;

	// Self link for disabling a port temporarily
	Link* disable_timing = nullptr;

    std::deque<Event*> init_events;

    int rtr_id = -1;
    // Number of virtual channels
    int num_vcs = 0;

    int num_vns = 0;
    std::string vn_remap_shm;
    int vn_remap_shm_size = 0;
    Shared::SharedArray<int> vn_remap;

	int max_link_width = 0;
	int cur_link_width = 0;

	// Delay before a link width can be increased (limited by PLL ~400ns to 1us).
	UnitAlgebra link_width_transition_delay;

    UnitAlgebra link_bw;
    UnitAlgebra flit_size;
    UnitAlgebra input_buf_size;
    UnitAlgebra output_buf_size;

    Topology* topo = nullptr;
    int port_number = -1;
    bool host_port = false;
    bool remote_rdy_for_credits = false;

    int remote_rtr_id = -1;
    int remote_port_number = -1;

    // One buffer for each virtual network.  At the NIC level, we just
    // provide a virtual network abstraction.
    port_queue_t* input_buf = nullptr;
    port_queue_t* output_buf = nullptr;

    // Need an output queue for topology events.  Incoming topology
    // events will be directed right to the topolgy object.
    ctrl_queue_t ctrl_queue;

    // We need to avoid checking all the queues every time we clock
    // the arbitration logic, so we'll have each PortControl put the
    // head of each of its VC queues into a single array to speed
    // things up.  This is an array passed into the constructor.
    internal_router_event** vc_heads = nullptr;

    int* input_buf_count = nullptr;
    int* output_buf_count = nullptr;

    // Keep track of how many items are in the output queues.  This
    // can be used by topology objects to make adaptive routing
    // decisions.
    int* output_queue_lengths = nullptr;

    // Tracks whether queue_lengths are per VC or for the entire port.
    // If for the entire port, it will increment and decrement all VCs
    // so that they track the enire queue occupancy for the port.
    bool oql_track_port = false;

    // Tracks whether or not the output_queue_lengths will track only
    // the local output port, or will also track the next input queue.
    // In this mode, it only decrements the count on returned credits
    // from next router.
    bool oql_track_remote = false;



    // Variables to keep track of credits.  You need to keep track of
    // the credits available for your next buffer, as well as track
    // the credits you need to return to the buffer sending data to
    // you,
    int* xbar_in_credits = nullptr;

    int* port_ret_credits = nullptr;
    int* port_out_credits = nullptr;

    // Represents the start of when a port was idle
    // If the buffer was empty we instantiate this to the current time
    SimTime_t idle_start = 0;
	// Represents the start of a data transmit
	SimTime_t active_start = 0;

	// Tells us whether the link was in an idle or active state.
	// Used to let us know when we need to record an idle window
	// as a statistic.
	bool is_idle = false;
	bool is_active = false;

    // Vairable to tell us if we are waiting for something to happen
    // before we begin more output.  The two things we are waiting on
    // is: 1 - adding new data to output buffers, or 2 - getting
    // credits back from the router.
    bool waiting = false;
    // Tracks whether we have packets while waiting.  If we do, that
    // means we're blocked and we need to keep track of block time
    bool have_packets = false;
    SimTime_t start_block = 0;

    Router* parent = nullptr;
    bool connected = false;

    // Statistics
    Statistic<uint64_t>* send_bit_count;
    Statistic<uint64_t>* send_packet_count;
    Statistic<uint64_t>* output_port_stalls;
    Statistic<uint64_t>* idle_time;
    Statistic<uint64_t>* width_adj_count;

	// SAI Metrics (S+A+I=1) corresponds to
	// sai_win_start to (sai_win_start + sai_win_length)
	double stalled = 0.0;
	double active = 0.0;
	double idle = 0.0;

	// The length of time SAI represents
	double sai_win_length = 0.0;
	uint64_t sai_win_length_nano = 0;
	uint64_t sai_win_length_pico = 0;
	// The start time of the window
	SimTime_t sai_win_start = 0;
	// The amount of delay from adjusting a link width
	SimTime_t sai_adj_delay = 0;
	// After adjusting a link the port is disable for sai_adj_delay
	bool sai_port_disabled = false;

    // Specifies if the active time is going past the window
	bool ongoing_transmit = false;
	uint64_t time_active_nano_remaining = 0;

    RtrInitEvent* checkInitProtocol(Event* ev, RtrInitEvent::Commands command, uint32_t line, const char* file, const char* func);

    Output& output;

    PortInterface::OutputArbitration* output_arb = nullptr;

    // For supporting congestion management
    struct CongestionInfo {
        int32_t  src = 0;
        uint32_t  count = 0;
        uint32_t  total_count = 0;
        uint32_t  flit_count = 0;
        uint32_t  throttle = 0;
        SimTime_t last_seen = 0;
        SimTime_t expiration_time = 0;
        bool active = false;
        bool reported_done = false;

        CongestionInfo() = default;

        CongestionInfo(uint32_t src) : src(src), count(0), total_count(0), flit_count(0), throttle(0),last_seen(0), expiration_time(0), active(false), reported_done(false)  {}

        void serialize_order(SST::Core::Serialization::serializer& ser) {
            SST_SER(src);
            SST_SER(count);
            SST_SER(total_count);
            SST_SER(flit_count);
            SST_SER(throttle);
            SST_SER(last_seen);
            SST_SER(expiration_time);
            SST_SER(active);
            SST_SER(reported_done);
        }
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

    SimTime_t mtu_ser_time = 0;
    SimTime_t flit_ser_time = 0;
    bool enable_congestion_management = false;
    bool cm_activated = false;
    int cm_outstanding_threshold = 0;
    int cm_incast_threshold = 0;
    int cm_pktsize_threshold = 0;
    double cm_window_factor = 0.0;

    std::map<uint32_t,CongestionInfo> congestion_map;
    std::priority_queue<CongestionInfo*, std::vector<CongestionInfo* >, congestion_info_expiration_pq_order> expiration_queue;
    int current_incast = 0;
    int total_flits_incoming = 0;
    int total_incast_flits = 0;
    int congestion_events = 0;
    int congestion_count_at_last_throttle = 0;

public:

    void recvCtrlEvent(CtrlRtrEvent* ev) override;
    void sendCtrlEvent(CtrlRtrEvent* ev) override;
    // Returns true if there is space in the output buffer and false
    // otherwise.
    void send(internal_router_event* ev, int vc) override;
    // Returns true if there is space in the output buffer and false
    // otherwise.
    bool spaceToSend(int vc, int flits) override;
    // Returns NULL if no event in input_buf[vc]. Otherwise, returns
    // the next event.
    internal_router_event* recv(int vc) override;
    internal_router_event** getVCHeads() override
    {
    	return vc_heads;
    }
    virtual void reportIncomingEvent(internal_router_event* ev) override;

    PortControl() : PortInterface(), output(getSimulationOutput()) {}

    // time_base is a frequency which represents the bandwidth of the link in flits/second.
    PortControl(ComponentId_t cid, Params& params, Router* rif, int rtr_id, int port_number, Topology *topo);

    void initVCs(int vns, int* vcs_per_vn, internal_router_event** vc_heads, int* xbar_in_credits, int* output_queue_lengths) override;

    // Re-establish non-owning pointers after checkpoint restart
    void restoreSharedArrays(internal_router_event** vc_heads_in, int* xbar_in_credits_in, int* output_queue_lengths_in)
    {
        vc_heads = vc_heads_in;
        if ( connected ) {
            xbar_in_credits = xbar_in_credits_in;
            output_queue_lengths = output_queue_lengths_in;
        }
    }

    // After checkpoint restart, repopulate vc_heads from deserialized
    // input_buf fronts. Events were already routed pre-checkpoint so
    // we must NOT call route_packet() again.
    void repopulateVCHeads()
    {
        if ( !connected ) return;
        for ( int i = 0; i < num_vcs; i++ ) {
            if ( !input_buf[i].empty() ) {
                vc_heads[i] = input_buf[i].front();
            }
        }
    }


    ~PortControl();

    void serialize_order(SST::Core::Serialization::serializer& ser) override;
    ImplementSerializable(SST::Merlin::PortControl)
    void setup() override;
    void finish() override;
    void init(unsigned int phase) override;
    void complete(unsigned int phase) override;


    void sendUntimedData(Event *ev) override;
    Event* recvUntimedData() override;

    void dumpState(std::ostream& stream) override;
    void printStatus(Output& out, int out_port_busy, int in_port_busy) override;

	bool decreaseLinkWidth() override;
	bool increaseLinkWidth() override;

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
