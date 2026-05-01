// -*- mode: c++ -*-

// Copyright 2009-2026 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2026, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// of the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#ifndef COMPONENTS_MERLIN_TOPOLOGY_SINGLEROUTER_H
#define COMPONENTS_MERLIN_TOPOLOGY_SINGLEROUTER_H

#include <sst/core/event.h>
#include <sst/core/link.h>
#include <sst/core/params.h>

#include "sst/elements/merlin/router.h"

namespace SST {
namespace Merlin {


class topo_singlerouter: public Topology {

public:

    SST_ELI_REGISTER_SUBCOMPONENT(
        topo_singlerouter,
        "merlin",
        "singlerouter",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "Simple, single-router topology object",
        SST::Merlin::Topology
    )


private:
    int num_ports = 0;
    int num_vns = 0;

public:
    topo_singlerouter(ComponentId_t cid, Params& params, int num_ports, int rtr_id, int nm_vns);
    ~topo_singlerouter();

    topo_singlerouter() = default;

    void serialize_order(SST::Core::Serialization::serializer& ser) override {
        Topology::serialize_order(ser);
        SST_SER(num_ports);
        SST_SER(num_vns);
    }

    ImplementSerializable(SST::Merlin::topo_singlerouter)

    virtual void route_packet(int port, int vc, internal_router_event* ev) override;
    virtual internal_router_event* process_input(RtrEvent* ev) override;

    virtual void routeUntimedData(int port, internal_router_event* ev, std::vector<int> &outPorts) override;
    virtual internal_router_event* process_UntimedData_input(RtrEvent* ev) override;

    virtual PortState getPortState(int port) const override;

    virtual int getEndpointID(int port)  override { return port; }

    virtual void getVCsPerVN(std::vector<int>& vcs_per_vn)  override
    {
        for ( int i = 0; i < num_vns; ++i ) {
            vcs_per_vn[i] = 1;
        }
    }
};

}
}

#endif // COMPONENTS_MERLIN_TOPOLOGY_SINGLEROUTER_H
