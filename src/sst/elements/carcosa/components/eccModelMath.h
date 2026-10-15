#ifndef SST_ELEMENTS_CARCOSA_ECC_MODEL_MATH_H
#define SST_ELEMENTS_CARCOSA_ECC_MODEL_MATH_H

#include "sst/elements/carcosa/components/eccScheme.h"
#include <algorithm>
#include <cstdint>
#include <string>

namespace SST { namespace Carcosa { namespace EccModelMath {

inline double fitEventRate(double fit_per_mbit_per_hour, double dram_mb,
                           double sim_time_per_event_ns) {
    if (fit_per_mbit_per_hour <= 0.0 || dram_mb <= 0.0 ||
        sim_time_per_event_ns <= 0.0) return 0.0;
    const double failures_per_hour = fit_per_mbit_per_hour * 1e-9 * dram_mb * 8.0;
    return std::min(1.0, failures_per_hour * sim_time_per_event_ns / 3.6e12);
}

inline double jedecEventRate(double explicit_rate, double ber,
                             uint32_t payload_bytes) {
    return explicit_rate > 0.0
        ? explicit_rate
        : ber * static_cast<double>(payload_bytes) * 8.0;
}

inline uint32_t wordCount(uint32_t payload_bytes, EccScheme scheme) {
    const uint32_t wb = eccWordBytes(scheme);
    if (wb == 0 || payload_bytes == 0) return payload_bytes ? 1u : 0u;
    return (payload_bytes + wb - 1) / wb;
}

inline uint32_t wordBits(uint32_t payload_bytes, EccScheme scheme,
                         uint32_t word_index) {
    const uint32_t wb = eccWordBytes(scheme);
    if (wb == 0) return word_index == 0 ? payload_bytes * 8 : 0;
    const uint64_t used = static_cast<uint64_t>(word_index) * wb;
    if (used >= payload_bytes) return 0;
    return static_cast<uint32_t>(std::min<uint64_t>(wb, payload_bytes - used) * 8);
}

inline bool resetCampaignEntry(const std::string& observed,
                               std::string& previous, uint64_t& count) {
    if (observed == previous) return false;
    previous = observed;
    count = 0;
    return true;
}

}}} // namespace SST::Carcosa::EccModelMath

#endif
