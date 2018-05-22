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

#ifndef COMPONENTS_MERLIN_CIRCUITCOUNTER_H
#define COMPONENTS_MERLIN_CIRCUITCOUNTER_H

#include <sst/core/elementinfo.h>
#include <sst/core/subcomponent.h>
#include <sst/core/interfaces/simpleNetwork.h>
#include <sst/core/threadsafe.h>

namespace SST {
using namespace SST::Interfaces;
namespace Merlin {

class CircNetworkInspector : public SimpleNetwork::NetworkInspector {

public:

    SST_ELI_REGISTER_SUBCOMPONENT(
        CircNetworkInspector,
        "merlin",
        "circuit_network_inspector",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "Used to count the number of network circuits (as in 'circuit switched' circuits)",
        "SST::Interfaces::SimpleNetwork:NetworkInspector")
    

private:
    typedef std::pair<SimpleNetwork::nid_t, SimpleNetwork::nid_t> SDPair;
    typedef std::set<SDPair> pairSet_t;
    pairSet_t *uniquePaths;
    std::string outFileName;

    typedef std::map<std::string, pairSet_t*> setMap_t;
    // Map which makes sure that all the inspectors on one router use
    // the same pairSet. This structure can be accessed by multiple
    // threads during intiailize, so it needs to be protected.
    static setMap_t setMap;
    static SST::Core::ThreadSafe::Spinlock mapLock;
public:
    CircNetworkInspector(SST::Component* parent, SST::Params &params);

    void initialize(std::string id);
    void finish();

    void inspectNetworkData(SimpleNetwork::Request* req);


};


} // namespace Merlin
} // namespace SST
#endif
