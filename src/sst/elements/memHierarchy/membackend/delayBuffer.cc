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


#include <sst_config.h>
#include <sst/core/link.h>
#include "membackend/delayBuffer.h"
#include "sst/elements/memHierarchy/util.h"

using namespace SST;
using namespace SST::MemHierarchy;

/*------------------------------- Simple Backend ------------------------------- */
DelayBuffer::DelayBuffer(Component *comp, Params &params) : SimpleMemBackend(comp, params){
    
    // Get parameters
    fixupParams( params, "clock", "backend.clock" ); 
    
    UnitAlgebra delay = params.find<UnitAlgebra>("request_delay", UnitAlgebra("0ns"));
    
    if (!(delay.hasUnits("s"))) {
        output->fatal(CALL_INFO, -1, "Invalid param(%s): request_delay - must have units of 's' (seconds). You specified %s.\n", comp->getName().c_str(), delay.toString().c_str());
    }
    
    // Create our backend & copy 'mem_size' through for now
    std::string backendName = params.find<std::string>("backend", "memHierarchy.simpleDRAM");
    Params backendParams = params.find_prefix_params("backend.");
    backendParams.insert("mem_size", params.find<std::string>("mem_size"));
    backend = dynamic_cast<SimpleMemBackend*>(loadSubComponent(backendName, backendParams));

    using std::placeholders::_1;
    backend->setResponseHandler( std::bind( &DelayBuffer::handleMemResponse, this, _1 )  );

    // Set up self links
    if (delay.getValue() != 0) {
        delay_self_link = comp->configureSelfLink("DelaySelfLink", delay.toString(), new Event::Handler<DelayBuffer>(this, &DelayBuffer::handleNextRequest));
    } else {
        delay_self_link = NULL;
    }
}

void DelayBuffer::handleNextRequest(SST::Event *event) {
    Req& req = requestBuffer.front();
    if (!backend->issueRequest(req.id,req.addr,req.isWrite,req.numBytes)) {
        delay_self_link->send(1, NULL);
    } else requestBuffer.pop();
}

bool DelayBuffer::issueRequest( ReqId req, Addr addr, bool isWrite, unsigned numBytes) {
    if (delay_self_link != NULL) {
        requestBuffer.push(Req(req,addr,isWrite,numBytes));
        delay_self_link->send(1, NULL);   // Just need a wakeup
        return true;
    } else {
        return backend->issueRequest(req,addr,isWrite,numBytes);
    }
}

/*
 * Call throughs to our backend
 */

bool DelayBuffer::clock(Cycle_t cycle) {
    return backend->clock(cycle);
}

void DelayBuffer::setup() {
    backend->setup();
}

void DelayBuffer::finish() {
    backend->finish();
}
