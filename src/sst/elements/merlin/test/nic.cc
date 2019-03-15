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
#include "sst/elements/merlin/test/nic.h"

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


nic::nic(ComponentId_t cid, Params& params) :
    Component(cid),
    last_vn(0),
    packets_sent(0),
    packets_recd(0),
    stalled_cycles(0),
    expected_recv_count(0),
    done(false),
    initialized(false),
    init_state(0),
    init_count(0),
    init_broadcast_count(0),
    output(Simulation::getSimulation()->getSimulationOutput())
{
    net_id = params.find<int>("id",-1);
    if ( net_id == -1 ) {
    }
    // std::cout << "id: " << id << "\n";
    // std::cout << "Nic ID:  " << id << " has Component id " << cid << "\n";

    num_peers = params.find<int>("num_peers",-1);
    if ( num_peers == -1 ) {
    }
    // std::cout << "num_peers: " << num_peers << "\n";
    
    num_vns = 1;
    
    std::string link_bw_s = params.find<std::string>("link_bw");
    if ( link_bw_s == "" ) {
    }
    // std::cout << "link_bw: " << link_bw_s << std::endl;
    // TimeConverter* tc = Simulation::getSimulation()->getTimeLord()->getTimeConverter(link_bw);
    UnitAlgebra link_bw(link_bw_s);
    
    num_msg = params.find<int>("num_messages",10);
    
    remap = params.find<int>("remap", 0);
    id = (net_id + remap) % num_peers;

    UnitAlgebra message_size = params.find<std::string>("message_size","64b");
    if ( message_size.hasUnits("B") ) message_size  *= UnitAlgebra("8b/B");
    msg_size = message_size.getRoundedValue();

    std::string linkcontrol_type = params.find<std::string>("linkcontrol_type","merlin.linkcontrol");
    // Create a LinkControl object
    // NOTE:  This MUST be the same length as 'num_vns'
    link_control = (SimpleNetwork*)loadSubComponent(linkcontrol_type, this, params);
    
    UnitAlgebra in_buf_size = params.find<UnitAlgebra>("in_buf_size","1kB");
    UnitAlgebra out_buf_size = params.find<UnitAlgebra>("out_buf_size","1kB");

    
    link_control->initialize("rtr", link_bw, num_vns, in_buf_size, out_buf_size);

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
    if ( init_count != num_peers ) {
        output.output("NIC %d didn't receive all complete point-to-point messages.  Only recieved %d\n",net_id,init_count);
    }

    if ( init_broadcast_count != (num_peers -1 ) ) {
        output.output("NIC %d didn't receive all complete broadcast messages.  Only recieved %d\n",net_id,init_broadcast_count);
    }

    link_control->finish();
    output.output("Nic %d had %d stalled cycles.\n",id,stalled_cycles);
}

void nic::setup()
{
    link_control->setup();
    if ( link_control->getEndpointID() != net_id ) {
        output.output("NIC ids don't match: param = %" PRIi64 ", LinkControl = %" PRIi64 "\n", 
		(int64_t) net_id, (int64_t) link_control->getEndpointID());
    }

    if ( init_count != num_peers ) {
        output.output("NIC %d didn't receive all init point-to-point messages.  Only recieved %d\n",net_id,init_count);
    }

    if ( init_broadcast_count != (num_peers -1 ) ) {
        output.output("NIC %d didn't receive all init broadcast messages.  Only recieved %d\n",net_id,init_broadcast_count);
    }
}

void nic::complete(unsigned int phase) {
    link_control->complete(phase);
    
    if ( phase == 0 ) {
        init_count = 0;
        init_broadcast_count = 0;
        init_state = 0;
    }

    init_complete(phase);
}

/*
  During init, each endpoint will send a broadcast and a point to
  point to every other endpoint.  To control the amount of memory
  used, we won't send them all at once.  To control the broadcasts,
  each rank will receive the number of broadcasts equal to its net_id
  before broadcasthing.

*/

void
nic::init(unsigned int phase) {
    link_control->init(phase);

    init_complete(phase);
}

void
nic::init_complete(unsigned int phase) {
    
    switch ( init_state ) {
    case 0:
    {
        // Wait until network is initialized
        if ( !link_control->isNetworkInitialized() ) break;
        net_id = link_control->getEndpointID();
        id = net_id;
        last_target = net_id;
        // output.output("%s: net_id = %d\n",getName().c_str(),net_id);

        if ( net_id == 0 ) {
            // output.output("Rank %d sending broadcast message\n",net_id);
            SimpleNetwork::Request* req =
                new SimpleNetwork::Request(SimpleNetwork::INIT_BROADCAST_ADDR, net_id,
                                           0, true, true);
            link_control->sendUntimedData(req);
            init_state = 2;
        }
        else {
            init_state = 1;
        }
        break;
    }
    case 1:
    {
        // SimpleNetwork::Request* req = link_control->recvInitData();
        SimpleNetwork::Request* req;
        while ( (req = link_control->recvInitData() ) != NULL ) {
            // std::cout << "NIC " << id << " Received an init event from " << req->src << " in phase " << phase << "!" << std::endl;
            delete req;
            init_broadcast_count++;
        }

        if ( init_broadcast_count >= net_id ) {
            // output.output("Rank %d sending broadcast message\n",net_id);
            SimpleNetwork::Request* req =
                new SimpleNetwork::Request(SimpleNetwork::INIT_BROADCAST_ADDR, net_id,
                                           0, true, true);
            link_control->sendUntimedData(req);
            init_state = 2;
        }
        break;
    }
    case 2:
    {   
        SimpleNetwork::Request* req;
        while ( (req = link_control->recvInitData() ) != NULL ) {
            // std::cout << "NIC " << id << " Received an init event from " << req->src << " in phase " << phase << "!" << std::endl;

            // It's possible some of the point to point will overlap
            // some of the broadcasts, so we need to check to see
            // which this is
            if ( req->dest == SimpleNetwork::INIT_BROADCAST_ADDR ) {
                init_broadcast_count++;
            }
            else {
                if ( req->dest != net_id ) output.output("%d: received event with dest %lld and src %lld\n",net_id,req->dest,req->src);
                init_count++;
            }
            delete req;
        }

        if ( init_broadcast_count == num_peers - 1 ) {
            if ( net_id == 0 ) {
                // Send a point to point message to all endpoints,
                // including myself
                // output.output("Rank %d sending point-to-point messages\n",net_id);
                for ( int i = 0; i < num_peers; ++i ) {
                    req = new SimpleNetwork::Request(i, net_id, 0, true, true);
                    link_control->sendUntimedData(req);
                }
                init_state = 4;
            }
            else {
                init_state = 3;
            }
        }        
        break;
    }
    case 3:
    {
        SimpleNetwork::Request* req;
        while ( (req = link_control->recvInitData() ) != NULL ) {
            // std::cout << "NIC " << id << " Received an init event in phase " << phase << "!" << std::endl;
            // Only point to points could arrive now
            // if ( req->dest != net_id ) output.output("%d: received event with dest %lld and src %lld\n",net_id,req->dest,req->src);
            init_count++;
            delete req;
        }

        if ( init_count == net_id ) {
            // output.output("Rank %d sending point-to-point messages\n",net_id);
            for ( int i = 0; i < num_peers; ++i ) {
                req = new SimpleNetwork::Request(i, net_id, 0, true, true);
                link_control->sendUntimedData(req);
            }
            init_state = 4;
        }        
        break;
    }
    case 4:
    {
        SimpleNetwork::Request* req;
        while ( (req = link_control->recvInitData() ) != NULL ) {
            // std::cout << "NIC " << id << " Received an init event in phase " << phase << "!" << std::endl;

            // It's possible some of the point to point will overlap
            // some of the broadcasts, so we need to check to see
            // which this is
            init_count++;
            delete req;
        }

        if ( init_count == num_peers ) {
            init_state = 5;
        }        
        
        break;
    }
    default:
        break;
    }
}

class MyRtrEvent : public Event {
public:
    int seq;
    MyRtrEvent() {}
    MyRtrEvent(int seq) : seq(seq)
    {}

    Event* clone(void) override
    {
        return new MyRtrEvent(*this);
    }

    void serialize_order(SST::Core::Serialization::serializer &ser)  override {
        Event::serialize_order(ser);
        ser & seq;
    }

    virtual void print(const std::string& header, Output &out) const  override {
        out.output("%s MyRtrEvent to be delivered at %" PRIu64 " with priority %d.  seq = %d\n",
                   header.c_str(), getDeliveryTime(), getPriority(), seq);
    }
    

private:

    ImplementSerializable(SST::Merlin::MyRtrEvent);
};


// bool
// nic::clock_handler(Cycle_t cycle)
// {
//     static const int send_vc = 0;
//     static const int size_in_bits = 64;
//     int expected_recv_count = (num_peers-1)*num_msg;

//     if ( !done && (packets_recd >= expected_recv_count) ) {
//         output.output("%" PRIu64 ": NIC %d received all packets (total of %d)!\n", cycle, id, expected_recv_count);
//         primaryComponentOKToEndSim();
//         done = true;
//     }
//     // Send packets
//     if ( packets_sent < expected_recv_count ) {
//         if ( link_control->spaceToSend(send_vc,size_in_bits) ) {
//             // std::cout << id << " sending packet number: " << packets_sent << std::endl;
//             last_target++;
//             if ( last_target == id ) last_target++;
//             last_target %= num_peers;
//             if ( last_target == id ) last_target++;
//             last_target %= num_peers;
            
//             MyRtrEvent* ev = new MyRtrEvent(packets_sent/(num_peers-1));
//             SimpleNetwork::Request* req = new SimpleNetwork::Request();
            
//             // req->dest = net_map[last_target];
//             req->dest = last_target;
//             req->src = net_id;

//             req->vn = 0;
//             req->size_in_bits = size_in_bits;
//             req->givePayload(ev);
//             // if ( net_id == 319 && last_target == 320 ) {
//             //     req->setTraceType(SST::Interfaces::SimpleNetwork::Request::FULL);
//             //     req->setTraceID(net_id*1000 + packets_sent);
//             // }

//             if ( req->dest == num_peers - 1 ) {
//                 bool sent = link_control->send(req,send_vc);
//                 assert( sent );
//             }
//             else {
//                 delete req;
//             }
//             //std::cout << cycle << ": " << id << " sent packet " << ev->seq << " to " << ev->dest << std::endl;
//             packets_sent++;
//             if ( packets_sent == expected_recv_count ) {
//                 output.output("%" PRIu64 ":  %d Finished sending packets (total of %d)\n",
//                               cycle, id, num_msg);
//             }
//         }
//         else {
//             stalled_cycles++;
//         }
//     }

//     for ( int vn = 0 ; vn < num_vns ; vn++ ) {
//         last_vn = (last_vn + 1) % num_vns; // round-robin
//         if ( link_control->requestToReceive(last_vn) ) {
//             SimpleNetwork::Request* req = link_control->recv(last_vn);
//             MyRtrEvent* ev = dynamic_cast<MyRtrEvent*>(req->takePayload());
//             if ( ev == NULL ) {
//                 Simulation::getSimulation()->getSimulationOutput().fatal(CALL_INFO, -1, "Aieeee!\n");
//             }
//             packets_recd++;
//             // int src = net_map[req->src];
//             int src = req->src;

//             if ( req->dest != net_id ) {
//                 output.fatal(CALL_INFO,-1,"%d received packet intended for %d\n",net_id,(int)req->dest);
//             }
//             else {
//                 output.output("--->%d received packet from %d\n",net_id,(int)req->src);
//             }

            
// #if 0
//             if ( next_seq[src] != ev->seq ) {
//                 output.output("%d received packet %d from %d Expected sequence number %d\n",
//                               id, ev->seq, ev->src, next_seq[ev->src]);
//                 assert(false);
//             }
// #endif
//             next_seq[src]++;
//             //std::cout << cycle << ": " << id << " Received an event on vn " << rec_ev->vn << " from " << rec_ev->src << " (packet "<<packets_recd<<" )"<< std::endl;
//             delete ev;
//             delete req;
//             break;
//         }
//     }

//     return false;
// }


bool
nic::clock_handler(Cycle_t cycle)
{
    static const int send_vc = 0;
    expected_recv_count = num_peers*num_msg;

    if ( !done && (packets_recd >= expected_recv_count) ) {
        output.output("%" PRIu64 ": NIC %d received all packets (total of %d)!\n", cycle, id, expected_recv_count);
        primaryComponentOKToEndSim();
        done = true;
    }

    // Send packets
    if ( packets_sent < expected_recv_count ) {
        if ( link_control->spaceToSend(send_vc,msg_size) ) {
            last_target++;
            last_target %= num_peers;
            
            MyRtrEvent* ev = new MyRtrEvent(packets_sent/num_peers);
            SimpleNetwork::Request* req = new SimpleNetwork::Request();
            
            // req->dest = net_map[last_target];
            req->dest = last_target;
            req->src = net_id;

            req->vn = send_vc;
            req->size_in_bits = msg_size;
            req->givePayload(ev);

            link_control->send(req,send_vc);
            // output.output("(%lld) %d: sent packet to %d\n",getCurrentSimTimeNano(),net_id,last_target);

            packets_sent++;


            if ( packets_sent == expected_recv_count ) {
                output.output("%" PRIu64 ":  %d Finished sending packets (total of %d)\n",
                              cycle, id, num_msg);
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
            packets_recd++;
            // int src = net_map[req->src];
            int src = req->src;

            if ( req->dest != net_id ) {
                output.fatal(CALL_INFO,-1,"%d received packet intended for %d\n",net_id,(int)req->dest);
            }

            
#if 0
            if ( next_seq[src] != ev->seq ) {
                output.output("%d received packet %d from %d Expected sequence number %d\n",
                              id, ev->seq, ev->src, next_seq[ev->src]);
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



} // namespace Merlin
} // namespace SST

