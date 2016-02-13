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

private:

    // SST::Interfaces::SimpleNetwork::nid_t id;
    int id;
    int net_id;
    int num_peers;
    int num_vns;
    int last_vn;

    int num_msg;
    int packets_sent;
    int packets_recd;
    int stalled_cycles;

    bool done;
    bool initialized;
    
    SST::Interfaces::SimpleNetwork* link_control;

    int last_target;
    
    int *next_seq;

    int remap;
    SST::Interfaces::SimpleNetwork::Mapping net_map;

    Output& output;
    
public:
    nic(ComponentId_t cid, Params& params);
    ~nic();

    void init(unsigned int phase);
    void setup(); 
    void finish();


private:
    bool clock_handler(Cycle_t cycle);
};

}
}

#endif // COMPONENTS_MERLIN_TEST_NIC_H
