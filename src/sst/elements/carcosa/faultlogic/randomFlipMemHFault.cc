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

#include "sst/elements/carcosa/faultlogic/randomFlipMemHFault.h"
#include "sst/elements/memHierarchy/memEvent.h"

using namespace SST::Carcosa;

RandomFlipMemHFault::RandomFlipMemHFault(Params& params, FaultInjectorBase* injector)
    : RandomFlipFault(params, injector) {}

bool RandomFlipMemHFault::faultLogic(Event*& ev) {
    auto* mem_ev = dynamic_cast<SST::MemHierarchy::MemEvent*>(ev);
    if (!mem_ev) return false;

    dataVec& payload = mem_ev->getPayload();
    if (payload.empty()) return false;

    std::pair<uint32_t, uint32_t> lucky_number = pickByteAndBit(payload.size());
    uint8_t byte = payload[lucky_number.first];
    uint8_t mask = static_cast<uint8_t>(1) << lucky_number.second;
    payload[lucky_number.first] = byte ^ mask;
    return true;
}
