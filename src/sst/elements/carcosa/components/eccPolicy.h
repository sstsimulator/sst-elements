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

#ifndef SST_ELEMENTS_CARCOSA_ECC_POLICY_H
#define SST_ELEMENTS_CARCOSA_ECC_POLICY_H

#include "sst/elements/carcosa/components/configParse.h"
#include "sst/elements/carcosa/components/eccScheme.h"
#include <cctype>
#include <cstdint>
#include <map>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

namespace SST {
namespace Carcosa {

// Latencies in picoseconds; ber is per-bit error probability per access.
struct EccPolicyEntry {
    EccScheme scheme                    = EccScheme::NONE;
    double    ber                       = 0.0;
    uint64_t  correctable_latency_ps    = 0;
    uint64_t  due_latency_ps            = 0;
    uint64_t  escape_latency_ps         = 0;
    bool      inherits_uniform          = true;
};

// Precedence: (kernel,region) > region > kernel > uniform.
// kernel/region "*" or "" = any. CSV: KERNEL[:@REGION]:scheme:ber:c_ps:d_ps:e_ps
class EccPolicyTable {
public:
    EccPolicyTable() = default;

    void setUniform(const EccPolicyEntry& e) { uniform_ = e; uniform_.inherits_uniform = false; }
    const EccPolicyEntry& uniform() const { return uniform_; }

    // Walk every concrete table entry; callback gets a tag like "uniform",
    // "kernel=PREFILL", "region=KV_CACHE". Used by EccGuard::setup() BER warnings.
    template <typename Fn>
    void forEachEntry(Fn&& fn) const {
        fn(std::string("uniform"), uniform_);
        for (const auto& kv : per_kernel_) {
            fn(std::string("kernel=") + kv.first, kv.second);
        }
        for (const auto& kv : per_region_) {
            fn(std::string("region=") + kv.first, kv.second);
        }
        for (const auto& kv : per_kernel_region_) {
            fn(std::string("kernel@region=") + kv.first, kv.second);
        }
    }

    // Backwards-compatible kernel-only resolver. Region-unaware callers stay on this path.
    const EccPolicyEntry& effectiveFor(const std::string& kernel_name) const {
        if (kernel_name.empty()) return uniform_;
        auto it = per_kernel_.find(kernel_name);
        if (it == per_kernel_.end()) return uniform_;
        return it->second.inherits_uniform ? uniform_ : it->second;
    }

    // (kernel,region) > region > kernel > uniform.
    // region_name "" or unmatched falls through to kernel-only.
    const EccPolicyEntry& effectiveFor(const std::string& kernel_name,
                                       const std::string& region_name) const {
        if (!region_name.empty()) {
            if (!kernel_name.empty()) {
                auto it = per_kernel_region_.find(makeComboKey(kernel_name, region_name));
                if (it != per_kernel_region_.end()) return it->second;
            }
            auto rit = per_region_.find(region_name);
            if (rit != per_region_.end()) return rit->second;
        }
        return effectiveFor(kernel_name);
    }

    void setPerKernel(const std::string& kernel_name, const EccPolicyEntry& e) {
        if (kernel_name.empty()) return;
        EccPolicyEntry copy = e;
        copy.inherits_uniform = false;
        per_kernel_[kernel_name] = copy;
    }

    void setPerRegion(const std::string& region_name, const EccPolicyEntry& e) {
        if (region_name.empty()) return;
        EccPolicyEntry copy = e;
        copy.inherits_uniform = false;
        per_region_[region_name] = copy;
    }

    void setPerKernelRegion(const std::string& kernel_name, const std::string& region_name,
                            const EccPolicyEntry& e) {
        if (kernel_name.empty() || region_name.empty()) return;
        EccPolicyEntry copy = e;
        copy.inherits_uniform = false;
        per_kernel_region_[makeComboKey(kernel_name, region_name)] = copy;
    }

    // CSV per entry. See class doc for accepted forms.
    int parseCsv(const std::string& csv, std::vector<std::string>& errors) {
        int parsed = 0;
        if (csv.empty()) return 0;

        std::string buf;
        std::istringstream ss(csv);
        while (std::getline(ss, buf, ',')) {
            trim(buf);
            if (buf.empty()) continue;

            std::vector<std::string> parts = splitColon(buf);
            if (parts.size() < 2) {
                errors.push_back("ecc_kernel_policy: malformed entry '" + buf + "'");
                continue;
            }
            for (auto& p : parts) trim(p);

            // Tag may be KERNEL, KERNEL@REGION, *@REGION, or KERNEL@*.
            std::string kernel_tok;
            std::string region_tok;
            splitAt(parts[0], kernel_tok, region_tok);

            bool kernel_any = (kernel_tok == "*" || kernel_tok.empty());
            bool region_any = (region_tok == "*" || region_tok.empty());

            EccPolicyEntry e;
            e.inherits_uniform = false;

            if (parts.size() >= 2) {
                if (!eccSchemeFromString(parts[1], e.scheme)) {
                    errors.push_back("ecc_kernel_policy: unknown scheme '" + parts[1] + "' for '" + parts[0] + "'");
                    continue;
                }
            }
            if (parts.size() >= 3 &&
                (!ConfigParse::parseDouble(parts[2], e.ber) ||
                 !ConfigParse::isProbability(e.ber))) {
                errors.push_back("ecc_kernel_policy: invalid BER '" + parts[2] + "' for '" + parts[0] + "'");
                continue;
            }
            if (parts.size() >= 4 &&
                !ConfigParse::parseUint64(parts[3], e.correctable_latency_ps)) {
                errors.push_back("ecc_kernel_policy: invalid correctable latency '" + parts[3] + "'");
                continue;
            }
            if (parts.size() >= 5 &&
                !ConfigParse::parseUint64(parts[4], e.due_latency_ps)) {
                errors.push_back("ecc_kernel_policy: invalid DUE latency '" + parts[4] + "'");
                continue;
            }
            if (parts.size() >= 6 &&
                !ConfigParse::parseUint64(parts[5], e.escape_latency_ps)) {
                errors.push_back("ecc_kernel_policy: invalid escape latency '" + parts[5] + "'");
                continue;
            }
            if (parts.size() > 6) {
                errors.push_back("ecc_kernel_policy: too many fields in '" + buf + "'");
                continue;
            }

            if (!kernel_any && !region_any) {
                setPerKernelRegion(kernel_tok, region_tok, e);
            } else if (!region_any) {
                setPerRegion(region_tok, e);
            } else if (!kernel_any) {
                setPerKernel(kernel_tok, e);
            } else {
                errors.push_back("ecc_kernel_policy: '*@*' not allowed; use ecc_scheme/ber for the uniform fallback");
                continue;
            }
            ++parsed;
        }
        return parsed;
    }

private:
    EccPolicyEntry uniform_{};
    std::unordered_map<std::string, EccPolicyEntry> per_kernel_;
    std::map<std::string, EccPolicyEntry> per_region_;
    std::map<std::string, EccPolicyEntry> per_kernel_region_;

    static std::string makeComboKey(const std::string& kernel,
                                    const std::string& region) {
        return kernel + "@" + region;
    }

    static void splitAt(const std::string& tag, std::string& kernel,
                        std::string& region) {
        auto pos = tag.find('@');
        if (pos == std::string::npos) {
            kernel = tag;
            region.clear();
        } else {
            kernel = tag.substr(0, pos);
            region = tag.substr(pos + 1);
        }
    }

    static void trim(std::string& s) {
        size_t b = 0;
        while (b < s.size() && std::isspace(static_cast<unsigned char>(s[b]))) ++b;
        size_t e = s.size();
        while (e > b && std::isspace(static_cast<unsigned char>(s[e - 1]))) --e;
        s = s.substr(b, e - b);
    }

    static std::vector<std::string> splitColon(const std::string& s) {
        std::vector<std::string> out;
        std::string buf;
        std::istringstream ss(s);
        while (std::getline(ss, buf, ':')) out.push_back(buf);
        return out;
    }

};

} // namespace Carcosa
} // namespace SST

#endif /* SST_ELEMENTS_CARCOSA_ECC_POLICY_H */
