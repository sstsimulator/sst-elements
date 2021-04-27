// Copyright 2009-2021 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2021, NTESS
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
#include "sst/elements/memHierarchy/util.h"
#include "membackend/requestReorderSimple.h"

using namespace SST;
using namespace SST::MemHierarchy;

/*------------------------------- Simple Backend ------------------------------- */
RequestReorderSimple::RequestReorderSimple(ComponentId_t id, Params &params) : SimpleMemBackend(id, params){ 
    fixupParams( params, "clock", "backend.clock" );

    reqsPerCycle = params.find<int>("max_issue_per_cycle", -1);
    searchWindowSize = params.find<int>("search_window_size", -1);

    // Create our backend & copy 'mem_size' through for now
    backend = loadUserSubComponent<SimpleMemBackend>("backend");
    if (!backend) {
        std::string backendName = params.find<std::string>("backend", "memHierarchy.simpleDRAM");
        Params backendParams = params.get_scoped_params("backend");
        backendParams.insert("mem_size", params.find<std::string>("mem_size"));
        backend = loadAnonymousSubComponent<SimpleMemBackend>(backendName, "backend", 0, ComponentInfo::SHARE_PORTS | ComponentInfo::INSERT_STATS, backendParams);
    }
    using std::placeholders::_1;
    backend->setResponseHandler( std::bind( &RequestReorderSimple::handleMemResponse, this, _1 )  );
    m_memSize = backend->getMemSize(); // inherit from backend
}

bool RequestReorderSimple::issueRequest(ReqId id, Addr addr, bool isWrite, unsigned numBytes ) {
#ifdef __SST_DEBUG_OUTPUT__
    output->debug(_L10_, "Reorderer received request for 0x%" PRIx64 "\n", (Addr)addr);
#endif
    requestQueue.push_back(Req(id,addr,isWrite,numBytes));
    return true;
}

/*
 * Issue as many requests as we can up to requestsPerCycle
 * by searching up to searchWindowSize requests
 */
bool RequestReorderSimple::clock(Cycle_t cycle) {

    if (!requestQueue.empty()) {

        int reqsIssuedThisCycle = 0;
        int reqsSearchedThisCycle = 0;

        std::list<Req>::iterator it = requestQueue.begin();

        while (it != requestQueue.end()) {

            bool issued = backend->issueRequest( (*it).id, (*it).addr, (*it).isWrite, (*it).numBytes );

            if (issued) {
#ifdef __SST_DEBUG_OUTPUT__
    output->debug(_L10_, "Reorderer issued request for 0x%" PRIx64 "\n", (Addr)(*it).addr);
#endif
                reqsIssuedThisCycle++;
                it = requestQueue.erase(it);
                if (reqsIssuedThisCycle == reqsPerCycle) break;
            } else {
#ifdef __SST_DEBUG_OUTPUT__
    output->debug(_L10_, "Reorderer could not issue 0x%" PRIx64 "\n", (Addr)(*it).addr);
#endif
                it++;
            }

            reqsSearchedThisCycle++;
            if (reqsSearchedThisCycle == searchWindowSize) break;
        }
    }
    bool unclock = backend->clock(cycle);
    return false;
}


/*
 * Call throughs to our backend
 */

void RequestReorderSimple::setup() {
    backend->setup();
}

void RequestReorderSimple::finish() {
    backend->finish();
}

