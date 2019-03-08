// Copyright 2018 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2018, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef _NEURON_H
#define _NEURON_H

#include <map>
#include <sst/core/rng/marsaglia.h>
#include "gna_lib.h"

namespace SST {
namespace GNAComponent {

using namespace std;

class neuron {
public:
    void configure(const Neuron_Loader_Types::T_NctFl &in,
                   RNG::MarsagliaRNG *r) {
        config = in;
        rng = r;
        rngCache = 0;
    }
    void deliverSpike(float str, uint when) {
        temporalBuffer[when] += str;
        //printf(" got %f @ %d\n", str, when);
    }
    // performs Leaky Integrate and Fire. Returns true if fired.
    bool lif(const uint now) {
        // Leak
        value -= config.NrnLkg;

        // Bound?
        // AFR: is this right?
        if (value < config.NrnMin) {
            value = 0;
        }

        // Integrate
        value += getCurrentSpikes(now);

        // Random Fire
        bool randFire = 0;
        if (0 != config.randomFireChance) {
            uint16_t roll = getRand8();
            if (roll < config.randomFireChance) {
                randFire = 1;
                //printf("rand Fire\n");
            }
        }

        // Fire?
        if (randFire || value > config.NrnThr) {
            value = config.NrnMin;

#if 0
            static int rf = 0;
            static int f = 0;
            if (randFire) rf++;
            f++;
            if ((f & 0xfff) == 0) {
                printf("%.2f %% rand %d\n", double(rf)/double(f)*100.0, f);
            }
#endif


            return true;
        } else {
            return false;
        }
    }
    void setWML(uint64_t addr, uint32_t entries) {
        WMLAddr = addr;
        WMLLen = entries;
    }
    uint32_t getWMLLen() const {return WMLLen;}
    uint32_t getWMLAddr() const {return WMLAddr;}
private:
    Neuron_Loader_Types::T_NctFl config;
    float value;
    RNG::MarsagliaRNG *rng;
    // temporal buffer
    typedef map<const uint, float> tBuf_t;
    tBuf_t temporalBuffer;
    // Neuron's white matter list
    uint64_t WMLAddr; // start
    uint32_t WMLLen; // number of entries in WML
    // random number cache
    uint64_t rngCache;

    uint16_t getRand8() {
        if (rngCache == 0) {
            rngCache = rng->generateNextUInt64();
        }

        uint16_t roll = rngCache & 0xff;
        rngCache >>= 8;
        return roll;
    }

    // get any current spike values
    float getCurrentSpikes(const int now) {
        tBuf_t::iterator i = temporalBuffer.find(now);
        if (i != temporalBuffer.end()) {
            float val = i->second;
            temporalBuffer.erase(i);
            //printf(" got current spike %f @ %d. val %f Trig %f\n", val, now, value, config.NrnThr);
            return val;
        } else {
            return 0;
        }
    }
};

}
}

#endif // _NEURON_H
