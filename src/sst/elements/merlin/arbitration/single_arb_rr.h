// -*- mode: c++ -*-

// Copyright 2009-2022 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2022, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// of the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#ifndef COMPONENTS_MERLIN_ARBITRATION_SINGLE_ARB_RR_H
#define COMPONENTS_MERLIN_ARBITRATION_SINGLE_ARB_RR_H

#include "sst/elements/merlin/arbitration/single_arb.h"

namespace SST {
namespace Merlin {

class single_arb_rr : public SingleArbitration {

public:

    SST_ELI_REGISTER_MODULE_DERIVED(
        single_arb_rr,
        "merlin",
        "arb.base.single.roundrobin",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "Base round robin module used when only one match per round is needed.",
        SST::Merlin::SingleArbitration
    )

private:
    int16_t size;
    int16_t current;


public:
    single_arb_rr(Params& params, int16_t size) :
        SingleArbitration(),
        size(size),
        current(-1)
    {}

    ~single_arb_rr() { }

    int next() {
        current++;
        if ( current == size ) current = 0;
        return current;
    }

    void satisfied() {}
};


}
}

#endif // COMPONENTS_MERLIN_ROUTER_H
