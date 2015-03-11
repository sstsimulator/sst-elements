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


#ifndef COMPONENTS_MERLIN_TEST_PT2PT_PT2PT_TEST_H
#define COMPONENTS_MERLIN_TEST_PT2PT_PT2PT_TEST_H

#include <sst/core/component.h>
#include <sst/core/event.h>
#include <sst/core/link.h>
#include <sst/core/timeConverter.h>


namespace SST {
namespace Merlin {

class LinkControl;

class pt2pt_test : public Component {

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

    virtual Event* clone(void)
    {
        return new pt2pt_test_event(*this);
    }
};

}
}

#endif // COMPONENTS_MERLIN_TEST_NIC_H
