// Copyright 2013-2017 Sandia Corporation. Under the terms
// of Contract DE-NA0003525 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2013-2017, Sandia Corporation
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef _MEMHIERARCHY_MULTITHREADL1_H_
#define _MEMHIERARCHY_MULTITHREADL1_H_

#include <map>
#include <queue>

#include <sst/core/event.h>
#include <sst/core/sst_types.h>
#include <sst/core/component.h>
#include <sst/core/link.h>
#include <sst/core/timeConverter.h>
#include <sst/core/output.h>
#include "memEvent.h"
#include "util.h"

using namespace std;

namespace SST { namespace MemHierarchy {

class MultiThreadL1 : public Component {

public:
    
    /** Constructor & destructor */
    MultiThreadL1(ComponentId_t id, Params &params);
    ~MultiThreadL1();
    
    /** SST component basic functions */
    void setup(void);
    void init(unsigned int phase);
    void finish(void);
    
    /** Handles response from memory hierarchy, passes to correct thread's CPU */
    void handleResponse(SST::Event *event);

    /** Handles request from CPU, passes on to memory hierarchy */
    void handleRequest(SST::Event *event, unsigned int threadid);
	
    /** Clock handler - requests/responses are sent on cycle boundaries */
    bool tick(SST::Cycle_t cycle);

private:
    /** Output and debug */
    Output debug;
    Output output;
    bool DEBUG_ALL;
    Addr DEBUG_ADDR;

    /** Links */
    SST::Link * cacheLink;
    vector<SST::Link*> threadLinks;

    /** Timestamp & clock control */
    uint64_t    timestamp;
    bool        clockOn;
    Clock::Handler<MultiThreadL1>*  clockHandler;
    TimeConverter* clock;

    /** Track outstanding requests for routing responses correctly */
    std::map< MemEvent::id_type, unsigned int > threadRequestMap;

    /** Throughput control */
    uint64_t requestsPerCycle;
    uint64_t responsesPerCycle;
    std::queue<MemEvent*> requestQueue;
    std::queue<MemEvent*> responseQueue;
    
    inline void enableClock();
};

}
}

#endif /* _MEMHIERARCHY_MULTITHREADL1_H_ */
