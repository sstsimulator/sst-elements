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

#include <sst_config.h>
#include "sst/core/serialization.h"

#include "testInspector.h"

using namespace std;
using namespace SST;
using namespace SST::Interfaces;

TestNetworkInspector::TestNetworkInspector(Component* parent) :
    SimpleNetwork::NetworkInspector(parent)
{}

void TestNetworkInspector::initialize(string id) {
    test_count = registerStatistic<uint64_t>("test_count", id);
}

void TestNetworkInspector::inspectNetworkData(SimpleNetwork::Request* req) {
    test_count->addData(1);
}

