// -*- mode: c++ -*-

// Copyright 2009-2013 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2013, Sandia Corporation
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#ifndef COMPONENTS_MERLIN_LINKCONTROL_H
#define COMPONENTS_MERLIN_LINKCONTROL_H

#include <sst_config.h>
#include <sst/core/module.h>
#include <sst/core/component.h>
#include <sst/core/event.h>
#include <sst/core/link.h>
#include <sst/core/timeConverter.h>

#include <queue>
#include <cstring>

#include "sst/elements/merlin/router.h"

using namespace SST;

namespace SST {
namespace Merlin {

// Whole class definition needs to be in the header file so that other
// libraries can use the class to talk with the merlin routers.


typedef std::queue<RtrEvent*> network_queue_t;

// Class to manage link between NIC and router.  A single NIC can have
// more than one link_control (and thus link to router).
class LinkControl : public Module {
private:
    // Link to router
    Link* rtr_link;
    // Self link for timing output.  This is how we manage bandwidth
    // usage
    Link* output_timing;

    std::deque<Event*> init_events;

    // Number of virtual channels
    int num_vcs;

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

    // Doing a round robin on the output.  Need to keep track of the
    // current virtual channel.
    int curr_out_vc;

    // Vairable to tell us if we are waiting for something to happen
    // before we begin more output.  The two things we are waiting on
    // is: 1 - adding new data to output buffers, or 2 - getting
    // credits back from the router.
    bool waiting;

    Component* parent;


public:
    LinkControl(Params &params);

    ~LinkControl();

    // Must be called before any other functions to configure the link.
    // Preferably during the owning component's constructor
    // time_base is a frequency which represents the bandwidth of the link in flits/second.
    void configureLink(Component* rif, std::string port_name, TimeConverter* time_base, int vcs, int* in_buf_size, int* out_buf_size);
    void setup();
    void init(unsigned int phase);

    // Returns true if there is space in the output buffer and false
    // otherwise.
    bool send(RtrEvent* ev, int vc);

    // Returns true if there is space in the output buffer and false
    // otherwise.
    bool spaceToSend(int vc, int flits);

    // Returns NULL if no event in input_buf[vc]. Otherwise, returns
    // the next event.
    RtrEvent* recv(int vc);

    void sendInitData(RtrEvent *ev);
    Event* recvInitData();




private:
    void handle_input(Event* ev);

    void handle_output(Event* ev);

};

}
}

#endif // COMPONENTS_MERLIN_LINKCONTROL_H
