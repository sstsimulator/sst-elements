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
#include "trafficgen/trafficgen.h"
#include <unistd.h>
#include <climits>
#include <signal.h>

#include <sst/core/params.h>
#include <sst/core/simulation.h>
#include <sst/core/timeLord.h>

#include "sst/elements/merlin/linkControl.h"

using namespace SST::Merlin;
using namespace SST::Interfaces;

#if ENABLE_FINISH_HACK
int TrafficGen::count = 0;
int TrafficGen::received = 0;
int TrafficGen::min_lat = 0xffffffff;
int TrafficGen::max_lat = 0;
int TrafficGen::mean_sum = 0;
#endif

TrafficGen::TrafficGen(ComponentId_t cid, Params& params) :
    Component(cid),
//    last_vc(0),
    packets_sent(0),
    packets_recd(0),
    done(false),
    packet_delay(0),
    packetDestGen(NULL),
    packetSizeGen(NULL),
    packetDelayGen(NULL)
{

    out.init(getName() + ": ", 0, 0, Output::STDOUT);
    
    id = params.find<int>("id",-1);
    if ( id == -1 ) {
        out.fatal(CALL_INFO, -1, "id must be set!\n");
    }

    num_peers = params.find<int>("num_peers",-1);
    if ( num_peers == -1 ) {
        out.fatal(CALL_INFO, -1, "num_peers must be set!\n");
    }

    // num_vns = params.find_integer("num_vns");
    // if ( num_vns == -1 ) {
    //     out.fatal(CALL_INFO, -1, "num_vns must be set!\n");
    // }
    num_vns = 1;
    
    std::string link_bw_s = params.find<std::string>("link_bw");
    if ( link_bw_s == "" ) {
        out.fatal(CALL_INFO, -1, "link_bw must be set!\n");
    }
    // TimeConverter* tc = Simulation::getSimulation()->getTimeLord()->getTimeConverter(link_bw);

    UnitAlgebra link_bw(link_bw_s);

    addressMode = SEQUENTIAL;

    // Create a LinkControl object

    std::string buf_len = params.find<std::string>("buffer_length", "1kB");
    // NOTE:  This MUST be the same length as 'num_vns'
    // int *buf_size = new int[num_vns];
    // for ( int i = 0 ; i < num_vns ; i++ ) {
    //     buf_size[i] = buf_len;
    // }

    UnitAlgebra buf_size(buf_len);
    
    link_control = (Merlin::LinkControl*)loadSubComponent("merlin.linkcontrol", this, params);
    link_control->initialize("rtr", link_bw, num_vns, buf_len, buf_len);
    // delete [] buf_size;

    packets_to_send = params.find<uint64_t>("packets_to_send", 1000);

    /* Distribution selection */
    packetDestGen = buildGenerator("PacketDest", params);
    assert(packetDestGen);
    packetDestGen->seed(id);

    /* Packet size */
    // base_packet_size = params.find_integer("packet_size", 64); // In Bits
    packetSizeGen = buildGenerator("PacketSize", params);
    if ( packetSizeGen ) packetSizeGen->seed(id);

    std::string packet_size_s = params.find<std::string>("packet_size", "8B");
    UnitAlgebra packet_size(packet_size_s);
    if ( packet_size.hasUnits("B") ) {
        packet_size *= UnitAlgebra("8b/B");
    }

    if ( !packet_size.hasUnits("b") ) {
        out.fatal(CALL_INFO, -1, "packet_size must be specified in units of either B or b!\n");
    }

    base_packet_size = packet_size.getRoundedValue();
    
    
    // base_packet_delay = params.find_integer("delay_between_packets", 0);
    packetDelayGen = buildGenerator("PacketDelay", params);
    if ( packetDelayGen ) packetDelayGen->seed(id);

    std::string packet_delay_s = params.find<std::string>("delay_between_packets", "0s");
    UnitAlgebra packet_delay(packet_delay_s);

    if ( !packet_delay.hasUnits("s") ) {
        out.fatal(CALL_INFO, -1, "packet_delay must be specified in units of s!\n");
    }

    base_packet_delay = packet_delay.getRoundedValue();

    registerAsPrimaryComponent();
    primaryComponentDoNotEndSim();
    clock_functor = new Clock::Handler<TrafficGen>(this,&TrafficGen::clock_handler);
    clock_tc = registerClock( params.find<std::string>("message_rate", "1GHz"), clock_functor, false);

    // Register a receive handler which will simply strip the events as they arrive
    link_control->setNotifyOnReceive(new LinkControl::Handler<TrafficGen>(this,&TrafficGen::handle_receives));
    send_notify_functor = new LinkControl::Handler<TrafficGen>(this,&TrafficGen::send_notify);
}


TrafficGen::~TrafficGen()
{
    delete link_control;
}


TrafficGen::Generator* TrafficGen::buildGenerator(const std::string &prefix, Params &params)
{
    Generator* gen = NULL;
    std::string pattern = params.find<std::string>(prefix + ":pattern");
    std::pair<int, int> range = std::make_pair(
        params.find<int>(prefix + ":RangeMin", 0),
        params.find<int>(prefix + ":RangeMax", INT_MAX));

    uint32_t rng_seed = params.find<uint32_t>(prefix + ":Seed", 1010101);

    if ( !pattern.compare("NearestNeighbor") ) {
        std::string shape = params.find<std::string>(prefix + ":NearestNeighbor:3DSize");
        int maxX, maxY, maxZ;
        assert (sscanf(shape.c_str(), "%d %d %d", &maxX, &maxY, &maxZ) == 3);
        gen = new NearestNeighbor(new UniformDist(0, 5), id, maxX, maxY, maxZ, 6);
    } else if ( !pattern.compare("Uniform") ) {
        gen = new UniformDist(range.first, range.second-1);
    } else if ( !pattern.compare("HotSpot") ) {
        int target = params.find<int>(prefix + ":HotSpot:target");
        float targetProb = params.find<float>(prefix + ":HotSpot:targetProbability");
        gen = new DiscreteDist(range.first, range.second, target, targetProb);
    } else if ( !pattern.compare("Normal") ) {
        float mean = params.find<float>(prefix + ":Normal:Mean", range.second/2.0f);
        float sigma = params.find<float>(prefix + ":Normal:Sigma", 1.0f);
        gen = new NormalDist(range.first, range.second, mean, sigma);
    } else if ( !pattern.compare("Exponential") ) {
        float lambda = params.find<float>(prefix + ":Exponential:Lambda", range.first);
        gen = new ExponentialDist(lambda);
    } else if ( !pattern.compare("Binomial") ) {
        int trials = params.find<int>(prefix + ":Binomial:Mean", range.second);
        float probability = params.find<float>(prefix + ":Binomial:Sigma", 0.5f);
        gen = new BinomialDist(range.first, range.second, trials, probability);
    } else if ( pattern.compare("") ) { // Allow none - non-pattern
        out.fatal(CALL_INFO, -1, "Unknown pattern '%s'\n", pattern.c_str());
    }

    if ( gen ) gen->seed(rng_seed);

    return gen;
}

void TrafficGen::finish()
{
    link_control->finish();
}

void TrafficGen::setup()
{
    link_control->setup();
#if ENABLE_FINISH_HACK
    count++;
#endif
}

void
TrafficGen::init(unsigned int phase) {
    return link_control->init(phase);
}


bool
TrafficGen::clock_handler(Cycle_t cycle)
{
    if ( done ) return true;
    else if (packets_sent >= packets_to_send) {
        // out.output("Node %d done sending.\n", id);
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


                SimpleNetwork::Request* req = new SimpleNetwork::Request();
                // req->givePayload(NULL);
                req->head = true;
                req->tail = true;
                
                switch ( addressMode ) {
                case SEQUENTIAL:
                    req->dest = target;
                    req->src = id;
                    break;
                case FATTREE_IP:
                    req->dest = fattree_ID_to_IP(target);
                    req->src = fattree_ID_to_IP(id);
                    break;
                }
                req->vn = 0;
                // ev->size_in_flits = packet_size;
                req->size_in_bits = packet_size;

                bool sent = link_control->send(req,0);
                assert( sent );

                ++packets_sent;
            }
            else {
                link_control->setNotifyOnSend(send_notify_functor);
                return true;
            }
        }
        packet_delay = getDelayNextPacket();
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

bool
TrafficGen::handle_receives(int vn)
{
    SimpleNetwork::Request* req = link_control->recv(vn);
    if ( req != NULL ) {
        packets_recd++;
        delete req;
    }
    return true;
}


bool
TrafficGen::send_notify(int vn)
{
    reregisterClock(clock_tc, clock_functor);
    return false;
}


int TrafficGen::getPacketDest(void)
{
    int dest = packetDestGen->getNextValue();
    assert ( dest >= 0 );
    return dest;
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



