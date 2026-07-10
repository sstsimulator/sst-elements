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

#ifndef CARCOSA_INTERCEPTIONAGENT_API_H
#define CARCOSA_INTERCEPTIONAGENT_API_H

#include <sst/core/subcomponent.h>
#include <sst/core/link.h>
#include <sst/elements/memHierarchy/memEvent.h>
#include <sst/elements/memHierarchy/memTypes.h>
#include "sst/elements/carcosa/components/haliEvent.h"
#include "sst/elements/carcosa/components/hyadesProtocol.h"
#include "sst/elements/carcosa/components/ringProtocol.h"
#include <cinttypes>
#include <cstdio>
#include <cstdint>

namespace SST {
namespace Carcosa {

// Transport-neutral control access: data-plane MemEvents or MMIO StandardMem
// both normalize to ControlAccess -> handleControlAccess.

enum class ControlResult {
    Ignored,    // agent does not recognize this offset; hub applies default handling
    Handled,    // agent consumed it (read: readValue set; write: state updated)
    Deferred    // read parked; agent will complete it later via ControlChannel
};

struct ControlAccess {
    bool     isWrite   = false;  // in:  true for a store, false for a load
    uint64_t offset    = 0;      // in:  byte offset from the control-region base
    uint32_t value     = 0;      // in:  store payload (when isWrite)
    bool     posted    = false;  // in:  store needs no acknowledgement
    uint32_t readValue = 0;      // out: load result (when returning Handled)
};

// Hub handle to complete a previously Deferred read. At most one parked;
// hub answers on whichever transport delivered it.
class ControlChannel {
public:
    virtual ~ControlChannel() {}
    virtual void completePendingRead(uint32_t value) = 0;
};

class InterceptionAgentAPI : public SST::SubComponent
{
public:
    SST_ELI_REGISTER_SUBCOMPONENT_API(SST::Carcosa::InterceptionAgentAPI)

    InterceptionAgentAPI(ComponentId_t id, Params& params) : SubComponent(id) {}
    virtual ~InterceptionAgentAPI() {}

    /** Intercepted MemEvent: respond on highlink; return true if handled (not forwarded). */
    virtual bool handleInterceptedEvent(SST::MemHierarchy::MemEvent* ev, SST::Link* highlink) = 0;

    virtual void notifyPartnerDone(unsigned iteration) { (void)iteration; }

    // Default: RingTag::Done -> notifyPartnerDone. Accelerator agents override
    // for Cmd/SeqLen/Exit/Done (ringProtocol.h).
    virtual void handleRingEvent(SST::Carcosa::HaliEvent* ev) {
        if (ev && ev->getStr() == RingTag::Done) {
            notifyPartnerDone(ev->getNum());
        }
    }

    virtual void agentSetup() {}

    // Init-phase hook for agents that own a StandardMem iface (untimed handshake).
    // Default no-op for ring-only agents.
    virtual void agentInit(unsigned phase) { (void)phase; }

    virtual void setRingLink(SST::Link* leftLink) { (void)leftLink; }

    virtual void setInterceptBase(uint64_t base) { (void)base; }

    virtual void setHighlink(SST::Link* highlink) { (void)highlink; }

    // Receive the hub's control channel (for completing Deferred reads). Mirrors
    // the setHighlink/setRingLink injection pattern; default ignores it.
    virtual void setControlChannel(ControlChannel* ch) { (void)ch; }

    // Transport-neutral control hook. Default: not recognized (hub falls back
    // to legacy handling). Override for Hyades ABI on either transport.
    virtual ControlResult handleControlAccess(ControlAccess& acc) {
        (void)acc;
        return ControlResult::Ignored;
    }

    InterceptionAgentAPI() {}
    ImplementVirtualSerializable(SST::Carcosa::InterceptionAgentAPI);

protected:
    /** Log, delete ev, return true; no response (reads hang). For unreachable offsets only. */
    bool warnAndDropUnknownIntercept(SST::MemHierarchy::MemEvent* ev,
                                     uint64_t base) {
        uint64_t addr = static_cast<uint64_t>(ev->getAddr());
        uint64_t off  = addr - base;
        int cmdIdx = static_cast<int>(ev->getCmd());
        const char* cmdName = SST::MemHierarchy::CommandString[cmdIdx];
        fprintf(stderr,
                "[CARCOSA WARN] %s: unhandled intercepted access "
                "cmd=%s addr=0x%" PRIx64 " offset=+0x%" PRIx64
                " (event dropped, no response sent)\n",
                getName().c_str(), cmdName, addr, off);
        delete ev;
        return true;
    }
};

} // namespace Carcosa
} // namespace SST

#endif /* CARCOSA_INTERCEPTIONAGENT_API_H */
