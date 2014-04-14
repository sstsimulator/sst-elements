// -*- mode: c++ -*-
// Copyright 2009-2013 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2014, Sandia Corporation
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.
//

#ifndef COMPONENTS_MEMHIERARCHY_MEMORYINTERFACE
#define COMPONENTS_MEMHIERARCHY_MEMORYINTERFACE

#include <string>
#include <utility>
#include <map>

#include <sst/core/sst_types.h>
#include <sst/core/module.h>
#include <sst/core/link.h>
#include <sst/core/interfaces/simpleMem.h>
#include "memEvent.h"

namespace SST {

class Component;
class Event;

namespace MemHierarchy {

class MemHierarchyInterface : public Interfaces::SimpleMem {

public:
    MemHierarchyInterface(SST::Component *comp, Params &params);
    virtual bool initialize(const std::string &linkName, HandlerBase *handler = NULL);

    virtual SST::Link* getLink(void) const { return link; }

    virtual void sendInitData(Request *req);
    virtual void sendRequest(Request *req);
    virtual Request* recvResponse(void);


private:

    Interfaces::SimpleMem::Request* processIncoming(MemEvent *ev);
    void handleIncoming(SST::Event *ev);
    void updateRequest(Interfaces::SimpleMem::Request* req, MemEvent *me) const;
    MemEvent* createMemEvent(Interfaces::SimpleMem::Request* req) const;

    Component *owner;
    HandlerBase *recvHandler;
    SST::Link *link;
    std::map<MemEvent::id_type, Interfaces::SimpleMem::Request*> requests;

};

}
}

#endif
