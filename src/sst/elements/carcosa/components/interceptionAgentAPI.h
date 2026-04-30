// Copyright 2009-2024 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2024, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// of the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef CARCOSA_INTERCEPTIONAGENT_API_H
#define CARCOSA_INTERCEPTIONAGENT_API_H

#include <sst/core/subcomponent.h>
#include <sst/core/link.h>
#include <sst/elements/memHierarchy/memEvent.h>

namespace SST {
namespace Carcosa {

class InterceptionAgentAPI : public SST::SubComponent
{
public:
    SST_ELI_REGISTER_SUBCOMPONENT_API(SST::Carcosa::InterceptionAgentAPI)

    InterceptionAgentAPI(ComponentId_t id, Params& params) : SubComponent(id) {}
    virtual ~InterceptionAgentAPI() {}

    /** Called when Hali intercepts a MemEvent whose address falls in a registered range.
     * The agent must produce and send the response (via the provided highlink).
     * Returns true if the agent handled the event (Hali will not forward it). */
    virtual bool handleInterceptedEvent(SST::MemHierarchy::MemEvent* ev, SST::Link* highlink) = 0;

    /** Called when a partner Hali signals "done" via the ring. Not all agents need this. */
    virtual void notifyPartnerDone(unsigned iteration) {}

    /** Called during setup phase to allow the agent to initialize. */
    virtual void agentSetup() {}

    /** Optional: set the Hali ring link for sending "done" to partner. Default no-op. */
    virtual void setRingLink(SST::Link* leftLink) { (void)leftLink; }

    /** Optional: set the base address of the intercepted region (e.g. first range). Default no-op. */
    virtual void setInterceptBase(uint64_t base) { (void)base; }

    /** Optional: set the highlink for sending responses (e.g. command read response). Default no-op. */
    virtual void setHighlink(SST::Link* highlink) { (void)highlink; }

    InterceptionAgentAPI() {}
    ImplementVirtualSerializable(SST::Carcosa::InterceptionAgentAPI);
};

} // namespace Carcosa
} // namespace SST

#endif /* CARCOSA_INTERCEPTIONAGENT_API_H */
