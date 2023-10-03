// Copyright 2009-2023 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2023, NTESS
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


// class Trace ---------------------------------------------------------------

Trace::Trace()
{
    mode = 0;
}

Trace::~Trace()
{
    if (mode) free(mode);  // "mode" is created by strdup(), which effectively uses malloc().
}


// class Neuron --------------------------------------------------------------

std::map<std::string,OutputHolder *> Neuron::outputs;
float                                Neuron::dt;

Neuron::Neuron()
{
    synapseBase  = 0;
    synapseCount = 0;
    traces       = 0;
}

Neuron::~Neuron()
{
    Trace * t = traces;
    while (t) {
        Trace * next = t->next;
        delete t;
        t = next;
    }
}

void Neuron::deliverSpike(float str, uint when)
{
    // Do nothing
}


// NeuronLIF -----------------------------------------------------------------

SST::RNG::MarsagliaRNG NeuronLIF::rng(1,13);

NeuronLIF::NeuronLIF(float Vinit, float Vthreshold, float Vreset, float leak, float p)
:   V          (Vinit),
    Vthreshold (Vthreshold),
    Vreset     (Vreset),
    leak       (leak),
    p          (p)
{
}

void NeuronLIF::deliverSpike(float str, uint when)
{
    temporalBuffer[when] += str;
}

bool NeuronLIF::update(const uint now)
{
    // Add inputs
    temporalBuffer_t::iterator i = temporalBuffer.find(now);
    if (i != temporalBuffer.end()) {
        V += i->second;
        temporalBuffer.erase(i);
    }

    // Check for spike
    bool spiked = false;
    if (V > Vthreshold) {
        if (p >= 1  ||  p > 0  &&  rng.nextUniform() <= p) {
            V = Vreset;
            spiked = true;
        }
    } else {
        V *= leak;
    }

    // Outputs
    Trace * t = traces;
    while (t) {
        if (t->probe == 0) {
            if (spiked) t->holder->trace (now*dt, t->column, 1, t->mode);
        } else if (t->probe == 1) {
            t->holder->trace(now*dt, t->column, V, t->mode);
        }
        t = t->next;
    }

    return spiked;
}


// class NeuronInput ---------------------------------------------------------

NeuronInput::NeuronInput()
{
    nextSpike = 0;
}

bool NeuronInput::update(const uint now)
{
    if (nextSpike >= spikes.size()) return false;
    if (spikes[nextSpike] > now)    return false;
    nextSpike++;

    // Outputs
    Trace * t = traces;
    while (t) {
        if (t->probe == 0) t->holder->trace(now*dt, t->column, 1, t->mode);
        t = t->next;
    }

    return true;
}


// class SpikeEvent ----------------------------------------------------------

uint32_t
SpikeEvent::cls_id () const
{
    return 1234;  // TODO: how do you pick a class ID?
}

string
SpikeEvent::serialization_name () const
{
    return "SpikeEvent";
}

