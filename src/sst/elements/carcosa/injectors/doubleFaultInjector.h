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

#include "sst/elements/carcosa/faultInjectorBase.h"
#include <random>

namespace SST::Carcosa {

class DoubleFaultInjector : public FaultInjectorBase {
public:
    SST_ELI_REGISTER_PORTMODULE(
        DoubleFaultInjector,
        "carcosa",
        "DoubleFaultInjector",
        SST_ELI_ELEMENT_VERSION(0, 1, 0),
        "PortModule class used to simulate a data transfer lost at random OR a random bit flip in transit"
    )

    SST_ELI_DOCUMENT_PARAMS(
        {"flipVsDropProb", "The probability that a flip will be chosen over a drop when a fault is injected"}
    )

    DoubleFaultInjector(Params& params);

    DoubleFaultInjector() = default;
    ~DoubleFaultInjector() {}
protected:
    double flipVsDropProb_;

    void executeFaults(Event*& ev) override;

    SST::RNG::MersenneRNG rng_;

    void serialize_order(SST::Core::Serialization::serializer& ser) override
    {
        SST::PortModule::serialize_order(ser);
        // serialize parameters like `SST_SER(<param_member>)`
        SST_SER(flipVsDropProb_);
        SST_SER(rng_);
    }
    ImplementVirtualSerializable(SST::Carcosa::DoubleFaultInjector)
}; // class DoubleFaultInjector

} // namespace SST::Carcosa

#endif