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
#include <sst/core/elementinfo.h>
#include <sst/core/link.h>
#include <sst/core/output.h>
#include <sst/core/params.h>

#include "shogun_q.h"
#include "shogun_event.h"
#include "arb/shogunarb.h"

namespace SST {
namespace Shogun {

class ShogunComponent : public SST::Component {
public:

    // REGISTER THIS COMPONENT INTO THE ELEMENT LIBRARY
    SST_ELI_REGISTER_COMPONENT(
        ShogunComponent,
        "shogun",
        "ShogunXBar",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "Shogun Arbitrated Crossbar Component",
        COMPONENT_CATEGORY_PROCESSOR
    )

    SST_ELI_DOCUMENT_PARAMS(
	{ "port_count", "Number of ports on the Crossbar", 0 },
	{ "arbitration", "Select the arbitration scheme", "roundrobin" },
        { "clock",       "Clock Frequency for the crossbar", "1.0GHz" }
    )

    // Optional since there is nothing to document
    SST_ELI_DOCUMENT_STATISTICS(
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

private:
    ShogunComponent();  // for serialization only
    ShogunComponent(const ShogunComponent&); // do not implement
    void operator=(const ShogunComponent&); // do not implement

    virtual bool tick(SST::Cycle_t);

    void clearInputs();
    void clearOutputs();
    void populateInputs();
    void emitOutputs();

    int port_count;
    int queue_slots;

    SST::Link** links;

    ShogunQueue<ShogunEvent*>** inputQueues;
    ShogunEvent** pendingOutputs;
    int* remote_output_slots;
    ShogunArbitrator* arb;

    SST::Output* output;

};

} // namespace ShogunComponent
} // namespace SST

#endif /* _SHOGON_COMPONENT_H */
