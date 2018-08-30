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
#include "sst/elements/merlin/test/simple_patterns/shift.h"

#include <unistd.h>

#include <sst/core/event.h>
#include <sst/core/params.h>
#include <sst/core/simulation.h>
#include <sst/core/timeLord.h>
#include <sst/core/unitAlgebra.h>

#include <sst/core/interfaces/simpleNetwork.h>

namespace SST {
using namespace SST::Interfaces;

namespace Merlin {


shift_nic::shift_nic(ComponentId_t cid, Params& params) :
    Component(cid),
    send_seq(0),
    recv_seq(0),
    num_ooo(0),
    packets_sent(0),
    packets_recd(0),
    stalled_cycles(0),
    send_done(false),
    recv_done(false),
    initialized(false),
    output(Simulation::getSimulation()->getSimulationOutput())
{
    net_id = params.find<int>("id",-1);
    if ( net_id == -1 ) {
    }

    num_peers = params.find<int>("num_peers",-1);
    if ( num_peers == -1 ) {
    }
    
    shift = params.find<int>("shift",-1);
    if ( shift == -1 ) {
        // Abort
    }

    target = (net_id + shift) % num_peers;
    
    num_msg = params.find<int>("packets_to_send",10);

    // std::string packet_size_s = params.find<std::string>("packet_size", "64B");
    // UnitAlgebra packet_size(packet_size_s);
    UnitAlgebra packet_size = params.find<UnitAlgebra>("packet_size", UnitAlgebra("64B"));
    if ( packet_size.hasUnits("B") ) {
        packet_size *= UnitAlgebra("8b/B");
    }

    size_in_bits = packet_size.getRoundedValue();
    
    std::string link_bw_s = params.find<std::string>("link_bw");
    if ( link_bw_s == "" ) {
    }
    UnitAlgebra link_bw(link_bw_s);
    

    // remap = params.find<int>("remap", 0);
    // id = (net_id + remap) % num_peers;
    
    // Create a LinkControl object
    link_control = (SimpleNetwork*)loadSubComponent("merlin.reorderlinkcontrol", this, params);
    
    UnitAlgebra buf_size("1kB");
    link_control->initialize("rtr", link_bw, 1, buf_size, buf_size);

    // Register a clock
    registerClock( "1GHz", new Clock::Handler<shift_nic>(this,&shift_nic::clock_handler), false);

    link_control->setNotifyOnReceive(new SST::Interfaces::SimpleNetwork::Handler<shift_nic>(this,&shift_nic::handle_event));
    
    registerAsPrimaryComponent();
    primaryComponentDoNotEndSim();

}



shift_nic::~shift_nic()
{
    delete link_control;
}

void shift_nic::finish()
{
    link_control->finish();
    if ( num_ooo > 0 ) output.output("Nic %d had %d out of order packets.\n",net_id,num_ooo);
}

void shift_nic::setup()
{
    link_control->setup();
    if ( link_control->getEndpointID() != net_id ) {
        output.output("NIC ids don't match: parem = %" PRIi32 ", LinkControl = %" PRIi64 "\n",net_id, link_control->getEndpointID());
        // output.output("NIC ids don't match: parem = %d, LinkControl = %" PRIi64 "\n",net_id, link_control->getEndpointID());
    }

    // net_map.bind("global");
}

void
shift_nic::init(unsigned int phase) {
    link_control->init(phase);
    // if ( link_control->isNetworkInitialized() ) {
    //     // Put my address into the network mapping
    //     //SST::Interfaces::SimpleNetwork::addMappingEntry("global", id, net_id);
    // }
}

class ShiftEvent : public Event {
public:
    int seq;
    ShiftEvent() {}
    ShiftEvent(int seq) : seq(seq)
    {}

    Event* clone(void) override
    {
        return new ShiftEvent(*this);
    }

    void serialize_order(SST::Core::Serialization::serializer &ser)  override {
        Event::serialize_order(ser);
        ser & seq;
    }

private:

    ImplementSerializable(SST::Merlin::ShiftEvent);

};




bool
shift_nic::clock_handler(Cycle_t cycle)
{
    int expected_recv_count = num_msg;

    // if ( !done && (packets_recd >= expected_recv_count) ) {
    //     output.output("%" PRIu64 ": NIC %d received all packets!\n", cycle, net_id);
    //     primaryComponentOKToEndSim();
    //     done = true;
    // }

    // Send packets
    if ( packets_sent < expected_recv_count ) {
        if ( link_control->spaceToSend(0,size_in_bits) ) {
            ShiftEvent* ev = new ShiftEvent(send_seq++);
            SimpleNetwork::Request* req = new SimpleNetwork::Request();
            
            // req->dest = net_map[last_target];
            req->dest = target;
            req->src = net_id;

            req->vn = 0;
            req->size_in_bits = size_in_bits;
            req->givePayload(ev);
            // if ( net_id == 3 ) {
            //     req->setTraceType(SST::Interfaces::SimpleNetwork::Request::FULL);
            //     req->setTraceID(net_id*1000 + packets_sent);
            // }
            
            bool sent = link_control->send(req,0);
            assert( sent );
            packets_sent++;
            if ( packets_sent == expected_recv_count ) {
                output.output("%" PRIu64 ":  %d Finished sending packets (total of %d)\n",
                              cycle, net_id, num_msg);
            }
        }
        else {
            stalled_cycles++;
        }
    }
    else {
        send_done = true;
        if ( recv_done ) {
            primaryComponentOKToEndSim();
            // output.output("%d called primaryComponentOKToEndSim()\n",
            //               net_id);
        }
        return true;
    }

    // if ( link_control->requestToReceive(0) ) {
    //     SimpleNetwork::Request* req = link_control->recv(0);
    //     ShiftEvent* ev = dynamic_cast<ShiftEvent*>(req->takePayload());
    //     if ( ev == NULL ) {
    //         Simulation::getSimulation()->getSimulationOutput().fatal(CALL_INFO, -1, "Aieeee!\n");
    //     }
    //     packets_recd++;

    //     // Check to see if packets were received out of order
    //     if ( ev->seq < recv_seq ) {
    //         num_ooo++;
    //     }
    //     else {
    //         recv_seq = ev->seq;
    //     }

    //     delete ev;
    //     delete req;
    // }
    
    return false;
}

bool
shift_nic::handle_event(int vn)
{
    while ( link_control->requestToReceive(0) ) {
        SimpleNetwork::Request* req = link_control->recv(0);
        ShiftEvent* ev = dynamic_cast<ShiftEvent*>(req->takePayload());
        if ( ev == NULL ) {
            Simulation::getSimulation()->getSimulationOutput().fatal(CALL_INFO, -1, "Aieeee!\n");
        }
        packets_recd++;

        // Check to see if packets were received out of order
        if ( ev->seq < recv_seq ) {
            num_ooo++;
        }
        else {
            recv_seq = ev->seq;
        }

        delete ev;
        delete req;

    }
    if ( packets_recd == num_msg ) {
        output.output("%d Received all packets (total of %d)\n",
                      net_id, num_msg);
        recv_done = true;
        if ( send_done ) {
            primaryComponentOKToEndSim();
            // output.output("%d called primaryComponentOKToEndSim()\n",
            //               net_id);
        }
        return false;
    }
    else {
        return true;
    }
    
}

} // namespace Merlin
} // namespace SST

