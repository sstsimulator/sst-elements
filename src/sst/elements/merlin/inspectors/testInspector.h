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

#ifndef COMPONENTS_MERLIN_TESTINSPECTOR_H
#define COMPONENTS_MERLIN_TESTINSPECTOR_H

#include <sst/core/elementinfo.h>
#include <sst/core/subcomponent.h>
#include <sst/core/interfaces/simpleNetwork.h>
#include <sst/core/threadsafe.h>

using namespace std;
namespace SST {
using namespace SST::Interfaces;

namespace Merlin {

class TestNetworkInspector : public SimpleNetwork::NetworkInspector {

public:

    SST_ELI_REGISTER_SUBCOMPONENT(
        TestNetworkInspector,
        "merlin",
        "test_network_inspector",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "Used to test NetworkInspector functionality.  Duplicates send_packet_count in hr_router.",
        "SST::Interfaces::SimpleNetwork::NetworkInspector")
    
    SST_ELI_DOCUMENT_STATISTICS(
        { "test_count", "Count number of packets sent on link", "packets", 1}
    )
    

private:
    Statistic<uint64_t>* test_count;
public:
    TestNetworkInspector(Component* parent, Params& params);

    void initialize(string id);

    void inspectNetworkData(SimpleNetwork::Request* req);

};
} // namespace Merlin
} // namespace SST
#endif
