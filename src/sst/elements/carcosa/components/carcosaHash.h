// Copyright 2009-2026 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2026, NTESS
// All rights reserved.

#ifndef SST_ELEMENTS_CARCOSA_CARCOSA_HASH_H
#define SST_ELEMENTS_CARCOSA_CARCOSA_HASH_H

#include <cstddef>
#include <cstdint>

namespace SST {
namespace Carcosa {

// Shared FNV-1a 64-bit for all carcosa checksum producers (do not fork).
// Chaining is concatenation: fnv1a64(b, fnv1a64(a)) == fnv1a64(a || b).
constexpr uint64_t kFnv1a64OffsetBasis = 14695981039346656037ull;
constexpr uint64_t kFnv1a64Prime       = 1099511628211ull;

inline uint64_t fnv1a64(const uint8_t* data, size_t len,
                        uint64_t seed = kFnv1a64OffsetBasis) {
    uint64_t h = seed;
    for (size_t i = 0; i < len; ++i) {
        h ^= static_cast<uint64_t>(data[i]);
        h *= kFnv1a64Prime;
    }
    return h;
}

} // namespace Carcosa
} // namespace SST

#endif /* SST_ELEMENTS_CARCOSA_CARCOSA_HASH_H */
