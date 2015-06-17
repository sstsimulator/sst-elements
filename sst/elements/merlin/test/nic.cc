// Copyright 2009-2015 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
// 
// Copyright (c) 2009-2015, Sandia Corporation
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

#include <sst/core/element.h>
#include <sst/core/event.h>
#include <sst/core/params.h>
#include <sst/core/simulation.h>
#include <sst/core/timeLord.h>
#include <sst/core/unitAlgebra.h>

//#include "sst/elements/merlin/linkControl.h"
#include <sst/core/interfaces/simpleNetwork.h>

namespace SST {
using namespace SST::Interfaces;

namespace Merlin {


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
    
    num_msg = params.find_integer("num_messages",10);
    
    // Create a LinkControl object
    // NOTE:  This MUST be the same length as 'num_vns'
    link_control = (SimpleNetwork*)loadSubComponent("merlin.linkcontrol", this, params);
    
    UnitAlgebra buf_size("1kB");
    link_control->initialize("rtr", link_bw, num_vns, buf_size, buf_size);

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
        std::cout << "NIC ids don't match: param = " << id << ", LinkControl = "
                          << link_control->getEndpointID() << std::endl;
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
            
            SimpleNetwork::Request* req =
                new SimpleNetwork::Request(SimpleNetwork::INIT_BROADCAST_ADDR, id,
                                           0, true, true);
            link_control->sendInitData(req);
        }
    }
    else {
        SimpleNetwork::Request* req = link_control->recvInitData();
        if ( req != NULL ) {
            // std::cout << "NIC " << id << " Received an init event in phase " << phase << "!" << std::endl;
            delete req;
            initialized = true;
        }
    }
}

class MyRtrEvent : public Event {
public:
    int seq;
    MyRtrEvent(int seq) : seq(seq)
    {}

    Event* clone(void)
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
		ar & BOOST_SERIALIZATION_BASE_OBJECT_NVP(Event);
		ar & BOOST_SERIALIZATION_NVP(seq);
    }
};




bool
nic::clock_handler(Cycle_t cycle)
{
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
            SimpleNetwork::Request* req = new SimpleNetwork::Request();
            
            req->dest = last_target;
            req->src = id;

            req->vn = 0;
            req->size_in_bits = size_in_bits;
            req->givePayload(ev);
            // if ( id == 0 ) {
            //     ev->setTraceType(RtrEvent::FULL);
            //     ev->setTraceID(packets_sent);
            // }
            
            bool sent = link_control->send(req,send_vc);
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
        if ( link_control->requestToReceive(last_vn) ) {
            SimpleNetwork::Request* req = link_control->recv(last_vn);
            MyRtrEvent* ev = dynamic_cast<MyRtrEvent*>(req->takePayload());
            if ( ev == NULL ) {
                Simulation::getSimulation()->getSimulationOutput().fatal(CALL_INFO, -1, "Aieeee!\n");
            }
            // std::cout << id << " received a packet on VN" << last_vn << " from " << req->src << std::endl;
            packets_recd++;
            int src = req->src;
#if 0
            if ( next_seq[src] != ev->seq ) {
                std::cout << id << " received packet " << ev->seq << " from " << ev->src << " Expected sequence number " << next_seq[ev->src] << std::endl;
                assert(false);
            }
#endif
            next_seq[src]++;
            //std::cout << cycle << ": " << id << " Received an event on vn " << rec_ev->vn << " from " << rec_ev->src << " (packet "<<packets_recd<<" )"<< std::endl;
            delete ev;
            delete req;
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

} // namespace Merlin
} // namespace SST

BOOST_CLASS_EXPORT(SST::Merlin::MyRtrEvent)
