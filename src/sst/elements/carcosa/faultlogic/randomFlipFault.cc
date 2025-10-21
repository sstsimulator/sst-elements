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

#include "sst/elements/carcosa/faultlogic/randomFlipFault.h"

using namespace SST::Carcosa;

RandomFlipFault::RandomFlipFault(Params& params, FaultInjectorBase* injector) : FaultBase(params, injector) {
    this->int_distribution = uniform_int_distribution<uint32_t>(0,8);
}

bool RandomFlipFault::faultLogic(Event*& ev) {
    // check if this is the proper event type and get payload if it is
    dataVec payload = getMemEventPayload(ev);
    std::pair<uint32_t, uint32_t> lucky_number = pickByteAndBit();
    uint8_t byte = payload[lucky_number.first];
    uint8_t mask = static_cast<uint8_t>(1) << (lucky_number.second);
    payload[lucky_number.first] = byte ^ mask;
    setMemEventPayload(ev, payload);
    return true;
}

inline std::pair<uint32_t, uint32_t> RandomFlipFault::pickByteAndBit() {
    uint32_t byte = int_distribution(int_generator);
    uint32_t bit = int_distribution(int_generator);
#ifdef __SST_DEBUG_OUTPUT__
    getSimulationDebug()->debug(CALL_INFO_LONG, 1, 0, "Flipping bit %d in byte %d.\n", (int)bit, (int)byte);
#endif
    return make_pair(byte, bit);
}