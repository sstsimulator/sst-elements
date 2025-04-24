// Copyright 2013-2025 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2013-2025, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// of the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include <sst/core/sst_config.h>
#include "memLink.h"

using namespace SST;
using namespace SST::MemHierarchy;

/* Constructor */

MemLink::MemLink(ComponentId_t id, Params &params, TimeConverter* tc) : MemLinkBase(id, params, tc) {
    // Configure link
    bool found;
    std::string latency = params.find<std::string>("latency", "0ps", found);
    std::string port = params.find<std::string>("port", "port");

    if (found) {
        link_ = configureLink(port, latency, new Event::Handler2<MemLinkBase, &MemLinkBase::recvNotify>(this));
    } else {
        link_ = configureLink(port, new Event::Handler2<MemLinkBase, &MemLinkBase::recvNotify>(this));
    }

    if (!link_)
        dbg.fatal(CALL_INFO, -1, "%s, Error: unable to configure link on port '%s'\n", getName().c_str(), port.c_str());

    dbg.debug(_L10_, "%s memLink info is: Name: %s, addr: %" PRIu64 ", id: %" PRIu32 "\n",
            getName().c_str(), info.name.c_str(), info.addr, info.id);
}

/* init function */
void MemLink::init(unsigned int phase) {
    if (!phase) {
        // It's not easy to discern if we're on a lowlink or highlink and set the group to Dest or Source accordingly.
        // Anonymous subcomponents don't have the slot name in their name. For now, we don't strictly have to differentiate
        // Dest & Source, so just always put Dest. If there's a bus between this component and the component on the other side,
        // the bus will update the group to the correct one since it has a notion of "high" (source) vs "low" (dest) ports.
        // Otherwise, the group will be incorrect if this MemLink is loaded into a 'highlink' subcomponent slot. 
        // Not a problem unless we start treating source & dest differently.
        MemEventInitRegion * ev = new MemEventInitRegion(info.name, info.region, MemEventInitRegion::ReachableGroup::Dest);
        dbg.debug(_L10_, "%s sending region init message: %s\n", getName().c_str(), ev->getVerboseString().c_str());
        link_->sendUntimedData(ev);
    }

    SST::Event * ev;
    while ((ev = link_->recvUntimedData())) {
        MemEventInit * mEv = static_cast<MemEventInit*>(ev);
        if (mEv) {
            if (mEv->getInitCmd() == MemEventInit::InitCommand::Region) {
                MemEventInitRegion * mEvRegion = static_cast<MemEventInitRegion*>(mEv);
                if (mEvRegion->getGroup() != MemEventInitRegion::ReachableGroup::Peer) {
                    dbg.debug(_L10_, "%s received init message: %s\n", getName().c_str(), mEvRegion->getVerboseString().c_str());

                    EndpointInfo ep_info;
                    ep_info.name = mEvRegion->getSrc();
                    ep_info.addr = 0;
                    ep_info.id = 0;
                    ep_info.region = mEvRegion->getRegion();
                    addRemote(ep_info);
                } else {
                    EndpointInfo ep_info;
                    ep_info.name = mEvRegion->getSrc();
                    ep_info.addr = 0;
                    ep_info.id = 0;
                    ep_info.region = mEvRegion->getRegion();
                    peers_.insert(ep_info);
                    reachable_names_.insert(ep_info.name);
                    peer_names_.insert(ep_info.name);

                }
                delete ev;
            } else if (mEv->getInitCmd() == MemEventInit::InitCommand::Endpoint) { 
                // Intercept and record so that we know how to find this endpoint.
                // We don't need to record whether a particular region
                // is noncacheable because the StandardMem interfaces will enforce
                MemEventInitEndpoint * mEvEndPt = static_cast<MemEventInitEndpoint*>(mEv);
                dbg.debug(_L10_, "%s received init message: %s\n", getName().c_str(), mEvEndPt->getVerboseString().c_str());
                std::vector<std::pair<MemRegion,bool>> regions = mEvEndPt->getRegions();
                for (auto it = regions.begin(); it != regions.end(); it++) {
                    EndpointInfo ep_info;
                    ep_info.name = mEvEndPt->getSrc();
                    ep_info.addr = 0; // Not on a network so don't need it
                    ep_info.id = 0; // Not on a network so don't need it
                    ep_info.region = it->first;
                    addEndpoint(ep_info);
                }
                untimed_receive_queue_.push(mEv); // Our component will forward on all its other ports
            } else {
                untimed_receive_queue_.push(mEv);
            }
        } else
            delete ev;
    }
    
    // Attempt to drain send Q
    for (auto it = init_send_queue_.begin(); it != init_send_queue_.end(); ) {
        std::string dst = findTargetDestination((*it)->getRoutingAddress());
        if (dst != "") {
            dbg.debug(_L10_, "%s sending init message: %s\n", getName().c_str(), (*it)->getVerboseString().c_str());
            (*it)->setDst(dst);
            link_->sendUntimedData(*it);
            it = init_send_queue_.erase(it);
        } else {
            it++;
        }
    }

}


void MemLink::setup() {
    dbg.debug(_L10_, "Routing information for %s\n", getName().c_str());
    for (auto it = remotes_.begin(); it != remotes_.end(); it++) {
        dbg.debug(_L10_, "    Remote: %s\n", it->toString().c_str()); 
    }
    for (auto it = endpoints_.begin(); it != endpoints_.end(); it++) {
        dbg.debug(_L10_, "    Endpoint: %s\n", it->toString().c_str()); 
    }
    auto sources = getSources();
    if (sources->empty()) dbg.debug(_L10_, "    Source: NONE\n");
    for (auto it = sources->begin(); it != sources->end(); it++) {
        dbg.debug(_L10_, "    Source: %s\n", it->toString().c_str());
    }
    auto dests = getDests();
    if (dests->empty()) dbg.debug(_L10_, "    Destination: NONE\n");
    for (auto it = dests->begin(); it != dests->end(); it++) {
        dbg.debug(_L10_, "    Destination: %s\n", it->toString().c_str());
    }
    if (peers_.empty()) dbg.debug(_L10_, "    Peer: NONE\n");
    for (auto it = peers_.begin(); it != peers_.end(); it++) {
        dbg.debug(_L10_, "    Peer: %s\n", it->toString().c_str());
    }
    if (!init_send_queue_.empty()) {
        dbg.fatal(CALL_INFO, -1, "%s, Error: Unable to find destination for init event %s\n",
                getName().c_str(), (*init_send_queue_.begin())->getVerboseString().c_str());
    }
}

void MemLink::complete(unsigned int phase) {
    
    // Receive events
    SST::Event * ev;
    while ((ev = link_->recvUntimedData())) {
        MemEventInit * mEv = static_cast<MemEventInit*>(ev);
        untimed_receive_queue_.push(mEv);
    }
}

/**
 * send untimed data
 */
void MemLink::sendUntimedData(MemEventInit * event, bool broadcast = true, bool lookup_dst = true) {
    
    if (!broadcast && lookup_dst) {
        std::string dst = findTargetDestination(event->getRoutingAddress());
        if (dst == "") {
            /* Stall this until address is known */
            init_send_queue_.insert(event);
            return;
        }
        event->setDst(dst);
    }
    dbg.debug(_L10_, "%s sending untimed message: %s\n", getName().c_str(), event->getVerboseString().c_str());
    link_->sendUntimedData(event);
}

/**
 * receive untimed data
 */
MemEventInit * MemLink::recvUntimedData() {
    MemEventInit * me = nullptr;
    if (!untimed_receive_queue_.empty()) {
        me = untimed_receive_queue_.front();
        untimed_receive_queue_.pop();
    }
    return me;
}

void MemLink::addRemote(EndpointInfo info) {
    remotes_.insert(info);
    reachable_names_.insert(info.name);
}

void MemLink::addEndpoint(EndpointInfo info) {
    endpoints_.insert(info);
}

bool MemLink::isDest(std::string UNUSED(str)) {
    return true;
}

bool MemLink::isSource(std::string UNUSED(str)) {
    return true;
}

bool MemLink::isPeer(std::string str) {
    return peer_names_.find(str) != peer_names_.end();
}

std::set<MemLinkBase::EndpointInfo>* MemLink::getSources() {
    return &remotes_;
}

std::set<MemLinkBase::EndpointInfo>* MemLink::getDests() {
    return &remotes_;
}

std::set<MemLinkBase::EndpointInfo>* MemLink::getPeers() {
    return &peers_;
}

/**
 * send event on link
 */
void MemLink::send(MemEventBase *ev) {
    link_->send(ev);
}

/**
 * Polled receive
 */
MemEventBase * MemLink::recv() {
    SST::Event * ev = link_->recv();
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
    for (std::set<EndpointInfo>::const_iterator it = remotes_.begin(); it != remotes_.end(); it++) {
        error << it->name << " " << it->region.toString() << endl;
    }
    dbg.fatal(CALL_INFO, -1, "%s", error.str().c_str());
    return "";
}

std::string MemLink::findTargetDestination(Addr addr) {
    for (std::set<EndpointInfo>::const_iterator it = remotes_.begin(); it != remotes_.end(); it++) {
        if (it->region.contains(addr)) return it->name;
    }
    return "";
}

bool MemLink::isReachable(std::string dst) {
   return reachable_names_.find(dst) != reachable_names_.end();
}

std::string MemLink::getAvailableDestinationsAsString() {
    std::stringstream str;
    for (std::set<EndpointInfo>::const_iterator it = endpoints_.begin(); it != endpoints_.end(); it++) {
        str << it->toString() << std::endl;
    }
    return str.str();
}

void MemLink::serialize_order(SST::Core::Serialization::serializer& ser) {
    MemLinkBase::serialize_order(ser);

    SST_SER(link_);
    SST_SER(remotes_);
    SST_SER(peers_);
    SST_SER(endpoints_);
    SST_SER(reachable_names_);
    SST_SER(peer_names_);
    SST_SER(init_send_queue_); // Doesn't actually need to be included
}
