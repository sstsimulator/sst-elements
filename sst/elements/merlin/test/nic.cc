// Copyright 2009-2010 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2010, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.
#include <sst_config.h>
#include "sst/core/serialization/element.h"

#include <sst/core/element.h>
#include <sst/core/simulation.h>
#include <sst/core/timeLord.h>

#include "sst/elements/merlin/linkControl.h"
#include "sst/elements/merlin/test/nic.h"

nic::nic(ComponentId_t cid, Params& params) :
    Component(cid),
    packets_sent(0),
    packets_recd(0),
    stalled_cycles(0),
    done(false)
{
    id = params.find_integer("id");
    if ( id == -1 ) {
    }
    std::cout << "id: " << id << std::endl;
    std::cout << "Nic ID:  " << id << " has Component id " << cid << std::endl;

    num_peers = params.find_integer("num_peers");
    if ( num_peers == -1 ) {
    }
    std::cout << "num_peers: " << num_peers << std::endl;

    num_vcs = params.find_integer("num_vcs");
    if ( num_vcs == -1 ) {
    }
    std::cout << "num_vcs: " << num_vcs << std::endl;

    std::string link_bw = params.find_string("link_bw");
    if ( link_bw == "" ) {
    }
    std::cout << "link_bw: " << link_bw << std::endl;
    TimeConverter* tc = Simulation::getSimulation()->getTimeLord()->getTimeConverter(link_bw);
    
    // Create a LinkControl object
    int buf_size[2] = {10, 10};
    link_control = new LinkControl(this, "rtr", tc, 2, buf_size, buf_size);

    last_target = id;

    // Register a clock
    registerClock( "1GHz", new Clock::Handler<nic>(this,&nic::clock_handler), false);

    registerExit();

}

int
nic::Finish()
{
    std::cout << "Nic " << id << " had " << stalled_cycles << " stalled cycles." << std::endl;
    return 0;
}

int
nic::Setup()
{
    return link_control->Setup();
}

bool
nic::clock_handler(Cycle_t cycle)
{
    if ( !done && (cycle == 500 || packets_recd >= 10) ) {
        unregisterExit();
        done = true;
    }
    // Send packets
    if ( link_control->spaceToSend(0,5) ) {
        RtrEvent* ev = new RtrEvent();

        last_target++;
        if ( last_target == id ) last_target++;
        last_target %= num_peers;
        if ( last_target == id ) last_target++;
        last_target %= num_peers;

        ev->dest = last_target;

        ev->vc = 0;
        ev->size_in_flits = 5;
        link_control->send(ev,0);
        std::cout << cycle << ": " << id << " sent packet to " << ev->dest << std::endl;
    }
    else {
        stalled_cycles++;
    }

    RtrEvent* rec_ev = link_control->recv(0);
    if ( rec_ev != NULL ) {
        packets_recd++;
        std::cout << cycle << ": " << id << " Received an event on vc " << rec_ev->vc << " (packet "<<packets_recd<<" )"<< std::endl;
        delete rec_ev;
    }
    rec_ev = link_control->recv(1);
    if ( rec_ev != NULL ) {
        packets_recd++;
        std::cout << cycle << ": " << id << " Received an event on vc " << rec_ev->vc << " (packet "<<packets_recd<<" )"<< std::endl;
        delete rec_ev;
    }

    return false;
}
