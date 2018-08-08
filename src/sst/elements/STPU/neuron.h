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
#include "stpu_lib.h"

namespace SST {
namespace STPUComponent {

using namespace std;

class neuron {
public:
    void configure(const Neuron_Loader_Types::T_NctFl &in) {
        config = in;
    }
    void deliverSpike(float str, int when) {
        temporalBuffer[when] += str;
    }
    // performs Leaky Integrate and Fire. Returns true if fired.
    bool lif(const int now) {
        // Leak
        value -= config.NrnLkg;

        // Integrate
        value += getCurrentSpikes(now);

        // Fire?
        if (value > config.NrnThr) {
            value = config.NrnMin;
            return true;
        } else {
            return false;
        }
    }
private:
    Neuron_Loader_Types::T_NctFl config;
    float value;
    // temporal buffer
    typedef map<const int, float> tBuf_t;
    tBuf_t temporalBuffer;

    // get any current spike values
    float getCurrentSpikes(const int now) {
        tBuf_t::iterator i = temporalBuffer.find(now);
        if (i != temporalBuffer.end()) {
            float val = i->second;
            temporalBuffer.erase(i);
            return val;
        } else {
            return 0;
        }
    }
};

}
}

#endif // _NEURON_H
