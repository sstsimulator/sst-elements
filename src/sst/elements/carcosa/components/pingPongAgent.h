// Copyright 2009-2026 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2026, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// of the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef CARCOSA_PINGPONGAGENT_H
#define CARCOSA_PINGPONGAGENT_H

#include <sst/core/link.h>
#include <sst/core/output.h>
#include <sst/elements/carcosa/components/interceptionAgentAPI.h>
#include <sst/elements/memHierarchy/memEvent.h>
#include <climits>
#include <cstdint>

namespace SST {
namespace Carcosa {

/**
 * InterceptionAgent that implements the MMIO ping-pong coordination protocol
 * used with Vanadis (command register at base+0, status register at base+4).
 * Coordinates with a partner Hali via the ring; supports initial_command,
 * max_iterations, and exit (-1) semantics.
 */
class PingPongAgent : public InterceptionAgentAPI
{
public:
    SST_ELI_REGISTER_SUBCOMPONENT(
        PingPongAgent,
        "carcosa",
        "PingPongAgent",
        SST_ELI_ELEMENT_VERSION(1, 0, 0),
        "MMIO ping-pong coordination agent for Vanadis (command/status registers)",
        SST::Carcosa::InterceptionAgentAPI
    )

    SST_ELI_DOCUMENT_PARAMS(
        {"initial_command", "First command index to send to core.", "0"},
        {"max_iterations", "Max iterations before sending exit (-1).", "6"},
        {"verbose", "Enable verbose output.", "false"}
    )

    PingPongAgent(ComponentId_t id, Params& params);
    PingPongAgent() : InterceptionAgentAPI() {}
    ~PingPongAgent();

    bool handleInterceptedEvent(SST::MemHierarchy::MemEvent* ev, SST::Link* highlink) override;
    void notifyPartnerDone(unsigned iteration) override;
    void agentSetup() override;
    void setRingLink(SST::Link* leftLink) override;
    void setInterceptBase(uint64_t base) override;
    void setHighlink(SST::Link* highlink) override;

private:
    void checkBothDone();
    void sendCommandResponse(SST::MemHierarchy::MemEvent* request, int value);
    void sendWriteAck(SST::MemHierarchy::MemEvent* ev);

    SST::Output* out_;
    SST::Link* leftHaliLink_ = nullptr;
    SST::Link* highlink_ = nullptr;
    uint64_t controlAddrBase_ = 0;
    int initialCommand_ = 0;
    int maxIterations_ = 6;
    int currentIteration_ = 0;
    int nextCommand_ = INT_MIN;
    bool partnerDone_ = false;
    bool localDone_ = false;
    bool verbose_ = false;
    SST::MemHierarchy::MemEvent* pendingCommandRead_ = nullptr;
};

} // namespace Carcosa
} // namespace SST

#endif /* CARCOSA_PINGPONGAGENT_H */
