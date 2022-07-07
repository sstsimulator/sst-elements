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

#ifndef _GNA_H
#define _GNA_H

#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS
#endif
#include <inttypes.h>
#include <vector>

#include <sst/core/event.h>
#include <sst/core/sst_types.h>
#include <sst/core/component.h>
#include <sst/core/link.h>
#include <sst/core/timeConverter.h>
#include <sst/core/output.h>

#include <sst/core/interfaces/stdMem.h>
#include "neuron.h"
#include "sts.h"

namespace SST {
namespace GNAComponent {


class GNA : public SST::Component {
public:
/* Element Library Info */
    SST_ELI_REGISTER_COMPONENT(GNA, "GNA", "GNA", SST_ELI_ELEMENT_VERSION(1,0,0),
        "Spiking Temportal Processing Unit", COMPONENT_CATEGORY_PROCESSOR)

    SST_ELI_DOCUMENT_PARAMS(
        {"verbose",        "(uint) Determine how verbose the output from the CPU is",            "0"},
        {"clock",          "(string) Clock frequency",                                           "1GHz"},
        {"InputsPerTic",   "Max # of input spikes which can be delivered each clock cycle",      "2"},
        {"STSDispatch",    "Max # spikes that can be dispatched to the STS in a clock cycle",    "2"},
        {"STSParallelism", "Max # spikes the STS can process in parallel",                       "2"},
        {"MaxOutMem",      "Maximum # of outgoing memory requests per cycle",                    "STSParallelism"},
        {"neurons",        "(uint) number of neurons",                                           "32"}
    )

    SST_ELI_DOCUMENT_PORTS( {"mem_link", "Connection to memory", { "memHierarchy.MemEventBase" } } )

    SST_ELI_DOCUMENT_SUBCOMPONENT_SLOTS( {"memory", "Interface to memory (e.g., caches)", "SST::Interfaces::StandardMem"} )

    /* Begin class definiton */
    GNA(SST::ComponentId_t id, SST::Params& params);
    void finish();

    void deliver(float val, int targetN, int time);
    Neuron & getNeuron(int n);
    void readMem(Interfaces::StandardMem::Request *req, STS *requestor);

protected:
    GNA();  // for serialization only
    GNA(const GNA&); // do not implement
    void operator=(const GNA&); // do not implement
    void init(unsigned int phase);

    void handleEvent( SST::Interfaces::StandardMem::Request * req );
    virtual bool clockTic( SST::Cycle_t );
    bool deliverInputs();
    void assignSTS();
    void processFire();
    void lifAll();

    typedef enum {IDLE, PROCESS_FIRE, LIF, LAST_STATE} gnaState_t;
    gnaState_t state;

    Output out;
    Interfaces::StandardMem * memory;
    uint numNeurons;
    uint InputsPerTic;
    uint STSDispatch;
    uint STSParallelism;
    uint maxOutMem;
    uint now;
    uint numFirings;
    uint numDeliveries;
    std::queue<SST::Interfaces::StandardMem::Request *> outgoingReqs;

    Neuron *neurons;
    std::vector<STS> STSUnits;

    typedef std::multimap<const uint, Synapse> inputBuffer_t;
    inputBuffer_t inputBuffer;

    std::deque<uint> firedNeurons;
    std::map<uint64_t, STS*> requests;

    TimeConverter *clockTC;
    Clock::HandlerBase *clockHandler;

};

}
}
#endif /* _GNA_H */
