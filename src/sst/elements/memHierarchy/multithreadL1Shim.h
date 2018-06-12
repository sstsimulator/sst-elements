// Copyright 2013-2018 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2013-2018, NTESS
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
#include <sst/core/elementinfo.h>
#include <sst/core/link.h>
#include <sst/core/timeConverter.h>
#include <sst/core/output.h>

#include "sst/elements/memHierarchy/memEventBase.h"
#include "sst/elements/memHierarchy/util.h"

using namespace std;

namespace SST { namespace MemHierarchy {

class MultiThreadL1 : public Component {
public:
/* Element Library Info */
    SST_ELI_REGISTER_COMPONENT(MultiThreadL1, "memHierarchy", "multithreadL1", SST_ELI_ELEMENT_VERSION(1,0,0),
            "Layer to connect multiple CPUs to a single L1 as if multiple hardware threads", COMPONENT_CATEGORY_MEMORY)

    SST_ELI_DOCUMENT_PARAMS(
            {"clock",               "(string) Clock frequency or period with units (Hz or s; SI units OK).", NULL},
            {"requests_per_cycle",  "(uint) Number of requests to forward to L1 each cycle (for all threads combined). 0 indicates unlimited", "0"},
            {"responses_per_cycle", "(uint) Number of responses to forward to threads each cycle (for all threads combined). 0 indicates unlimited", "0"},
            {"debug",               "(uint) Where to print debug output. Options: 0[no output], 1[stdout], 2[stderr], 3[file]", "0"},
            {"debug_level",         "(uint) Debug verbosity level. Between 0 and 10", "0"},
            {"debug_addr",          "(comma separated uint) Address(es) to be debugged. Leave empty for all, otherwise specify one or more, comma-separated values. Start and end string with brackets",""} )
      
    SST_ELI_DOCUMENT_PORTS(           
          {"cache", "Link to L1 cache", {"memHierarchy.MemEventBase"} },
          {"thread%(port)d", "Links to threads/cores", {"memHierarchy.MemEventBase"} } )

/* Begin class definition */
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
    std::set<Addr> DEBUG_ADDR;

    /** Links */
    SST::Link * cacheLink;
    vector<SST::Link*> threadLinks;

    /** Timestamp & clock control */
    uint64_t    timestamp;
    bool        clockOn;
    Clock::Handler<MultiThreadL1>*  clockHandler;
    TimeConverter* clock;

    /** Track outstanding requests for routing responses correctly */
    std::map<Event::id_type, unsigned int> threadRequestMap;

    /** Throughput control */
    uint64_t requestsPerCycle;
    uint64_t responsesPerCycle;
    std::queue<MemEventBase*> requestQueue;
    std::queue<MemEventBase*> responseQueue;
    
    inline void enableClock();
};

}
}

#endif /* _MEMHIERARCHY_MULTITHREADL1_H_ */
