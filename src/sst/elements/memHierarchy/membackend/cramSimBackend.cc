// Copyright 2009-2016 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2016, Sandia Corporation
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

CramSimMemory::CramSimMemory(Component *comp, Params &params) : SimpleMemBackend(comp, params){
    std::string access_time = params.find<std::string>("access_time", "100 ns");
    cramsim_link = comp->configureLink( "cube_link", access_time,
            new Event::Handler<CramSimMemory>(this, &CramSimMemory::handleCramsimEvent));

    output->init("CramSimMemory[@p:@l]: ", 10, 0, Output::STDOUT);
}


//bool DRAMSimMemory::issueRequest(ReqId id, Addr addr, bool isWrite, unsigned ){

bool CramSimMemory::issueRequest(ReqId reqId, Addr addr, bool isWrite, unsigned numBytes ){
#ifdef __SST_DEBUG_OUTPUT__
    output->debug(_L10_, "Issued transaction to Cube Chain for address %" PRIx64 "\n", (Addr)addr);
#endif
    // TODO:  FIX THIS:  ugly hardcoded limit on outstanding requests
    if (dramReqs.size() > 255) {
        return false;
    }

    if (dramReqs.find(reqId) != dramReqs.end())
        output->fatal(CALL_INFO, -1, "Assertion failed");

    dramReqs.insert( reqId );
    cramsim_link->send( new CramSim::MemReqEvent(reqId,addr,isWrite,numBytes,0) );
    return true;
}


void CramSimMemory::handleCramsimEvent(SST::Event *event){
    //TODO: Implement a handler for respense event from CramSim
    CramSim::MemRespEvent *ev = dynamic_cast<CramSim::MemRespEvent*>(event);

    if (ev) {
        if ( dramReqs.find( ev->getReqId() ) != dramReqs.end() ) {
            dramReqs.erase( ev->getReqId() );
            handleMemResponse( ev->getReqId());
      		delete event;
        } else {  
            output->fatal(CALL_INFO, -1, "Could not match incoming request from cubes\n");
		}
    } else {
        output->fatal(CALL_INFO, -1, "Recived wrong event type from cubes\n");
    }
}
