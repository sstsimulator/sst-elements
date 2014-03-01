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

#include <sst_config.h>
#include <sst/core/serialization.h>
#include "memNIC.h"

#include <algorithm>

#include <sst/core/params.h>
#include <sst/core/simulation.h>
#include <sst/core/timeLord.h>



using namespace SST;
using namespace SST::MemHierarchy;

const int MemNIC::num_vcs = 3;


/* Translates a MemEvent string destination to an integer */
int MemNIC::addrForDest(const std::string &target)
{
    dbg.output(CALL_INFO, "Translated address %s to %d\n", target.c_str(), addrMap[target]);
    return addrMap[target];
}

/* Get size in flits for a MemEvent */
int MemNIC::getFlitSize(MemEvent *ev)
{
    /* addr (8B) + cmd (1B) + size */
    return std::max((unsigned long)((9 + ev->getSize()) / flitSize), 1UL);
}


MemNIC::MemNIC(Component *comp, ComponentInfo &ci, Event::HandlerBase *handler) :
    comp(comp), ci(ci), recvHandler(handler)
{
    dbg.init("@t:MemNIC::@p():@l " + comp->getName() + ": ", 0, 0, Output::NONE); //TODO: Parameter

    flitSize = 16; // 16 Bytes as a default:  TODO: Parameterize this
    last_recv_vc = 0;

    if ( ci.link_tc == NULL )
        ci.link_tc = Simulation::getSimulation()->getTimeLord()->getTimeConverter(ci.link_bandwidth);


    Params params; // LinkControl doesn't actually use the params
    link_control = (Merlin::LinkControl*)comp->loadModule("merlin.linkcontrol", params);
    int *buf_size = new int[num_vcs];
    for ( int i = 0 ; i < num_vcs ; i++ ) buf_size[i] = 100;
    link_control->configureLink(comp, ci.link_port, ci.link_tc, num_vcs, buf_size, buf_size);
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
        dbg.output(CALL_INFO, "Sent init data!\n");
    }
    while ( SST::Event *ev = link_control->recvInitData() ) {
        InitMemRtrEvent *imre = dynamic_cast<InitMemRtrEvent*>(ev);
        if ( imre ) {
            dbg.output(CALL_INFO, "Creating IMRE for %d (%s)\n", imre->address, imre->name.c_str());
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


void MemNIC::clock(void)
{
    /* If stuff to send, and space to send it, send */
    if ( !sendQueue.empty() ) {
        dbg.output(CALL_INFO, "SendQueue has %zu elements in it.\n", sendQueue.size());
        MemRtrEvent *head = sendQueue.front();
        if ( link_control->spaceToSend(0, head->size_in_flits) ) {
            bool sent = link_control->send(head, 0);
            if ( sent ) {
                dbg.output(CALL_INFO, "Sent message ((%#016llx, %d) %s %#016llx) to (%d) [%s]\n", head->event->getID().first, head->event->getID().second, CommandString[head->event->getCmd()], head->event->getAddr(), head->dest, head->event->getDst().c_str());
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
    dbg.output(CALL_INFO, "Adding event (%#016llx, %d) to send queue\n", ev->getID().first, ev->getID().second);
    MemRtrEvent *mre = new MemRtrEvent(ev);
    mre->src = ci.network_addr;
    mre->dest = addrForDest(ev->getDst());
    mre->size_in_flits = getFlitSize(ev);
    mre->vc = 0;

    sendQueue.push_back(mre);
}


BOOST_CLASS_EXPORT(MemNIC::MemRtrEvent)
BOOST_CLASS_EXPORT(MemNIC::InitMemRtrEvent)

