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
            new Event::Handler<SimpleMemory>(this, &SimpleMemory::handleSelfEvent));

    m_maxReqPerCycle = params.find<>("max_requests_per_cycle", 1);
}

void SimpleMemory::handleSelfEvent(SST::Event *event){
    MemCtrlEvent *ev = static_cast<MemCtrlEvent*>(event);
#ifdef __SST_DEBUG_OUTPUT__
    output->debug(_L10_, "%s: Transaction done for id %" PRIx64 "\n", getName().c_str(),ev->reqId);
#endif
    handleMemResponse(ev->reqId);
    delete event;
}

bool SimpleMemory::issueRequest(ReqId id, Addr addr, bool isWrite, unsigned numBytes ){
#ifdef __SST_DEBUG_OUTPUT__
    output->debug(_L10_, "%s: Issued transaction for address %" PRIx64 " id %" PRIx64"\n", getName().c_str(),(Addr)addr,id);
#endif
    self_link->send(1, new MemCtrlEvent(id));
    return true;
}

