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

#include "sst/elements/carcosa/faultlogic/faultBase.h"
#include <random>
#include <vector>
#include <utility>

namespace SST::Carcosa {

typedef std::vector<uint8_t> dataVec;

class RandomDropFault : public FaultBase {
public:
    RandomDropFault(Params& params, FaultInjectorBase* injector);

    RandomDropFault() = default;
    ~RandomDropFault() {}

    bool faultLogic(Event*& ev) override;
protected:
    void serialize_order(SST::Core::Serialization::serializer& ser) override {
        FaultBase::serialize_order(ser);
    }
    ImplementVirtualSerializable(RandomDropFault)
}; // RandomDropFault

} // namespace SST::Carcosa

#endif // SST_ELEMENTS_CARCOSA_RANDOMDROPFAULT_H