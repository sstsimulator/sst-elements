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


#ifndef COMPONENTS_MERLIN_TOPOLOGY_FATTREE_H
#define COMPONENTS_MERLIN_TOPOLOGY_FATTREE_H

#include <sst/core/elementinfo.h>
#include <sst/core/event.h>
#include <sst/core/link.h>
#include <sst/core/params.h>

#include "sst/elements/merlin/router.h"

namespace SST {
namespace Merlin {

class topo_fattree: public Topology {

public:

    SST_ELI_REGISTER_SUBCOMPONENT(
        topo_fattree,
        "merlin",
        "fattree",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "Fattree topology object",
        "SST::Merlin::Topology")
    
    SST_ELI_DOCUMENT_PARAMS(
        {"fattree:shape",               "Shape of the fattree"},
        {"fattree:routing_alg",         "Routing algorithm to use. [deterministic | adaptive]","deterministic"},
        {"fattree:adaptive_threshold",  "Threshold used to determine if a packet will adaptively route."}
    )

    
private:
    int rtr_level;
    int level_id;
    int level_group;

    int high_host;
    int low_host;

    int down_route_factor;

//    int levels;
    int id;
    int up_ports;
    int down_ports;
    int num_ports;
    int num_vcs;
    
    int const* outputCredits;
    int* thresholds;
    bool allow_adaptive;
    double adaptive_threshold;
    
    void parseShape(const std::string &shape, int *downs, int *ups) const;

    
public:
    topo_fattree(Component* comp, Params& params);
    ~topo_fattree();

    virtual void route(int port, int vc, internal_router_event* ev);
    virtual void reroute(int port, int vc, internal_router_event* ev);
    virtual internal_router_event* process_input(RtrEvent* ev);

    virtual void routeInitData(int port, internal_router_event* ev, std::vector<int> &outPorts);
    virtual internal_router_event* process_InitData_input(RtrEvent* ev);

    virtual int getEndpointID(int port);

    virtual PortState getPortState(int port) const;

    virtual void setOutputBufferCreditArray(int const* array, int vcs);

    virtual int computeNumVCs(int vns) {return vns;}
    
};

}
}

#endif // COMPONENTS_MERLIN_TOPOLOGY_FATTREE_H
