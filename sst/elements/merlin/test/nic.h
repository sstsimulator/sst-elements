// -*- mode: c++ -*-

// Copyright 2009-2014 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2014, Sandia Corporation
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


namespace SST {
namespace Merlin {

class LinkControl;

class nic : public Component {

private:

    enum AddressMode { SEQUENTIAL, FATTREE_IP };

    AddressMode addressMode;

    int id;
    int ft_loading;
    int ft_radix;
    int num_peers;
    int num_vns;
    int last_vn;

    int packets_sent;
    int packets_recd;
    int stalled_cycles;

    bool done;
    bool initialized;
    
    LinkControl* link_control;

    int last_target;

    int *next_seq;

public:
    nic(ComponentId_t cid, Params& params);
    ~nic();

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

#endif // COMPONENTS_MERLIN_TEST_NIC_H
