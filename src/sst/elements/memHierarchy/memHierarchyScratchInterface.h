// -*- mode: c++ -*-
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
//

#ifndef COMPONENTS_MEMHIERARCHY_SCRATCHINTERFACE
#define COMPONENTS_MEMHIERARCHY_SCRATCHINTERFACE

#include <string>
#include <utility>
#include <map>
#include <queue>

#include <sst/core/sst_types.h>
#include <sst/core/link.h>
#include <sst/core/interfaces/simpleMem.h>
#include <sst/core/elementinfo.h>
#include <sst/core/output.h>

#include "sst/elements/memHierarchy/memEventBase.h"
#include "sst/elements/memHierarchy/moveEvent.h"
#include "sst/elements/memHierarchy/memEvent.h"

namespace SST {

class Component;
class Event;

namespace MemHierarchy {

/** Class is used to interface a compute mode (CPU, GPU) to MemHierarchy Scratchpad */
class MemHierarchyScratchInterface : public Interfaces::SimpleMem {

public:
/* Element Library Info */
    SST_ELI_REGISTER_SUBCOMPONENT(MemHierarchyScratchInterface, "memHierarchy", "scratchInterface", SST_ELI_ELEMENT_VERSION(1,0,0),
            "Interface to a scratchpad", "SST::Interfaces::SimpleMem")

/* Begin class definition */
    MemHierarchyScratchInterface(SST::Component *comp, Params &params);
    
    /** Initialize the link to be used to connect with MemHierarchy */
    virtual bool initialize(const std::string &linkName, HandlerBase *handler = NULL);

    /** Link getter */
    virtual SST::Link* getLink(void) const { return link_; }

    virtual void init(unsigned int phase);

    virtual void sendInitData(Request *req);
    virtual void sendRequest(Request *req);
    virtual Request* recvResponse(void);
    
    Output output;

private:

    /** Convert any incoming events to updated Requests, and fire handler */
    void handleIncoming(SST::Event *ev);
    
    /** Process ScratchEvents into updated Requests*/
    Interfaces::SimpleMem::Request* processIncoming(MemEventBase *ev);

    /** Update Request with results of ScratchEvent */
    void updateRequest(Interfaces::SimpleMem::Request* req, MemEventBase *me) const;
    
    /** Function used internally to create the ScratchEvent that will be used by MemHierarchy */
    MoveEvent* createMoveEvent(Interfaces::SimpleMem::Request* req) const;
    MemEvent* createMemEvent(Interfaces::SimpleMem::Request* req) const;

    Component*      owner_;
    HandlerBase*    recvHandler_;
    SST::Link*      link_;
    std::map<SST::Event::id_type, Interfaces::SimpleMem::Request*> requests_;
    Addr baseAddrMask_;
    std::string rqstr_;
    Addr remoteMemStart_;
    bool allNoncache_;

    bool initDone_;
    std::queue<MemEventInit*> initSendQueue_;
};

}
}

#endif
