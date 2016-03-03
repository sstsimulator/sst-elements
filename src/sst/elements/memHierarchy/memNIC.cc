// Copyright 2013-2015 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2013-2015, Sandia Corporation
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include <sst_config.h>
#include <sst/core/serialization.h>
#include "memNIC.h"

#include <algorithm>

#include <sst/core/params.h>
#include <sst/core/simulation.h>
#include <sst/core/timeLord.h>



using namespace SST;
using namespace SST::MemHierarchy;
using namespace SST::Interfaces;

/* Translates a MemEvent string destination to a network address (integer) */
int MemNIC::addrForDest(const std::string &target) const
{
  std::map<std::string, int>::const_iterator addrIter = addrMap.find(target);
  if ( addrIter == addrMap.end() )
      dbg->fatal(CALL_INFO, -1, "Address for target %s not found in addrMap.\n", target.c_str());
  return addrIter->second;
}

bool MemNIC::isValidDestination(std::string target) {
    return (addrMap.find(target) != addrMap.end());
}

/* Get size in bytes for a MemEvent */
int MemNIC::getFlitSize(MemEvent *ev)
{
    /* addr (8B) + cmd (1B) + size */
    return 8 + ev->getPayloadSize();
}


void MemNIC::moduleInit(ComponentInfo &ci, Event::HandlerBase *handler)
{
    this->ci = ci;
    this->recvHandler = handler;

    num_vcs = ci.num_vcs;

    flitSize = 16; // 16 Bytes as a default:  TODO: Parameterize this
    last_recv_vc = 0;

    Params params; // LinkControl doesn't actually use the params
    link_control = (SimpleNetwork*)comp->loadSubComponent("merlin.linkcontrol", comp, params);
    link_control->initialize(ci.link_port, UnitAlgebra(ci.link_bandwidth), num_vcs, UnitAlgebra(ci.link_inbuf_size), UnitAlgebra(ci.link_outbuf_size));

    recvNotifyHandler = new SimpleNetwork::Handler<MemNIC>(this, &MemNIC::recvNotify);
    link_control->setNotifyOnReceive(recvNotifyHandler);
}


MemNIC::MemNIC(Component *comp, Output* output, Addr dAddr, ComponentInfo &ci, Event::HandlerBase *handler) :
    typeInfoSent(false), comp(comp)
{
    dbg = output;
    DEBUG_ADDR = dAddr;
    DEBUG_ALL = (dAddr == -1);
    moduleInit(ci, handler);
}


MemNIC::MemNIC(Component *comp, Params& params) :
    typeInfoSent(false), comp(comp)
{
}


void MemNIC::setup(void)
{
    link_control->setup();
    while ( initQueue.size() ) {
        delete initQueue.front();
        initQueue.pop_front();
    }
}


void MemNIC::init(unsigned int phase)
{

    link_control->init(phase);
    if ( !phase ) {
        InitMemRtrEvent *ev;
        SimpleNetwork::Request* req;
        if ( typeInfoList.empty() ) {
            ev = new InitMemRtrEvent(comp->getName(), ci.network_addr, ci.type);
            req = new SimpleNetwork::Request();
            req->dest = SimpleNetwork::INIT_BROADCAST_ADDR;
            req->src = ci.network_addr;
            req->givePayload(ev);
            link_control->sendInitData(req);
        } else {
            for ( std::vector<ComponentTypeInfo>::iterator i = typeInfoList.begin() ;
                    i != typeInfoList.end() ; ++i ) {
                ev = new InitMemRtrEvent(comp->getName(), ci.network_addr, ci.type, *i);
                req = new SimpleNetwork::Request();
                req->dest = SimpleNetwork::INIT_BROADCAST_ADDR;
                req->src = ci.network_addr;
                req->givePayload(ev);
                link_control->sendInitData(req);
            }
        }
        typeInfoSent = true;
        dbg->debug(_L10_, "Sent init data!\n");
    }
    // while ( SST::Event *ev = link_control->recvInitData() ) {
    while ( SimpleNetwork::Request *req = link_control->recvInitData() ) {
        // InitMemRtrEvent *imre = dynamic_cast<InitMemRtrEvent*>(ev);
        Event* payload = req->takePayload();
        InitMemRtrEvent *imre = dynamic_cast<InitMemRtrEvent*>(payload);
        if ( imre ) {
            addrMap[imre->name] = imre->address;
            
            ComponentInfo peerCI;
            peerCI.name = imre->name;
            peerCI.network_addr = imre->address;
            peerCI.type = imre->compType;
            peers.push_back(std::make_pair(peerCI, imre->compInfo));

            // save a copy for lookups later if we should be sending requests to this entity
            if ((ci.type == MemNIC::TypeCache || ci.type == MemNIC::TypeNetworkCache) && (peerCI.type == MemNIC::TypeDirectoryCtrl || peerCI.type == MemNIC::TypeNetworkDirectory)) { // cache -> dir
                destinations[imre->compInfo] = imre->name;
            } else if (ci.type == MemNIC::TypeCacheToCache && peerCI.type == MemNIC::TypeNetworkCache) { // higher cache -> lower cache
                destinations[imre->compInfo] = imre->name;
            } else if (ci.type == MemNIC::TypeSmartMemory && (peerCI.type == MemNIC::TypeSmartMemory || peerCI.type == MemNIC::TypeDirectoryCtrl || peerCI.type == MemNIC::TypeNetworkDirectory ) ) {
                destinations[imre->compInfo] = imre->name;
            }
        } else {
            initQueue.push_back(static_cast<MemRtrEvent*>(payload));
        }
        delete req;
    }

}


void MemNIC::finish(void)
{
    link_control->finish();
}


void MemNIC::sendInitData(MemEvent *ev)
{
    MemRtrEvent *mre = new MemRtrEvent(ev);
    SimpleNetwork::Request* req = new SimpleNetwork::Request();
    /* TODO:  Better addressing */
    req->dest = SimpleNetwork::INIT_BROADCAST_ADDR;
    req->givePayload(mre);
    link_control->sendInitData(req);
}

MemEvent* MemNIC::recvInitData(void)
{
    if ( initQueue.size() ) {
        MemRtrEvent *mre = initQueue.front();
        initQueue.pop_front();
        MemEvent *ev = mre->event;
        delete mre;
        return ev;
    }
    return NULL;
}

bool MemNIC::initDataReady() {
    return (initQueue.size() > 0);
}

std::string MemNIC::findTargetDestination(Addr addr)
{
    for ( std::map<MemNIC::ComponentTypeInfo, std::string>::const_iterator i = destinations.begin() ;
            i != destinations.end() ; ++i ) {
        if ( i->first.contains(addr) ) return i->second;
    }
    dbg->fatal(CALL_INFO,-1,"MemNIC %s cannot find a target for address 0x%" PRIx64 "\n",comp->getName().c_str(),addr);
    return "";
}


bool MemNIC::clock(void)
{
    /* If stuff to send, and space to send it, send */
    bool empty = sendQueue.empty();
    if (!empty) {
        // MemRtrEvent *head = sendQueue.front();
        SimpleNetwork::Request *head = sendQueue.front();
        if ( link_control->spaceToSend(0, head->size_in_bits) ) {
            bool sent = link_control->send(head, 0);
            if ( sent ) {
                if ( static_cast<MemRtrEvent*>(head->inspectPayload())->hasClientData() ) {
                    MemEvent* event = static_cast<MemEvent*>((static_cast<MemRtrEvent*>(head->inspectPayload()))->event);
#ifdef __SST_DEBUG_OUTPUT__
                    if (DEBUG_ALL || DEBUG_ADDR == event->getBaseAddr()) {
                        dbg->debug(_L8_, "Sent message ((%" PRIx64 ", %d) %s %" PRIx64 ") to (%" PRIu64 ") [%s]\n", 
                                event->getID().first, event->getID().second, CommandString[event->getCmd()], event->getAddr(), head->dest, event->getDst().c_str());
                    }
#endif
                }
                sendQueue.pop_front();
            }
        }
    }

    return (empty == true);
}

/*  
 *  Notify MemNIC on receiving an event
 */
bool MemNIC::recvNotify(int) {
    MemEvent * me = recv();
    if (me) {
#ifdef __SST_DEBUG_OUTPUT__
        if (DEBUG_ALL || DEBUG_ADDR == me->getBaseAddr())
            dbg->debug(_L9_, "%s, memNIC recv: src: %s. (Cmd: %s, Rqst size: %u, Payload size: %u)\n", 
                    comp->getName().c_str(), me->getSrc().c_str(), CommandString[me->getCmd()], me->getSize(), me->getPayloadSize());
#endif
        (*recvHandler)(me);
    }
    return true;
}

MemEvent* MemNIC::recv(void)
{
    /* Check for received stuff */
    for ( int vc = 0; vc < num_vcs ; vc++ ) {
        // round-robin
        last_recv_vc = (last_recv_vc+1) % num_vcs;

        SimpleNetwork::Request* req = link_control->recv(last_recv_vc);
        if ( NULL != req ) {
            MemRtrEvent *mre = static_cast<MemRtrEvent*>(req->takePayload());
            delete req;
            if ( mre->hasClientData() ) {
                MemEvent *deliverEvent = mre->event;
                deliverEvent->setDeliveryLink(mre->getLinkId(), NULL);
                delete mre;
                return deliverEvent;
            } else {
                InitMemRtrEvent *imre = static_cast<InitMemRtrEvent*>(mre);

                // We shouldn't have any new players on the network - just updated address ranges.
                // Ideally, we should support removing of address ranges as well.  Future work...

                ComponentInfo peerCI;
                peerCI.name = imre->name;
                peerCI.network_addr = imre->address;
                peerCI.type = imre->compType;
                // If user has not cleared info...
                if ( !peers.empty() )
                    peers.push_back(std::make_pair(peerCI, imre->compInfo));

                // Save any new address ranges.
                if ((ci.type == MemNIC::TypeCache || ci.type == MemNIC::TypeNetworkCache) && (peerCI.type == MemNIC::TypeDirectoryCtrl || peerCI.type == MemNIC::TypeNetworkDirectory)) { // cache -> dir
                    destinations[imre->compInfo] = imre->name;
                } else if (ci.type == MemNIC::TypeCacheToCache && peerCI.type == MemNIC::TypeNetworkCache) { // higher cache -> lower cache
                    destinations[imre->compInfo] = imre->name;
                }
            }
        }
    }
    return NULL;
}


void MemNIC::send(MemEvent *ev)
{
    SimpleNetwork::Request* req = new SimpleNetwork::Request();
    MemRtrEvent *mre = new MemRtrEvent(ev);
    req->src = ci.network_addr;
    req->dest = addrForDest(ev->getDst());
    req->size_in_bits = 8 * getFlitSize(ev);
    req->vn = 0;
#ifdef __SST_DEBUG_OUTPUT__
    if (DEBUG_ALL || DEBUG_ADDR == ev->getBaseAddr())
        dbg->debug(_L9_, "%s, memNIC send: dst: %s; bits: %zu. (Cmd: %s, Rqst size: %u, Payload size: %u)\n", 
                comp->getName().c_str(), ev->getDst().c_str(), req->size_in_bits, CommandString[ev->getCmd()], ev->getSize(), ev->getPayloadSize());
#endif
    req->givePayload(mre);
    
    sendQueue.push_back(req);
}


void MemNIC::sendNewTypeInfo(const ComponentTypeInfo &cti)
{
    for ( std::map<std::string, int>::const_iterator i = addrMap.begin() ; i != addrMap.end() ; ++i ) {
        InitMemRtrEvent *imre = new InitMemRtrEvent(comp->getName(), ci.network_addr, ci.type, cti);
        SimpleNetwork::Request* req = new SimpleNetwork::Request();

        req->dest = i->second;
        req->size_in_bits = 128;  // 2* 64bit address (for a range)
        req->vn = 0;
        req->givePayload(imre);
        sendQueue.push_back(req);
    }
}
