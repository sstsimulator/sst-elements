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


#ifndef COMPONENTS_MERLIN_TEST_BISECTION_BISECTION_TEST_H
#define COMPONENTS_MERLIN_TEST_BISECTION_BISECTION_TEST_H

#include <sst/core/component.h>
#include <sst/core/elementinfo.h>
#include <sst/core/event.h>
#include <sst/core/link.h>
#include <sst/core/timeConverter.h>

#include <sst/core/interfaces/simpleNetwork.h>


namespace SST {
namespace Merlin {


class bisection_test : public Component {

public:

    SST_ELI_REGISTER_COMPONENT(
        bisection_test,
        "merlin",
        "bisection_test",
        SST_ELI_ELEMENT_VERSION(0,9,0),
        "Simple NIC to test bisection bandwidth of a network.",
        COMPONENT_CATEGORY_NETWORK)
    
    SST_ELI_DOCUMENT_PARAMS(
        {"num_peers",       "Number of peers on the network (must be even number)"},
        {"link_bw",         "Bandwidth of the router link specified in either b/s or B/s (can include SI prefix).","2GB/s"},
        {"packet_size",     "Packet size specified in either b or B (can include SI prefix).","64B"},
        {"packets_to_send", "Number of packets to send in the test.", "32"},
        {"buffer_size",     "Size of input and output buffers specified in b or B (can include SI prefix).", "128B"},
        {"networkIF",       "Network interface to use.  Must inherit from SimpleNetwork", "merlin.linkcontrol"}
    )

    SST_ELI_DOCUMENT_PORTS(
        {"rtr",  "Port that hooks up to router.", { "merlin.RtrEvent", "merlin.credit_event" } }
    )

    
private:
    int id;
    int partner_id;
    int num_vns;
    int num_peers;
    
    int packets_sent;
    int packets_recd;

    SimTime_t start_time;
    
    int packets_to_send;
    int packet_size;
    UnitAlgebra buffer_size;
    
    SST::Interfaces::SimpleNetwork* link_control;
    Link* self_link;

public:
    bisection_test(ComponentId_t cid, Params& params);
    ~bisection_test() {}

    void init(unsigned int phase);
    void setup(); 
    void finish();


private:
    bool clock_handler(Cycle_t cycle);
    void handle_complete(Event* ev);
    bool receive_handler(int vn);
    bool send_handler(int vn);
};

class bisection_test_event : public Event {

 public:
    SimTime_t start_time;

    bisection_test_event() {}
    
    virtual Event* clone(void) override
    {
        return new bisection_test_event(*this);
    }

    void serialize_order(SST::Core::Serialization::serializer &ser)  override {
        Event::serialize_order(ser);
        ser & start_time;
    }

private:
    ImplementSerializable(SST::Merlin::bisection_test_event)
    
};
} // namespace merlin
} // namespace sst
#endif // COMPONENTS_MERLIN_TEST_NIC_H
