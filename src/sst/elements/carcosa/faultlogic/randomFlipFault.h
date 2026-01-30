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

#include "sst/elements/carcosa/faultlogic/faultBase.h"

namespace SST::Carcosa {

class RandomFlipFault : public FaultBase {
public:
    RandomFlipFault(Params& params, FaultInjectorBase* injector);

    RandomFlipFault() = default;
    ~RandomFlipFault() {}

    bool faultLogic(Event*& ev) override;
protected:
    /**
     * Randomly choose which bit in which byte to flip
     * @return (byte, bit)
     */
    inline std::pair<uint32_t, uint32_t> pickByteAndBit(size_t payload_sz);
protected:
    void serialize_order(SST::Core::Serialization::serializer& ser) override {
        FaultBase::serialize_order(ser);
    }
    ImplementVirtualSerializable(RandomFlipFault)
}; // RandomFlipFault
}

#endif // SST_ELEMENTS_CARCOSA_RANDOMFLIPFAULT_H