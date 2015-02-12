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
#include <sst/core/serialization.h>
#include "trafficgen/trafficgen.h"
#include <unistd.h>
#include <signal.h>

#include <sst/core/element.h>
#include <sst/core/params.h>
#include <sst/core/simulation.h>
#include <sst/core/timeLord.h>

#include "sst/elements/merlin/linkControl.h"

#ifdef HAVE_MPI
#include <boost/mpi.hpp>
#endif


using namespace SST::Merlin;

#if ENABLE_FINISH_HACK
int TrafficGen::count = 0;
int TrafficGen::received = 0;
int TrafficGen::min_lat = 0xffffffff;
int TrafficGen::max_lat = 0;
int TrafficGen::mean_sum = 0;
#endif

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

    // num_vns = params.find_integer("num_vns");
    // if ( num_vns == -1 ) {
    //     _abort(TrafficGen, "num_vns must be set!\n");
    // }
    num_vns = 1;
    
    std::string link_bw_s = params.find_string("link_bw");
    if ( link_bw_s == "" ) {
        _abort(TrafficGen, "link_bw must be set!\n");
    }
    // TimeConverter* tc = Simulation::getSimulation()->getTimeLord()->getTimeConverter(link_bw);

    UnitAlgebra link_bw(link_bw_s);

    addressMode = SEQUENTIAL;

    // if ( !params.find_string("topology").compare("merlin.fattree") ) {
    //     addressMode = FATTREE_IP;
    //     ft_loading = params.find_integer("fattree:loading");
    //     ft_radix = params.find_integer("fattree:radix");
    // }

    // Create a LinkControl object

    std::string buf_len = params.find_string("buffer_length", "1kB");
    // NOTE:  This MUST be the same length as 'num_vns'
    // int *buf_size = new int[num_vns];
    // for ( int i = 0 ; i < num_vns ; i++ ) {
    //     buf_size[i] = buf_len;
    // }

    UnitAlgebra buf_size(buf_len);
    
    link_control = (Merlin::LinkControl*)loadModule("merlin.linkcontrol", params);
    link_control->configureLink(this, "rtr", link_bw, num_vns, buf_len, buf_len, true);
    // delete [] buf_size;

    packets_to_send = (uint64_t)params.find_integer("packets_to_send", 1000);

    /* Distribution selection */
    packetDestGen = buildGenerator("PacketDest", params);
    assert(packetDestGen);
    packetDestGen->seed(id);


    /* Packet size */
    // base_packet_size = params.find_integer("packet_size", 64); // In Bits
    packetSizeGen = buildGenerator("PacketSize", params);
    if ( packetSizeGen ) packetSizeGen->seed(id);

    std::string packet_size_s = params.find_string("packet_size", "8B");
    UnitAlgebra packet_size(packet_size_s);
    if ( packet_size.hasUnits("B") ) {
        packet_size *= UnitAlgebra("8b/B");
    }

    if ( !packet_size.hasUnits("b") ) {
        _abort(TrafficGen, "packet_size must be specified in units of either B or b!\n");
    }

    base_packet_size = packet_size.getRoundedValue();
    
    
    // base_packet_delay = params.find_integer("delay_between_packets", 0);
    packetDelayGen = buildGenerator("PacketDelay", params);
    if ( packetDelayGen ) packetDelayGen->seed(id);

    std::string packet_delay_s = params.find_string("delay_between_packets", "0s");
    UnitAlgebra packet_delay(packet_delay_s);

    if ( !packet_delay.hasUnits("s") ) {
        _abort(TrafficGen, "packet_delay must be specified in units of s!\n");
    }

    base_packet_delay = packet_delay.getRoundedValue();

    registerAsPrimaryComponent();
    primaryComponentDoNotEndSim();
    clock_functor = new Clock::Handler<TrafficGen>(this,&TrafficGen::clock_handler);
    clock_tc = registerClock( params.find_string("message_rate", "1GHz"), clock_functor, false);

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
    std::string pattern = params.find_string(prefix + ":pattern");
    std::pair<int, int> range = std::make_pair(
            params.find_integer(prefix + ":RangeMin", 0),
            params.find_integer(prefix + ":RangeMax", INT_MAX));

    if ( !pattern.compare("NearestNeighbor") ) {
        std::string shape = params.find_string(prefix + ":NearestNeighbor:3DSize");
        int maxX, maxY, maxZ;
        assert (sscanf(shape.c_str(), "%d %d %d", &maxX, &maxY, &maxZ) == 3);
        gen = new NearestNeighbor(new UniformDist(0, 5), id, maxX, maxY, maxZ, 6);
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
    link_control->finish();
    const LinkControl::PacketStats &stats = link_control->getPacketStats();

    if ( 0 == id ) {
        out.output("id,#Sent,#Recv,#NIC_Recv,MinLat,MaxLat,AvgLat,StdDevLat\n");
    }

    out.output("%d,%" PRIu64 ",%" PRIu64 ",%" PRIu64 ",%" PRIu64 ",%" PRIu64 ",%lg,%lg\n",
            id, packets_sent, packets_recd, stats.getNumPkts(),
            stats.getMinLatency(), stats.getMaxLatency(),
            stats.getMeanLatency(), stats.getStdDevLatency());

#if ENABLE_FINISH_HACK
    received += packets_recd;
    if ( stats.getMinLatency() < min_lat ) min_lat = stats.getMinLatency();
    if ( stats.getMaxLatency() > max_lat ) max_lat = stats.getMaxLatency();
    mean_sum += (stats.getMeanLatency() * stats.getNumPkts());
    count--;

    if ( count == 0 ) {
        int output_recv = received;
        uint32_t output_min = min_lat;
        uint32_t output_max = max_lat;
        uint32_t output_mean_sum = mean_sum;
#if HAVE_MPI
        boost::mpi::communicator world;
        int input_recv = received;
        uint32_t input_min = min_lat;
        uint32_t input_max = max_lat;
        uint32_t input_mean_sum = mean_sum;
        all_reduce( world, &input_recv, 1, &output_recv, std::plus<int>() );        
        all_reduce( world, &input_min, 1, &output_min, boost::mpi::minimum<uint32_t>() );        
        all_reduce( world, &input_max, 1, &output_max, boost::mpi::maximum<uint32_t>() );        
        all_reduce( world, &input_mean_sum, 1, &output_mean_sum, std::plus<uint32_t>() );        
#endif
        
        if ( Simulation::getSimulation()->getRank() == 0 ) {
            out.output("Total packets received: %d\n",output_recv); 
            SimTime_t finish = getCurrentSimTimeNano();
            out.output("Rate: %lu Billion messages per second\n", (output_recv / finish));

            out.output("\nMin latency: %u ns\n",output_min);
            out.output("Max latency: %u ns\n",output_max);
            out.output("Mean latency: %u ns\n",output_mean_sum / output_recv);

        }
    }
#endif
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
                ev->vn = 0;
                // ev->size_in_flits = packet_size;
                ev->size_in_bits = packet_size;

                bool sent = link_control->send(ev,0);
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
    RtrEvent* ev = link_control->recv(vn);
    if ( ev != NULL ) {
        packets_recd++;
        delete ev;
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



