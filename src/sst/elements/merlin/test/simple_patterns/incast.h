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


#ifndef COMPONENTS_MERLIN_TEST_SIMPLE_PATTERNS_INCAST_H
#define COMPONENTS_MERLIN_TEST_SIMPLE_PATTERNS_INCAST_H

#include <sst/core/component.h>
#include <sst/core/event.h>
#include <sst/core/link.h>
#include <sst/core/timeConverter.h>
#include <sst/core/interfaces/simpleNetwork.h>


namespace SST {

namespace Merlin {

class incast_nic : public Component {

public:

    SST_ELI_REGISTER_COMPONENT(
        incast_nic,
        "merlin",
        "simple_patterns.incast",
        SST_ELI_ELEMENT_VERSION(0,9,0),
        "Simple pattern NIC doing an incast pattern.",
        COMPONENT_CATEGORY_NETWORK)

    SST_ELI_DOCUMENT_PARAMS(
        {"num_peers",       "Total number of endpoints in network."},
        {"target_nids",     "List of NIDs to target incast to.  All other nids will round robin packets to target nids"},
        {"packets_to_send", "Number of packets to send to each target nid.  If set to -1, it will send continuously, but won't register as a primary component","10"},
        {"packet_size",     "Packet size specified in either b or B (can include SI prefix).","64B"},
        {"delay_start",     "Amount of time to wait before starting to send packets","0ns"},
    )

    SST_ELI_DOCUMENT_PORTS(
    )

    SST_ELI_DOCUMENT_SUBCOMPONENT_SLOTS(
        {"networkIF", "Network interface", "SST::Interfaces::SimpleNetwork" }
    )


private:

    int id;
    int num_peers;
    std::vector<int> targets;
    bool target;

    int packet_size_in_bits;

    int curr_packets;
    int total_packets;

    SST::Interfaces::SimpleNetwork* link_control;
    Link* self_link;

    SimTime_t start_time;
    SimTime_t end_time;

    UnitAlgebra delay_start;

    Output& output;

public:
    incast_nic(ComponentId_t cid, Params& params);
    ~incast_nic();

    void init(unsigned int phase);
    void setup();
    void finish();


private:
    // for targets
    bool handle_event(int vn);
    void handle_complete(Event* ev);

    // for sources
    bool handle_sends(int vn);
    void handle_start(Event* ev);

};

}
}

#endif // COMPONENTS_MERLIN_TEST_SIMPLE_PATTERNS_SHIFT_H
