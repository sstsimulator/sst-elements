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

#include <sst_config.h>
#include "sts.h"
#include "GNA.h"

using namespace SST;
using namespace SST::GNAComponent;

void STS::assign(int neuronNum) {
    const Neuron & neuron = myGNA->getNeuron(neuronNum);
    numSpikes = neuron.synapseCount;
    uint64_t listAddr = neuron.synapseBase;

    for (int i = 0; i < numSpikes; i++) {
        // AFR: should throttle
        using namespace Interfaces;
        StandardMem::Read *req = new StandardMem::Read(listAddr, sizeof(Synapse));
        myGNA->readMem(req, this);
        listAddr += sizeof(Synapse);
    }
}

bool STS::isFree() {
    return (numSpikes == 0);
}

void STS::advance(uint now) {
    // AFR: should throttle
    while (! incomingReqs.empty()) {
        // get the request
        SST::Interfaces::StandardMem::Request *  req  = incomingReqs.front();
        SST::Interfaces::StandardMem::ReadResp * resp = dynamic_cast<SST::Interfaces::StandardMem::ReadResp*>(req);
        assert(resp);

        // deliver the spike
        Synapse * s = (Synapse *) &resp->data[0];
        myGNA->deliver(s->weight, s->target, s->delay+now);
        numSpikes--;

        incomingReqs.pop();
        delete req;
    }
}

