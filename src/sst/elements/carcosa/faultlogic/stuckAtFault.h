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

#ifndef SST_ELEMENTS_CARCOSA_STUCKATFAULT_H
#define SST_ELEMENTS_CARCOSA_STUCKATFAULT_H

#include "sst/elements/carcosa/faultlogic/faultBase.h"
#include <map>
#include <utility>
#include <vector>
#include <sstream>
#include <bitset>

namespace SST::Carcosa {

typedef std::vector<uint8_t> dataVec;
typedef SST::MemHierarchy::Addr Addr;

/** Force configured bits to stuck-at-0 / stuck-at-1 values. */
class StuckAtFault : public FaultBase
{
public:

    StuckAtFault(Params& params, FaultInjectorBase* injector);

    StuckAtFault() = default;
    ~StuckAtFault() {}

    bool faultLogic(Event*& ev) override;
protected:

    // map of addr->{byte, mask} for saving stuck bit values
    std::map<Addr, std::vector<std::pair<int, uint8_t>>> stuckAtZeroMask_;
    // add stuckAtOneMask
    std::map<Addr, std::vector<std::pair<int, uint8_t>>> stuckAtOneMask_;
    // false = little; true = big
    bool endianness_ = false;

    typedef struct maskParam {
        Addr addr;
        int byte;
        uint8_t zeroMask;
        uint8_t oneMask;
    } maskParam_t;

    std::vector<maskParam_t> convertString(std::vector<std::string>& paramVecStr);
    uint32_t computeByte(Addr addr, Addr base_addr, uint32_t byte);

    void serialize_order(SST::Core::Serialization::serializer& ser) override {
        FaultBase::serialize_order(ser);
        SST_SER(stuckAtZeroMask_);
        SST_SER(stuckAtOneMask_);
        SST_SER(endianness_);
    }
    ImplementVirtualSerializable(StuckAtFault)
};

} // namespace SST::Carcosa

#endif // SST_ELEMENTS_CARCOSA_STUCKATFAULT_H