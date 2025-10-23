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
    //
}

bool RandomFlipFault::faultLogic(Event*& ev) {
    // check if this is the proper event type and get payload if it is
    dataVec payload = getMemEventPayload(ev);
    std::pair<uint32_t, uint32_t> lucky_number = pickByteAndBit(payload.size());
    uint8_t byte = payload[lucky_number.first];
    uint8_t mask = static_cast<uint8_t>(1) << (lucky_number.second);
    payload[lucky_number.first] = byte ^ mask;
    setMemEventPayload(ev, payload);
    return true;
}

inline std::pair<uint32_t, uint32_t> RandomFlipFault::pickByteAndBit(size_t payload_sz) {
    uint32_t byte = injector_->randUInt32(0, payload_sz);
    uint32_t bit = injector_->randUInt32(0, 8);
#ifdef __SST_DEBUG_OUTPUT__
    getSimulationDebug()->debug(CALL_INFO_LONG, 1, 0, "Flipping bit %u in byte %u.\n", (uint32_t)bit, (uint32_t)byte);
#endif
    return make_pair(byte, bit);
}