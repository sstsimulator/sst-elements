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

#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS
#endif
#include <inttypes.h>
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
    done(false),
    packet_delay(0),
    packetDestGen(NULL),
    packetSizeGen(NULL),
    packetDelayGen(NULL)
{

    out.init(getName() + ": ", 0, 0, Output::STDOUT);

    id = params.find_integer("id");
    if ( id == -1 ) {
        _abort(TrafficGen, "id must be set!\n");
    }

    num_peers = params.find_integer("num_peers");
    if ( num_peers == -1 ) {
        _abort(TrafficGen, "num_peers must be set!\n");
    }

    num_vcs = params.find_integer("num_vcs");
    if ( num_vcs == -1 ) {
        _abort(TrafficGen, "num_vcs must be set!\n");
    }

    std::string link_bw = params.find_string("link_bw");
    if ( link_bw == "" ) {
        _abort(TrafficGen, "link_bw must be set!\n");
    }
    TimeConverter* tc = Simulation::getSimulation()->getTimeLord()->getTimeConverter(link_bw);

    addressMode = SEQUENTIAL;

    if ( !params.find_string("topology").compare("merlin.fattree") ) {
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

    link_control = (Merlin::LinkControl*)loadModule("merlin.linkcontrol", params);
    link_control->configureLink(this, "rtr", tc, num_vcs, buf_size, buf_size);
    delete [] buf_size;

    packets_to_send = (uint64_t)params.find_integer("packets_to_send", 1000);

    /* Distribution selection */
    packetDestGen = buildGenerator("PacketDest", params);
    assert(packetDestGen);
    packetDestGen->seed(id);


    /* Packet size */
    base_packet_size = params.find_integer("packet_size", 5); // In Flits
    packetSizeGen = buildGenerator("PacketSize", params);
    if ( packetSizeGen ) packetSizeGen->seed(id);


    base_packet_delay = params.find_integer("delay_between_packets", 0);
    packetDelayGen = buildGenerator("PacketDelay", params);
    if ( packetDelayGen ) packetDelayGen->seed(id);




    registerAsPrimaryComponent();
    primaryComponentDoNotEndSim();
    registerClock( params.find_string("message_rate", "1GHz"),
            new Clock::Handler<TrafficGen>(this,&TrafficGen::clock_handler), false);


}


TrafficGen::~TrafficGen()
{
    delete link_control;
}


TrafficGen::Generator* TrafficGen::buildGenerator(const std::string &prefix, Params &params)
{
    Generator* gen = NULL;
    std::string pattern = params.find_string(prefix + ":pattern");
    std::pair<int, int> range = std::make_pair(
            params.find_integer(prefix + ":RangeMin", 0),
            params.find_integer(prefix + ":RangeMax", INT_MAX));

    if ( !pattern.compare("NearestNeighbor") ) {
        std::string shape = params.find_string(prefix + ":NearestNeighbor:3DSize");
        int maxX, maxY, maxZ;
        assert (sscanf(shape.c_str(), "%d %d %d", &maxX, &maxY, &maxZ) == 3);
        gen = new NearestNeighbor(new UniformDist(range.first, range.second-1), id, maxX, maxY, maxZ, 6);
    } else if ( !pattern.compare("Uniform") ) {
        gen = new UniformDist(range.first, range.second-1);
    } else if ( !pattern.compare("HotSpot") ) {
        int target = params.find_integer(prefix + ":HotSpot:target");
        float targetProb = params.find_floating(prefix + ":HotSpot:targetProbability");
        gen = new DiscreteDist(range.first, range.second, target, targetProb);
    } else if ( !pattern.compare("Normal") ) {
        float mean = params.find_floating(prefix + ":Normal:Mean", range.second/2.0f);
        float sigma = params.find_floating(prefix + ":Normal:Sigma", 1.0f);
        gen = new NormalDist(range.first, range.second, mean, sigma);
    } else if ( !pattern.compare("Binomial") ) {
        int trials = params.find_floating(prefix + ":Binomial:Mean", range.second);
        float probability = params.find_floating(prefix + ":Binomial:Sigma", 0.5f);
        gen = new BinomialDist(range.first, range.second, trials, probability);
    } else if ( pattern.compare("") ) { // Allow none - non-pattern
        _abort(TrafficGen, "Unknown pattern '%s'\n", pattern.c_str());
    }
    return gen;
}

void TrafficGen::finish()
{
    out.output("%d, %"PRIu64", %"PRIu64"\n", id, packets_sent, packets_recd);
}

void TrafficGen::setup()
{
    link_control->setup();
}

void
TrafficGen::init(unsigned int phase) {
    return link_control->init(phase);
}


bool
TrafficGen::clock_handler(Cycle_t cycle)
{
    if ( !done && (packets_sent >= packets_to_send) ) {
        out.output("Node %d done sending.\n", id);
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
                ev->vc = 0;
                ev->size_in_flits = packet_size;

                bool sent = link_control->send(ev,0);
                assert( sent );

                ++packets_sent;
            }
        }
    }

    for ( int vc = 0 ; vc < num_vcs ; vc++ ) {
        last_vc = (last_vc + 1) % num_vcs; // round-robin
        RtrEvent* ev = link_control->recv(last_vc);
        if ( ev != NULL ) {
            packets_recd++;
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
    out.output("Converted NIC id %d to %u.%u.%u.%u.\n", id, addr.x[0], addr.x[1], addr.x[2], addr.x[3]\n);
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
        return base_packet_delay;
    }
}



