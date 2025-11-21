// Copyright 2009-2025 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2025, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// of the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#include <sst_config.h>
#include <sst/core/link.h>
#include "sst/elements/memHierarchy/util.h"
#include "membackend/simpleMemBackend.h"
#include "membackend/simpleMemBackendConvertor.h"

using namespace SST;
using namespace SST::MemHierarchy;

/*------------------------------- Simple Backend ------------------------------- */
SimpleMemory::SimpleMemory(ComponentId_t id, Params &params) : SimpleMemBackend(id, params){ 
    std::string access_time = params.find<std::string>("access_time", "100 ns");
    self_link = configureSelfLink("Self", access_time,
            new Event::Handler2<SimpleMemory, &SimpleMemory::handleSelfEvent>(this));

    m_maxReqPerCycle = params.find<>("max_requests_per_cycle", 1);
}

void SimpleMemory::handleSelfEvent(SST::Event *event){
    MemCtrlEvent *ev = static_cast<MemCtrlEvent*>(event);
    output->verbose(CALL_INFO, 1, 0,
        "[diag-simpleMem] complete id=%" PRIx64 "\n", ev->reqId);
    handleMemResponse(ev->reqId);
    delete event;
}

bool SimpleMemory::issueRequest(ReqId id, Addr addr, bool isWrite, unsigned numBytes ){
    output->verbose(CALL_INFO, 1, 0,
        "[diag-simpleMem] issue id=%" PRIx64 " addr=0x%" PRIx64 " isWrite=%d size=%u\n",
        id, (Addr)addr, (int)isWrite, numBytes);
    self_link->send(1, new MemCtrlEvent(id));
    return true;
}
