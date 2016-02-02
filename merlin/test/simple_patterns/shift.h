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


#ifndef COMPONENTS_MERLIN_TEST_SIMPLE_PATTERNS_SHIFT_H
#define COMPONENTS_MERLIN_TEST_SIMPLE_PATTERNS_SHIFT_H

#include <sst/core/component.h>
#include <sst/core/event.h>
#include <sst/core/link.h>
#include <sst/core/timeConverter.h>
#include <sst/core/interfaces/simpleNetwork.h>


namespace SST {

namespace Merlin {

class shift_nic : public Component {

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
    SST::Interfaces::SimpleNetwork::Mapping net_map;

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
