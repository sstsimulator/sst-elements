// Copyright 2022 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2022, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// of the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#include <sst_config.h>
#include "neuron.h"

using namespace SST::GNAComponent;
using namespace std;

SST::RNG::MarsagliaRNG Neuron::rng(1,13);

void Neuron::configure(float Vthreshold, float Vreset, float leak, float p) {
    this->Vthreshold = Vthreshold;
    this->Vreset     = Vreset;
    this->leak       = leak;
    this->p          = p;
}

void Neuron::deliverSpike(float str, uint when) {
    temporalBuffer[when] += str;
    //printf(" got %f @ %d\n", str, when);
}

bool Neuron::lif(const uint now) {
    // Add inputs
    temporalBuffer_t::iterator i = temporalBuffer.find(now);
    if (i != temporalBuffer.end()) {
        V += i->second;
        temporalBuffer.erase(i);
        //printf(" got current spike %f @ %d\n", val, now);
    }

    // Check for spike
    if (V > Vthreshold) {
        if (p >= 1  ||  p > 0  &&  rng.nextUniform() <= p) {
            V = Vreset;
            return true;
        }
    }

    // Decay
    V *= leak;
    return false;
}

