// -*- mode: c++ -*-

// Copyright 2009-2017 Sandia Corporation. Under the terms
// of Contract DE-NA0003525 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2017, Sandia Corporation
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
#include <sst/core/elementinfo.h>
#include <sst/core/event.h>
#include <sst/core/link.h>
#include <sst/core/timeConverter.h>


namespace SST {
namespace Merlin {

class LinkControl;

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
        {"id",               "Network ID of endpoint."},
        {"num_vns",          "Number of requested virtual networks."},
        {"link_bw",          "Bandwidth of the router link specified in either b/s or B/s (can include SI prefix)."},
        {"packet_size",      "Packet size specified in either b or B (can include SI prefix)."},
        {"packets_to_send",  "Number of packets to send in the test."},
        {"buffer_size",      "Size of input and output buffers specified in b or B (can include SI prefix)."}
    )

    SST_ELI_DOCUMENT_PORTS(
        {"rtr",  "Port that hooks up to router.", { "merlin.RtrEvent", "merlin.credit_event" } }
    )


private:
    int id;
    int num_vns;
    
    int packets_sent;
    int packets_recd;

    SimTime_t start_time;
    SimTime_t latency;
    
    int packets_to_send;
    int packet_size;
    UnitAlgebra buffer_size;
    
    LinkControl* link_control;
    Link* self_link;

public:
    pt2pt_test(ComponentId_t cid, Params& params);
    ~pt2pt_test() {}

    void init(unsigned int phase);
    void setup(); 
    void finish();


private:
    bool clock_handler(Cycle_t cycle);
    void handle_complete(Event* ev);
    
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
