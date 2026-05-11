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


#ifndef COMPONENTS_MERLIN_TOPOLOGY_FATTREE_H
#define COMPONENTS_MERLIN_TOPOLOGY_FATTREE_H

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
        SST::Merlin::Topology
    )

    SST_ELI_DOCUMENT_PARAMS(
        // Parameters needed for use with old merlin python module
        {"fattree.shape",               "Shape of the fattree"},
        {"fattree.routing_alg",         "Routing algorithm to use. [deterministic | adaptive]","deterministic"},
        {"fattree.adaptive_threshold",  "Threshold used to determine if a packet will adaptively route."},

        {"shape",               "Shape of the fattree"},
        {"routing_alg",         "Routing algorithm to use. [deterministic | adaptive]","deterministic"},
        {"adaptive_threshold",  "Threshold used to determine if a packet will adaptively route."}
    )


private:
    int rtr_level = 0;
    int level_id = 0;
    int level_group = 0;

    int high_host = 0;
    int low_host = 0;

    int down_route_factor = 0;

//    int levels;
    int id = -1;
    int up_ports = 0;
    int down_ports = 0;
    int num_ports = 0;
    int num_vns = 0;
    int num_vcs = 0;

    int const* outputCredits;
    int* thresholds = nullptr;
    double adaptive_threshold = 0.0;

    struct vn_info
    {
        int start_vc = 0;
        int num_vcs = 0;
        bool allow_adaptive = false;

        void serialize_order(SST::Core::Serialization::serializer& ser)
        {
            SST_SER(start_vc);
            SST_SER(num_vcs);
            SST_SER(allow_adaptive);
        }
    };

    vn_info* vns = nullptr;

    void parseShape(const std::string &shape, int *downs, int *ups) const;


public:
    topo_fattree(ComponentId_t cid, Params& params, int num_ports, int rtr_id, int num_vns);
    ~topo_fattree();

    topo_fattree() : Topology() {}

    void serialize_order(SST::Core::Serialization::serializer& ser) override;

    ImplementSerializable(SST::Merlin::topo_fattree)

    virtual void route_packet(int port, int vc, internal_router_event* ev) override;
    virtual internal_router_event* process_input(RtrEvent* ev) override;

    virtual void routeUntimedData(int port, internal_router_event* ev, std::vector<int> &outPorts) override;
    virtual internal_router_event* process_UntimedData_input(RtrEvent* ev) override;

    virtual int getEndpointID(int port) override;

    virtual PortState getPortState(int port) const override;

    virtual void setOutputBufferCreditArray(int const* array, int vcs) override;

    virtual void getVCsPerVN(std::vector<int>& vcs_per_vn)  override
    {
        for ( int i = 0; i < num_vns; ++i ) {
            vcs_per_vn[i] = vns[i].num_vcs;
        }
    }

private:
    void route_deterministic(int port, int vc, internal_router_event* ev);
};



}
}

#endif // COMPONENTS_MERLIN_TOPOLOGY_FATTREE_H
