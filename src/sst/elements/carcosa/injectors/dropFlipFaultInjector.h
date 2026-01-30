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

#ifndef SST_ELEMENTS_CARCOSA_DOUBLEFAULTINJECTOR_H
#define SST_ELEMENTS_CARCOSA_DOUBLEFAULTINJECTOR_H

#include "sst/elements/carcosa/injectors/faultInjectorBase.h"
#include <random>

namespace SST::Carcosa {

class DropFlipFaultInjector : public FaultInjectorBase {
public:
    SST_ELI_REGISTER_PORTMODULE(
        DropFlipFaultInjector,
        "carcosa",
        "DropFlipFaultInjector",
        SST_ELI_ELEMENT_VERSION(0, 1, 0),
        "PortModule class used to simulate a data transfer lost at random and a random bit flip in transit"
    )

    SST_ELI_DOCUMENT_PARAMS(
        {"drop_probability", "The probability that a drop will be injected. Default = 0.0"},
        {"flip_probability", "The probability that a flip will be injected. Default = 0.0"}
    )

    DropFlipFaultInjector(Params& params);

    DropFlipFaultInjector() = default;
    ~DropFlipFaultInjector() {}
protected:
    double drop_probability_;
    double flip_probability_;
    // Byte array representing triggered fault. First is drop, second is flip.
    std::array<bool, 2> triggered_injection_;

    bool doInjection() override;
    void executeFaults(Event*& ev) override;

    void serialize_order(SST::Core::Serialization::serializer& ser) override
    {
        SST::PortModule::serialize_order(ser);
        // serialize parameters like `SST_SER(<param_member>)`
        SST_SER(drop_probability_);
        SST_SER(flip_probability_);
        SST_SER(triggered_injection_);
    }
    ImplementVirtualSerializable(SST::Carcosa::DoubleFaultInjector)
}; // class DoubleFaultInjector

} // namespace SST::Carcosa

#endif