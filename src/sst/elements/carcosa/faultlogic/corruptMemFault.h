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

#ifndef SST_ELEMENTS_CARCOSA_CORRUPTMEMFAULT_H
#define SST_ELEMENTS_CARCOSA_CORRUPTMEMFAULT_H

#include "sst/elements/carcosa/faultInjectorBase.h"
#include <vector>
#include <utility>
#include <string>
#include <sstream>
#include <random>

namespace SST::Carcosa {

typedef std::vector<uint8_t> dataVec;
typedef SST::MemHierarchy::Addr Addr;

/**
 * This fault is intended to be placed on the input/output ports 
 * of memory components such as DRAM or HBM. Events that pass through
 * it, and whose data addresses fall within the ranges set in this 
 * module's parameters, will have their payloads randomly altered
 * to simulate corruption in the affected region of memory.
 */
class CorruptMemFault : public FaultInjectorBase::FaultBase 
{
public:

    CorruptMemFault(Params& params, FaultInjectorBase* injector);

    CorruptMemFault() = default;
    ~CorruptMemFault() {}

    /**
     * 1. Read in event
     * 2. Test if event is in specified region
     * 3. Corrupt event payload if necessary
     * 4. Replace payload
     */
    void faultLogic(Event*& ev) override;
protected:

    std::vector<std::pair<uint64_t, uint64_t>> corruptionRegions;

    std::default_random_engine generator;
    std::uniform_int_distribution<uint8_t> distribution;

    std::pair<uint64_t,uint64_t> convertString(std::string& region);
}; // CorruptMemFault
} // namespace SST::Carcosa

#endif // SST_ELEMENTS_CARCOSA_CORRUPTMEMFAULT_H