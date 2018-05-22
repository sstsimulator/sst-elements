// -*- mode: c++ -*-

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


#ifndef COMPONENTS_MERLIN_GENERATORS_TRAFFICEGEN_H
#define COMPONENTS_MERLIN_GENERATORS_TRAFFICEGEN_H

#include <cstdlib>

#include <sst/core/rng/mersenne.h>
#include <sst/core/rng/gaussian.h>
#include <sst/core/rng/discrete.h>
#include <sst/core/rng/expon.h>
#include <sst/core/rng/uniform.h>

#include <sst/core/component.h>
#include <sst/core/elementinfo.h>
#include <sst/core/event.h>
#include <sst/core/link.h>
#include <sst/core/timeConverter.h>
#include <sst/core/output.h>

#include "sst/elements/merlin/merlin.h"
#include "sst/elements/merlin/linkControl.h"

#define ENABLE_FINISH_HACK 0

namespace SST {
namespace Merlin {

class LinkControl;

class TrafficGen : public Component {

public:

    SST_ELI_REGISTER_COMPONENT(
        TrafficGen,
        "merlin",
        "trafficgen",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "Pattern-based traffic generator.",
        COMPONENT_CATEGORY_NETWORK)
    
    SST_ELI_DOCUMENT_PARAMS(
        {"id",                                    "Network ID of endpoint."},
        {"num_peers",                             "Total number of endpoints in network."},
        {"num_vns",                               "Number of requested virtual networks."},
        {"link_bw",                               "Bandwidth of the router link specified in either b/s or B/s (can include SI prefix)."},
        {"topology",                              "Name of the topology subcomponent that should be loaded to control routing."},
        {"buffer_length",                         "Length of input and output buffers.","1kB"},
        {"packets_to_send",                       "Number of packets to send in the test.","1000"},
        {"packet_size",                           "Packet size specified in either b or B (can include SI prefix).","5"},
        {"delay_between_packets",                 "","0"},
        {"message_rate",                          "","1GHz"},
        {"PacketDest:pattern",                    "Address pattern to be used (NearestNeighbor, Uniform, HotSpot, Normal, Binomial)",NULL},
        {"PacketDest:Seed",                       "Sets the seed of the RNG", "11" },
        {"PacketDest:RangeMax",                   "Minumum address to send packets.","0"},
        {"PacketDest:RangeMin",                   "Maximum address to send packets.","INT_MAX"},
        {"PacketDest:NearestNeighbor:3DSize",     "For Nearest Neighbors, the 3D size \"x y z\" of the mesh", ""},
        {"PacketDest:HotSpot:target",             "For HotSpot, which node is the target", ""},
        {"PacketDest:HotSpot:targetProbability",  "For HotSpot, with what probability is the target targeted", ""},
        {"PacketDest:Normal:Mean",                "In a normal distribution, the mean", ""},
        {"PacketDest:Normal:Sigma",               "In a normal distribution, the mean variance", ""},
        {"PacketDest:Binomial:Mean",              "In a binomial distribution, the mean", ""},
        {"PacketDest:Binomial:Sigma",             "In a binomial distribution, the variance", ""},
        {"PacketSize:pattern",                    "Address pattern to be used (Uniform, HotSpot, Normal, Binomial)",NULL},
        {"PacketSize:Seed",                       "Sets the seed of the RNG", "11" },
        {"PacketSize:RangeMax",                   "Minumum size of packets.","0"},
        {"PacketSize:RangeMin",                   "Maximum size of packets.","INT_MAX"},
        {"PacketSize:HotSpot:target",             "For HotSpot, the target packet size", ""},
        {"PacketSize:HotSpot:targetProbability",  "For HotSpot, with what probability is the target targeted", ""},
        {"PacketSize:Normal:Mean",                "In a normal distribution, the mean", ""},
        {"PacketSize:Normal:Sigma",               "In a normal distribution, the mean variance", "1.0"},
        {"PacketSize:Binomial:Mean",              "In a binomial distribution, the mean", ""},
        {"PacketSize:Binomial:Sigma",             "In a binomial distribution, the variance", "0.5"},
        {"PacketDelay:pattern",                   "Address pattern to be used (Uniform, HotSpot, Normal, Binomial)",NULL},
        {"PacketDelay:Seed",                      "Sets the seed of the RNG", "11" },
        {"PacketDelay:RangeMax",                  "Minumum delay between packets.","0"},
        {"PacketDelay:RangeMin",                  "Maximum delay between packets.","INT_MAX"},
        {"PacketDelay:HotSpot:target",            "For HotSpot, the target packet delay", ""},
        {"PacketDelay:HotSpot:targetProbability", "For HotSpot, with what probability is the target targeted", ""},
        {"PacketDelay:Normal:Mean",               "In a normal distribution, the mean", ""},
        {"PacketDelay:Normal:Sigma",              "In a normal distribution, the mean variance", "1.0"},
        {"PacketDelay:Binomial:Mean",             "In a binomial distribution, the mean", ""},
        {"PacketDelay:Binomial:Sigma",            "In a binomial distribution, the variance", "0.5"}
    )

    SST_ELI_DOCUMENT_PORTS(
        {"rtr",  "Port that hooks up to router.", { "merlin.RtrEvent", "merlin.credit_event" } }
    )


private:

#if ENABLE_FINISH_HACK
    static int count;
    static int received;
    static int min_lat;
    static int max_lat;
    static int mean_sum;
#endif

    class Generator {
    public:
        virtual int getNextValue(void) = 0;
        virtual void seed(uint32_t val) = 0;
    };
    
    class NearestNeighbor : public Generator {
        Generator *dist;
        int *neighbors;
    public:
        NearestNeighbor(Generator *dist, int id, int maxX, int maxY, int maxZ, int numNeighbors) :
            dist(dist)
        {
            int myX = id % maxX;
            int myY = (id / maxX) % maxY;
            int myZ = id / (maxX*maxY);

            neighbors = new int[numNeighbors];
            switch (numNeighbors) {
            case 6: {
                neighbors[0] = (((myX-1) + maxX) % maxX) + myY*maxX                         + myZ*(maxX*maxY);
                neighbors[1] = ((myX+1) % maxX)          + myY*maxX                         + myZ*(maxX*maxY);
                neighbors[2] = myX                       + (((myY-1) + maxY) % maxY) * maxX + myZ*(maxX*maxY);
                neighbors[3] = myX                       + (((myY+1)) % maxY) * maxX        + myZ*(maxX*maxY);
                neighbors[4] = myX                       + myY*maxX                         + (((myZ-1) + maxZ) % maxZ) * (maxX*maxY);
                neighbors[5] = myX                       + myY*maxX                         + (((myZ+1)) % maxZ) * (maxX*maxY);
                break;
            }
            default:
                Simulation::getSimulation()->getSimulationOutput().fatal(CALL_INFO, -1,
                    "Unsure how to deal with %d neighbors\n", numNeighbors);
            }
        }
        
        int getNextValue(void)
        {
            int neighbor = dist->getNextValue();
            return neighbors[neighbor];
        }

        void seed(uint32_t val)
        {
            dist->seed(val);
        }
    };
    
    class ExponentialDist : public Generator {
        MersenneRNG* gen;
        SSTExponentialDistribution* dist;
        
    public:
        ExponentialDist(int lambda)
        {
            gen = new MersenneRNG();
	    dist = new SSTExponentialDistribution((double) lambda);
        }

        ~ExponentialDist() {
            delete dist;
            delete gen;
        }

        int getNextValue(void)
        {
            return (int) dist->getNextDouble();
        }

        void seed(uint32_t val)
        {
            delete gen;
            gen = new MersenneRNG((unsigned int) val);
        }
    };


    class UniformDist : public Generator {
        MersenneRNG* gen;
        SSTUniformDistribution* dist;

        int dist_size;
        
    public:
        UniformDist(int min, int max)
        {
		gen = new MersenneRNG();

		dist_size = std::max(1, max-min);
		dist = new SSTUniformDistribution(dist_size, gen);
	}

	~UniformDist() {
		delete dist;
		delete gen;
	}

        int getNextValue(void)
        {
            return (int) dist->getNextDouble();
        }

        void seed(uint32_t val)
        {
            delete dist;
            delete gen;
            gen = new MersenneRNG((unsigned int) val);
            dist = new SSTUniformDistribution(dist_size,gen);
        }
    };


    class DiscreteDist : public Generator {
	MersenneRNG* gen;
	SSTDiscreteDistribution* dist;
        int minValue;
    public:
        DiscreteDist(int min, int max, int target, double targetProb) : minValue(min)
        {
            int size = std::max(max - min, 1);
            double dfltP = (1.0 - targetProb) / (size - 1);
            std::vector<double> probs(size);
            for ( int i = 0 ; i < size ; i++ ) {
                probs[i] = dfltP;
            }
            probs[target] = targetProb;

	    gen = new MersenneRNG();
	    dist = new SSTDiscreteDistribution(&probs[0], size, gen);
        }

	~DiscreteDist() {
		delete dist;
		delete gen;
	}

        int getNextValue(void)
        {
            return ((int) dist->getNextDouble()) + minValue;
        }

        void seed(uint32_t val)
        {
            gen = new MersenneRNG((unsigned int) val);
        }
    };

    class NormalDist : public Generator {
	SSTGaussianDistribution* dist;
	MersenneRNG* gen;

        int minValue;
        int maxValue;
    public:
        NormalDist(int min, int max, double mean, double stddev) : minValue(min), maxValue(max)
        {
            gen = new MersenneRNG();
            dist = new SSTGaussianDistribution(mean, stddev);
        }

	~NormalDist() {
		delete dist;
		delete gen;
	}

        int getNextValue(void)
        {
            for (;;) {
                int val = (int) dist->getNextDouble();
                if ( val < maxValue && val >= minValue ) return val;
            }
        }

        void seed(uint32_t val)
        {
            gen = new MersenneRNG((unsigned int) val);
        }
    };

    class BinomialDist : public Generator {
        int minValue;
    public:
        BinomialDist(int min, int max, int trials, float probability) : minValue(min)
        {
            merlin_abort.fatal(CALL_INFO, -1, "BinomialDist is not currently supported\n");
        }
        virtual int getNextValue(void)
        {
            // return dist(gen) + minValue;
            return 0;
        }
        virtual void seed(uint32_t val)
        {
            // gen.seed(val);
        }
    };



    enum AddressMode { SEQUENTIAL, FATTREE_IP };

    AddressMode addressMode;

    Output out;
    int id;
    int ft_loading;
    int ft_radix;
    int num_peers;
    int num_vns;
//    int last_vc;

    uint64_t packets_sent;
    uint64_t packets_recd;

    bool done;

    LinkControl* link_control;
    LinkControl::Handler<TrafficGen>* send_notify_functor;
    Clock::Handler<TrafficGen>* clock_functor;
    TimeConverter* clock_tc;
    
    int base_packet_size;
    uint64_t packets_to_send;

    int base_packet_delay;
    int packet_delay;

    Generator *packetDestGen;
    Generator *packetSizeGen;
    Generator *packetDelayGen;

public:
    TrafficGen(ComponentId_t cid, Params& params);
    ~TrafficGen();

    void init(unsigned int phase);
    void setup();
    void finish();


private:
    Generator* buildGenerator(const std::string &prefix, Params& params);
    bool clock_handler(Cycle_t cycle);
    int fattree_ID_to_IP(int id);
    int IP_to_fattree_ID(int id);
    bool handle_receives(int vn);
    bool send_notify(int vn);
    
protected:
    int getPacketDest(void);
    int getPacketSize(void);
    int getDelayNextPacket(void);

};

} //namespace Merlin
} //namespace SST

#endif
