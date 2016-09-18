// Copyright 2009-2016 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2016, Sandia Corporation
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#include <sst_config.h>
#include "membackend/delayBuffer.h"

using namespace SST;
using namespace SST::MemHierarchy;

/*------------------------------- Simple Backend ------------------------------- */
DelayBuffer::DelayBuffer(Component *comp, Params &params) : MemBackend(comp, params){
    
    // Get parameters
    
    UnitAlgebra delay = params.find<UnitAlgebra>("request_delay", UnitAlgebra("0ns"));
    
    if (!(delay.hasUnits("s"))) {
        output->fatal(CALL_INFO, -1, "Invalid param(%s): request_delay - must have units of 's' (seconds). You specified %s.\n", ctrl->getName().c_str(), delay.toString().c_str());
    }
    
    // Create our backend & copy 'mem_size' through for now
    std::string backendName = params.find<std::string>("backend", "memHierarchy.simpleDRAM");
    Params backendParams = params.find_prefix_params("backend.");
    backendParams.insert("mem_size", params.find<std::string>("mem_size"));
    backend = dynamic_cast<MemBackend*>(loadSubComponent(backendName, backendParams));

    // Set up self links
    if (delay.getRoundedValue() == 0) {
        delay_self_link = ctrl->configureSelfLink("DelaySelfLink", delay.toString(), new Event::Handler<DelayBuffer>(this, &DelayBuffer::handleNextRequest));
    } else {
        delay_self_link = NULL;
    }
}

void DelayBuffer::handleNextRequest(SST::Event *event) {
    DRAMReq * req = requestBuffer.front();
    requestBuffer.pop();
    backend->issueRequest(req);
}

bool DelayBuffer::issueRequest(DRAMReq *req) {
    if (delay_self_link != NULL) {
        requestBuffer.push(req);
        delay_self_link->send(1, NULL);   // Just need a wakeup
    } else {
        backend->issueRequest(req);
    }
}

/*
 * Call throughs to our backend
 */

void DelayBuffer::clock() {
    backend->clock();
}

void DelayBuffer::setup() {
    backend->setup();
}

void DelayBuffer::finish() {
    backend->finish();
}

