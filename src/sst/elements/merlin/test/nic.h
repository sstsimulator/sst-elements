// -*- mode: c++ -*-

// Copyright 2009-2020 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2020, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#ifndef COMPONENTS_MERLIN_TEST_NIC_H
#define COMPONENTS_MERLIN_TEST_NIC_H

#include <sst/core/component.h>
#include <sst/core/event.h>
#include <sst/core/link.h>
#include <sst/core/timeConverter.h>
#include <sst/core/interfaces/simpleNetwork.h>


namespace SST {

namespace Merlin {


class nic : public Component {

public:

    SST_ELI_REGISTER_COMPONENT(
        nic,
        "merlin",
        "test_nic",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "Simple NIC to test base functionality.",
        COMPONENT_CATEGORY_NETWORK)

    SST_ELI_DOCUMENT_PARAMS(
        {"id",           "Network ID of endpoint."},
        {"num_peers",    "Total number of endpoints in network."},
        {"num_messages", "Total number of messages to send to each endpoint."},
        {"message_size", "Size of each message to be sent specified in either b or B (can include SI prefix)."},
        {"send_untimed_broadcast",   "Controls whether data is sent in init and complete.","false"},
    )

    SST_ELI_DOCUMENT_PORTS(
    )

    SST_ELI_DOCUMENT_SUBCOMPONENT_SLOTS(
        {"networkIF", "Network interface", "SST::Interfaces::SimpleNetwork" }
    )

private:

    // SST::Interfaces::SimpleNetwork::nid_t id;
    int id;

    // passed in parameters
    int net_id;
    int num_peers;
    int msg_size;
    int num_msg;
    int group_offset;
    int group_peers;

    int packets_sent;
    int packets_recd;
    int stalled_cycles;
    int expected_recv_count;

    bool done;
    bool initialized;
    int init_state;
    int init_count;
    int init_broadcast_count;

    bool send_untimed_bcast;

    SST::Interfaces::SimpleNetwork* link_control;

    int last_target;

    int *next_seq;

    Output& output;

public:
    nic(ComponentId_t cid, Params& params);
    ~nic();

    void init(unsigned int phase);
    void complete(unsigned int phase);
    void setup();
    void finish();


private:
    bool clock_handler(Cycle_t cycle);
    void init_complete(unsigned int phase);

};

}
}

#endif // COMPONENTS_MERLIN_TEST_NIC_H
