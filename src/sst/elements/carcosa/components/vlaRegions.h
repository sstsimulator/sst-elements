// Copyright 2009-2026 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2026, NTESS
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef CARCOSA_VLA_REGIONS_H
#define CARCOSA_VLA_REGIONS_H

// Publish labeled regions into PipelineStateRegistry for region-aware ECC.
// Slot 0 = MMIO control; CSV NAME:BASE:SIZE fills slots 1..N (hex or decimal).

#include <sst/core/output.h>
#include <sst/elements/carcosa/components/pipelineStateRegistry.h>
#include <cctype>
#include <cstdint>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace SST {
namespace Carcosa {

inline uint64_t parseUint64Token(const std::string& tok) {
    if (tok.empty()) return 0;
    try {
        if (tok.size() > 2 && (tok.substr(0, 2) == "0x" || tok.substr(0, 2) == "0X"))
            return std::stoull(tok, nullptr, 16);
        return std::stoull(tok, nullptr, 10);
    } catch (...) {
        return 0;
    }
}

inline void trimRegionTok(std::string& s) {
    size_t b = 0;
    while (b < s.size() && std::isspace(static_cast<unsigned char>(s[b]))) ++b;
    size_t e = s.size();
    while (e > b && std::isspace(static_cast<unsigned char>(s[e - 1]))) --e;
    s = s.substr(b, e - b);
}

/**
 * Publish CSV regions into slots 1..N for stateKey; creates snapshot if missing.
 */
inline int publishUserRegions(const std::string& stateKey,
                              const std::string& csv,
                              SST::Output* log,
                              const char* who) {
    if (stateKey.empty() || csv.empty()) return 0;

    PipelineStateBase* s =
        PipelineStateRegistry<PipelineStateBase>::getOrCreate(stateKey);

    int published = 0;
    int slot = 1;  // slot 0 reserved for mmio_control
    std::stringstream ss(csv);
    std::string entry;
    while (std::getline(ss, entry, ',')) {
        trimRegionTok(entry);
        if (entry.empty()) continue;

        std::vector<std::string> parts;
        {
            std::stringstream es(entry);
            std::string p;
            while (std::getline(es, p, ':')) {
                trimRegionTok(p);
                parts.push_back(p);
            }
        }
        if (parts.size() != 3) {
            if (log) log->output("%s: regions CSV: malformed entry '%s' (expected name:base:size)\n",
                                  who, entry.c_str());
            continue;
        }

        const std::string& name = parts[0];
        uint64_t base = parseUint64Token(parts[1]);
        uint64_t size = parseUint64Token(parts[2]);

        s->ensureRegionSlot(slot);
        s->regions[slot].base  = base;
        s->regions[slot].size  = size;
        s->regions[slot].valid = (size > 0);
        s->regions[slot].id    = slot;
        s->regions[slot].name  = name;
        ++slot;
        ++published;
    }
    return published;
}

/** Pop frameAbortRequested once so one DUE => one frame drop. */
inline bool consumeFrameAbort(const std::string& stateKey) {
    if (stateKey.empty()) return false;
    PipelineStateBase* s =
        PipelineStateRegistry<PipelineStateBase>::getMutable(stateKey);
    if (!s) return false;
    if (!s->frameAbortRequested) return false;
    s->frameAbortRequested = false;
    s->framesDropped += 1;
    return true;
}

} // namespace Carcosa
} // namespace SST

#endif /* CARCOSA_VLA_REGIONS_H */
