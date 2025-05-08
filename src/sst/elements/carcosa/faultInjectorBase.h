// Copyright 2009-2025 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2025, NTESS
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef SST_ELEMENTS_CARCOSA_FAULTINJECTORBASE_H
#define SST_ELEMENTS_CARCOSA_FAULTINJECTORBASE_H

#include "sst/core/component.h"
#include "sst/core/event.h"

namespace SST::FaultInjectorBase {

/**
 * Base class containing required virtual functions and basic required data for
 * creating fault injection on component ports
 */
class FaultInjectorBase : public SST::PortModule
{
public:
    SST_ELI_REGISTER_PORTMODULE(
        FaultInjectorBase,
        "carcosa",
        "faultInjectorBase",
        SST_ELI_ELEMENT_VERSION(0, 1, 0),
        "Barebones PortModule used to connect fault injection logic to components"
    )

    SST_ELI_DOCUMENT_PARAMS(
        //
    )

    FaultInjectorBase(Params& params);

    FaultInjectorBase() = dafault;
    ~FaultInjectorBase() {}

    void eventSent(uintptr_t key, Event*& ev) override;
    void interceptHandler(uintptr_t key, Event*& data, bool& cancel) override;

    bool installOnReceive() override;
    bool installOnSend() override;

private:
    void (*eventSentPtr)(uintptr_t, Event*&);
    void (*interceptHandlerPtr)(uintptr_t, Event*&, bool&);
}

} // namespace SST::FaultInjectorBase