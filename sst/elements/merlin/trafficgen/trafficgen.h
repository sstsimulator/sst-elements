// -*- mode: c++ -*-

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


#ifndef COMPONENTS_MERLIN_GENERATORS_TRAFFICEGEN_H
#define COMPONENTS_MERLIN_GENERATORS_TRAFFICEGEN_H

#include <boost/random/mersenne_twister.hpp>
#include <boost/random/uniform_int_distribution.hpp>
#include <boost/random/discrete_distribution.hpp>

#include <sst/core/component.h>
#include <sst/core/event.h>
#include <sst/core/link.h>
#include <sst/core/timeConverter.h>


namespace SST {
namespace Merlin {

class LinkControl;

class TrafficGen : public Component {

private:

    class Generator {
    public:
        virtual int getNextValue(void) = 0;
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
                _abort(TrafficGen, "Unsure how to deal with %d neighbors\n", numNeighbors);
            }
        }
        virtual int getNextValue(void)
        {
            int neighbor = dist->getNextValue();
            return neighbors[neighbor];
        }
    };


    class UniformDist : public Generator {
        boost::random::mt19937 gen;
        boost::random::uniform_int_distribution<> dist;
    public:
        UniformDist(int min, int max) :
            dist(min, max)
        {
            gen.seed(0);
        }
        virtual int getNextValue(void)
        {
            return dist(gen);
        }
    };


    class DiscreteDist : public Generator {
        boost::random::mt19937 gen;
        boost::random::discrete_distribution<> dist;
        int minValue;
    public:
        DiscreteDist(int min, int max, int target, float targetProb) : minValue(min)
        {
            int size = max - min;
            float dfltP = (1.0 - targetProb) / (size -1);
            std::vector<float> probs(size);
            for ( int i = 0 ; i < size ; i++ ) {
                probs[i] = dfltP;
            }
            probs[target] = targetProb;
            dist = boost::random::discrete_distribution<>(probs.begin(), probs.end());
            gen.seed(0);
        }
        virtual int getNextValue(void)
        {
            return dist(gen) + minValue;
        }
    };



    enum AddressMode { SEQUENTIAL, FATTREE_IP };

    AddressMode addressMode;

    int id;
    int ft_loading;
    int ft_radix;
    int num_peers;
    int num_vcs;
    int last_vc;

    int packets_sent;
    int packets_recd;

    bool done;

    LinkControl* link_control;

    int base_packet_size;
    int packets_to_send;

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
    bool clock_handler(Cycle_t cycle);
    int fattree_ID_to_IP(int id);
    int IP_to_fattree_ID(int id);

protected:
    int getPacketDest(void);
    int getPacketSize(void);
    int getDelayNextPacket(void);

};

}
}

#endif
