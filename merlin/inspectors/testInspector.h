// Copyright 2016 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2015, Sandia Corporation
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef COMPONENTS_MERLIN_TESTINSPECTOR_H
#define COMPONENTS_MERLIN_TESTINSPECTOR_H

#include <sst/core/subcomponent.h>
#include <sst/core/interfaces/simpleNetwork.h>
#include <sst/core/threadsafe.h>

using namespace std;
using namespace SST;
using namespace SST::Interfaces;

class TestNetworkInspector : public SimpleNetwork::NetworkInspector {
private:
    Statistic<uint64_t>* test_count;
public:
    TestNetworkInspector(Component* parent);

    void initialize(string id);

    void inspectNetworkData(SimpleNetwork::Request* req);
};

#endif
