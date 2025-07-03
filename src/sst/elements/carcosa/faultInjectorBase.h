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
#include "sst/elements/memHierarchy/memEvent.h"
#include <map>
#include <utility>

namespace SST::Carcosa {

// NOTE: currently unsure if BOTH is actually valid
enum installDirection {
    Send = 0,
    Receive,
    //Both,
    Invalid
};

// Enum to select basic fault injection logic or indicate a custom input
enum injectorLogic {
    StuckAt = 0,
    RandomFlip,
    RandomDrop,
    CorruptMemRegion,
    Custom
};

/**
 * Base class containing required functions and basic data for
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
        {"installDirection", "Flag which direction the injector should read from on a port. Valid optins are \'Send\', \'Receive\', and \'Both\'. Default is \'Receive\'."},
        {"injectionProbability", "The probability with which an injection should occur. Valid inputs range from 0 to 1. Default = 0.5."},
    )

    FaultInjectorBase(Params& params);

    FaultInjectorBase() = default;
    ~FaultInjectorBase() {}

    void eventSent(uintptr_t key, Event*& ev) override;
    void interceptHandler(uintptr_t key, Event*& data, bool& cancel) override;

    bool installOnReceive() override
    {
        switch (installDirection_) {
            case Send:
                return false;
            case Receive:
            //case Both:
            default:
                return true;
        }
    }
    bool installOnSend() override
    {
        switch (installDirection_) {
            case Send:
            //case Both:
                return true;
            case Receive:
            default:
                return false;
        }
    }

private:
    void (SST::Carcosa::FaultInjectorBase::* faultLogic)(Event*&);

    installDirection installDirection_ = installDirection::Receive;
    double injectionProbability_ = 0.5;

    void serialize_order(SST::Core::Serialization::serializer& ser) override
    {
        SST::PortModule::serialize_order(ser);
        // serialize parameters like `SST_SER(<param_member>)`
        SST_SER(installDirection_);
        SST_SER(injectionProbability_);
    }
    ImplementSerializable(SST::Carcosa::FaultInjectorBase)
};

} // namespace SST::FaultInjectorBase

#endif // SST_ELEMENTS_CARCOSA_FAULTINJECTORBASE_H