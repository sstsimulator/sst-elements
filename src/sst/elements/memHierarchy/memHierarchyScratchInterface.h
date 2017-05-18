// -*- mode: c++ -*-
// Copyright 2009-2017 Sandia Corporation. Under the terms
// of Contract DE-NA0003525 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2017, Sandia Corporation
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.
//

#ifndef COMPONENTS_MEMHIERARCHY_SCRATCHINTERFACE
#define COMPONENTS_MEMHIERARCHY_SCRATCHINTERFACE

#include <string>
#include <utility>
#include <map>

#include <sst/core/sst_types.h>
#include <sst/core/link.h>
#include <sst/core/interfaces/simpleMem.h>
#include "scratchEvent.h"
#include "sst/core/output.h"

namespace SST {

class Component;
class Event;

namespace MemHierarchy {

/** Class is used to interface a compute mode (CPU, GPU) to MemHierarchy Scratchpad */
class MemHierarchyScratchInterface : public Interfaces::SimpleMem {

public:
    MemHierarchyScratchInterface(SST::Component *comp, Params &params);
    
    /** Initialize the link to be used to connect with MemHierarchy */
    virtual bool initialize(const std::string &linkName, HandlerBase *handler = NULL);

    /** Link getter */
    virtual SST::Link* getLink(void) const { return link_; }

    virtual void sendInitData(Request *req);
    virtual void sendRequest(Request *req);
    virtual Request* recvResponse(void);
    
    Output output;

private:

    /** Convert any incoming events to updated Requests, and fire handler */
    void handleIncoming(SST::Event *ev);
    
    /** Process ScratchEvents into updated Requests*/
    Interfaces::SimpleMem::Request* processIncoming(ScratchEvent *ev);

    /** Update Request with results of ScratchEvent */
    void updateRequest(Interfaces::SimpleMem::Request* req, ScratchEvent *me) const;
    
    /** Function used internally to create the ScratchEvent that will be used by MemHierarchy */
    ScratchEvent* createScratchEvent(Interfaces::SimpleMem::Request* req) const;

    Component*      owner_;
    HandlerBase*    recvHandler_;
    SST::Link*      link_;
    std::map<ScratchEvent::id_type, Interfaces::SimpleMem::Request*> requests_;
};

}
}

#endif
