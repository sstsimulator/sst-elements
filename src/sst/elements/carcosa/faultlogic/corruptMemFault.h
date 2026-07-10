// Copyright 2009-2026 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2026, NTESS
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef SST_ELEMENTS_CARCOSA_CORRUPTMEMFAULT_H
#define SST_ELEMENTS_CARCOSA_CORRUPTMEMFAULT_H

#include "sst/elements/carcosa/faultlogic/faultBase.h"
#include "sst/core/rng/mersenne.h"
#include <vector>
#include <utility>
#include <string>
#include <sstream>

namespace SST::Carcosa {

typedef std::vector<uint8_t> dataVec;
typedef SST::MemHierarchy::Addr Addr;

/** Corrupt payloads whose addresses fall in configured regions. */
class CorruptMemFault : public FaultBase
{
public:

    CorruptMemFault(Params& params, FaultInjectorBase* injector);

    CorruptMemFault() = default;
    ~CorruptMemFault() {}

    bool faultLogic(Event*& ev) override;

    std::vector<uint32_t>* checkAddrUsage(Event*& ev);
protected:

    std::vector<std::pair<uint64_t, uint64_t>> corruptionRegions_;

    std::vector<uint32_t> regionsToUse_;

    std::pair<uint64_t,uint64_t> convertString(std::string& region);

    int32_t computeStartIndex(Addr base_addr, size_t payload_sz, Addr region_start);
    int32_t computeEndIndex(Addr base_addr, size_t payload_sz, Addr region_end);

    void serialize_order(SST::Core::Serialization::serializer& ser) override {
        FaultBase::serialize_order(ser);
        SST_SER(corruptionRegions_);
        SST_SER(regionsToUse_);
    }
    ImplementVirtualSerializable(CorruptMemFault)
}; // CorruptMemFault
} // namespace SST::Carcosa

#endif // SST_ELEMENTS_CARCOSA_CORRUPTMEMFAULT_H