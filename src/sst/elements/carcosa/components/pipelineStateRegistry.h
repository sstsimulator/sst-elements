// Copyright 2009-2026 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2026, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory of the distribution.
//
// This file is part of the SST software package. For license information,
// see the LICENSE file in the top level directory of the distribution.

#ifndef SST_ELEMENTS_CARCOSA_PIPELINE_STATE_REGISTRY_H
#define SST_ELEMENTS_CARCOSA_PIPELINE_STATE_REGISTRY_H

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <map>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <vector>

namespace SST {
namespace Carcosa {

/**
 * Labeled memory range for address-in-region predicates (slot index = region id).
 */
struct MemoryRegion {
    uint64_t    base  = 0;
    uint64_t    size  = 0;
    bool        valid = false;
    int         id    = -1;
    /** Optional label for region_names predicates (e.g. "weights"). */
    std::string name;
};

/**
 * FSM snapshot for PortModules; currentKernelName is the canonical key.
 */
struct PipelineStateBase {
    int         currentKernel     = -1;
    std::string currentKernelName;
    std::string actuationKernelName = "ACTUATE";
    int         pipelineCycle     = 0;

    uint64_t stagedBase = 0;
    uint64_t stagedSize = 0;

    std::vector<MemoryRegion> regions;

/**
 * EccGuard DUE+drop_frame flag; agents fast-forward FSM then clear.
 */
    bool     frameAbortRequested = false;

    /** Cumulative count of pipeline cycles that were aborted due to DUE. */
    int      framesDropped = 0;

/**
 * Per-frame record for ActionScorer (checksum + escape snapshot at close).
 */
    struct FrameRecord {
        int         pipelineCycle      = 0;
        int         kernelAtClose      = -1;
        std::string kernelAtCloseName;
        // Kernel with the most EccGuard escapes this frame (else kernelAtClose).
        int         attributingKernel  = -1;
        std::string attributingKernelName;
        bool        dropped            = false;
        uint64_t    actionChecksum     = 0;
        // Quantized-action fingerprint via HYADES_ACTION_TOKEN (sub-bin noise
        // insensitive). 0 = unpublished; scorer falls back to checksum.
        uint64_t    actionToken        = 0;
        uint64_t    cumulativeEscapes  = 0;
        uint64_t    cumulativeFlips    = 0;
        uint64_t    simTimePs          = 0;
    };
    std::vector<FrameRecord> frames;

/**
 * Per-frame per-kernel SilentEscape counts; agent argmaxes then resets.
 */
    std::unordered_map<std::string, uint64_t> eccPerFrameEscapesByKernel;

/**
 * Argmax over eccPerFrameEscapesByKernel; "" if empty/all-zero.
 */
    std::string argmaxEccPerFrameEscapesByKernel() const {
        std::string best_name;
        uint64_t    best_v = 0;
        for (const auto& kv : eccPerFrameEscapesByKernel) {
            if (kv.second > best_v) {
                best_v    = kv.second;
                best_name = kv.first;
            }
        }
        return best_name;
    }

    /** Helper: zero out the per-frame escape map (consumer at frame close). */
    void resetEccPerFrameEscapesByKernel() {
        for (auto& kv : eccPerFrameEscapesByKernel) kv.second = 0u;
    }

    /** Cumulative flip/escape counters from EccGuard for per-frame deltas. */
    uint64_t eccCumulativeEscapes = 0;
    uint64_t eccCumulativeFlips   = 0;

    /** Watcher checksum during ACTUATE; prefer over MMIO when valid. */
    uint64_t watcherActionChecksum     = 0;
    bool     watcherActionChecksumValid = false;
    /** True if any CPU-observed byte in the critical window differed this frame. */
    bool     watcherCriticalCorrupted  = false;
    /** Per-run count of frames where watcherCriticalCorrupted was set (finish stat). */
    uint64_t framesCriticalRegionCorrupted = 0;

    /** Returns the region id (== slot index) whose range contains addr, or -1. */
    int regionIdForAddress(uint64_t addr) const {
        for (size_t i = 0; i < regions.size(); ++i) {
            const MemoryRegion& r = regions[i];
            if (r.valid && addr >= r.base && addr < r.base + r.size)
                return static_cast<int>(i);
        }
        return -1;
    }

    /** Grows the region table so that slot `id` is valid (filling intermediate slots). */
    void ensureRegionSlot(size_t id) {
        if (regions.size() <= id) {
            size_t old = regions.size();
            regions.resize(id + 1);
            for (size_t i = old; i <= id; ++i) regions[i].id = static_cast<int>(i);
        }
    }

    /** Promote staged base/size into regions[id] (HYADES_REGION_* ABI). */
    void commitStagedRegion(size_t id) {
        ensureRegionSlot(id);
        regions[id].base  = stagedBase;
        regions[id].size  = stagedSize;
        regions[id].valid = stagedSize > 0;
        regions[id].id    = static_cast<int>(id);
        stagedBase = 0;
        stagedSize = 0;
    }

    virtual ~PipelineStateBase() = default;
};

/**
 * String-keyed snapshot rendezvous; agents getOrCreate, PortModules get() lazily.
 */
template <typename T = PipelineStateBase>
class PipelineStateRegistry {
    static_assert(std::is_base_of<PipelineStateBase, T>::value ||
                  std::is_same<T, PipelineStateBase>::value,
                  "PipelineStateRegistry<T>: T must derive from PipelineStateBase");

public:
    /** Returns a pointer to the snapshot for `key`, creating a default-constructed entry if absent. */
    static T* getOrCreate(const std::string& key) {
        auto& m = map_();
        auto it = m.find(key);
        if (it == m.end()) it = m.emplace(key, T{}).first;
        return &it->second;
    }

    /** Read-only lookup; returns nullptr if no entry exists for `key`. */
    static const T* get(const std::string& key) {
        const auto& m = map_();
        auto it = m.find(key);
        return it == m.end() ? nullptr : &it->second;
    }

    /** Mutable lookup without insertion; returns nullptr if no entry exists for `key`. */
    static T* getMutable(const std::string& key) {
        auto& m = map_();
        auto it = m.find(key);
        return it == m.end() ? nullptr : &it->second;
    }

    static void   clear() { map_().clear(); }
    static size_t size()  { return map_().size(); }

private:
    static std::map<std::string, T>& map_() {
        static std::map<std::string, T> m;
        return m;
    }
};

} // namespace Carcosa
} // namespace SST

#endif /* SST_ELEMENTS_CARCOSA_PIPELINE_STATE_REGISTRY_H */
