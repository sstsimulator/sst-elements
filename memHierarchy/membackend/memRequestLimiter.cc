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
#include "membackend/memRequestLimiter.h"

using namespace SST;
using namespace SST::MemHierarchy;

/********************* Request limiter **********************************/
/* Acts as a limiter between the controller and a memory backend        */

MemoryRequestLimiter::MemoryRequestLimiter(Component *comp, Params &params) : MemBackend(comp, params){
    std::string backendName = params.find_string("backend", "memHierarchy.simpleMem");
    
    Params backendParams = params.find_prefix_params("backend.");
    backendParams.insert("mem_size", params.find_string("mem_size"), false);   // Set mem_size to be copied through
    backend = dynamic_cast<MemBackend*>(loadSubComponent(backendName, backendParams));
    if (!backend) 
       ctrl->dbg.fatal(CALL_INFO,-1,"%s, Unable to load subcomponent %s as backend\n", ctrl->getName().c_str(), backendName.c_str());

    // Assume that a paramter <= 0 means unlimited requests
    maxReqsPerCycle = params.find_integer("max_requests_per_cycle", 1);
    if (maxReqsPerCycle == 0) 
        maxReqsPerCycle = -1;   // Simplify comparison later
}

bool MemoryRequestLimiter::issueRequest(DRAMReq *req){
    if (maxReqsPerCycle == requestCount) {
        return false;
    }
    if (backend->issueRequest(req)) {
        requestCount++;
        return true;
    }
    return false;
}

void MemoryRequestLimiter::clock() {
    requestCount = 0;
    backend->clock();
}

void MemoryRequestLimiter::setup() {
    backend->setup();
}

void MemoryRequestLimiter::finish() {
    backend->finish();
}
