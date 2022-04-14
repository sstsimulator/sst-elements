// Copyright 2013-2022 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2013-2022, NTESS
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

#include <sst/core/sst_config.h>

#include <sst/core/component.h>
#include <sst/core/interfaces/stdMem.h>

#include <string>
#include <fstream>
#include <cinttypes>

#include "graph/graph.h"
#include "lsQueue.h"
#include "llyrTypes.h"
#include "pes/peList.h"
#include "mappers/llyrMapper.h"

using namespace SST::Interfaces;

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
        { "device_addr",    "Address of device (must be non-zero if not standalone)", "0" },
        { "starting_addr",  "Address of device memory", "0" },
        { "clockcount",     "Number of clock ticks to execute", "100000" },
        { "application",    "Application in affine IR", "app.in" },
        { "hardware_graph", "Hardware connectivity graph", "grid.cfg" },
        { "mapping_tool",   "External mapping tool", "" },
        { "mem_init",       "Memory initialization file", "" },
        { "ls_entries",     "Number of L/S entries to process each tick", "1" },
        { "queue_depth",    "Number of buffer elements", "256" },
        { "arith_latency",  "Number of clock ticks for ARITH operations", "1" },
        { "int_latency",    "Number of clock ticks for INT operations", "1" },
        { "fp_latency",     "Number of clock ticks for FP OTHER operations", "4" },
        { "fp_mul_latency", "Number of clock ticks for FP MUL operations", "4" },
        { "fp_div_latency", "Number of clock ticks for FP DIV operations", "4" }
    )

    ///TODO
    SST_ELI_DOCUMENT_STATISTICS(
    )

    SST_ELI_DOCUMENT_PORTS(
        { "cache_link",     "Link to Memory Controller", { "memHierarchy.memEvent" , "" } }
    )

    SST_ELI_DOCUMENT_SUBCOMPONENT_SLOTS(
        { "memory",         "The memory interface to use (e.g., interface to caches)", "Interfaces::SST::StandardMem" }
    )

    LlyrComponent(SST::ComponentId_t id, SST::Params& params);
    ~LlyrComponent();

    void setup();
    void finish();

    void init( uint32_t phase );

protected:


private:
    LlyrComponent();                            // for serialization only
    LlyrComponent( const LlyrComponent& );      // do not implement
    void operator=( const LlyrComponent& );     // do not implement

    virtual bool tick( SST::Cycle_t currentCycle );

    void handleEvent(StandardMem::Request* req);
    /* Handlers for StandardMem::Request types */
    class LlyrMemHandlers : public StandardMem::RequestHandler {
    public:
        friend class LlyrComponent;
        friend class LSQueue;

        LlyrMemHandlers(LlyrComponent* llyr, LSQueue* ls_queue, SST::Output* out) :
            StandardMem::RequestHandler(out), ls_queue_(ls_queue), llyr_(llyr) {}
        virtual ~LlyrMemHandlers() {}

        virtual void handle(StandardMem::Read* read) override;
        virtual void handle(StandardMem::Write* write) override;
        virtual void handle(StandardMem::ReadResp* resp) override;
        virtual void handle(StandardMem::WriteResp* resp) override;

        LSQueue*        ls_queue_;
        LlyrComponent*  llyr_;
    };

    LlyrMemHandlers*    mem_handlers_;
    StandardMem*        mem_interface_;
    Addr                device_addr_;
    Addr                starting_addr_;

    std::string         mapping_tool_;

    SST::TimeConverter*     time_converter_;
    Clock::HandlerBase*     clock_tick_handler_;
    bool                    handler_registered_;
    bool                    clock_enabled_;

    bool compute_complete;

    SST::Link**  links_;
    SST::Link*   clockLink_;
    SST::Output* output_;

    Statistic< uint64_t >* zeroEventCycles_;
    Statistic< uint64_t >* eventCycles_;

    LlyrConfig* configData_;
    LlyrGraph< opType > hardwareGraph_;
    LlyrGraph< AppNode > applicationGraph_;
    LlyrGraph< ProcessingElement* > mappedGraph_;

    LlyrMapper* llyr_mapper_;

    void constructHardwareGraph( std::string fileName );
    void constructSoftwareGraph( std::string fileName );
    void constructSoftwareGraphIR( std::ifstream& inputStream );
    void constructSoftwareGraphApp( std::ifstream& inputStream );
    std::vector< uint64_t >* constructMemory( std::string fileName );
    std::string memFileName_;

    uint32_t ls_entries_;
    LSQueue* ls_queue_;
    void doLoadStoreOps( uint32_t numOps );

};

} // namespace LLyr
} // namespace SST

#endif /* _LLYR_H */
