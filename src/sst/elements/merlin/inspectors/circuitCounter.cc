// Copyright 2009-2019 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2019, NTESS
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

#include "circuitCounter.h"

using namespace std;

namespace SST {
namespace Merlin {

SST::Core::ThreadSafe::Spinlock CircNetworkInspector::mapLock;
CircNetworkInspector::setMap_t CircNetworkInspector::setMap;

CircNetworkInspector::CircNetworkInspector(SST::ComponentId_t id, 
                                           SST::Params &params, const std::string& sub_id) :
        SimpleNetwork::NetworkInspector(id) {
    outFileName = params.find<std::string>("output_file");
    if (outFileName.empty()) {
        outFileName = "RouterCircuits";
    }

    {
        mapLock.lock();
        
        // use router name as the key
        // Get router name from my name
        string fullname = getName();

        // Nmae of router is the name before the first :
        int index = fullname.find(":");
        
        const string &key = index == string::npos ? fullname : fullname.substr(0,index);
        // look up our key
        setMap_t::iterator iter = setMap.find(key);
        if (iter == setMap.end()) {
            // we're first!
            pairSet_t *ps = new pairSet_t;
            setMap[key] = ps;
            uniquePaths = ps;
        } else {
            // someone else created the set already
            uniquePaths = iter->second;
        }
        
        mapLock.unlock();
    }
}    


#ifndef SST_ENABLE_PREVIEW_BUILD
void CircNetworkInspector::initialize(string id) {
    // critical section for accessing the map
    {
        mapLock.lock();
        
        // use router name as the key
        // Get router name from my name
        string fullname = getName();

        // Nmae of router is the name before the first :
        int index = fullname.find(":");
        
        const string &key = index == string::npos ? fullname : fullname.substr(0,index);
        // look up our key
        setMap_t::iterator iter = setMap.find(key);
        if (iter == setMap.end()) {
            // we're first!
            pairSet_t *ps = new pairSet_t;
            setMap[key] = ps;
            uniquePaths = ps;
        } else {
            // someone else created the set already
            uniquePaths = iter->second;
        }
        
        mapLock.unlock();
    }
}
#endif

void CircNetworkInspector::inspectNetworkData(SimpleNetwork::Request* req) {
    uniquePaths->insert(SDPair(req->src, req->dest));
}

// Print out all the stats. We have the first component print all the
// stats and peform cleanup, everyone else finds an empty map.
void CircNetworkInspector::finish() {
    // critical section for accessing the map
    {
        mapLock.lock();
        
        if (!setMap.empty()) {
            // create new file
            SST::Output* output_file = new SST::Output("",0,0,
                                                       SST::Output::FILE, 
                                                       outFileName);
            
            for(setMap_t::iterator i = setMap.begin();
                i != setMap.end(); ++i) {
                // print
                output_file->output(CALL_INFO, "%s %" PRIu64 "\n", 
                                    i->first.c_str(), 
                                    (unsigned long long)i->second->size());
                // clean up
                delete(i->second);
            }
        }
        
        setMap.clear();
        
        mapLock.unlock();
    }
}

} // namespace Merlin
} // namespace SST
