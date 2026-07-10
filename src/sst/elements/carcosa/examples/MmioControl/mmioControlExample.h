// Copyright 2009-2026 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2026, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory of the distribution.
//
// This file is part of the SST software package. For license information,
// see the LICENSE file in the top level directory of the distribution.

#ifndef CARCOSA_MMIO_CONTROL_EXAMPLE_H
#define CARCOSA_MMIO_CONTROL_EXAMPLE_H

#include <sst/core/component.h>
#include <sst/core/output.h>
#include <sst/core/interfaces/stdMem.h>
#include "sst/elements/carcosa/components/interceptionAgentAPI.h"
#include <cstdint>
#include <vector>

namespace SST {
namespace Carcosa {

/**
 * Control agent: Deferred read@value until write@arm completes the parked read.
 */
class ExampleControlAgent : public InterceptionAgentAPI {
public:
    SST_ELI_REGISTER_SUBCOMPONENT(
        ExampleControlAgent, "carcosa", "ExampleControlAgent",
        SST_ELI_ELEMENT_VERSION(1, 0, 0),
        "Minimal handleControlAccess example agent (read parks until armed)",
        SST::Carcosa::InterceptionAgentAPI)

    static constexpr uint64_t kValueOffset = 0x00; // R: armed value (blocks until armed)
    static constexpr uint64_t kArmOffset   = 0x04; // W: arm a value, completes a parked read

    ExampleControlAgent(ComponentId_t id, Params& params)
        : InterceptionAgentAPI(id, params) {}
    ExampleControlAgent() : InterceptionAgentAPI() {}

    ControlResult handleControlAccess(ControlAccess& acc) override {
        if (!acc.isWrite) {
            if (acc.offset == kValueOffset) {
                if (armed_) { acc.readValue = value_; return ControlResult::Handled; }
                parked_ = true;
                return ControlResult::Deferred;
            }
            return ControlResult::Ignored;
        }
        if (acc.offset == kArmOffset) {
            value_ = acc.value;
            armed_ = true;
            if (parked_) {
                parked_ = false;
                if (channel_) channel_->completePendingRead(value_);
            }
            return ControlResult::Handled;
        }
        return ControlResult::Ignored;
    }

    void setControlChannel(ControlChannel* ch) override { channel_ = ch; }

    bool handleInterceptedEvent(SST::MemHierarchy::MemEvent* ev,
                                SST::Link* highlink) override {
        (void)ev; (void)highlink; return false;
    }

private:
    ControlChannel* channel_ = nullptr;
    uint32_t value_  = 0;
    bool     armed_  = false;
    bool     parked_ = false;
};

/**
 * Scripted MMIO requestor: park read -> arm write -> verify (watchdog on stall).
 */
class ExampleMmioDriver : public SST::Component {
public:
    SST_ELI_REGISTER_COMPONENT(
        ExampleMmioDriver, "carcosa", "ExampleMmioDriver",
        SST_ELI_ELEMENT_VERSION(1, 0, 0),
        "Scripted StandardMem requestor that tests an MMIO control peripheral",
        COMPONENT_CATEGORY_UNCATEGORIZED)

    SST_ELI_DOCUMENT_PARAMS(
        {"clock",     "Clock frequency.",                       "1GHz"},
        {"mmio_base", "Base address of the MMIO peripheral.",   "0xBEEF0000"},
        {"arm_value", "Value to arm and expect on round-trip.", "43981"})

    SST_ELI_DOCUMENT_SUBCOMPONENT_SLOTS(
        {"mem_iface", "StandardMem interface to the MMIO peripheral",
         "SST::Interfaces::StandardMem"})

    ExampleMmioDriver(ComponentId_t id, Params& params);
    ~ExampleMmioDriver();

    void init(unsigned phase) override;
    void setup() override;
    void finish() override;

private:
    bool tick(SST::Cycle_t cycle);
    void handleResponse(SST::Interfaces::StandardMem::Request* req);
    void sendRead(uint64_t offset);
    void sendWrite(uint64_t offset, uint32_t value);

    class RespHandler : public SST::Interfaces::StandardMem::RequestHandler {
    public:
        RespHandler(ExampleMmioDriver* d, SST::Output* o)
            : SST::Interfaces::StandardMem::RequestHandler(o), drv_(d) {}
        void handle(SST::Interfaces::StandardMem::ReadResp* resp) override;
        void handle(SST::Interfaces::StandardMem::WriteResp* resp) override;
    private:
        ExampleMmioDriver* drv_;
    };

    SST::Output*                  out_         = nullptr;
    SST::Interfaces::StandardMem* iface_       = nullptr;
    RespHandler*                  respHandler_ = nullptr;

    uint64_t mmioBase_     = 0xBEEF0000;
    uint32_t armValue_     = 0xABCD;
    int      step_         = 0;
    int      waitCycles_   = 0;
    bool     gotReadResp_  = false;
    uint32_t readValue_    = 0;
};

} // namespace Carcosa
} // namespace SST

#endif // CARCOSA_MMIO_CONTROL_EXAMPLE_H
