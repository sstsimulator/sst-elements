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


#ifndef COMPONENTS_MERLIN_BACKGROUND_TRAFFIC_H
#define COMPONENTS_MERLIN_BACKGROUND_TRAFFIC_H

// #include <cstdlib>

// #include <sst/core/rng/mersenne.h>
// #include <sst/core/rng/gaussian.h>
// #include <sst/core/rng/discrete.h>
// #include <sst/core/rng/expon.h>
// #include <sst/core/rng/uniform.h>

#include <sst/core/component.h>
#include <sst/core/event.h>
#include <sst/core/link.h>
#include <sst/core/timeConverter.h>
#include <sst/core/output.h>
#include "sst/core/interfaces/simpleNetwork.h"

#include "sst/elements/merlin/target_generator/target_generator.h"

namespace SST {
namespace Merlin {


class background_traffic_event : public Event {
public:
    background_traffic_event() : Event() {}

    virtual ~background_traffic_event() {  }

    void serialize_order(SST::Core::Serialization::serializer &ser)  override {
        Event::serialize_order(ser);
    }

private:
    ImplementSerializable(SST::Merlin::background_traffic_event)

};




class BackgroundTraffic : public Component {

public:

    SST_ELI_REGISTER_COMPONENT(
        BackgroundTraffic,
        "merlin",
        "background_traffic",
        SST_ELI_ELEMENT_VERSION(0,0,1),
        "Pattern-based traffic generator to create background traffic.",
        COMPONENT_CATEGORY_NETWORK)

    SST_ELI_DOCUMENT_PARAMS(
        {"num_peers",        "Total number of endpoints in network."},
        {"packet_size",      "Packet size specified in either b or B (can include SI prefix).","32B"},
        {"pattern",          "Traffic pattern to use.","merlin.targetgen.uniform"},
        {"offered_load",     "Load to be offered to network.  Valid range: 0 < offered_load <= 1.0."},
    )

    SST_ELI_DOCUMENT_PORTS(
        {"rtr",  "Port that hooks up to router.", { "merlin.RtrEvent", "merlin.credit_event" } }
    )

    SST_ELI_DOCUMENT_SUBCOMPONENT_SLOTS(
        {"networkIF", "Network interface", "SST::Interfaces::SimpleNetwork" },
        {"pattern_gen", "Target address generator", "SST::Merlin::TargetGenerator" }
    )


private:

    double offered_load;
    UnitAlgebra link_bw;

    Params* pattern_params;

    UnitAlgebra serialization_time;

    SimTime_t next_time;
    SimTime_t send_interval;

    TimeConverter* base_tc;

    SST::Interfaces::SimpleNetwork* link_if;
    SST::Interfaces::SimpleNetwork::Handler<BackgroundTraffic>* send_notify_functor;
    SST::Interfaces::SimpleNetwork::Handler<BackgroundTraffic>* recv_notify_functor;


    TargetGenerator *packetDestGen;

    int id;
    int num_peers;
    int packet_size; // in bits

    uint64_t packets_sent;
    uint64_t packets_recd;

    Link* timing_link;

public:
    BackgroundTraffic(ComponentId_t cid, Params& params);
    ~BackgroundTraffic();

    void init(unsigned int phase);
    void setup();
    void complete(unsigned int phase);
    void finish();


private:
    bool handle_receives(int vn);
    bool send_notify(int vn);

    void output_timing(Event* ev);
    void progress_messages(SimTime_t current_time);

};

} //namespace Merlin
} //namespace SST

#endif
