// -*- mode: c++ -*-

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


#ifndef COMPONENTS_MERLIN_GENERATORS_TRAFFICEGEN_H
#define COMPONENTS_MERLIN_GENERATORS_TRAFFICEGEN_H

#include <boost/random/mersenne_twister.hpp>
//#include <boost/random/uniform_int_distribution.hpp>
//#include <boost/random/discrete_distribution.hpp>
//#include <boost/random/normal_distribution.hpp>
#include <boost/random/binomial_distribution.hpp>

#include <sst/core/rng/mersenne.h>
#include <sst/core/rng/gaussian.h>
#include <sst/core/rng/discrete.h>
#include <sst/core/rng/expon.h>
#include <sst/core/rng/uniform.h>

#include <sst/core/component.h>
#include <sst/core/event.h>
#include <sst/core/link.h>
#include <sst/core/timeConverter.h>
#include <sst/core/output.h>

#include "sst/elements/merlin/linkControl.h"

#define ENABLE_FINISH_HACK 0

namespace SST {
namespace Merlin {

class LinkControl;

class TrafficGen : public Component {

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
        boost::random::mt19937 gen;
        boost::random::binomial_distribution<> dist;
        int minValue;
    public:
        BinomialDist(int min, int max, int trials, float probability) : minValue(min)
        {
            dist = boost::random::binomial_distribution<>(trials, probability);
        }
        virtual int getNextValue(void)
        {
            return dist(gen) + minValue;
        }
        virtual void seed(uint32_t val)
        {
            gen.seed(val);
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
