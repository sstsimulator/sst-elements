// -*- mode: c++ -*-

// Copyright 2009-2016 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2016, Sandia Corporation
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#ifndef COMPONENTS_MERLIN_TOPOLOGY_SINGLEROUTER_H
#define COMPONENTS_MERLIN_TOPOLOGY_SINGLEROUTER_H

#include <sst/core/elementinfo.h>
#include <sst/core/event.h>
#include <sst/core/link.h>
#include <sst/core/params.h>

#include "sst/elements/merlin/router.h"

namespace SST {
namespace Merlin {


class topo_singlerouter: public Topology {

    int num_ports;

public:
    topo_singlerouter(Component* comp, Params& params);
    ~topo_singlerouter();

    virtual void route(int port, int vc, internal_router_event* ev);
    virtual internal_router_event* process_input(RtrEvent* ev);

    virtual void routeInitData(int port, internal_router_event* ev, std::vector<int> &outPorts);
    virtual internal_router_event* process_InitData_input(RtrEvent* ev);

    virtual PortState getPortState(int port) const;

    virtual int getEndpointID(int port) { return port; }

    SST_ELI_REGISTER_SUBCOMPONENT(topo_singlerouter,"merlin","singlerouter","Simple, single-router topology object","SST::Merlin::Topology")
    
    SST_ELI_DOCUMENT_PARAMS(
    )

    SST_ELI_DOCUMENT_STATISTICS(
    )

    SST_ELI_DOCUMENT_PORTS(
    )


};

}
}

#endif // COMPONENTS_MERLIN_TOPOLOGY_SINGLEROUTER_H
