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

#ifndef CARCOSA_SENSORCOMPONENT_H
#define CARCOSA_SENSORCOMPONENT_H

// SSTSnippet::component-header::start
#include <sst/core/component.h>
#include <sst/core/link.h>

// SSTSnippet::component-header::pause
namespace SST {
namespace Carcosa {


// SSTSnippet::component-header::start
class SensorComponent : public SST::Component
{
public:
// SSTSnippet::component-header::pause

    SST_ELI_REGISTER_COMPONENT(
        SensorComponent,
        "carcosa",
        "SensorComponent",
        SST_ELI_ELEMENT_VERSION(1,0,0),
        "Mimic sensor behavior in vehicle simulations",
        COMPONENT_CATEGORY_PROCESSOR
    )

    SST_ELI_DOCUMENT_PARAMS(
        { "eventsToSend",   "How many events this component should send to other component.", NULL},
        { "verbose",        "Set to true to print every event this component sends.", "false"}
    )

    SST_ELI_DOCUMENT_PORTS(
        {"ifl",  "Link from sensor to Hali (IFL)", { "carcosa.SensorEvent" } },
    )

// SSTSnippet::component-header::start
    SensorComponent(SST::ComponentId_t id, SST::Params& params);
// SSTSnippet::component-header::pause

// SSTSnippet::component-header::start
    virtual ~SensorComponent();

// SSTSnippet::component-header::pause
    virtual void init(unsigned phase) override;
    virtual void setup() override;
    virtual void complete(unsigned phase) override;
    virtual void finish() override;
    virtual void emergencyShutdown() override;
    virtual void printStatus(Output& out) override;

    void handleSensorEvent(SST::Event *ev);


// SSTSnippet::component-header::start
private:
    bool mainTick(SST::Cycle_t cycle);
    unsigned eventsToSend;
    unsigned eventsReceived;
    unsigned eventsForwarded;
// SSTSnippet::component-header::pause
    bool verbose;
// SSTSnippet::component-header::start

    unsigned eventsSent;
    std::string iflMsg;

    SST::Output* out;

    SST::Link* iflLink;
};
// SSTSnippet::component-header::end

} // namespace Carcosa
} // namespace SST

#endif // CARCOSA_SENSORCOMPONENT_H
