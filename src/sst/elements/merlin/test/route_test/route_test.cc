// Copyright 2009-2018 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2018, NTESS
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
#include "sst/elements/merlin/test/route_test/route_test.h"

#include <unistd.h>
#include <signal.h>

#include <sst/core/event.h>
#include <sst/core/params.h>
#include <sst/core/simulation.h>
#include <sst/core/timeLord.h>
#include <sst/core/unitAlgebra.h>

#include <sst/core/interfaces/simpleNetwork.h>

namespace SST {
using namespace SST::Interfaces;

namespace Merlin {


route_test::route_test(ComponentId_t cid, Params& params) :
    Component(cid),
    sending(false),
    done(false),
    initialized(false)
{
    id = params.find<int>("id",-1);
    if ( id == -1 ) {
    }

    num_peers = params.find<int>("num_peers",-1);
    if ( num_peers == -1 ) {
    }

    std::string link_bw_s = params.find<std::string>("link_bw");
    if ( link_bw_s == "" ) {
    }
    UnitAlgebra link_bw(link_bw_s);
        
    // Create a LinkControl object
    // NOTE:  This MUST be the same length as 'num_vns'
    link_control = (SimpleNetwork*)loadSubComponent("merlin.linkcontrol", this, params);

    int num_vns = 1;
    UnitAlgebra buf_size("1kB");
    link_control->initialize("rtr", link_bw, num_vns, buf_size, buf_size);

    // // Register a clock
    // registerClock( "1GHz", new Clock::Handler<route_test>(this,&route_test::clock_handler), false);

    registerAsPrimaryComponent();
    primaryComponentDoNotEndSim();

    link_control->setNotifyOnReceive(new SST::Interfaces::SimpleNetwork::Handler<route_test>(this,&route_test::handle_event));
}


route_test::~route_test()
{
    delete link_control;
}

void route_test::finish()
{
    link_control->finish();
}

void
route_test::init(unsigned int phase) {
    link_control->init(phase);
    if ( id == 0 && !initialized ) {
        if ( link_control->isNetworkInitialized() ) {
            initialized = true;
            
            SimpleNetwork::Request* req =
                new SimpleNetwork::Request(SimpleNetwork::INIT_BROADCAST_ADDR, id,
                                           0, true, true);
            link_control->sendInitData(req);
        }
    }
    else {
        SimpleNetwork::Request* req = link_control->recvInitData();
        if ( req != NULL ) {
            // std::cout << "ROUTE_TEST " << id << " Received an init event in phase " << phase << "!" << std::endl;
            delete req;
            initialized = true;
        }
    }
}

void route_test::setup()
{
    link_control->setup();
    if ( link_control->getEndpointID() != id ) {
        std::cout << "NIC ids don't match: param = " << id << ", LinkControl = "
                          << link_control->getEndpointID() << std::endl;
    }
    if ( !initialized ) {
        std::cout << "Nic " << id << ": Broadcast failed!" << std::endl;  
    }

    if ( 0 == id ){
        sending = true;
        // Send first event to kick things off
        SST::Interfaces::SimpleNetwork::Request* req =
            new SST::Interfaces::SimpleNetwork::Request(num_peers - 1, 0, 64, true, true);
        link_control->send(req,0);
        // std::cout << req->src << " sending to " << req->dest << std::endl;
    }
}

bool route_test::handle_event(int vn)
{
    SST::Interfaces::SimpleNetwork::Request* req = link_control->recv(vn);
    if ( sending ) {
        if ( id == num_peers - 1 ) {
            primaryComponentOKToEndSim();
        }
        // If we are set to sending and this is from node id-1, then
        // we are starting from the top
        else if ( req->src == id - 1 ) {
            req->dest = num_peers - 1;
            req->src = id;
            link_control->send(req,0);
            // std::cout << req->src << " sending to " << req->dest << std::endl;
        }
        else {
            // Send to the next lowest host
            SST::Interfaces::SimpleNetwork::nid_t next = req->src - 1;
            if ( next == id ) {
                sending = false;
                primaryComponentOKToEndSim();
                // Also, send an ack back to start the other person
                // sending
                req->dest = req->src;
                req->src = id;
                link_control->send(req,0);
                // std::cout << req->src << " sending to " << req->dest << std::endl;
            }
            else {
                req->dest = next;
                req->src = id;
                link_control->send(req,0);
                // std::cout << req->src << " sending to " << req->dest << std::endl;
            }
        }
    }
    else {
        // If we aren't sending, then we just send an ack
        req->dest = req->src;
        req->src = id;
        link_control->send(req,0);
        // std::cout << req->src << " sending to " << req->dest << std::endl;
        // However, if we just received this from id - 1, then it's
        // our turn to start sending.
        if ( req->dest == id - 1 ) {
            sending = true;
        }
    }
    return true;
}


    

} // namespace Merlin
} // namespace SST

