// Copyright 2009-2026 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2026, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// of the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef COMPONENTS_MERLIN_CIRCUITCOUNTER_H
#define COMPONENTS_MERLIN_CIRCUITCOUNTER_H

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
        SST::Interfaces::SimpleNetwork::NetworkInspector
    )


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
    CircNetworkInspector(SST::ComponentId_t, SST::Params &params, const std::string& sub_id);

    void finish() override;

    void inspectNetworkData(SimpleNetwork::Request* req) override;

    CircNetworkInspector() :
        SimpleNetwork::NetworkInspector(),
        uniquePaths(nullptr) {}

    void serialize_order(SST::Core::Serialization::serializer& ser) override {
        SimpleNetwork::NetworkInspector::serialize_order(ser);
        SST_SER(outFileName);

        // Static setMap is rebuilt from individual instances on UNPACK:
        // first inspector per router creates the shared pairSet, subsequent ones reuse it.
        if ( ser.mode() == SST::Core::Serialization::serializer::UNPACK ) {
            uniquePaths = new pairSet_t;
            std::string fullname = getName();
            int index = fullname.find(":");
            const std::string& key = index == std::string::npos ? fullname : fullname.substr(0, index);
            mapLock.lock();
            setMap_t::iterator iter = setMap.find(key);
            if ( iter != setMap.end() ) {
                delete uniquePaths;
                uniquePaths = iter->second;
            } else {
                setMap[key] = uniquePaths;
            }
            mapLock.unlock();
        }

        size_t set_size = (uniquePaths != nullptr) ? uniquePaths->size() : 0;
        SST_SER(set_size);
        if ( ser.mode() == SST::Core::Serialization::serializer::UNPACK ) {
            for ( size_t i = 0; i < set_size; i++ ) {
                SimpleNetwork::nid_t src = 0, dest = 0;
                SST_SER(src);
                SST_SER(dest);
                uniquePaths->insert(SDPair(src, dest));
            }
        } else {
            for ( auto& pair : *uniquePaths ) {
                SimpleNetwork::nid_t src = pair.first;
                SimpleNetwork::nid_t dest = pair.second;
                SST_SER(src);
                SST_SER(dest);
            }
        }
    }

    ImplementSerializable(SST::Merlin::CircNetworkInspector)
};


} // namespace Merlin
} // namespace SST
#endif
