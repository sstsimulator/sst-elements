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

#ifndef SST_ELEMENTS_CARCOSA_STUCKATFAULT_H
#define SST_ELEMENTS_CARCOSA_STUCKATFAULT_H

#include "sst/elements/carcosa/faultInjectorBase.h"
#include <map>
#include <utility>

namespace SST::Carcosa {

class StuckAtFault : public FaultInjectorBase::FaultBase
{
public:

    StuckAtFault(Params& params, FaultInjectorBase* injector);

    StuckAtFault() = default;
    ~StuckAtFault() {}

    /**
     * Read event payload and perform the following:
     *  - If stuckAtMap.at(addr) exists, compare all listed bits with payload value
     *  - If payload value does not match mapped value, add bit to flip mask
     *  - Once all stored bit values have been compared, use flip mask to modify address data
     */
    void faultLogic(Event*& ev) override;
protected:

    // map of addr->{bit, value} for saving stuck bit values
    std::map<SST::MemHierarchy::Addr, std::vector<std::pair<int, bool>>> stuckAtMap;
};

} // namespace SST::Carcosa

#endif // SST_ELEMENTS_CARCOSA_STUCKATFAULT_H