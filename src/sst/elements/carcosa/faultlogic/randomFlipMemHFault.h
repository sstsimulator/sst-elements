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

#ifndef SST_ELEMENTS_CARCOSA_RANDOMFLIPMEMHFAULT_H
#define SST_ELEMENTS_CARCOSA_RANDOMFLIPMEMHFAULT_H

#include "sst/elements/carcosa/faultlogic/randomFlipFault.h"

namespace SST::Carcosa {

/**
 * MemHierarchy-aware variant of RandomFlipFault.
 * Safely skips events that are not MemEvents or carry no payload
 * (e.g. GetS requests, Inv, AckInv), avoiding the modulo-by-zero
 * that occurs when pickByteAndBit receives payload_sz == 0.
 */
class RandomFlipMemHFault : public RandomFlipFault {
public:
    RandomFlipMemHFault(Params& params, FaultInjectorBase* injector);

    RandomFlipMemHFault() = default;
    ~RandomFlipMemHFault() {}

    bool faultLogic(Event*& ev) override;

protected:
    void serialize_order(SST::Core::Serialization::serializer& ser) override {
        RandomFlipFault::serialize_order(ser);
    }
    ImplementVirtualSerializable(RandomFlipMemHFault)
};

} // namespace SST::Carcosa

#endif // SST_ELEMENTS_CARCOSA_RANDOMFLIPMEMHFAULT_H
