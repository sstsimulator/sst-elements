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
#include "membackend/vaultSimBackend.h"
#include "sst/elements/VaultSimC/memReqEvent.h"

using namespace SST;
using namespace SST::MemHierarchy;
using namespace SST::VaultSim;

VaultSimMemory::VaultSimMemory(Component *comp, Params &params) : HMCMemBackend(comp, params){
    std::string access_time = params.find<std::string>("access_time", "100 ns");
    cube_link = comp->configureLink( "cube_link", access_time,
            new Event::Handler<VaultSimMemory>(this, &VaultSimMemory::handleCubeEvent));

    output->init("VaultSimMemory[@p:@l]: ", 10, 0, Output::STDOUT);
}



bool VaultSimMemory::issueRequest(ReqId reqId, Addr addr, bool isWrite, uint32_t flags, unsigned numBytes ){
#ifdef __SST_DEBUG_OUTPUT__
    output->debug(_L10_, "Issued transaction to Cube Chain for address %" PRIx64 "\n", (Addr)addr);
#endif
    // TODO:  FIX THIS:  ugly hardcoded limit on outstanding requests
    if (outToCubes.size() > 255) {
        return false;
    }

    if (outToCubes.find(reqId) != outToCubes.end())
        output->fatal(CALL_INFO, -1, "Assertion failed");

    outToCubes.insert( reqId );
    cube_link->send( new VaultSim::MemReqEvent(reqId,addr,isWrite,numBytes) ); 
    return true;
}


void VaultSimMemory::handleCubeEvent(SST::Event *event){
    VaultSim::MemRespEvent *ev = dynamic_cast<VaultSim::MemRespEvent*>(event);

    if (ev) {
        if ( outToCubes.find( ev->reqId ) != outToCubes.end() ) {
            outToCubes.erase( ev->reqId );
            getConvertor()->handleMemResponse( ev->reqId, ev->flags );
      		delete event;
        } else {  
            output->fatal(CALL_INFO, -1, "Could not match incoming request from cubes\n");
		}
    } else {
        output->fatal(CALL_INFO, -1, "Recived wrong event type from cubes\n");
    }
}
