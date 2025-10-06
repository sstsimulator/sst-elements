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

#ifndef SST_ELEMENTS_CARCOSA_RANDOMDROPFAULTINJECTOR_H
#define SST_ELEMENTS_CARCOSA_RANDOMDROPFAULTINJECTOR_H

#include "sst/elements/carcosa/faultInjectorBase.h"

namespace SST::Carcosa {

class RandomDropFaultInjector : public FaultInjectorBase {
public:
    SST_ELI_REGISTER_PORTMODULE(
        FaultInjectorBase,
        "carcosa",
        "RandomFlipFaultInjector",
        SST_ELI_ELEMENT_VERSION(0, 1, 0),
        "PortModule class used to simulate a data transfer lost at random"
    )

    SST_ELI_DOCUMENT_PARAMS(
    )

    RandomDropFaultInjector(Params& params);

    RandomDropFaultInjector() = default;
    ~RandomDropFaultInjector() {}
protected:
    //
}; // class RandomDropFaultInjector
    
} // namespace SST::Carcosa

#endif