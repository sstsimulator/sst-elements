// Copyright 2009-2014 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2014, Sandia Corporation
// All rights reserved.
// 
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.
#include <sst_config.h>
#include "sst/core/serialization.h"
#include "sst/elements/merlin/test/nic.h"

#include <unistd.h>
#include <signal.h>

#include <sst/core/debug.h>
#include <sst/core/element.h>
#include <sst/core/params.h>
#include <sst/core/simulation.h>
#include <sst/core/timeLord.h>
#include <sst/core/unitAlgebra.h>

#include "sst/elements/merlin/linkControl.h"

using namespace SST::Merlin;

nic::nic(ComponentId_t cid, Params& params) :
    Component(cid),
    last_vn(0),
    packets_sent(0),
    packets_recd(0),
    stalled_cycles(0),
    done(false),
    initialized(false)
{
    id = params.find_integer("id");
    if ( id == -1 ) {
    }
    // std::cout << "id: " << id << "\n";
    // std::cout << "Nic ID:  " << id << " has Component id " << cid << "\n";

    num_peers = params.find_integer("num_peers");
    if ( num_peers == -1 ) {
    }
    // std::cout << "num_peers: " << num_peers << "\n";

    // num_vns = params.find_integer("num_vns");
    // if ( num_vns == -1 ) {
    // }
    num_vns = 1;
    // std::cout << "num_vns: " << num_vns << "\n";
    
    std::string link_bw_s = params.find_string("link_bw");
    if ( link_bw_s == "" ) {
    }
    // std::cout << "link_bw: " << link_bw_s << std::endl;
    // TimeConverter* tc = Simulation::getSimulation()->getTimeLord()->getTimeConverter(link_bw);
    UnitAlgebra link_bw(link_bw_s);
    
    addressMode = SEQUENTIAL;

    // Create a LinkControl object
    // NOTE:  This MUST be the same length as 'num_vns'
    link_control = (Merlin::LinkControl*)loadModule("merlin.linkcontrol", params);

    UnitAlgebra buf_size("1kB");
    link_control->configureLink(this, "rtr", link_bw, num_vns, buf_size, buf_size);

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
    link_control->finish();
    std::cout << "Nic " << id << " had " << stalled_cycles << " stalled cycles." << std::endl;
}

void nic::setup()
{
    link_control->setup();
    if ( link_control->getEndpointID() != id ) {
        if ( addressMode == FATTREE_IP ) {
            if ( IP_to_fattree_ID(link_control->getEndpointID()) != id ) {
                std::cout << "NIC ids don't match: param = " << id << ", LinkControl = "
                          << link_control->getEndpointID() << std::endl;
            }
        }
    }
    if ( !initialized ) {
        std::cout << "Nic " << id << ": Broadcast failed!" << std::endl;  
    }
}

void
nic::init(unsigned int phase) {
    link_control->init(phase);
    if ( id == 0 && !initialized ) {
        if ( link_control->isNetworkInitialized() ) {
            initialized = true;
            
            RtrEvent *re = new RtrEvent();
            re->src = id;
            re->dest = INIT_BROADCAST_ADDR;

            link_control->sendInitData(re);
        }
    }
    else {
        Event*ev = link_control->recvInitData();
        if ( ev != NULL ) {
            // std::cout << "NIC " << id << " Received an init event in phase " << phase << "!" << std::endl;
            delete ev;
            initialized = true;
        }
    }
}

class MyRtrEvent : public RtrEvent {
public:
    int seq;
    MyRtrEvent(int seq) : seq(seq)
    {}
    virtual RtrEvent* clone(void)
    {
        return new MyRtrEvent(*this);
    }
private:
    MyRtrEvent() {}

	friend class boost::serialization::access;
	template<class Archive>
	void
	serialize(Archive & ar, const unsigned int version )
	{
		ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(RtrEvent);
		ar & BOOST_SERIALIZATION_NVP(seq);
    }
};


BOOST_CLASS_EXPORT(MyRtrEvent)


bool
nic::clock_handler(Cycle_t cycle)
{
    static const int num_msg = 100;
    static const int send_vc = 0;
    static const int size_in_bits = 400;
    int expected_recv_count = (num_peers-1)*num_msg;

    if ( !done && (packets_recd >= expected_recv_count) ) {
        std::cout << cycle << ": NIC " << id << " received all packets!" << std::endl;
        primaryComponentOKToEndSim();
        done = true;
    }
    // Send packets
    if ( packets_sent < expected_recv_count ) {
        if ( link_control->spaceToSend(send_vc,size_in_bits) ) {
            // std::cout << id << " sending packet number: " << packets_sent << std::endl;
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

            ev->vn = 0;
            ev->size_in_bits = size_in_bits;
            // if ( id == 0 ) {
            //     ev->setTraceType(RtrEvent::FULL);
            //     ev->setTraceID(packets_sent);
            // }
            
            bool sent = link_control->send(ev,send_vc);
            assert( sent );
            //std::cout << cycle << ": " << id << " sent packet " << ev->seq << " to " << ev->dest << std::endl;
            packets_sent++;
            if ( packets_sent == expected_recv_count ) {
                std::cout << cycle << ":  " << id << " Finished sending packets (total of " <<
                    num_msg << ")" << std::endl;
            }
        }
        else {
            stalled_cycles++;
        }
    }

    for ( int vn = 0 ; vn < num_vns ; vn++ ) {
        last_vn = (last_vn + 1) % num_vns; // round-robin
        RtrEvent* rec_ev = link_control->recv(last_vn);
        MyRtrEvent* ev = dynamic_cast<MyRtrEvent*>(rec_ev);
        if ( ev == NULL && rec_ev != NULL ) {
            _abort(nic, "Aieeee!\n");
        }
        if ( ev != NULL ) {
            // std::cout << id << " received a packet on VN" << last_vn << std::endl;
            packets_recd++;
            int src = (addressMode == FATTREE_IP) ? IP_to_fattree_ID(ev->src) : ev->src;
#if 0
            if ( next_seq[src] != ev->seq ) {
                std::cout << id << " received packet " << ev->seq << " from " << ev->src << " Expected sequence number " << next_seq[ev->src] << std::endl;
                assert(false);
            }
#endif
            next_seq[src]++;
            //std::cout << cycle << ": " << id << " Received an event on vn " << rec_ev->vn << " from " << rec_ev->src << " (packet "<<packets_recd<<" )"<< std::endl;
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
