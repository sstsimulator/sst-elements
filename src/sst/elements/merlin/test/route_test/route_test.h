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


#ifndef COMPONENTS_MERLIN_TEST_ROUTE_TEST_H
#define COMPONENTS_MERLIN_TEST_ROUTE_TEST_H

#include <sst/core/component.h>
#include <sst/core/elementinfo.h>
#include <sst/core/event.h>
#include <sst/core/link.h>
#include <sst/core/timeConverter.h>


namespace SST {

namespace Interfaces {
    class SimpleNetwork;
}

namespace Merlin {


class route_test : public Component {

public:

    SST_ELI_REGISTER_COMPONENT(
        route_test,
        "merlin",
        "route_test",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "Simple NIC to test routing.",
        COMPONENT_CATEGORY_NETWORK)
    
    SST_ELI_DOCUMENT_PARAMS(
        {"id",        "Network ID of endpoint."},
        {"num_peers", "Total number of endpoints in network."},
        {"link_bw",   "Bandwidth of the router link specified in either b/s or B/s (can include SI prefix)."}
    )

    SST_ELI_DOCUMENT_PORTS(
        {"rtr",  "Port that hooks up to router.", { "merlin.RtrEvent", "merlin.credit_event" } }
    )

    
private:

    int id;
    int num_peers;
    bool sending;


    bool done;
    bool initialized;
    
    SST::Interfaces::SimpleNetwork* link_control;

public:
    route_test(ComponentId_t cid, Params& params);
    ~route_test();

    void init(unsigned int phase);
    void setup(); 
    void finish();


private:
    bool clock_handler(Cycle_t cycle);

    bool handle_event(int vn);
};

}
}

#endif // COMPONENTS_MERLIN_TEST_ROUTE_TEST_H
