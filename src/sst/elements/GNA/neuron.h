// Copyright 2018-2022 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2018-2022, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// of the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef _NEURON_H
#define _NEURON_H

#include <map>
#include <cstdint>

#include <sst/core/interfaces/stdMem.h>  // supplies type uint


namespace SST {
namespace GNAComponent {

struct Synapse {
    float    weight;
    uint16_t delay;
    uint16_t target;
};

class Neuron {
public:
    float    V;
    float    threshold;
    float    minimum;
    float    leak;
    uint64_t synapseBase;  // address in memory of synapse list
    uint32_t synapseCount; // number of entries in synapse list

    // temporal buffer
    typedef std::map<const uint, float> temporalBuffer_t;
    temporalBuffer_t temporalBuffer;

    void     configure   (float threshold, float minimum, float leak);
    void     deliverSpike(float str, uint when);
    bool     lif         (const uint now);  ///< performs Leaky Integrate and Fire. Returns true if fired.
};

}
}

#endif // _NEURON_H
