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


#ifndef COMPONENTS_MERLIN_TEST_PT2PT_PT2PT_TEST_H
#define COMPONENTS_MERLIN_TEST_PT2PT_PT2PT_TEST_H

#include <sst/core/component.h>
#include <sst/core/event.h>
#include <sst/core/link.h>
#include <sst/core/output.h>
#include <sst/core/timeConverter.h>
#include <sst/core/interfaces/simpleNetwork.h>

namespace SST {
namespace Merlin {

class pt2pt_test : public Component {

public:

    SST_ELI_REGISTER_COMPONENT(
        pt2pt_test,
        "merlin",
        "pt2pt_test",
        SST_ELI_ELEMENT_VERSION(0,9,0),
        "Simple NIC to test basic pt2pt performance.",
        COMPONENT_CATEGORY_NETWORK)

    SST_ELI_DOCUMENT_PARAMS(
        {"link_bw",          "Bandwidth of the router link specified in either b/s or B/s (can include SI prefix)."},
        {"packet_size",      "Packet size specified in either b or B (can include SI prefix)."},
        {"packets_to_send",  "Number of packets to send in the test."},
        {"buffer_size",      "Size of input and output buffers specified in b or B (can include SI prefix)."},
        {"src",              "Array of IDs of NICs that will send data."},
        {"dest",             "Array of IDs of NICs to send data to."},
        {"stream_delays",    "Array of times before starting each of the streams.  Default is to start all of them at simulation start."},
        {"report_interval",  "Intefval to report bandwidth numbers.  Default is to only print at the end", "0ns"},
        {"linkcontrol",      "SimpleNetwork class to use to talk to network."}
    )

    SST_ELI_DOCUMENT_PORTS(
        {"rtr",  "Port that hooks up to router.", { "merlin.RtrEvent", "merlin.credit_event" } }
    )

    SST_ELI_DOCUMENT_SUBCOMPONENT_SLOTS(
        {"networkIF", "Network interface", "SST::Interfaces::SimpleNetwork" }
    )

private:

    Output out;

    struct recv_data {
        SimTime_t first_arrival;
        SimTime_t end_arrival;
        int packets_recd;

        recv_data():
            first_arrival(0),
            end_arrival(0),
            packets_recd(0) {}
    };

    int id;

    std::vector<int> src;
    std::vector<int> dest;
    std::vector<UnitAlgebra> delays;

    int my_dest;
    UnitAlgebra my_delay;

    std::map<int,recv_data> my_recvs;

    int packets_sent;
    int packets_recd;

    SimTime_t start_time;
    SimTime_t latency;

    int packets_to_send;
    int packet_size;
    UnitAlgebra buffer_size;

    SST::Interfaces::SimpleNetwork* link_control;
    Link* self_link;
    Link* report_timing;

    void start(Event* ev);

    // Variables need to print out bandwidth in intervals
    UnitAlgebra bw_multiplier;
    UnitAlgebra bw_multiplier_partial;
    UnitAlgebra report_interval;
    UnitAlgebra interval_start_bw;
    SimTime_t last_pkt_recd;
    SimTime_t pkt_ser_cycles;
    int pkts_in_interval;

    void report_bw(Event* ev);

public:
    pt2pt_test(ComponentId_t cid, Params& params);
    ~pt2pt_test() {}

    void init(unsigned int phase);
    void setup();
    void finish();


private:
    // bool clock_handler(Cycle_t cycle);
    // void handle_complete(Event* ev);

    bool send_handler(int vn);
    bool recv_handler(int vn);


};

class pt2pt_test_event : public Event {

 public:
    SimTime_t start_time;

    pt2pt_test_event() {}

    virtual Event* clone(void) override
    {
        return new pt2pt_test_event(*this);
    }

    ImplementSerializable(SST::Merlin::pt2pt_test_event)
};

}
}

#endif // COMPONENTS_MERLIN_TEST_NIC_H
