// Copyright 2009-2026 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2026, NTESS
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef CARCOSA_FOURSTATEAGENT_H
#define CARCOSA_FOURSTATEAGENT_H

#include <sst/core/link.h>
#include <sst/core/output.h>
#include <sst/elements/carcosa/components/interceptionAgentAPI.h>
#include <sst/elements/carcosa/components/pipelineStateRegistry.h>
#include <sst/elements/memHierarchy/memEvent.h>
#include <climits>
#include <cstdint>
#include <string>
#include <vector>

namespace SST {
namespace Carcosa {

/**
 * PingPong-compatible MMIO agent; publishes currentKernel into PipelineStateRegistry.
 */
class FourStateAgent : public InterceptionAgentAPI
{
public:
    SST_ELI_REGISTER_SUBCOMPONENT(
        FourStateAgent,
        "carcosa",
        "FourStateAgent",
        SST_ELI_ELEMENT_VERSION(0, 1, 0),
        "N-kernel MMIO agent that publishes PipelineStateBase (currentKernel = CPU command index in flight).",
        SST::Carcosa::InterceptionAgentAPI
    )

    SST_ELI_DOCUMENT_PARAMS(
        {"state_key",       "PipelineStateRegistry key this agent publishes into. Required.", ""},
        {"region_size",     "Size in bytes of the published MMIO control region (regions[0]).", "4096"},
        {"regions",         "Optional CSV of workload-labeled DRAM regions for region-aware EccGuard/PortModuleStateGate policies. 'name:base:size' triples; slot 0 reserved for mmio_control. Used by the Phase 6 region-routing smoke test.", ""},
        {"initial_command", "First command index returned to the CPU.", "0"},
        {"num_commands",    "Number of command indices to cycle through (must match the jump_table length in the Vanadis binary; 2 for pingpong, 4 for fourstate).", "4"},
        {"max_iterations",  "Max iterations before sending exit (-1).",  "12"},
        {"kernel_names",    "Optional list of workload kernel names for command indices 0..num_commands-1, published into PipelineStateBase::currentKernelName for name-keyed consumers (EccGuard kernel_policy, CriticalActionWatcher). Index k falls back to 'K<k>' when unset; the idle state always publishes 'IDLE'.", ""},
        {"verbose",         "Enable verbose per-event output.", "false"}
    )

    FourStateAgent(ComponentId_t id, Params& params);
    FourStateAgent() : InterceptionAgentAPI() {}
    ~FourStateAgent() override;

    bool handleInterceptedEvent(SST::MemHierarchy::MemEvent* ev, SST::Link* highlink) override;
    void notifyPartnerDone(unsigned iteration) override;
    void agentSetup() override;
    void setRingLink(SST::Link* leftLink) override;
    void setInterceptBase(uint64_t base) override;
    void setHighlink(SST::Link* highlink) override;

    /** currentKernel sentinel while CPU is idle between kernels. */
    static constexpr int IDLE = -1;

private:
    void checkBothDone();
    void sendCommandResponse(SST::MemHierarchy::MemEvent* request, int value);
    void sendWriteAck(SST::MemHierarchy::MemEvent* ev);

    /** Publish currentKernel / name / pipelineCycle into the registry. */
    void publishState(int kernel);

    /** Name published into currentKernelName for a given kernel index:
     *  kernel_names[k] when configured, else "K<k>"; IDLE maps to "IDLE". */
    std::string kernelNameFor(int kernel) const;

    SST::Output* out_        = nullptr;
    SST::Link*   leftHaliLink_ = nullptr;
    SST::Link*   highlink_     = nullptr;

    // Ping/pong-compatible protocol state (kept identical so hyades.h works)
    uint64_t controlAddrBase_ = 0;
    uint64_t regionSize_      = 4096;
    int      initialCommand_  = 0;
    int      numCommands_     = 4;
    int      maxIterations_   = 12;
    int      currentIteration_ = 0;
    int      nextCommand_      = INT_MIN;
    bool     partnerDone_      = false;
    bool     localDone_        = false;
    bool     verbose_          = false;
    SST::MemHierarchy::MemEvent* pendingCommandRead_ = nullptr;

    // Registry publishing state
    std::string stateKey_;
    std::string regionsCsv_;
    std::vector<std::string> kernelNames_;
    int         publishedKernel_ = IDLE;
};

} // namespace Carcosa
} // namespace SST

#endif /* CARCOSA_FOURSTATEAGENT_H */
