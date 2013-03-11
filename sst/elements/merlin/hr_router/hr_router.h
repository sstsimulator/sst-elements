// Copyright 2009-2010 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2010, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#ifndef COMPONENTS_HR_ROUTER_HR_ROUTER_H
#define COMPONENTS_HR_ROUTER_HR_ROUTER_H

#include <sst/core/component.h>
#include <sst/core/event.h>
#include <sst/core/link.h>
#include <sst/core/timeConverter.h>

#include <queue>

#include "sst/elements/merlin/router.h"

using namespace SST;

class PortControl;

class hr_router : public Component {

private:
    static int num_routers;
    static int print_debug;
    int id;
    int num_ports;
    int num_vcs;

    Topology* topo;
    XbarArbitration* arb;
    
    PortControl** ports;

    int* in_port_busy;
    int* out_port_busy;
    int* progress_vcs;
    
    bool clock_handler(Cycle_t cycle);
    bool debug_clock_handler(Cycle_t cycle);
    static void sigHandler(int signal);


public:
    hr_router(ComponentId_t cid, Params& params);
    ~hr_router();
    
    void init(unsigned int phase);
    int Setup();
    int Finish() {return false;}

    void dumpState(std::ostream& stream);

};

#endif // COMPONENTS_HR_ROUTER_HR_ROUTER_H
