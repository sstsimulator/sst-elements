// -*- mode: c++ -*-

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


#ifndef COMPONENTS_MERLIN_TEST_SIMPLE_PATTERNS_SHIFT_H
#define COMPONENTS_MERLIN_TEST_SIMPLE_PATTERNS_SHIFT_H

#include <sst/core/component.h>
#include <sst/core/elementinfo.h>
#include <sst/core/event.h>
#include <sst/core/link.h>
#include <sst/core/timeConverter.h>
#include <sst/core/interfaces/simpleNetwork.h>


namespace SST {

namespace Merlin {

class shift_nic : public Component {

public:

    SST_ELI_REGISTER_COMPONENT(
        shift_nic,
        "merlin",
        "shift_nic",
        SST_ELI_ELEMENT_VERSION(0,9,0),
        "Simple pattern NIC doing a shift pattern.",
        COMPONENT_CATEGORY_NETWORK)
    
    SST_ELI_DOCUMENT_PARAMS(
        {"id",              "Network ID of endpoint."},
        {"num_peers",       "Total number of endpoints in network."},
        {"shift",           "Number of logical network endpoints to shift to use as destination for packets."},
        {"packets_to_send", "Number of packets to send in the test.","10"},
        {"packet_size",     "Packet size specified in either b or B (can include SI prefix).","64B"},
        {"link_bw",         "Bandwidth of the router link specified in either b/s or B/s (can include SI prefix)."},
        {"remap",           "Creates a logical to physical mapping shifted by remap amount.", "0"}
    )

    SST_ELI_DOCUMENT_PORTS(
        {"rtr",  "Port that hooks up to router.", { "merlin.RtrEvent", "merlin.credit_event" } }
    )

    

private:

    int id;
    int net_id;
    int num_peers;
    int shift;
    int target;
    int send_seq;
    int recv_seq;
    int num_ooo;

    int num_msg;
    // UnitAlgebra packet_size;
    int size_in_bits;
    int packets_sent;
    int packets_recd;
    int stalled_cycles;

    bool send_done;
    bool recv_done;
    bool initialized;
    
    SST::Interfaces::SimpleNetwork* link_control;

    int remap;

    Output& output;
    
public:
    shift_nic(ComponentId_t cid, Params& params);
    ~shift_nic();

    void init(unsigned int phase);
    void setup(); 
    void finish();


private:
    bool clock_handler(Cycle_t cycle);
    bool handle_event(int vn);

};

}
}

#endif // COMPONENTS_MERLIN_TEST_SIMPLE_PATTERNS_SHIFT_H
