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
#include "membackend/cramSimBackend.h"
#include "sst/elements/CramSim/memReqEvent.hpp"

using namespace SST;
using namespace SST::MemHierarchy;
using namespace SST::CramSim;

CramSimMemory::CramSimMemory(ComponentId_t id, Params &params) : SimpleMemBackend(id, params){ 
    std::string access_time = params.find<std::string>("access_time", "100 ns");
    if (isPortConnected("cramsim_link"))
        cramsim_link = configureLink( "cramsim_link", access_time,
                new Event::Handler<CramSimMemory>(this, &CramSimMemory::handleCramsimEvent));
    else
        cramsim_link = configureLink( "cube_link", access_time,
                new Event::Handler<CramSimMemory>(this, &CramSimMemory::handleCramsimEvent));

    m_maxNumOutstandingReqs = params.find<int>("max_outstanding_requests",256);
    output= new Output("CramSimMemory[@p:@l]: ", 1, 0, Output::STDOUT);
}



bool CramSimMemory::issueRequest(ReqId reqId, Addr addr, bool isWrite, unsigned numBytes ){

    if(memReqs.size()>=m_maxNumOutstandingReqs) {
        output->verbose(CALL_INFO, 1, 0, "the number of outstanding requests reaches to max %d\n",
                        m_maxNumOutstandingReqs);
        return false;
    }

    if (memReqs.find(reqId) != memReqs.end())
        output->fatal(CALL_INFO, -1, "Assertion failed");

    memReqs.insert( reqId );
    cramsim_link->send( new CramSim::MemReqEvent(reqId,addr,isWrite,numBytes,0) );
    return true;
}


void CramSimMemory::handleCramsimEvent(SST::Event *event){

    CramSim::MemRespEvent *ev = dynamic_cast<CramSim::MemRespEvent*>(event);

    if (ev) {
        if ( memReqs.find( ev->getReqId() ) != memReqs.end() ) {
            memReqs.erase( ev->getReqId() );
            handleMemResponse( ev->getReqId());
      		delete event;
        } else {
            output->fatal(CALL_INFO, -1, "Could not match incoming request from cubes\n");
		}
    } else {
        output->fatal(CALL_INFO, -1, "Recived wrong event type from cubes\n");
    }
}
