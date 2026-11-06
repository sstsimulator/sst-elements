// Copyright 2009-2026 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.

#ifndef SST_ELEMENTS_CARCOSA_CONFIG_PARSE_H
#define SST_ELEMENTS_CARCOSA_CONFIG_PARSE_H

#include <cmath>
#include <cstdint>
#include <string>

namespace SST::Carcosa::ConfigParse {

inline bool parseDouble(const std::string& text, double& value) {
    try {
        size_t parsed = 0;
        value = std::stod(text, &parsed);
        return parsed == text.size() && std::isfinite(value);
    } catch (...) {
        return false;
    }
}

inline bool parseUint64(const std::string& text, uint64_t& value) {
    if (text.empty() || text.front() == '-') return false;
    try {
        size_t parsed = 0;
        value = std::stoull(text, &parsed, 0);
        return parsed == text.size();
    } catch (...) {
        return false;
    }
}

inline bool parseInt(const std::string& text, int& value) {
    try {
        size_t parsed = 0;
        value = std::stoi(text, &parsed, 0);
        return parsed == text.size();
    } catch (...) {
        return false;
    }
}

inline bool isProbability(double value) {
    return value >= 0.0 && value <= 1.0;
}

} // namespace SST::Carcosa::ConfigParse

#endif
