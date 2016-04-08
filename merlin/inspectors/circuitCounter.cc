#include <sst_config.h>
#include "sst/core/serialization.h"

#include "circuitCounter.h"

SST::Core::ThreadSafe::Spinlock CircNetworkInspector::mapLock;
CircNetworkInspector::setMap_t CircNetworkInspector::setMap;

CircNetworkInspector::CircNetworkInspector(SST::Component* parent, 
                                           SST::Params &params) :
        SimpleNetwork::NetworkInspector(parent) {
    
    outFileName = params.find_string("output_file");
    if (outFileName.empty()) {
      outFileName = "RouterCircuits";
    }
}

void CircNetworkInspector::initialize(string id) {
    // critical section for accessing the map
    {
        mapLock.lock();
        
        // use router name as the key
        const string &key = parent->getName();
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
