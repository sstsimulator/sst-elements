// Copyright 2013-2014 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2013-2014, Sandia Corporation
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

/* Translates a MemEvent string destination to a network address (integer) */
int MemNIC::addrForDest(const std::string &target) const
{
  std::map<std::string, int>::const_iterator addrIter = addrMap.find(target);
  if ( addrIter == addrMap.end() )
      dbg.fatal(CALL_INFO, -1, "Address for target %s not found in addrMap.\n", target.c_str());
  return addrIter->second;
}

bool MemNIC::isValidDestination(std::string target) {
    return (addrMap.find(target) != addrMap.end());
}

/* Get size in bytes for a MemEvent */
int MemNIC::getFlitSize(MemEvent *ev)
{
    /* addr (8B) + cmd (1B) + size */
    return 8 + ev->getSize();
}


void MemNIC::moduleInit(ComponentInfo &ci, Event::HandlerBase *handler)
{
    dbg.init("@t:MemNIC::@p():@l " + comp->getName() + ": ", 0, 0, Output::NONE); //TODO: Parameter

    this->ci = ci;
    this->recvHandler = handler;

    num_vcs = ci.num_vcs;

    flitSize = 16; // 16 Bytes as a default:  TODO: Parameterize this
    last_recv_vc = 0;

    Params params; // LinkControl doesn't actually use the params
    link_control = (Merlin::LinkControl*)comp->loadSubComponent("merlin.linkcontrol", comp, params);
    UnitAlgebra buf_size("1KB");
    link_control->configure(ci.link_port, UnitAlgebra(ci.link_bandwidth), num_vcs, buf_size, buf_size);

}


MemNIC::MemNIC(Component *comp, ComponentInfo &ci, Event::HandlerBase *handler) :
    typeInfoSent(false), comp(comp)
{
    moduleInit(ci, handler);
}


MemNIC::MemNIC(Component *comp) :
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
        if ( typeInfoList.empty() ) {
            ev = new InitMemRtrEvent(comp->getName(), ci.network_addr, ci.type);
            ev->dest = SST::Merlin::INIT_BROADCAST_ADDR;
            link_control->sendInitData(ev);
        } else {
            for ( std::vector<ComponentTypeInfo>::iterator i = typeInfoList.begin() ;
                    i != typeInfoList.end() ; ++i ) {
                ev = new InitMemRtrEvent(comp->getName(), ci.network_addr, ci.type, *i);
                ev->dest = SST::Merlin::INIT_BROADCAST_ADDR;
                link_control->sendInitData(ev);
            }
        }
        typeInfoSent = true;
        dbg.debug(_L10_, "Sent init data!\n");
    }
    while ( SST::Event *ev = link_control->recvInitData() ) {
        InitMemRtrEvent *imre = dynamic_cast<InitMemRtrEvent*>(ev);
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
            }
        } else {
            initQueue.push_back(static_cast<MemRtrEvent*>(ev));
        }
    }

}


void MemNIC::finish(void)
{
    link_control->finish();
}


void MemNIC::sendInitData(MemEvent *ev)
{
    MemRtrEvent *mre = new MemRtrEvent(ev);
    /* TODO:  Better addressing */
    mre->dest = Merlin::INIT_BROADCAST_ADDR;
    link_control->sendInitData(mre);
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
    dbg.fatal(CALL_INFO,-1,"MemNIC %s cannot find a target for address 0x%" PRIx64 "\n",comp->getName().c_str(),addr);
    return "";
}


bool MemNIC::clock(void)
{
    /* If stuff to send, and space to send it, send */
    bool empty = sendQueue.empty();
    if (!empty) {
        MemRtrEvent *head = sendQueue.front();
        if ( link_control->spaceToSend(0, head->size_in_bits) ) {
            bool sent = link_control->send(head, 0);
            if ( sent ) {
                if ( head->hasClientData() ) {
                    dbg.debug(_L10_, "Sent message ((%" PRIx64 ", %d) %s %" PRIx64 ") to (%d) [%s]\n", head->event->getID().first, head->event->getID().second, CommandString[head->event->getCmd()], head->event->getAddr(), head->dest, head->event->getDst().c_str());
                }
                sendQueue.pop_front();
            }
        }
    }

    if ( NULL != recvHandler ) {
        MemEvent *me = recv();
        if ( me ) {
            (*recvHandler)(me);
        }
    }
    
    return (empty == true);
}

MemEvent* MemNIC::recv(void)
{
    /* Check for received stuff */
    for ( int vc = 0; vc < num_vcs ; vc++ ) {
        // round-robin
        last_recv_vc = (last_recv_vc+1) % num_vcs;

        MemRtrEvent *mre = (MemRtrEvent*)link_control->recv(last_recv_vc);
        if ( NULL != mre ) {
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
    MemRtrEvent *mre = new MemRtrEvent(ev);
    mre->src = ci.network_addr;
    mre->dest = addrForDest(ev->getDst());
    mre->size_in_bits = 8 * getFlitSize(ev);
    mre->vn = 0;

    sendQueue.push_back(mre);
}


void MemNIC::sendNewTypeInfo(const ComponentTypeInfo &cti)
{
    for ( std::map<std::string, int>::const_iterator i = addrMap.begin() ; i != addrMap.end() ; ++i ) {
        InitMemRtrEvent *imre = new InitMemRtrEvent(comp->getName(), ci.network_addr, ci.type, cti);
        imre->dest = i->second;
        imre->size_in_bits = 128;  // 2* 64bit address (for a range)
        imre->vn = 0;
        sendQueue.push_back(imre);
    }
}
