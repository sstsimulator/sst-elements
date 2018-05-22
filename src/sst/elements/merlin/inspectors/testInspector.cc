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

#include <sst_config.h>

#include "testInspector.h"

namespace SST {
namespace Merlin {

TestNetworkInspector::TestNetworkInspector(Component* parent, Params& params) :
    SimpleNetwork::NetworkInspector(parent)
{}

void TestNetworkInspector::initialize(string id) {
    test_count = registerStatistic<uint64_t>("test_count", id);
}

void TestNetworkInspector::inspectNetworkData(SimpleNetwork::Request* req) {
    test_count->addData(1);
}

} // namespace Merlin
} // namespace SST
