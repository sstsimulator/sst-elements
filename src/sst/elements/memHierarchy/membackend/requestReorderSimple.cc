// Copyright 2009-2015 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2015, Sandia Corporation
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#include <sst_config.h>
#include "membackend/requestReorderSimple.h"

using namespace SST;
using namespace SST::MemHierarchy;

/*------------------------------- Simple Backend ------------------------------- */
RequestReorderSimple::RequestReorderSimple(Component *comp, Params &params) : MemBackend(comp, params){
    
    reqsPerCycle = params.find<int>("max_requests_per_cycle", -1);
    searchWindowSize = params.find<int>("search_window_size", -1);

    // Create our backend & copy 'mem_size' through for now
    std::string backendName = params.find<std::string>("backend", "memHierarchy.simpleDRAM");
    Params backendParams = params.find_prefix_params("backend.");
    backendParams.insert("mem_size", params.find<std::string>("mem_size"));
    backend = dynamic_cast<MemBackend*>(loadSubComponent(backendName, backendParams));
}

bool RequestReorderSimple::issueRequest(DRAMReq *req) {
#ifdef __SST_DEBUG_OUTPUT__
    uint64_t addr = req->baseAddr_ + req->amtInProcess_;
    ctrl->dbg.debug(_L10_, "Reorderer received request for 0x%" PRIx64 "\n", (Addr)addr);
#endif
    requestQueue.push_back(req);
    return true;
}

/* 
 * Issue as many requests as we can up to requestsPerCycle
 * by searching up to searchWindowSize requests
 */
void RequestReorderSimple::clock() {
    
    if (!requestQueue.empty()) {
        
        int reqsIssuedThisCycle = 0;
        int reqsSearchedThisCycle = 0;
        
        std::list<DRAMReq*>::iterator it = requestQueue.begin();
        
        while (it != requestQueue.end()) {
            
            bool issued = backend->issueRequest(*it);
            
            if (issued) {
#ifdef __SST_DEBUG_OUTPUT__
    uint64_t addr = (*it)->baseAddr_ + (*it)->amtInProcess_;
    ctrl->dbg.debug(_L10_, "Reorderer issued request for 0x%" PRIx64 "\n", (Addr)addr);
#endif
                reqsIssuedThisCycle++;
                it = requestQueue.erase(it);
                if (reqsIssuedThisCycle == reqsPerCycle) break;
            } else {
#ifdef __SST_DEBUG_OUTPUT__
    uint64_t addr = (*it)->baseAddr_ + (*it)->amtInProcess_;
    ctrl->dbg.debug(_L10_, "Reorderer could not issue 0x%" PRIx64 "\n", (Addr)addr);
#endif
                it++;
            }
        
            reqsSearchedThisCycle++;
            if (reqsSearchedThisCycle == searchWindowSize) break;
        }
    }
    backend->clock();
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

