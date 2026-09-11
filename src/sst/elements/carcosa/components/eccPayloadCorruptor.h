// Copyright 2009-2026 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.

#ifndef SST_ELEMENTS_CARCOSA_ECC_PAYLOAD_CORRUPTOR_H
#define SST_ELEMENTS_CARCOSA_ECC_PAYLOAD_CORRUPTOR_H

#include "sst/elements/carcosa/components/eccScheme.h"

#include <sst/core/rng/mersenne.h>
#include <sst/elements/memHierarchy/memEvent.h>

#include <cstdint>
#include <string>
#include <vector>

namespace SST::Carcosa {

enum class EccPayloadDtype : uint8_t { Bytes, Bf16, Fp8, Int8 };

struct EccPayloadFlipCount {
    unsigned total = 0;
    unsigned high = 0;
    unsigned low = 0;
};

class EccPayloadCorruptor {
public:
    static bool parseDtype(const std::string& value, EccPayloadDtype& dtype);
    static const char* dtypeName(EccPayloadDtype dtype);

    static EccPayloadFlipCount flipRandom(
        SST::MemHierarchy::MemEvent& event, uint32_t word_index,
        EccScheme scheme, unsigned count, EccPayloadDtype dtype,
        SST::RNG::MersenneRNG& rng);

    static EccPayloadFlipCount flipExact(
        SST::MemHierarchy::MemEvent& event, uint32_t word_index,
        EccScheme scheme, const std::vector<uint32_t>& bits,
        EccPayloadDtype dtype);
};

} // namespace SST::Carcosa

#endif
