// Copyright 2013 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2013, Sandia Corporation
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS
#endif
#include <inttypes.h>
#include <algorithm>

#include <sst_config.h>

#include <sst/core/serialization/element.h>
#include <sst/core/simulation.h>
#include <sst/core/timeLord.h>

#include "memNIC.h"


#define DPRINTF( fmt, args...) __DBG( DBG_CACHE, MemNIC, "%s: " fmt, comp->getName().c_str(), ## args )

using namespace SST;
using namespace SST::MemHierarchy;

const int MemNIC::num_vcs = 3;


/* Translates a MemEvent string destination to an integer */
int MemNIC::addrForDest(const std::string &target)
{
    return addrMap[target];
}

/* Get size in flits for a MemEvent */
int MemNIC::getFlitSize(MemEvent *ev)
{
    /* addr (8B) + cmd (1B) + size */
    return std::max((9 + ev->getSize()) / flitSize, 1UL);
}


MemNIC::MemNIC(Component *comp, ComponentInfo &ci, Event::HandlerBase *handler) :
    comp(comp), ci(ci), recvHandler(handler)
{
    flitSize = 16; // 16 Bytes as a default:  TODO: Parameterize this
    last_recv_vc = 0;

    TimeConverter *tc = Simulation::getSimulation()->getTimeLord()->getTimeConverter(ci.link_bandwidth);


    int *buf_size = new int[num_vcs];
    for ( int i = 0 ; i < num_vcs ; i++ ) buf_size[i] = 100;
    link_control = new Merlin::LinkControl(comp, ci.link_port, tc, num_vcs, buf_size, buf_size);
    delete [] buf_size;

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
        DPRINTF("Sent init data!\n");
    }
    while ( SST::Event *ev = link_control->recvInitData() ) {
        InitMemRtrEvent *imre = dynamic_cast<InitMemRtrEvent*>(ev);
        if ( imre ) {
            DPRINTF("Creating IMRE for %d (%s)\n", imre->address, imre->name.c_str());
            addrMap[imre->name] = imre->address;
            ComponentInfo ci;
            ci.name = imre->name;
            ci.network_addr = imre->address;
            ci.type = imre->compType;
            ci.typeInfo = imre->compInfo;
            peers.push_back(ci);
        } else {
            initQueue.push_back(static_cast<MemRtrEvent*>(ev));
        }
    }

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


void MemNIC::clock(void)
{
    /* If stuff to send, and space to send it, send */
    if ( !sendQueue.empty() ) {
        DPRINTF("SendQueue has %zu elements in it.\n", sendQueue.size());
        MemRtrEvent *head = sendQueue.front();
        if ( link_control->spaceToSend(0, head->size_in_flits) ) {
            bool sent = link_control->send(head, 0);
            if ( sent ) {
                DPRINTF("Sent message ((%"PRIu64", %d) %s 0x%"PRIx64") to (%d) [%s]\n", head->event->getID().first, head->event->getID().second, CommandString[head->event->getCmd()], head->event->getAddr(), head->dest, head->event->getDst().c_str());
                sendQueue.pop_front();
            }
        }
    }

    /* Check for received stuff */
    for ( int vc = 0; vc < num_vcs ; vc++ ) {
        // round-robin
        last_recv_vc = (last_recv_vc+1) % num_vcs;

        MemRtrEvent *mre = (MemRtrEvent*)link_control->recv(last_recv_vc);
        if ( mre != NULL ) {
            MemEvent *deliverEvent = mre->event;
            deliverEvent->setDeliveryLink(mre->getLinkId(), NULL);
            (*recvHandler)(deliverEvent);

            delete mre;
            break; // Only do one delivery per clock
        }
    }

}


void MemNIC::send(MemEvent *ev)
{
    DPRINTF("Adding event (%"PRIu64", %d) to send queue\n", ev->getID().first, ev->getID().second);
    MemRtrEvent *mre = new MemRtrEvent(ev);
    mre->src = ci.network_addr;
    mre->dest = addrForDest(ev->getDst());
    mre->size_in_flits = getFlitSize(ev);
    mre->vc = 0;

    sendQueue.push_back(mre);
}


BOOST_CLASS_EXPORT(MemNIC::MemRtrEvent)
BOOST_CLASS_EXPORT(MemNIC::InitMemRtrEvent)

