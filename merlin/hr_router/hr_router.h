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


#ifndef COMPONENTS_HR_ROUTER_HR_ROUTER_H
#define COMPONENTS_HR_ROUTER_HR_ROUTER_H

#include <sst/core/clock.h>
#include <sst/core/component.h>
#include <sst/core/event.h>
#include <sst/core/link.h>
#include <sst/core/output.h>
#include <sst/core/timeConverter.h>

#include <sst/core/statapi/stataccumulator.h>

#include <queue>

#include "sst/elements/merlin/router.h"

using namespace SST;

namespace SST {
namespace Merlin {

class PortControl;

class hr_router : public Router {

private:
    static int num_routers;
    static int print_debug;
    int id;
    int num_ports;
//    int requested_vns;
    int num_vcs;
    bool vcs_initialized;
        
    Topology* topo;
    XbarArbitration* arb;
    
    PortControl** ports;
    internal_router_event** vc_heads;
    int* xbar_in_credits;

#if VERIFY_DECLOCKING
    bool clocking;
#endif

    int* in_port_busy;
    int* out_port_busy;
    int* progress_vcs;

    /* int input_buf_size; */
    /* int output_buf_size; */
    UnitAlgebra input_buf_size;
    UnitAlgebra output_buf_size;
    
    Cycle_t unclocked_cycle;
    std::string xbar_bw;
    TimeConverter* xbar_tc;
    Clock::Handler<hr_router>* my_clock_handler;

    std::vector<std::string> inspector_names;
    
    bool clock_handler(Cycle_t cycle);
    // bool debug_clock_handler(Cycle_t cycle);
    static void sigHandler(int signal);

    void init_vcs();
    Statistic<uint64_t>** xbar_stalls;

    Output& output;
    
public:
    hr_router(ComponentId_t cid, Params& params);
    ~hr_router();
    
    void init(unsigned int phase);
    void setup();
    void finish();

    void notifyEvent();
    int const* getOutputBufferCredits() {return xbar_in_credits;}

    void sendTopologyEvent(int port, TopologyEvent* ev);
    void recvTopologyEvent(int port, TopologyEvent* ev);
    
    void reportRequestedVNs(int port, int vns);
    void reportSetVCs(int port, int vcs);
    
    void dumpState(std::ostream& stream);
    void printStatus(Output& out);
};

}
}

#endif // COMPONENTS_HR_ROUTER_HR_ROUTER_H
