// Copyright 2013-2018 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2013-2018, NTESS
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
#include "memLink.h"

#include <sst/core/simulation.h>

using namespace SST;
using namespace SST::MemHierarchy;

/* Constructor */
MemLink::MemLink(Component * parent, Params &params) : MemLinkBase(parent, params) {
    
    // Configure link
    std::string latency = params.find<std::string>("latency", "50ps");
    std::string port = params.find<std::string>("port", "");

    link = configureLink(port, latency, new Event::Handler<MemLink>(this, &MemLink::recvNotify));
   
    dbg.debug(_L10_, "%s memLink info is: Name: %s, addr: %" PRIu64 ", id: %" PRIu32 "\n",
            getName().c_str(), info.name.c_str(), info.addr, info.id);

}

/* init function */
void MemLink::init(unsigned int phase) {
    if (!phase) {
        MemEventInitRegion * ev = new MemEventInitRegion(getName(), info.region, false);
        dbg.debug(_L10_, "%s sending region init message: %s\n", getName().c_str(), ev->getVerboseString().c_str());
        link->sendInitData(ev);
    }

    SST::Event * ev;
    while ((ev = link->recvInitData())) {
        MemEventInit * mEv = dynamic_cast<MemEventInit*>(ev);
        if (mEv) {
            if (mEv->getInitCmd() == MemEventInit::InitCommand::Region) {
                MemEventInitRegion * mEvRegion = static_cast<MemEventInitRegion*>(mEv);
                dbg.debug(_L10_, "%s received init message: %s\n", getName().c_str(), mEvRegion->getVerboseString().c_str());
            
                EndpointInfo epInfo;
                epInfo.name = mEvRegion->getSrc();
                epInfo.addr = 0;
                epInfo.id = 0;
                epInfo.node = node;
                epInfo.region = mEvRegion->getRegion();
                //sourceEndpointInfo.insert(epInfo);
                //destEndpointInfo.insert(epInfo);
                addSource(epInfo);
                addDest(epInfo);

                if (mEvRegion->getSetRegion() && acceptRegion) {
                    dbg.debug(_L10_, "\tUpdating local region\n");
                    info.region = mEvRegion->getRegion();
                }
                delete ev;
            } else { /* No need to filter by source since this is a direct link */
                initReceiveQ.push(mEv);
            }
        } else 
            delete ev;
    }
}

/**
 * send init data 
 */
void MemLink::sendInitData(MemEventInit * event) {
    dbg.debug(_L10_, "%s sending init message: %s\n", getName().c_str(), event->getVerboseString().c_str());
    link->sendInitData(event);
}

/**
 * receive init data
 */
MemEventInit * MemLink::recvInitData() {
    MemEventInit * me = nullptr;
    if (!initReceiveQ.empty()) {
        me = initReceiveQ.front();
        initReceiveQ.pop();
    }
    return me;
}


/**
 * send event on link
 */
void MemLink::send(MemEventBase *ev) {
    link->send(ev);
}

/**
 * Polled receive
 */
MemEventBase * MemLink::recv() {
    SST::Event * ev = link->recv();
    MemEventBase * mEv = dynamic_cast<MemEventBase*>(ev);
    if (mEv) return mEv;

    if (ev) delete ev;

    return nullptr;
}
