// -*- mode: c++ -*-

// Copyright 2009-2021 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2021, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#ifndef COMPONENTS_MERLIN_ARBITRATION_OUTPUT_ARB_BASIC_H
#define COMPONENTS_MERLIN_ARBITRATION_OUTPUT_ARB_BASIC_H

#include <sst/elements/merlin/router.h>
#include <sst/elements/merlin/arbitration/single_arb.h>

namespace SST {
namespace Merlin {

class output_arb_basic : public PortInterface::OutputArbitration {

public:

    SST_ELI_REGISTER_SUBCOMPONENT_DERIVED(
        output_arb_basic,
        "merlin",
        "arb.output.basic",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "Basic output arbitration for PortControl",
        SST::Merlin::PortInterface::OutputArbitration)

    SST_ELI_DOCUMENT_PARAMS(
        {"arb",    "Tyoe of arbitration to use","merlin.arb.base.single.roundrobin"},
    )


private:


    int num_vcs;
    std::string arb_name;
    SingleArbitration* arb;

public:

    output_arb_basic(ComponentId_t cid, Params& params) :
        OutputArbitration(cid)
    {
        arb_name = params.find<std::string>("arb","merlin.arb.base.single.roundrobin");
    }

    ~output_arb_basic() {
        delete arb;
    }

    void setVCs(int n_vns, int* vcs_per_vn) {
        num_vcs = 0;
        for ( int i = 0; i < n_vns; ++i ) {
            num_vcs += vcs_per_vn[i];
        }
        Params empty;
        arb = loadModule<SingleArbitration>(arb_name,empty,num_vcs);

    }

    int arbitrate(Cycle_t UNUSED(cycle), PortInterface::port_queue_t* out_q, int* port_out_credits, bool isHostPort, bool& have_packets) {
        int vc_to_send = -1;
        bool found = false;
        internal_router_event* send_event = NULL;
        have_packets = false;

        for ( int i = 0; i < num_vcs; ++i ) {
            int vc = arb->next();
            if ( out_q[vc].empty() ) continue;
            have_packets = true;
            send_event = out_q[vc].front();
            if ( port_out_credits[isHostPort ? send_event->getVN() : vc] < send_event->getFlitCount() ) continue;
            vc_to_send = vc;
            found = true;
            arb->satisfied();
            break;
        }
        return vc_to_send;
    }

    void dumpState(std::ostream& stream) {
    }

};

}
}

#endif // COMPONENTS_MERLIN_ROUTER_H
