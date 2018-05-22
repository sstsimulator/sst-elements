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


#ifndef COMPONENTS_MERLIN_OFFERED_LOAD_H
#define COMPONENTS_MERLIN_OFFERED_LOAD_H

// #include <cstdlib>

// #include <sst/core/rng/mersenne.h>
// #include <sst/core/rng/gaussian.h>
// #include <sst/core/rng/discrete.h>
// #include <sst/core/rng/expon.h>
// #include <sst/core/rng/uniform.h>

#include <sst/core/component.h>
#include <sst/core/elementinfo.h>
#include <sst/core/event.h>
#include <sst/core/link.h>
#include <sst/core/timeConverter.h>
#include <sst/core/output.h>
#include "sst/core/interfaces/simpleNetwork.h"

#include "sst/elements/merlin/target_generator/target_generator.h"

namespace SST {
namespace Merlin {


class offered_load_event : public Event {
public:
    SimTime_t start_time;

    offered_load_event() : Event() {}
    offered_load_event(SimTime_t start_time) :
        Event(),
        start_time(start_time)
    {}

    virtual ~offered_load_event() {  }

    void serialize_order(SST::Core::Serialization::serializer &ser)  override {
        Event::serialize_order(ser);
        ser & start_time;
    }

private:
    ImplementSerializable(SST::Merlin::offered_load_event)

};


class offered_load_complete_event : public Event {
public:
    int generation;
    SimTime_t sum;
    SimTime_t sum_of_squares;
    SimTime_t min;
    SimTime_t max;
    uint64_t  count;
    SimTime_t backup;
    
    offered_load_complete_event(int generation) :
        Event(),
        generation(generation),
        sum(0),
        sum_of_squares(0),
        min(MAX_SIMTIME_T),
        max(0),
        count(0)
        {}

    virtual ~offered_load_complete_event() {  }

    virtual offered_load_complete_event* clone(void)  override {
        offered_load_complete_event *ret = new offered_load_complete_event(*this);
        return ret;
    }

    
    void serialize_order(SST::Core::Serialization::serializer &ser)  override {
        Event::serialize_order(ser);
        ser & generation;
        ser & sum;
        ser & sum_of_squares;
        ser & min;
        ser & max;
        ser & count;
        ser & backup;
    }

private:

    offered_load_complete_event() : Event() {}

    ImplementSerializable(SST::Merlin::offered_load_complete_event)

};



class OfferedLoad : public Component {

public:

    SST_ELI_REGISTER_COMPONENT(
        OfferedLoad,
        "merlin",
        "offered_load",
        SST_ELI_ELEMENT_VERSION(0,0,1),
        "Pattern-based traffic generator to study latency versus offered load.",
        COMPONENT_CATEGORY_NETWORK)
    
    SST_ELI_DOCUMENT_PARAMS(
        {"num_peers",        "Total number of endpoints in network."},
        {"link_bw",          "Bandwidth of the router link specified in either b/s or B/s (can include SI prefix)."},
        {"linkcontrol",      "SimpleNetwork object to use as interface to network.","merlin.linkcontrol"},
        {"buffer_size",      "Size of input and output buffers.","1kB"},
        {"packet_size",      "Packet size specified in either b or B (can include SI prefix).","32B"},
        {"pattern",          "Traffic pattern to use.","merlin.targetgen.uniform"},
        {"offered_load",     "Load to be offered to network.  Valid range: 0 < offered_load <= 1.0."},
        {"warmup_time",      "Time to wait before recording latencies","1us"},
        {"collect_time",     "Time to collect data after warmup","20us"},        
        {"drain_time",       "Time to drain network before stating next round","50us"},
    )

    SST_ELI_DOCUMENT_PORTS(
        {"rtr",  "Port that hooks up to router.", { "merlin.RtrEvent", "merlin.credit_event" } }
    )



private:

    std::vector<double> offered_load;
    UnitAlgebra link_bw;

    UnitAlgebra serialization_time;
    
    SimTime_t next_time;
    SimTime_t send_interval;

    SimTime_t start_time;
    SimTime_t end_time;

    SimTime_t drain_time;
    SimTime_t warmup_time;
    SimTime_t collect_time;
    
    int generation;
    
    TimeConverter* base_tc;

    SST::Interfaces::SimpleNetwork* link_if;
    SST::Interfaces::SimpleNetwork::Handler<OfferedLoad>* send_notify_functor;
    SST::Interfaces::SimpleNetwork::Handler<OfferedLoad>* recv_notify_functor;


    TargetGenerator *packetDestGen;
    
    Output out;
    int id;
    int num_peers;
    int packet_size; // in bits
    
    uint64_t packets_sent;
    uint64_t packets_recd;

    Link* timing_link;
    Link* end_link;
    
    // Generator *packetSizeGen;
    // Generator *packetDelayGen;

    std::vector<offered_load_complete_event*> complete_event;
    
public:
    OfferedLoad(ComponentId_t cid, Params& params);
    ~OfferedLoad();

    void init(unsigned int phase);
    void setup();
    void complete(unsigned int phase);
    void finish();


private:
    bool handle_receives(int vn);
    bool send_notify(int vn);

    void output_timing(Event* ev);
    void progress_messages(SimTime_t current_time);

    void end_handler(Event* ev);
    
};

} //namespace Merlin
} //namespace SST

#endif
