// -*- mode: c++ -*-

// Copyright 2009-2012 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2012, Sandia Corporation
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


using namespace SST;

class LinkControl;

class nic : public Component {

private:
    int id;
    int num_peers;
    int num_vcs;
    
    int packets_sent;
    int packets_recd;
    int stalled_cycles;
    
    bool done;
    
    LinkControl* link_control;

    int last_target;

public:
    nic(ComponentId_t cid, Params& params);
    ~nic() {}

    int Setup();
    int Finish();


private:
    bool clock_handler(Cycle_t cycle);
    
};

#endif // COMPONENTS_MERLIN_TEST_NIC_H
