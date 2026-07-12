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

#include <sst/elements/carcosa/components/configParse.h>
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
        uint64_t base = 0, size = 0;
        if (!ConfigParse::parseUint64(parts[1], base) ||
            !ConfigParse::parseUint64(parts[2], size)) {
            if (log) log->fatal(CALL_INFO, -1,
                                "%s: invalid region entry '%s'.\n", who,
                                entry.c_str());
            return published;
        }

        s->publishRegion(slot, base, size, name);
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
    return s->consumeFrameAbort();
}

} // namespace Carcosa
} // namespace SST

#endif /* CARCOSA_VLA_REGIONS_H */
