// -*- mode: c++ -*-

// Copyright 2009-2020 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2020, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#ifndef COMPONENTS_MERLIN_TEST_SIMPLE_PATTERNS_NULL_H
#define COMPONENTS_MERLIN_TEST_SIMPLE_PATTERNS_NULL_H

#include <sst/core/component.h>
#include <sst/core/interfaces/simpleNetwork.h>


namespace SST {

namespace Merlin {

class empty_nic : public Component {

public:

    SST_ELI_REGISTER_COMPONENT(
        empty_nic,
        "merlin",
        "simple_patterns.empty",
        SST_ELI_ELEMENT_VERSION(0,9,0),
        ".",
        COMPONENT_CATEGORY_NETWORK)

    SST_ELI_DOCUMENT_PARAMS(
    )

    SST_ELI_DOCUMENT_PORTS(
    )

    SST_ELI_DOCUMENT_SUBCOMPONENT_SLOTS(
        {"networkIF", "Network interface", "SST::Interfaces::SimpleNetwork" }
    )

    void finish();
    void init(unsigned int phase);
    void setup();

    
private:
    SST::Interfaces::SimpleNetwork* link_control;

public:
    empty_nic(ComponentId_t cid, Params& params);
    ~empty_nic();
};

}
}

#endif // COMPONENTS_MERLIN_TEST_SIMPLE_PATTERNS_SHIFT_H
