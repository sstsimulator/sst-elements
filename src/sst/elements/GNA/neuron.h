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
#include <sst/core/rng/marsaglia.h>

#include "OutputHolder.h"


namespace SST {
namespace GNAComponent {

struct Synapse {
    float    weight;
    uint16_t delay;  // in cycles
    uint16_t target; // neuron index
};

struct Trace {
    OutputHolder * holder;
    std::string    column;
    char *         mode;
    int            probe; ///< 0=spike; 1=V
    Trace *        next;  ///< singly-linked list; null to terminate

    Trace();
    ~Trace();
};

class Neuron {
public:
    uint64_t synapseBase;  // address in memory of synapse list
    uint32_t synapseCount; // number of entries in synapse list

    static float dt;
    static std::map<std::string,OutputHolder *> outputs;
    Trace * traces;

    Neuron();
    virtual ~Neuron();

    virtual void deliverSpike(float str, uint when);
    virtual bool update      (const uint now) = 0;  ///< performs Leaky Integrate and Fire. Returns true if fired.
};

class NeuronLIF : public Neuron {
public:
    float V;          // "voltage"; generally in the normal range [0,1]
    float Vthreshold; // value of V which triggers a spike
    float Vreset;     // value of V immediately after a spike
    float leak;       // fraction of V to retain after present cycle, in [0,1]
    float p;          // probability of firing when over threshold, in [0,1]

    static SST::RNG::MarsagliaRNG rng;

    // temporal buffer
    typedef std::map<const uint, float> temporalBuffer_t;
    temporalBuffer_t temporalBuffer;

    NeuronLIF(float Vthreshold = 1, float Vreset = 0, float leak = 1, float p = 1);

    virtual void deliverSpike(float str, uint when);
    virtual bool update      (const uint now);
};

class NeuronInput : public Neuron {
public:
    std::vector<uint16_t> spikes;  // If we are an input neuron, then this is a list of times when we should spike. List is in ascending order.
    int nextSpike;

    NeuronInput();

    virtual bool update(const uint now);
};
}
}

#endif // _NEURON_H
