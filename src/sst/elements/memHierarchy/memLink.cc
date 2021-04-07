// Copyright 2013-2021 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2013-2021, NTESS
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

MemLink::MemLink(ComponentId_t id, Params &params) : MemLinkBase(id, params) {
    // Configure link
    std::string latency = params.find<std::string>("latency", "50ps");
    std::string port = params.find<std::string>("port", "port");

    link = configureLink(port, latency, new Event::Handler<MemLink>(this, &MemLink::recvNotify));

    if (!link)
        dbg.fatal(CALL_INFO, -1, "%s, Error: unable to configure link on port '%s'\n", getName().c_str(), port.c_str());

    dbg.debug(_L10_, "%s memLink info is: Name: %s, addr: %" PRIu64 ", id: %" PRIu32 "\n",
            getName().c_str(), info.name.c_str(), info.addr, info.id);

}

/* init function */
void MemLink::init(unsigned int phase) {
    if (!phase) {
        MemEventInitRegion * ev = new MemEventInitRegion(info.name, info.region, false);
        dbg.debug(_L10_, "%s sending region init message: %s\n", getName().c_str(), ev->getVerboseString().c_str());
        link->sendInitData(ev);
    }

    SST::Event * ev;
    while ((ev = link->recvInitData())) {
        MemEventInit * mEv = static_cast<MemEventInit*>(ev);
        if (mEv) {
            if (mEv->getInitCmd() == MemEventInit::InitCommand::Region) {
                MemEventInitRegion * mEvRegion = static_cast<MemEventInitRegion*>(mEv);
                dbg.debug(_L10_, "%s received init message: %s\n", getName().c_str(), mEvRegion->getVerboseString().c_str());

                EndpointInfo epInfo;
                epInfo.name = mEvRegion->getSrc();
                epInfo.addr = 0;
                epInfo.id = 0;
                epInfo.region = mEvRegion->getRegion();
                addRemote(epInfo);

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

void MemLink::addRemote(EndpointInfo info) {
    remotes.insert(info);
}

bool MemLink::isDest(std::string UNUSED(str)) {
    return true;
}

bool MemLink::isSource(std::string UNUSED(str)) {
    return true;
}

std::set<MemLinkBase::EndpointInfo>* MemLink::getSources() {
    return &remotes;
}

std::set<MemLinkBase::EndpointInfo>* MemLink::getDests() {
    return &remotes;
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
    MemEventBase * mEv = static_cast<MemEventBase*>(ev);
    if (mEv) return mEv;

    if (ev) delete ev;

    return nullptr;
}

std::string MemLink::findTargetDestination(Addr addr) {
    for (std::set<EndpointInfo>::const_iterator it = remotes.begin(); it != remotes.end(); it++) {
        if (it->region.contains(addr)) return it->name;
    }

    stringstream error;
    error << getName() + " (MemLink) cannot find a destination for address " << addr << endl;
    error << "Known destinations: " << endl;
    for (std::set<EndpointInfo>::const_iterator it = remotes.begin(); it != remotes.end(); it++) {
        error << it->name << " " << it->region.toString() << endl;
    }
    dbg.fatal(CALL_INFO, -1, "%s", error.str().c_str());
    return "";
}

