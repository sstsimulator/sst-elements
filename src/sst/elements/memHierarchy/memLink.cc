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

MemLink::MemLink(ComponentId_t id, Params &params, TimeConverter* tc) : MemLinkBase(id, params, tc) {
    // Configure link
    bool found;
    std::string latency = params.find<std::string>("latency", "0ps", found);
    std::string port = params.find<std::string>("port", "port");

    if (found) {
        link = configureLink(port, latency, new Event::Handler<MemLink>(this, &MemLink::recvNotify));
    } else {
        link = configureLink(port, new Event::Handler<MemLink>(this, &MemLink::recvNotify));
    }

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

                delete ev;
            } else if (mEv->getInitCmd() == MemEventInit::InitCommand::Endpoint) { 
                // Intercept and record so that we know how to find this endpoint.
                // We don't need to record whether a particular region
                // is noncacheable because the StandardMem interfaces will enforce
                MemEventInitEndpoint * mEvEndPt = static_cast<MemEventInitEndpoint*>(mEv);
                dbg.debug(_L10_, "%s received init message: %s\n", getName().c_str(), mEvEndPt->getVerboseString().c_str());
                std::vector<std::pair<MemRegion,bool>> regions = mEvEndPt->getRegions();
                for (auto it = regions.begin(); it != regions.end(); it++) {
                    EndpointInfo epInfo;
                    epInfo.name = mEvEndPt->getSrc();
                    epInfo.addr = 0; // Not on a network so don't need it
                    epInfo.id = 0; // Not on a network so don't need it
                    epInfo.region = it->first;
                    addEndpoint(epInfo);
                }
                initReceiveQ.push(mEv); // Our component will forward on all its other ports
            } else {
                initReceiveQ.push(mEv);
            }
        } else
            delete ev;
    }
    
    // Attempt to drain send Q
    for (auto it = initSendQ.begin(); it != initSendQ.end(); ) {
        std::string dst = findTargetDestination((*it)->getRoutingAddress());
        if (dst != "") {
            dbg.debug(_L10_, "%s sending init message: %s\n", getName().c_str(), (*it)->getVerboseString().c_str());
            (*it)->setDst(dst);
            link->sendInitData(*it);
            it = initSendQ.erase(it);
        } else {
            it++;
        }
    }

}


void MemLink::setup() {
    dbg.debug(_L10_, "Routing information for %s\n", getName().c_str());
    for (auto it = remotes.begin(); it != remotes.end(); it++) {
        dbg.debug(_L10_, "    Remote: %s\n", it->toString().c_str()); 
    }
    for (auto it = endpoints.begin(); it != endpoints.end(); it++) {
        dbg.debug(_L10_, "    Endpoint: %s\n", it->toString().c_str()); 
    }

    if (!initSendQ.empty()) {
        dbg.fatal(CALL_INFO, -1, "%s, Error: Unable to find destination for init event %s\n",
                getName().c_str(), (*initSendQ.begin())->getVerboseString().c_str());
    }
}


/**
 * send init data
 */
void MemLink::sendInitData(MemEventInit * event, bool broadcast) {
    if (!broadcast) {
        std::string dst = findTargetDestination(event->getRoutingAddress());
        if (dst == "") {
            /* Stall this until address is known */
            initSendQ.insert(event);
            return;
        }
        event->setDst(dst);
    }
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
    remoteNames.insert(info.name);
}

void MemLink::addEndpoint(EndpointInfo info) {
    endpoints.insert(info);
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

std::string MemLink::getTargetDestination(Addr addr) {
    std::string dst = findTargetDestination(addr);
    if ("" != dst) {
        return dst;
    }
    stringstream error;
    error << getName() + " (MemLink) cannot find a destination for address " << std::hex << addr << std::dec << endl;
    error << "Known destinations: " << endl;
    for (std::set<EndpointInfo>::const_iterator it = remotes.begin(); it != remotes.end(); it++) {
        error << it->name << " " << it->region.toString() << endl;
    }
    dbg.fatal(CALL_INFO, -1, "%s", error.str().c_str());
    return "";
}

std::string MemLink::findTargetDestination(Addr addr) {
    for (std::set<EndpointInfo>::const_iterator it = remotes.begin(); it != remotes.end(); it++) {
        if (it->region.contains(addr)) return it->name;
    }
    return "";
}

bool MemLink::isReachable(std::string dst) {
   return remoteNames.find(dst) != remoteNames.end();
}

std::string MemLink::getAvailableDestinationsAsString() {
    std::stringstream str;
    for (std::set<EndpointInfo>::const_iterator it = endpoints.begin(); it != endpoints.end(); it++) {
        str << it->toString() << std::endl;
    }
    return str.str();
}
