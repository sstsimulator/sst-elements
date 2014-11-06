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
  dbg.debug(_L10_, "Translated address %s to %d\n", target.c_str(), addrIter->second);
  return addrIter->second;
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
    link_control = (Merlin::LinkControl*)comp->loadModule("merlin.linkcontrol", params);
    UnitAlgebra buf_size("1KB");
    link_control->configureLink(comp, ci.link_port, UnitAlgebra(ci.link_bandwidth), num_vcs, buf_size, buf_size);

}


MemNIC::MemNIC(Component *comp, ComponentInfo &ci, Event::HandlerBase *handler) :
    comp(comp)
{
    moduleInit(ci, handler);
}


MemNIC::MemNIC(Component *comp) :
    comp(comp)
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
        InitMemRtrEvent *ev = new InitMemRtrEvent(comp->getName(),
                ci.network_addr, ci.type, ci.typeInfo);
        ev->dest = SST::Merlin::INIT_BROADCAST_ADDR;
        link_control->sendInitData(ev);
        dbg.debug(_L10_, "Sent init data!\n");
    }
    while ( SST::Event *ev = link_control->recvInitData() ) {
        InitMemRtrEvent *imre = dynamic_cast<InitMemRtrEvent*>(ev);
        if ( imre ) {
            dbg.debug(_L10_, "Creating IMRE for %d (%s)\n", imre->address, imre->name.c_str());
            addrMap[imre->name] = imre->address;
            ComponentInfo ci;
            ci.name = imre->name;
            ci.network_addr = imre->address;
            ci.type = imre->compType;
            ci.typeInfo = imre->compInfo;
            peers.push_back(ci);

	    // save a copy for directory controller lookups later
	    if (ci.type == MemNIC::TypeDirectoryCtrl) {
	      directories.push_back(ci);
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

std::string MemNIC::findTargetDirectory(Addr addr)
{
  for ( std::vector<MemNIC::ComponentInfo>::const_iterator i = directories.begin() ;
	i != directories.end() ; ++i ) {
    const MemNIC::ComponentTypeInfo &di = i->typeInfo;
    //dbg.debug(_L10_, "Comparing address 0x%"PRIx64" to %s [0x%"PRIx64" - 0x%"PRIx64" by 0x%"PRIx64", 0x%"PRIx64"]\n",
    //        addr, i->name.c_str(), di.dirctrl.rangeStart, di.dirctrl.rangeEnd, di.dirctrl.interleaveStep, di.dirctrl.interleaveSize);
    if ( addr >= di.dirctrl.rangeStart && addr < di.dirctrl.rangeEnd ) {
      if ( 0 == di.dirctrl.interleaveSize ) {
	return i->name;
      } else {
	Addr temp = addr - di.dirctrl.rangeStart;
	Addr offset = temp % di.dirctrl.interleaveStep;
	if ( offset < di.dirctrl.interleaveSize ) {
	  return i->name;
	}
      }
    }
  }
  _abort(DMAEngine, "Unable to find directory for address 0x%"PRIx64"\n", addr);
  return "";
}


bool MemNIC::clock(void)
{
    /* If stuff to send, and space to send it, send */
    bool empty = sendQueue.empty();
    if (!empty) {
        dbg.debug(_L10_, "SendQueue has %zu elements in it.\n", sendQueue.size());
        MemRtrEvent *head = sendQueue.front();
        if ( link_control->spaceToSend(0, head->size_in_bits) ) {
            bool sent = link_control->send(head, 0);
            if ( sent ) {
                dbg.debug(_L10_, "Sent message ((%"PRIx64", %d) %s %"PRIx64") to (%d) [%s]\n", head->event->getID().first, head->event->getID().second, CommandString[head->event->getCmd()], head->event->getAddr(), head->dest, head->event->getDst().c_str());
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
            MemEvent *deliverEvent = mre->event;
            deliverEvent->setDeliveryLink(mre->getLinkId(), NULL);
            delete mre;
            return deliverEvent;
        }
    }
    return NULL;
}


void MemNIC::send(MemEvent *ev)
{
    dbg.debug(_L10_, "Adding event (%"PRIx64", %d) to send queue\n", ev->getID().first, ev->getID().second);
    MemRtrEvent *mre = new MemRtrEvent(ev);
    mre->src = ci.network_addr;
    mre->dest = addrForDest(ev->getDst());
    mre->size_in_bits = 8 * getFlitSize(ev);
    mre->vn = 0;

    sendQueue.push_back(mre);
}


