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

#ifndef SST_ELEMENTS_CARCOSA_RANDOMFLIPFAULT_H
#define SST_ELEMENTS_CARCOSA_RANDOMFLIPFAULT_H

#include "sst/elements/carcosa/faultInjectorBase.h"

namespace SST::Carcosa {

class RandomFlipFault : public FaultInjectorBase::FaultBase {
public:
    RandomFlipFault(Params& params, FaultInjectorBase* injector);

    RandomFlipFault() = default;
    ~RandomFlipFault() {}

    void faultLogic(Event*& ev) override;
}; // RandomFlipFault
}

#endif // SST_ELEMENTS_CARCOSA_RANDOMFLIPFAULT_H