// Copyright 2009-2018 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2018, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef _SHOGON_COMPONENT_H
#define _SHOGON_COMPONENT_H

#include <sst/core/component.h>
#include <sst/core/link.h>
#include <sst/core/output.h>
#include <sst/core/params.h>

#include "arb/shogunarb.h"
#include "shogun_event.h"
#include "shogun_q.h"
#include "shogun_stat_bundle.h"

namespace SST {
namespace Shogun {

class ShogunComponent : public SST::Component {
public:

    SST_ELI_REGISTER_COMPONENT(
        ShogunComponent,
        "shogun",
        "ShogunXBar",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "Shogun Arbitrated Crossbar Component",
        COMPONENT_CATEGORY_PROCESSOR
    )

    SST_ELI_DOCUMENT_PARAMS(
        { "verbose",                "Level of output verbosity, higher is more output, 0 is no output", 0 },
        { "port_count",             "Number of ports on the Crossbar", "0" },
        { "arbitration",            "Select the arbitration scheme", "roundrobin" },
        { "clock",                  "Clock Frequency for the crossbar", "1.0GHz" },
        { "queue_slots",            "Depth of input queue", "64" },
        { "in_msg_per_cycle",       "Number of messages injested per cycle; -1 is unlimited", "1" },
        { "out_msg_per_cycle",      "Number of messages ejected per cycle; -1 is unlimited", "1" },
    )

    SST_ELI_DOCUMENT_STATISTICS(
        { "cycles_zero_events",  "Number of cycles where there were no events to process, x-bar was quiet", "cycles", 1 },
        { "cycles_events",       "Number of cycles where events needed to be processed, x-bar may have been busy.", "cycles", 1 },
        { "packets_moved",       "Number of packets moved each cycle", "packets", 1 },
        { "output_packet_count", "Number of communication packets which have been output", "packets", 1 },
        { "input_packet_count",  "Number of communication packets which have been input", "packets", 1 }
    )

    SST_ELI_DOCUMENT_PORTS(
        { "port%(port_count)d", "Link to port X", { "shogun.ShogunEvent"} }
    )

    // Optional since there is nothing to document
    SST_ELI_DOCUMENT_SUBCOMPONENT_SLOTS(
    )

    ShogunComponent(SST::ComponentId_t id, SST::Params& params);
    ~ShogunComponent();

    virtual void init(unsigned int phase);

    void setup() { }
    void finish() { }

    void printStatus();

private:
    ShogunComponent();  // for serialization only
    ShogunComponent(const ShogunComponent&); // do not implement
    void operator=(const ShogunComponent&); // do not implement

    virtual bool tick(SST::Cycle_t);
    //void tickEvent( SST::Event* ev );
    void handleIncoming(SST::Event* event);

    void clearInputs();
    void clearOutputs();
    void populateInputs();
    void emitOutputs();

    uint64_t previousCycle;

    int32_t port_count;
    int32_t queue_slots;
    int32_t pending_events;

    int32_t  input_message_slots;
    int32_t  output_message_slots;

    ShogunStatisticsBundle* stats;
    SST::Link** links;
    SST::Link* clockLink;

    ShogunQueue<ShogunEvent*>** inputQueues;
    ShogunEvent*** pendingOutputs;
    int32_t* remote_output_slots;
    ShogunArbitrator* arb;

    SST::Output* output;
    Statistic<uint64_t>* zeroEventCycles;
    Statistic<uint64_t>* eventCycles;

    int64_t clockPS;

    TimeConverter* tc;
    Clock::HandlerBase* clockTickHandler;
    bool handlerRegistered;
};


} // namespace ShogunComponent
} // namespace SST

#endif /* _SHOGON_COMPONENT_H */
