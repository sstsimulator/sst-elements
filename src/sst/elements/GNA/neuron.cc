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

void Neuron::configure(float threshold, float minimum, float leak) {
    this->threshold = threshold;
    this->minimum   = minimum;
    this->leak      = leak;
}

void Neuron::deliverSpike(float str, uint when) {
    temporalBuffer[when] += str;
    //printf(" got %f @ %d\n", str, when);
}

bool Neuron::lif(const uint now) {
    V -= leak;
    if (V < minimum) V = minimum;

    temporalBuffer_t::iterator i = temporalBuffer.find(now);
    if (i != temporalBuffer.end()) {
        V += i->second;
        temporalBuffer.erase(i);
        //printf(" got current spike %f @ %d\n", val, now);
    }

    if (V > threshold) {
        V = minimum;
        return true;
    } else {
        return false;
    }
}

