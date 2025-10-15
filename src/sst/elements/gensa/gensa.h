// Copyright 2018-2025 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2018-2025, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// of the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef _gensa_h
#define _gensa_h

#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS
#endif

#include <cstdint>
#include <set>
#include <string>
#include <inttypes.h>
#include <queue>
#include <vector>

#include <sst/core/event.h>
#include <sst/core/sst_types.h>
#include <sst/core/component.h>
#include <sst/core/link.h>
#include <sst/core/timeConverter.h>
#include <sst/core/output.h>
#include <sst/core/interfaces/stdMem.h>
#include <sst/core/interfaces/simpleNetwork.h>

#include "neuron.h"


namespace SST {
namespace gensaComponent {

class gensa : public SST::Component {
public:
    // Element Library Info

    SST_ELI_REGISTER_COMPONENT(gensa, "gensa", "core", SST_ELI_ELEMENT_VERSION(1,0,0),
        "Spiking Processor", COMPONENT_CATEGORY_PROCESSOR)

    SST_ELI_DOCUMENT_PARAMS(
        {"verbose",        "(uint) Determine how verbose the output from the CPU is",            "0"},
        {"clock",          "(string) Clock frequency",                                           "1GHz"},
        {"modelPath",      "(string) Path to neuron file",                                       "model"},
        {"steps",          "(uint) how many ticks the simulation should last",                   "1000"},
        {"dt",             "(float) duration of one tick in sim time; used for output",          "1"}
    )

    SST_ELI_DOCUMENT_PORTS( {"mem_link", "Connection to memory", { "memHierarchy.MemEventBase" } } )

    SST_ELI_DOCUMENT_SUBCOMPONENT_SLOTS(
		{"memory", "Interface to memory (e.g., caches)", "SST::Interfaces::StandardMem"},
        {"networkIF", "Network interface", "SST::Interfaces::SimpleNetwork"}
    )

    // class definiton

    Output      out;             ///< Log file
    std::string modelPath;       ///< Where to load network configuration from
    uint32_t    now;             ///< Current step of simulation
    uint32_t    steps;           ///< maximum number of steps the sim should take
    uint32_t    numFirings;      ///< Statistics
    uint32_t    numDeliveries;   ///< Statistics
    int         neuronIndex;     ///< Current neuron being processed in current cycle
    int         synapseIndex;    ///< Current downstream synapse (associated with current neuron) being sent a spike
    bool        syncSent;
    uint32_t    maxRequestDepth; ///< Shared by memory and network. Should be a pretty small number like 2 or 3.

    std::vector<Neuron*> neurons;

    TimeConverter *             clockTC;
    Interfaces::StandardMem *   memory;
    Interfaces::SimpleNetwork * link;
    std::set<uint64_t>                                   memoryRequests;
    std::queue<SST::Interfaces::SimpleNetwork::Request*> networkRequests;

    gensa (SST::ComponentId_t id, SST::Params& params);  // regular constructor
    gensa ();                                            // for serialization only
    gensa (const gensa&) = delete;
    ~gensa();

    void operator= (const gensa&) = delete;

    void init     (unsigned int phase);
    void setup    ();
    void complete (unsigned int phase);
    void finish   ();

    virtual bool clockTic (SST::Cycle_t);
    void send ();  ///< Try to send next network request in queue.
    void handleMemory (SST::Interfaces::StandardMem::Request * req);
    bool handleNetwork (int vn);
};

class SyncEvent : public SST::Event
{
public:
    int phase;

    virtual uint32_t cls_id () const;
    virtual std::string serialization_name () const;
};


}
}
#endif /* _gensa_h */
