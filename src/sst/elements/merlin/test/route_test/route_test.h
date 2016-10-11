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


#ifndef COMPONENTS_MERLIN_TEST_ROUTE_TEST_H
#define COMPONENTS_MERLIN_TEST_ROUTE_TEST_H

#include <sst/core/component.h>
#include <sst/core/event.h>
#include <sst/core/link.h>
#include <sst/core/timeConverter.h>


namespace SST {

namespace Interfaces {
    class SimpleNetwork;
}

namespace Merlin {


class route_test : public Component {

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
