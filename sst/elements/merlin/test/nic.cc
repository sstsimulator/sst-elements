// Copyright 2009-2013 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2013, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.
#include <unistd.h>
#include <signal.h>
#include <sst_config.h>
#include "sst/core/serialization/element.h"

#include <sst/core/element.h>
#include <sst/core/simulation.h>
#include <sst/core/timeLord.h>

#include "sst/elements/merlin/linkControl.h"
#include "sst/elements/merlin/test/nic.h"

using namespace SST::Merlin;

nic::nic(ComponentId_t cid, Params& params) :
    Component(cid),
    last_vc(0),
    packets_sent(0),
    packets_recd(0),
    stalled_cycles(0),
    done(false)
{
    id = params.find_integer("id");
    if ( id == -1 ) {
    }
    std::cout << "id: " << id << "\n";
    std::cout << "Nic ID:  " << id << " has Component id " << cid << "\n";

    num_peers = params.find_integer("num_peers");
    if ( num_peers == -1 ) {
    }
    std::cout << "num_peers: " << num_peers << "\n";

    num_vcs = params.find_integer("num_vcs");
    if ( num_vcs == -1 ) {
    }
    std::cout << "num_vcs: " << num_vcs << "\n";

    std::string link_bw = params.find_string("link_bw");
    if ( link_bw == "" ) {
    }
    std::cout << "link_bw: " << link_bw << std::endl;
    TimeConverter* tc = Simulation::getSimulation()->getTimeLord()->getTimeConverter(link_bw);

    addressMode = SEQUENTIAL;

    if ( !params.find_string("topology").compare("fattree") ) {
        addressMode = FATTREE_IP;
        ft_loading = params.find_integer("fattree:loading");
        ft_radix = params.find_integer("fattree:radix");
    }

    // Create a LinkControl object
    // NOTE:  This MUST be the same length as 'num_vcs'
    int buf_size[3] = {100, 100, 100};
    link_control = new LinkControl(this, "rtr", tc, num_vcs, buf_size, buf_size);

    last_target = id;
    next_seq = new int[num_peers];
    for ( int i = 0 ; i < num_peers ; i++ )
        next_seq[i] = 0;

    // Register a clock
    registerClock( "1GHz", new Clock::Handler<nic>(this,&nic::clock_handler), false);

    registerAsPrimaryComponent();
    primaryComponentDoNotEndSim();

}


nic::~nic()
{
    delete link_control;
    delete [] next_seq;
}

void nic::finish()
{
    std::cout << "Nic " << id << " had " << stalled_cycles << " stalled cycles." << std::endl;
}

void nic::setup() 
{
    link_control->Setup();
}

void
nic::init(unsigned int phase) {
    return link_control->init(phase);
}

class MyRtrEvent : public RtrEvent {
public:
    int seq;
    MyRtrEvent(int seq) : seq(seq)
    {}
};

bool
nic::clock_handler(Cycle_t cycle)
{
    static const int num_msg = 10;
    int expected_recv_count = (num_peers-1)*num_msg;

    if ( !done && (packets_recd >= expected_recv_count) ) {
        std::cout << cycle << ": NIC " << id << " received all packets!" << std::endl;
      primaryComponentOKToEndSim();
        done = true;
    }
    // Send packets
    if ( packets_sent < expected_recv_count ) {
        if ( link_control->spaceToSend(0,5) ) {
            last_target++;
            if ( last_target == id ) last_target++;
            last_target %= num_peers;
            if ( last_target == id ) last_target++;
            last_target %= num_peers;

            MyRtrEvent* ev = new MyRtrEvent(packets_sent/(num_peers-1));

            switch ( addressMode ) {
            case SEQUENTIAL:
                ev->dest = last_target;
                ev->src = id;
                break;
            case FATTREE_IP:
                ev->dest = fattree_ID_to_IP(last_target);
                ev->src = fattree_ID_to_IP(id);
                break;
            }

            ev->vc = 0;
            ev->size_in_flits = 5;
            bool sent = link_control->send(ev,0);
            assert( sent );
            //std::cout << cycle << ": " << id << " sent packet " << ev->seq << " to " << ev->dest << std::endl;
            packets_sent++;
            if ( packets_sent == expected_recv_count ) {
                std::cout << cycle << ":  " << id << " Finished sending packets" << std::endl;
            }
        }
        else {
            stalled_cycles++;
        }
    }

    for ( int vc = 0 ; vc < num_vcs ; vc++ ) {
        last_vc = (last_vc + 1) % num_vcs; // round-robin
        RtrEvent* rec_ev = link_control->recv(last_vc);
        MyRtrEvent* ev = dynamic_cast<MyRtrEvent*>(rec_ev);
        if ( ev == NULL && rec_ev != NULL ) {
            _abort(nic, "Aieeee!\n");
        }
        if ( ev != NULL ) {
            packets_recd++;
            int src = (addressMode == FATTREE_IP) ? IP_to_fattree_ID(ev->src) : ev->src;
#if 0
            if ( next_seq[src] != ev->seq ) {
                std::cout << id << " received packet " << ev->seq << " from " << ev->src << " Expected sequence number " << next_seq[ev->src] << std::endl;
                assert(false);
            }
#endif
            next_seq[src]++;
            //std::cout << cycle << ": " << id << " Received an event on vc " << rec_ev->vc << " from " << rec_ev->src << " (packet "<<packets_recd<<" )"<< std::endl;
            delete ev;
            break;
        }
    }

    return false;
}


int nic::fattree_ID_to_IP(int id)
{
    union Addr {
        uint8_t x[4];
        int32_t s;
    };

    Addr addr;

    int edge_switch = (id / ft_loading);
    int pod = edge_switch / (ft_radix/2);
    int subnet = edge_switch % (ft_radix/2);

    addr.x[0] = 10;
    addr.x[1] = pod;
    addr.x[2] = subnet;
    addr.x[3] = 2 + (id % ft_loading);

#if 0
    printf("Converted NIC id %d to %u.%u.%u.%u.\n", id, addr.x[0], addr.x[1], addr.x[2], addr.x[3]\n);
#endif

    return addr.s;
}


int nic::IP_to_fattree_ID(int ip)
{
    union Addr {
        uint8_t x[4];
        int32_t s;
    };

    Addr addr;
    addr.s = ip;

    int id = 0;
    id += addr.x[1] * (ft_radix/2) * ft_loading;
    id += addr.x[2] * ft_loading;
    id += addr.x[3] -2;

    return id;
}
