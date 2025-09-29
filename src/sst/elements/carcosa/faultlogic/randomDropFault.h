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

#ifndef SST_ELEMENTS_CARCOSA_RANDOMDROPFAULT_H
#define SST_ELEMENTS_CARCOSA_RANDOMDROPFAULT_H

#include "sst/elements/carcosa/faultInjectorBase.h"
#include <random>
#include <vector>

namespace SST::Carcosa {

class RandomDropFault : public FaultInjectorBase::FaultBase {
public:
    RandomDropFault(Params& params, FaultInjectorBase* injector);

    RandomDropFault() = default;
    ~RandomDropFault() {}

    void faultLogic(Event*& ev) override;
protected:
    std::default_random_engine generator;
    std::uniform_real_distribution<float> distribution;
}; // RandomDropFault

} // namespace SST::Carcosa

#endif // SST_ELEMENTS_CARCOSA_RANDOMDROPFAULT_H