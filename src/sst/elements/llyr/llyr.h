// Copyright 2009-2020 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2020, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef _LLYR_H
#define _LLYR_H

#include <sst/core/component.h>

#include "graph.h"
#include "processingElement.h"

namespace SST {
namespace Llyr {

class LlyrComponent : public SST::Component
{
public:

    // REGISTER THIS COMPONENT INTO THE ELEMENT LIBRARY
    SST_ELI_REGISTER_COMPONENT(
        LlyrComponent,
        "llyr",
        "LlyrDataflow",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "Configurable Dataflow Component",
        COMPONENT_CATEGORY_UNCATEGORIZED
    )

    SST_ELI_DOCUMENT_PARAMS(
        { "verbose",        "Level of output verbosity, higher is more output, 0 is no output", 0 },
        { "clock",          "Clock frequency", "1GHz" },
        { "clockcount",     "Number of clock ticks to execute", "100000" },
        { "application",    "Application in affine IR", "" },
        { "hardwareGraph",  "Hardware connectivity graph", "grid.cfg" },
        { "intLatency",     "Number of clock ticks for integer operations", "1" },
        { "fpLatency",      "Number of clock ticks for integer operations", "4" }
    )

    ///TODO
    SST_ELI_DOCUMENT_STATISTICS(
    )

    SST_ELI_DOCUMENT_PORTS(
        { "cache_link",     "Link to Memory Controller", { "memHierarchy.memEvent" , "" } }
    )

    SST_ELI_DOCUMENT_SUBCOMPONENT_SLOTS(
        { "memory",         "The memory interface to use (e.g., interface to caches)", "SST::Interfaces::SimpleMem" }
    )

    LlyrComponent(SST::ComponentId_t id, SST::Params& params);
    ~LlyrComponent();

    void setup()  { }
    void finish() { }

private:
    LlyrComponent();                        // for serialization only
    LlyrComponent(const LlyrComponent&);    // do not implement
    void operator=(const LlyrComponent&);   // do not implement

    virtual bool tick(SST::Cycle_t);

    TimeConverter*      tc;
    Clock::HandlerBase* clockTickHandler;
    bool handlerRegistered;

    int clock_count;

    SST::Link** links;
    SST::Link* clockLink;
    SST::Output* output;

    Statistic<uint64_t>* zeroEventCycles;
    Statistic<uint64_t>* eventCycles;

    LlyrGraph hardwareGraph;
    LlyrGraph applicationGraph;
    LlyrGraph mappedGraph;

    void constructHardwareGraph(std::string fileName);
    void constructSoftwareGraph(std::string fileName);
    LlyrGraph subgraphIsomorphism( LlyrGraph largeGraph, LlyrGraph smallGraph );

};

} // namespace LLyr
} // namespace SST

#endif /* _LLYR_H */
