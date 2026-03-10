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

#ifndef SST_ELEMENTS_CARCOSA_RANDOMFLIPFAULTINJECTOR_H
#define SST_ELEMENTS_CARCOSA_RANDOMFLIPFAULTINJECTOR_H

#include "sst/elements/carcosa/injectors/faultInjectorBase.h"

namespace SST::Carcosa {

class RandomFlipFaultInjector : public FaultInjectorBase {
public:
    SST_ELI_REGISTER_PORTMODULE(
        RandomFlipFaultInjector,
        "carcosa",
        "RandomFlipFaultInjector",
        SST_ELI_ELEMENT_VERSION(0, 1, 0),
        "PortModule class used to simulate a random bit flip when transferring data"
    )

    SST_ELI_DOCUMENT_PARAMS(
        {"injection_probability", "Probability for fault injection to trigger. Default = 0.0"}
    )

    RandomFlipFaultInjector(Params& params);

    RandomFlipFaultInjector() = default;
    ~RandomFlipFaultInjector() {}
protected:
    double injection_probability_;


    bool doInjection() override;
    void executeFaults(Event*& ev) override;

    void serialize_order(SST::Core::Serialization::serializer& ser) override
    {
        SST::PortModule::serialize_order(ser);
        // serialize parameters like `SST_SER(<param_member>)`
        SST_SER(injection_probability_);
    }
    ImplementVirtualSerializable(SST::Carcosa::RandomFlipFaultInjector)
}; // class RandomFlipFaultInjector

} // namespace SST::Carcosa

#endif