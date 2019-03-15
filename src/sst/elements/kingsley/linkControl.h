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


#ifndef COMPONENTS_KINGSLEY_LINKCONTROL_H
#define COMPONENTS_KINGSLEY_LINKCONTROL_H

#include <sst/core/elementinfo.h>
#include <sst/core/subcomponent.h>
#include <sst/core/unitAlgebra.h>

#include <sst/core/interfaces/simpleNetwork.h>

// #include <sst/core/statapi/statbase.h>

#include "sst/elements/kingsley/nocEvents.h"

#include <queue>

namespace SST {

class Component;

namespace Kingsley {

// Whole class definition needs to be in the header file so that other
// libraries can use the class to talk with the kingsley routers.

typedef std::queue<NocPacket*> network_queue_t;

// Class to manage link between NIC and router.  A single NIC can have
// more than one link_control (and thus link to router).
class LinkControl : public SST::Interfaces::SimpleNetwork {

public:

    SST_ELI_REGISTER_SUBCOMPONENT(
        LinkControl,
        "kingsley",
        "linkcontrol",
        SST_ELI_ELEMENT_VERSION(0,1,0),
        "Link Control module for building Kingsley-enabled NICs",
        "SST::Interfaces::SimpleNetwork")
    
    SST_ELI_DOCUMENT_PARAMS(
    )

    SST_ELI_DOCUMENT_STATISTICS(
        { "packet_latency",     "Histogram of latencies for received packets", "latency", 1},
        // { "send_bit_count",     "Count number of bits sent on link", "bits", 1},
        // { "output_port_stalls", "Time output port is stalled (in units of core timebase)", "time in stalls", 1},
        // { "idle_time",          "Number of (in unites of core timebas) that port was idle", "time spent idle", 1},
    )

    
private:

    int init_state;

    // Link to router
    Link* rtr_link;
    // Self link for timing output.  This is how we manage bandwidth
    // usage
    Link* output_timing;

    UnitAlgebra link_bw;
    UnitAlgebra inbuf_size;
    UnitAlgebra outbuf_size;
    int flit_size; // in bits
    
    std::deque<NocPacket*> init_events;
    
    // Number of virtual channels
    int req_vns;

    int id;
    
    // One buffer for each virtual network.  At the NIC level, we just
    // provide a virtual channel abstraction.
    network_queue_t* input_buf;
    network_queue_t* output_buf;

    // Variables to keep track of credits.  You need to keep track of
    // the credits available for your next buffer, as well as track
    // the credits you need to return to the buffer sending data to
    // you,
    int* outbuf_credits;

    int* rtr_credits;
    int* in_ret_credits;

    // Vairable to tell us if we are waiting for something to happen
    // before we begin more output.
    bool waiting;
    bool have_packets;
    
    // Functors for notifying the parent when there is more space in
    // output queue or when a new packet arrives
    HandlerBase* receiveFunctor;
    HandlerBase* sendFunctor;
    
    // PacketStats stats;
    // Statistics
    Statistic<uint64_t>* packet_latency;
    // Statistic<uint64_t>* send_bit_count;
    // Statistic<uint64_t>* output_port_stalls;
    // Statistic<uint64_t>* idle_time;

    Output& output;
    
public:
    LinkControl(Component* parent, Params &params);

    ~LinkControl();

    // Must be called before any other functions to configure the link.
    // Preferably during the owning component's constructor
    // time_base is a frequency which represents the bandwidth of the link in flits/second.
    bool initialize(const std::string& port_name, const UnitAlgebra& link_bw_in,
                    int vns, const UnitAlgebra& in_buf_size,
                    const UnitAlgebra& out_buf_size);
    void setup();
    void init(unsigned int phase);
    void complete(unsigned int phase);
    void finish();

    // Returns true if there is space in the output buffer and false
    // otherwise.
    bool send(SST::Interfaces::SimpleNetwork::Request* req, int vn);

    // Returns true if there is space in the output buffer and false
    // otherwise.
    bool spaceToSend(int vn, int flits);

    // Returns NULL if no event in input_buf[vn]. Otherwise, returns
    // the next event.
    SST::Interfaces::SimpleNetwork::Request* recv(int vn);

    // Returns true if there is an event in the input buffer and false 
    // otherwise.
    bool requestToReceive( int vn ) { return ! input_buf[vn].empty(); }

    void sendInitData(SST::Interfaces::SimpleNetwork::Request* ev);
    SST::Interfaces::SimpleNetwork::Request* recvInitData();

    // const PacketStats& getPacketStats(void) const { return stats; }

    inline void setNotifyOnReceive(HandlerBase* functor) { receiveFunctor = functor; }
    inline void setNotifyOnSend(HandlerBase* functor) { sendFunctor = functor; }

    inline bool isNetworkInitialized() const { return network_initialized; }
    inline nid_t getEndpointID() const { return id; }
    inline const UnitAlgebra& getLinkBW() const { return link_bw; }

    void printStatus(Output& out);
    
private:
    bool network_initialized;

    void handle_input(Event* ev);
    void handle_output(Event* ev);

    
};

}
}

#endif // COMPONENTS_KINGSLEY_LINKCONTROL_H
