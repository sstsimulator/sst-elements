// Copyright 2009-2026 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.

#include "sst_config.h"
#include "sst/elements/carcosa/components/eccPayloadCorruptor.h"

#include <algorithm>
#include <set>

using namespace SST::Carcosa;

namespace {

unsigned dtypeBytes(EccPayloadDtype dtype)
{
    return dtype == EccPayloadDtype::Bf16 ? 2 : 1;
}

bool isHighBlastBit(EccPayloadDtype dtype, unsigned bit)
{
    switch (dtype) {
    case EccPayloadDtype::Bf16:
        return bit == 15 || bit == 13 || bit == 14;
    case EccPayloadDtype::Fp8:
        return bit == 7 || bit == 6;
    case EccPayloadDtype::Int8:
        return bit == 7;
    case EccPayloadDtype::Bytes:
        return false;
    }
    return false;
}

void countBlast(EccPayloadFlipCount& result, EccPayloadDtype dtype,
                uint32_t byte, uint32_t bit)
{
    bool high = dtype != EccPayloadDtype::Bytes &&
        isHighBlastBit(dtype, (byte % dtypeBytes(dtype)) * 8u + bit);
    if (high) ++result.high;
    else ++result.low;
    ++result.total;
}

} // namespace

bool EccPayloadCorruptor::parseDtype(const std::string& value,
                                     EccPayloadDtype& dtype)
{
    if (value == "bytes" || value == "BYTES") dtype = EccPayloadDtype::Bytes;
    else if (value == "bf16" || value == "BF16") dtype = EccPayloadDtype::Bf16;
    else if (value == "fp8" || value == "FP8") dtype = EccPayloadDtype::Fp8;
    else if (value == "int8" || value == "INT8") dtype = EccPayloadDtype::Int8;
    else return false;
    return true;
}

const char* EccPayloadCorruptor::dtypeName(EccPayloadDtype dtype)
{
    switch (dtype) {
    case EccPayloadDtype::Bytes: return "bytes";
    case EccPayloadDtype::Bf16:  return "bf16";
    case EccPayloadDtype::Fp8:   return "fp8";
    case EccPayloadDtype::Int8:  return "int8";
    }
    return "unknown";
}

EccPayloadFlipCount EccPayloadCorruptor::flipRandom(
    SST::MemHierarchy::MemEvent& event, uint32_t word_index,
    EccScheme scheme, unsigned count, EccPayloadDtype dtype,
    SST::RNG::MersenneRNG& rng)
{
    EccPayloadFlipCount result;
    auto& payload = event.getPayload();
    if (payload.empty() || count == 0) return result;

    uint32_t word_bytes = eccWordBytes(scheme);
    uint32_t start = word_bytes == 0 ? 0 : word_index * word_bytes;
    uint32_t end = word_bytes == 0
        ? payload.size() : std::min<uint32_t>(payload.size(), start + word_bytes);
    if (start >= end) return result;

    uint32_t span_bits = (end - start) * 8;
    count = std::min(count, span_bits);
    std::set<uint32_t> used;
    while (result.total < count) {
        uint32_t selected = rng.generateNextUInt32() % span_bits;
        if (!used.insert(selected).second) continue;
        uint32_t byte = start + selected / 8u;
        uint32_t bit = selected % 8u;
        payload[byte] ^= static_cast<uint8_t>(1u << bit);
        countBlast(result, dtype, byte, bit);
    }
    return result;
}

EccPayloadFlipCount EccPayloadCorruptor::flipExact(
    SST::MemHierarchy::MemEvent& event, uint32_t word_index,
    EccScheme scheme, const std::vector<uint32_t>& bits,
    EccPayloadDtype dtype)
{
    EccPayloadFlipCount result;
    auto& payload = event.getPayload();
    if (payload.empty() || bits.empty()) return result;

    uint32_t total_bits = payload.size() * 8;
    uint32_t word_bytes = eccWordBytes(scheme);
    uint32_t start = word_bytes == 0 ? 0 : word_index * word_bytes * 8;
    uint32_t end = word_bytes == 0
        ? total_bits : std::min(total_bits, start + word_bytes * 8);
    for (uint32_t selected : bits) {
        if (selected < start || selected >= end) continue;
        uint32_t byte = selected / 8u;
        uint32_t bit = selected % 8u;
        payload[byte] ^= static_cast<uint8_t>(1u << bit);
        countBlast(result, dtype, byte, bit);
    }
    return result;
}
