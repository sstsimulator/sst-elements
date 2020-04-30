// Copyright 2009-2020 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2020, NTESS
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
#include <sst/core/output.h>
#include <sst/core/interfaces/simpleNetwork.h>

#include "shogun_event.h"
#include "shogun_init_event.h"
#include "shogun_credit_event.h"

#include "shogun_nic.h"

using namespace SST::Shogun;

ShogunNIC::ShogunNIC(SST::ComponentId_t id, Params& params, int vns = 1)
    : SimpleNetwork(id)
    , netID(-1), bw(UnitAlgebra("1GB/s")) {

    const int verbosity = params.find<uint32_t>("verbose", 0);

    //TODO: output = new ...
    output = new SST::Output("ShogunNIC-Startup ", verbosity, 0, Output::STDOUT);
    reqQ = nullptr;
    remote_input_slots = -1;

    onSendFunctor = nullptr;
    onRecvFunctor = nullptr;

    std::string portName = params.find<std::string>("port_name", "port");

    output->verbose(CALL_INFO, 4, 0, "Configuring port %s...\n", portName.c_str());

    link = configureLink(portName, "1ps", new Event::Handler<ShogunNIC>(this, &ShogunNIC::recvLinkEvent));

    if (!link)
        output->fatal(CALL_INFO, -1, "%s, Error: attempt to configure link on port '%s' was unsuccessful.\n", getName().c_str(), portName.c_str());
}

ShogunNIC::~ShogunNIC()
{
    delete output;
}

void ShogunNIC::sendInitData(SimpleNetwork::Request* req)
{
    output->verbose(CALL_INFO, 8, 0, "Send init-data called.\n");

    ShogunEvent* ev = new ShogunEvent();
    ev->setSource(netID);
    ev->setPayload(req);

    link->sendInitData(ev);

    output->verbose(CALL_INFO, 8, 0, "Send init-data completed.\n");
}

SimpleNetwork::Request* ShogunNIC::recvInitData()
{
    output->verbose(CALL_INFO, 8, 0, "Recv init-data on net: %5" PRId64 " init-events have %5zu events.\n", netID, initReqs.size());

    if (!initReqs.empty()) {
        SimpleNetwork::Request* req = initReqs.front();
        initReqs.erase(initReqs.begin());

        output->verbose(CALL_INFO, 8, 0, "-> returning event to caller.\n");
        return req;
    } else {
        return nullptr;
    }
}

bool ShogunNIC::send(SimpleNetwork::Request* req, int vn)
{
    if (netID > -1) {
        output->verbose(CALL_INFO, 8, 0, "Send: remote-slots: %5" PRIi32 "\n", remote_input_slots);

        if (remote_input_slots > 0) {
            ShogunEvent* newEv = new ShogunEvent(req->dest, netID);
            newEv->setPayload(req);

            link->send(newEv);

            if (nullptr != onSendFunctor) {
                if ((*onSendFunctor)(vn)) {
                    onSendFunctor = nullptr;
                }
            }

            remote_input_slots--;
            output->verbose(CALL_INFO, 8, 0, "-> sent, remote slots now %5" PRId32 ", dest=%5" PRId64 "\n", remote_input_slots, req->dest);

            return true;
        } else {
            output->verbose(CALL_INFO, 8, 0, "-> called send but no free remote slots, so cannot send request\n");
            return false;
        }
    } else {
        output->fatal(CALL_INFO, -1, "Error: send operation was called but the network is not configured yet (netID still equals -1)\n");
    }
    return false; // eliminate compile warning
}

SimpleNetwork::Request* ShogunNIC::recv(int vn)
{
    if (netID > -1) {
        output->verbose(CALL_INFO, 8, 0, "Recv called, pending local entries: %" PRIi32 "\n", reqQ->count());

        if (!reqQ->empty()) {
            output->verbose(CALL_INFO, 8, 0, "-> Popping request from local entries queue.\n");
            SimpleNetwork::Request* req = reqQ->pop();

            if (nullptr != req) {
                output->verbose(CALL_INFO, 8, 0, "-> request src: %" PRId64 "\n", req->src);
            }

            link->send(new ShogunCreditEvent(netID));
            return req;
        } else {
            output->verbose(CALL_INFO, 8, 0, "-> request-q empty, nothing to receive\n");
            return nullptr;
        }
    } else {
        output->fatal(CALL_INFO, -1, "Error: recv was called but the network is not configured yet, netID is still -1\n");
        return nullptr;
    }
}

void ShogunNIC::setup() {}

void ShogunNIC::init(unsigned int phase)
{
    //	if( 0 == phase ) {
    //		// Let the crossbar know how many slots we have in our incoming queue
    //		link->sendInitData( new ShogunInitEvent( -1, -1, -1 ) );
    //	}

    SST::Event* ev = link->recvInitData();

    while (nullptr != ev) {
        ShogunInitEvent* initEv = dynamic_cast<ShogunInitEvent*>(ev);

        if (nullptr != initEv) {
            reconfigureNIC(initEv);
            delete ev;
        } else {
            ShogunCreditEvent* creditEv = dynamic_cast<ShogunCreditEvent*>(ev);

            if (nullptr == creditEv) {
                ShogunEvent* shogunEvent = dynamic_cast<ShogunEvent*>(ev);

                if (nullptr == shogunEvent) {
                    fprintf(stderr, "ERROR: UNKNOWN EVENT TYPE AT INIT IN NIC\n");
                } else {
                    initReqs.push_back(shogunEvent->getPayload());
                }
            } else {
                delete ev;
            }
        }

        ev = link->recvInitData();
    }
}

void ShogunNIC::complete(unsigned int UNUSED(phase)) {}
void ShogunNIC::finish() {}

bool ShogunNIC::spaceToSend(int vn, int num_bits)
{
    output->verbose(CALL_INFO, 4, 0, "Space to send? %s\n",
        (remote_input_slots > 0) ? "yes" : "no");
    return (remote_input_slots > 0);
}

bool ShogunNIC::requestToReceive(int vn)
{
    return !(reqQ->empty());
}

void ShogunNIC::setNotifyOnReceive(SimpleNetwork::HandlerBase* functor)
{
    output->verbose(CALL_INFO, 4, 0, "Set recv-notify functor.\n");
    onRecvFunctor = functor;
}

void ShogunNIC::setNotifyOnSend(SimpleNetwork::HandlerBase* functor)
{
    output->verbose(CALL_INFO, 4, 0, "Set send-notify functor\n");
    onSendFunctor = functor;
}

bool ShogunNIC::isNetworkInitialized() const
{
    const bool netReady = (netID > -1);

    if (netReady) {
        output->verbose(CALL_INFO, 16, 0, "network-config: ready\n");
    } else {
        output->verbose(CALL_INFO, 16, 0, "network-config: not-ready\n");
    }

    return netReady;
}

SimpleNetwork::nid_t ShogunNIC::getEndpointID() const
{
    return netID;
}

const UnitAlgebra& ShogunNIC::getLinkBW() const
{
    return bw;
}

void ShogunNIC::recvLinkEvent(SST::Event* ev)
{
    output->verbose(CALL_INFO, 8, 0, "RECV-LINK-EVENT CALLED\n");

    ShogunEvent* inEv = dynamic_cast<ShogunEvent*>(ev);

    if (nullptr != inEv) {
        output->verbose(CALL_INFO, 8, 0, "Recv link in handler: current pending request count: %5" PRIi32 "\n", reqQ->count());

        if (reqQ->full()) {
            output->fatal(CALL_INFO, -1, "Error - received a message but the NIC queues are full.\n");
            exit(-1);
        }

        reqQ->push(inEv->getPayload());

        if (nullptr != onRecvFunctor) {
            if (!(*onRecvFunctor)(inEv->getPayload()->vn)) {
                onRecvFunctor = nullptr;
            }
        }
    } else {
        ShogunCreditEvent* creditEv = dynamic_cast<ShogunCreditEvent*>(ev);

        if (nullptr != creditEv) {
            remote_input_slots++;
            output->verbose(CALL_INFO, 8, 0, "Recv link credit event, remote_input_slots now set to: %5" PRIi32 "\n", remote_input_slots);
        } else {
            ShogunInitEvent* initEv = dynamic_cast<ShogunInitEvent*>(ev);

            if (nullptr != initEv) {
                reconfigureNIC(initEv);
            } else {
                output->fatal(CALL_INFO, -1, "Unknown event type received. Not sure how to process.\n");
            }
        }

        delete ev;
    }

    output->verbose(CALL_INFO, 8, 0, "RECV-LINK-EVENT CALL END\n");
}

void ShogunNIC::reconfigureNIC(ShogunInitEvent* initEv)
{
    remote_input_slots = initEv->getQueueSlotCount();

    if (nullptr != reqQ) {
        delete reqQ;
    }

    reqQ = new ShogunQueue<SimpleNetwork::Request*>(initEv->getQueueSlotCount());

    netID = initEv->getNetworkID();
    port_count = initEv->getPortCount();

    output->verbose(CALL_INFO, 4, 0, "Shogun NIC reconfig (%s) with %5" PRIi32 " slots, xbar-total-ports: %5" PRIi32 "\n",
        getName().c_str(), remote_input_slots, port_count);

    if (nullptr != output && netID > -1) {
        const uint32_t currentVerbosity = output->getVerboseLevel();
        delete output;
        char outPrefix[256];

        sprintf(outPrefix, "[t=@t][NIC%5" PRId64 "][%25s][%5" PRId64 "]: ", netID, getName().c_str(), netID);
        output = new SST::Output(outPrefix, currentVerbosity, 0, SST::Output::STDOUT);
    }
}
