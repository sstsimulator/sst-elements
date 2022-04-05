// Copyright 2009-2021 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2021, NTESS
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
#include "sst/elements/merlin/test/simple_patterns/incast.h"

#include <unistd.h>

#include <sst/core/event.h>
#include <sst/core/params.h>
#include <sst/core/output.h>
#include <sst/core/timeLord.h>
#include <sst/core/unitAlgebra.h>

#include <sst/core/interfaces/simpleNetwork.h>

namespace SST {
using namespace SST::Interfaces;

namespace Merlin {


incast_nic::incast_nic(ComponentId_t cid, Params& params) :
    Component(cid),
    id(-1),
    target(false),
    curr_packets(0),
    start_time(0),
    end_time(0),
    output(getSimulationOutput())
{
    num_peers = params.find<int>("num_peers",-1);
    if ( num_peers == -1 ) {
    }

    params.find_array<int>("target_nids", targets);

    total_packets = params.find<int>("packets_to_send",10);
    if ( total_packets != -1 ) {
        registerAsPrimaryComponent();
        primaryComponentDoNotEndSim();
    }

    if ( total_packets == -1 ) {
        total_packets = std::numeric_limits<int>::max();
    }
    else {
        total_packets *= targets.size();
    }

    // std::string packet_size_s = params.find<std::string>("packet_size", "64B");
    // UnitAlgebra packet_size(packet_size_s);
    UnitAlgebra packet_size = params.find<UnitAlgebra>("packet_size", UnitAlgebra("64B"));
    if ( packet_size.hasUnits("B") ) {
        packet_size *= UnitAlgebra("8b/B");
    }

    packet_size_in_bits = packet_size.getRoundedValue();

    // Create a LinkControl object
    // First see if it is defined in the python
    link_control = loadUserSubComponent<SST::Interfaces::SimpleNetwork>
        ("networkIF", ComponentInfo::SHARE_NONE, 1 /* vns */);

    delay_start = params.find<UnitAlgebra>("delay_start", UnitAlgebra("0ns"));
}



incast_nic::~incast_nic()
{
    delete link_control;
}

void
incast_nic::init(unsigned int phase) {
    link_control->init(phase);

    if ( link_control->isNetworkInitialized() && id == -1 ) {
        id = link_control->getEndpointID();

        // See if I am a target
        for ( auto x : targets ) {
            if ( id == x ) {
                target = true;
                // Compute total packets to recv          --> because we multiplied by targets.size in const <--
                total_packets = ( num_peers - targets.size() ) * ( total_packets / targets.size() );
            }
        }

        if ( target ) {
            UnitAlgebra bw = link_control->getLinkBW();
            if ( bw.hasUnits("B/s") ) bw *= UnitAlgebra("8b/B");
            UnitAlgebra size = UnitAlgebra("1 b") * packet_size_in_bits;
            UnitAlgebra ser_time = size / bw;

            self_link = configureSelfLink("complete_link", ser_time.toString(), new Event::Handler<incast_nic>(this,&incast_nic::handle_complete));
            link_control->setNotifyOnReceive(new SST::Interfaces::SimpleNetwork::Handler<incast_nic>(this,&incast_nic::handle_event));
        }
        else {
            link_control->setNotifyOnSend(new SST::Interfaces::SimpleNetwork::Handler<incast_nic>(this,&incast_nic::handle_sends));
            self_link = configureSelfLink("start_link", delay_start.toString(), new Event::Handler<incast_nic>(this,&incast_nic::handle_start));
        }
    }
}

void incast_nic::handle_start(Event* ev)
{
    handle_sends(0);
}

void incast_nic::setup()
{
    link_control->setup();

    if ( !target ) {
        // handle_sends(0);
        self_link->send(1,nullptr);
    }
}

void incast_nic::finish()
{
    link_control->finish();

    if ( !target ) return;

    if ( end_time == 0 ) {
        UnitAlgebra final_time = getEndSimTime();
        final_time /= UnitAlgebra("1ns");
        end_time = final_time.getRoundedValue();
    }
    double total_sent = (packet_size_in_bits * curr_packets);
    double total_time = (double)end_time - (double)start_time;
    double bw = total_sent / total_time;

    output.output("%d:\n",id);
    output.output("  Total packets recieved = %d\n",curr_packets);
    output.output("  Start time = %" PRIu64 "\n",start_time);
    output.output("  End time = %" PRIu64 "\n",end_time);

    output.output("  Total sent = %lf bits\n",total_sent);

    output.output("  BW = %lf Gbits/sec\n",bw);
}

// class IncastEvent : public Event {
// public:
//     ShiftEvent() {}

//     Event* clone(void) override
//     {
//         return new ShiftEvent(*this);
//     }

//     void serialize_order(SST::Core::Serialization::serializer &ser)  override {
//         Event::serialize_order(ser);
//     }

// private:

//     ImplementSerializable(SST::Merlin::IncastEvent);

// };


bool
incast_nic::handle_event(int vn)
{
    while ( link_control->requestToReceive(0) ) {
        SimpleNetwork::Request* req = link_control->recv(0);
        curr_packets++;

        if ( curr_packets == 1 ) {
            start_time = getCurrentSimTimeNano();
        }
        delete req;

    }
    if ( curr_packets == total_packets ) {
        // Send on handle_complete
        self_link->send( 1, nullptr);
        return false;
    }

    return true;
}

void
incast_nic::handle_complete(Event* ev) {
    if ( nullptr != ev ) {
        delete ev;
    }

    end_time = getCurrentSimTimeNano();

    primaryComponentOKToEndSim();

}

bool
incast_nic::handle_sends(int vn)
{
    // Check for end case
    if ( curr_packets == total_packets ) {
        // Done
        primaryComponentOKToEndSim();
        return false;
    }

    // Just send until there is no more space
    while ( link_control->spaceToSend(0,packet_size_in_bits) ) {
        SimpleNetwork::Request* req = new SimpleNetwork::Request();
        req->dest = targets[curr_packets % targets.size()];
        req->src = id;
        req->vn = 0;
        req->size_in_bits = packet_size_in_bits;

        link_control->send(req,0);

        curr_packets++;
    }
    return true;
}


} // namespace Merlin
} // namespace SST
