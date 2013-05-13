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
#include "sst/elements/merlin/trafficgen/trafficgen.h"

using namespace SST::Merlin;

TrafficGen::TrafficGen(ComponentId_t cid, Params& params) :
    Component(cid),
    last_vc(0),
    packets_sent(0),
    packets_recd(0),
    packetDestGen(NULL),
    packetSizeGen(NULL),
    packetDelayGen(NULL),
    packet_delay(0),
    done(false)
{
    id = params.find_integer("id");
    if ( id == -1 ) {
    }
    std::cout << "id: " << id << "\n";

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

    int buf_len = params.find_integer("buffer_length", 100);
    // NOTE:  This MUST be the same length as 'num_vcs'
    int *buf_size = new int[num_vcs];
    for ( int i = 0 ; i < num_vcs ; i++ ) {
        buf_size[i] = buf_len;
    }

    link_control = new LinkControl(this, "rtr", tc, num_vcs, buf_size, buf_size);
    delete buf_size;

    packets_to_send = params.find_integer("packets_to_send", 1000);
    base_packet_size = params.find_integer("packet_size", 5); // In Flits

    // Register a clock
    registerClock( params.find_string("message_rate", "1GHz"),
            new Clock::Handler<TrafficGen>(this,&TrafficGen::clock_handler), false);




    /* Distribution selection */
    std::string pattern = params.find_string("pattern");
    if ( !pattern.compare("NearestNeighbor") ) {
        std::string shape = params.find_string("NearestNeighbor:3DSize");
        int maxX, maxY, maxZ;
        assert (sscanf(shape.c_str(), "%d %d %d", &maxX, &maxY, &maxZ) == 3);
        packetDestGen = new NearestNeighbor(new UniformDist(0, num_peers), id, maxX, maxY, maxZ, 6);
    } else if ( !pattern.compare("Uniform") ) {
        packetDestGen = new UniformDist(0, num_peers-1);
    } else if ( !pattern.compare("HotSpot") ) {
        int target = params.find_integer("HotSpot:target");
        float targetProb = params.find_floating("HotSpot:targetProbability");
        packetDestGen = new DiscreteDist(0, num_peers, target, targetProb);
    } else {
        _abort(TrafficGen, "Unknown pattern '%s'\n", pattern.c_str());
    }





    registerAsPrimaryComponent();
    primaryComponentDoNotEndSim();

}


TrafficGen::~TrafficGen()
{
    delete link_control;
}

void TrafficGen::finish()
{
}

void TrafficGen::setup()
{
    link_control->Setup();
}

void
TrafficGen::init(unsigned int phase) {
    return link_control->init(phase);
}


bool
TrafficGen::clock_handler(Cycle_t cycle)
{
    if ( !done && (packets_sent >= packets_to_send) ) {
        printf("Node %d done sending.\n", id);
        primaryComponentOKToEndSim();
        done = true;
    }

    if ( packet_delay ) {
        --packet_delay;
    } else {
        // Send packets
        if ( packets_sent < packets_to_send ) {
            int packet_size = getPacketSize();
            if ( link_control->spaceToSend(0,packet_size) ) {
                int target = getPacketDest();


                RtrEvent* ev = new RtrEvent();

                switch ( addressMode ) {
                case SEQUENTIAL:
                    ev->dest = target;
                    ev->src = id;
                    break;
                case FATTREE_IP:
                    ev->dest = fattree_ID_to_IP(target);
                    ev->src = fattree_ID_to_IP(id);
                    break;
                }
                //ev->setTraceID((id<<16) | packets_sent);
                //ev->setTraceType(RtrEvent::FULL);
                ev->vc = 0;
                ev->size_in_flits = packet_size;

                bool sent = link_control->send(ev,0);
                assert( sent );

                //std::cout << cycle << ": " << id << " sent packet " << ev->seq << " to " << ev->dest << std::endl;
                packets_sent++;
            }
        }
    }

    for ( int vc = 0 ; vc < num_vcs ; vc++ ) {
        last_vc = (last_vc + 1) % num_vcs; // round-robin
        RtrEvent* ev = link_control->recv(last_vc);
        if ( ev != NULL ) {
            packets_recd++;
            //std::cout << cycle << ": " << id << " Received an event on vc " << rec_ev->vc << " from " << rec_ev->src << " (packet "<<packets_recd<<" )"<< std::endl;
            delete ev;
            break;
        }
    }

    return false;
}


int TrafficGen::fattree_ID_to_IP(int id)
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


int TrafficGen::IP_to_fattree_ID(int ip)
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



int TrafficGen::getPacketDest(void)
{
    return packetDestGen->getNextValue();
}


int TrafficGen::getPacketSize(void)
{
    if ( packetSizeGen ) {
        return packetSizeGen->getNextValue();
    } else {
        return base_packet_size;
    }
}


int TrafficGen::getDelayNextPacket(void)
{
    if ( packetDelayGen ) {
        return packetDelayGen->getNextValue();
    } else {
        return packet_delay;
    }
}



