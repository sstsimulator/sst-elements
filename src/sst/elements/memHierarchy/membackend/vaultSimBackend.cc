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
#include "membackend/vaultSimBackend.h"

using namespace SST;
using namespace SST::MemHierarchy;

VaultSimMemory::VaultSimMemory(Component *comp, Params &params) : MemBackend(comp, params){
    std::string access_time = params.find<std::string>("access_time", "100 ns");
#if 0
    cube_link = ctrl->configureLink( "cube_link", access_time,
            new Event::Handler<VaultSimMemory>(this, &VaultSimMemory::handleCubeEvent));
#endif
}



bool VaultSimMemory::issueRequest(ReqId reqId, Addr addr, bool isWrite, unsigned numBytes ){
#ifdef __SST_DEBUG_OUTPUT__
    ctrl->dbg.debug(_L10_, "Issued transaction to Cube Chain for address %" PRIx64 "\n", (Addr)addr);
#endif
#if 0
    // TODO:  FIX THIS:  ugly hardcoded limit on outstanding requests
    if (outToCubes.size() > 255) {
        req->status_ = DRAMReq::NEW;
        return false;
    }
    MemEvent::id_type reqID = req->reqEvent_->getID();
    if (outToCubes.find(reqID) != outToCubes.end())
        ctrl->dbg.fatal(CALL_INFO, -1, "Assertion failed");
    outToCubes[reqID] = req; // associate the memEvent w/ the DRAMReq
    MemEvent *outgoingEvent = new MemEvent(*req->reqEvent_); // we make a copy, because the dramreq keeps to 'original'
    cube_link->send(outgoingEvent); // send the event off
#endif
    return true;
}


#if 0
void VaultSimMemory::handleCubeEvent(SST::Event *event){
  MemEvent *ev = dynamic_cast<MemEvent*>(event);
  if (ev) {
    memEventToDRAMMap_t::iterator ri = outToCubes.find(ev->getResponseToID());
    if (ri != outToCubes.end()) {
      ctrl->handleMemResponse(ri->second);
      outToCubes.erase(ri);
      delete event;
    }
    else ctrl->dbg.fatal(CALL_INFO, -1, "Could not match incoming request from cubes\n");
  }
  else ctrl->dbg.fatal(CALL_INFO, -1, "Recived wrong event type from cubes\n");

}
#endif
