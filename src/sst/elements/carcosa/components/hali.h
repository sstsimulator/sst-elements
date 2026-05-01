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

#ifndef CARCOSA_HALI_COMPONENT_H
#define CARCOSA_HALI_COMPONENT_H

/**
 * Hali - interface layer between sensors/CPUs, the memory hierarchy (highlink/lowlink),
 * and other Hali instances on a ring. Provides fault-injection hooks via FaultInjManager.
 */

#include <cstdint>
#include <sst/core/component.h>
#include <sst/core/link.h>
#include "sst/elements/carcosa/components/carcosaMemCtrl.h"
#include "sst/elements/carcosa/components/faultInjManagerAPI.h"
#include "sst/elements/carcosa/components/interceptionAgentAPI.h"
#include <utility>
#include <vector>

namespace SST {
namespace MemHierarchy {
class MemEvent;
}

namespace Carcosa {

class Hali : public SST::Component {
public:
    SST_ELI_REGISTER_COMPONENT(
        Hali,
        "carcosa",
        "Hali",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "Interface layer for sensor data in vehicle simulations",
        COMPONENT_CATEGORY_PROCESSOR
    )

    SST_ELI_DOCUMENT_PARAMS(
        {"Sensors", "Number of SensorComponents this interface receives from.", NULL},
        {"CPUs", "Number of Compute components the Hali sends to.", NULL},
        {"verbose", "Enable verbose output for debugging.", "false"},
        {"intercept_ranges", "Semicolon-separated base,size pairs (e.g. '0xBEEF0000,4096') for addresses to hand to InterceptionAgent.", ""}
    )

    SST_ELI_DOCUMENT_SUBCOMPONENT_SLOTS(
        {"interceptionAgent", "Optional agent for intercepted memory accesses (e.g. carcosa.PingPongAgent). If unset, no interception.", "SST::Carcosa::InterceptionAgentAPI"}
    )

    SST_ELI_DOCUMENT_PORTS(
        {"sensor", "Link to SensorComponent", {"carcosa.SensorEvent"}},
        {"left", "Link to left Hali in ring", {"carcosa.HaliEvent"}},
        {"right", "Link to right Hali in ring", {"carcosa.HaliEvent"}},
        {"cpu", "Link to compute Component", {"carcosa.CpuEvent", "carcosa.FaultInjEvent"}},
        {"memCtrl", "Link to memory controller", {"carcosa.CpuEvent"}},
        {"highlink", "Link to memoryHierarchy (CPU/standardInterface side)", {}},
        {"lowlink", "Link to memoryHierarchy (Cache side)", {}}
    )

    Hali(SST::ComponentId_t id, SST::Params& params);
    virtual ~Hali();

    void init(unsigned phase) override;
    void setup() override;
    void complete(unsigned phase) override;
    void finish() override;
    void emergencyShutdown() override;
    void printStatus(Output& out) override;

    void handleHaliEvent(SST::Event* ev);
    void handleSensorEvent(SST::Event* ev);
    void handleCpuEvent(SST::Event* ev);
    void handleMemCtrlEvent(SST::Event* ev);
    void lowlinkMemEvent(SST::Event* ev);
    void highlinkMemEvent(SST::Event* ev);

    /** Returns true if addr falls within any registered intercept range. */
    bool isInterceptedAddress(uint64_t addr) const;

private:
    unsigned eventsToSend_;
    bool verbose_;

    unsigned eventsReceived_;
    unsigned eventsForwarded_;
    unsigned eventsSent_;
    unsigned cpuEventCount_;

    std::set<std::string> neighbors_;
    std::set<std::string>::iterator iter_;
    std::string leftHaliMsg_;
    std::string rightHaliMsg_;
    std::string cpuMsg_;
    std::string sensorMsg_;

    SST::Output* out_;

    SST::Link* leftHaliLink_;
    SST::Link* rightHaliLink_;
    SST::Link* sensorLink_;
    SST::Link* cpuLink_;
    SST::Link* memCtrlLink_;
    SST::Link* highlink_;
    SST::Link* lowlink_;

    FaultInjManagerAPI* faultInjManager_;
    InterceptionAgentAPI* interceptionAgent_;

    // Address ranges (base, end_exclusive) forwarded to the interception agent.
    std::vector<std::pair<uint64_t, uint64_t>> interceptRanges_;
};

} // namespace Carcosa
} // namespace SST

#endif // CARCOSA_HALI_COMPONENT_H
