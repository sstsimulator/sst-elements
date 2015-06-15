// -*- mode: c++ -*-

// Copyright 2009-2015 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2015, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#ifndef COMPONENTS_SCHEDULER_NICCOMPONENT_H
#define COMPONENTS_SCHEDULER_NICCOMPONENT_H

#include <sst/core/component.h>
#include <sst/core/event.h>
#include <sst/core/link.h>
#include <sst/core/timeConverter.h>

#include "events/MPIEvent.h"

namespace SST {

namespace Interfaces {
    class SimpleNetwork;
}

namespace Scheduler {


class nicComponent : public Component {

private:

    int id;
    int ft_loading;
    int ft_radix;
    int num_peers;
    int num_vns;
    int last_vn;

    int num_msg;
    int size_in_bits;
    int expected_recv_count;
    int packets_sent;
    int packets_recd;
    int stalled_cycles;

    bool sent_all;
    bool received_all;
    bool initialized;
    
    SST::Interfaces::SimpleNetwork* link_control;

    int last_target;
    
    int *next_seq;

    bool DoMPIMessaging;
    void handleMPIEvent(SST::Event* ev);
    SST::Link * NodeLink;
    void MPIsetup(MPIEvent * event); //sets the MPI messaging parameters for the current messaging phase
    void MPIreset(); //resets all MPI messaging parameters

public:
    nicComponent(ComponentId_t cid, Params& params);
    ~nicComponent();

    void init(unsigned int phase);
    void setup(); 
    void finish();

private:
    bool clock_handler(Cycle_t cycle);
    int fattree_ID_to_IP(int id);
    int IP_to_fattree_ID(int id);
};

}
}

#endif // COMPONENTS_SCHEDULER_NICCOMPONENT_H
