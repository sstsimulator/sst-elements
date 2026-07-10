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

#ifndef SST_ELEMENTS_CARCOSA_FRAME_PIPELINE_DRIVER_H
#define SST_ELEMENTS_CARCOSA_FRAME_PIPELINE_DRIVER_H

#include "sst/elements/carcosa/components/pipelineStateRegistry.h"
#include "sst/elements/memHierarchy/memEvent.h"
#include <sst/core/component.h>
#include <sst/core/link.h>
#include <sst/core/output.h>
#include <cstdint>
#include <string>
#include <vector>

namespace SST {
namespace Carcosa {

/**
 * VLA pipeline test double through CriticalActionWatcher; finish() fatals on mismatch.
 */
class FramePipelineDriver : public SST::Component {
public:
    SST_ELI_REGISTER_COMPONENT(
        FramePipelineDriver,
        "carcosa",
        "FramePipelineDriver",
        SST_ELI_ELEMENT_VERSION(1, 0, 0),
        "In-tree frame-pipeline test double: publishes kernel transitions and "
        "regions, drives scripted reads through a CriticalActionWatcher, and "
        "pushes FrameRecords into PipelineStateBase::frames for ActionScorer.",
        COMPONENT_CATEGORY_UNCATEGORIZED)

    SST_ELI_DOCUMENT_PARAMS(
        {"state_key",       "PipelineStateRegistry key to publish under (required).", ""},
        {"region_name",     "Name of the published critical region.", "action_queue"},
        {"region_base",     "Base address of the critical region (must be >= 16 so the straddling read has room below).", "8192"},
        {"region_size",     "Size in bytes of the critical region.", "64"},
        {"frames",          "Number of pipeline frames to run.", "3"},
        {"corrupt_frame",   "Frame index whose in-region ACTUATE read payload gets one bit flipped (-1 = none).", "-1"},
        {"close_kernel_id", "Kernel id stamped into FrameRecord::kernelAtClose (set differently from the golden log's kernel_at_close to exercise the cycle-only fallback).", "1"},
        {"extra_region",    "Optional second published region, 'name:base:size'. Used by the PortModuleStateGate tests: a gate on a published region no traffic touches must never fire.", ""},
        {"expect_checksums","Optional CSV of per-frame expected FrameRecord::actionChecksum values; finish() fatals on any mismatch.", ""},
        {"frame_tokens",    "Optional CSV of per-frame action tokens (0 means unavailable).", ""},
        {"dropped_frames",  "Optional CSV of frame indices to mark dropped.", ""},
        {"escape_frames",   "Optional CSV of frame indices that add one cumulative ECC escape.", ""},
        {"expect_corrupted_frames", "Expected PipelineStateBase::framesCriticalRegionCorrupted at finish (-1 = don't check).", "-1"},
        {"verbose",         "Enable verbose output.", "false"})

    SST_ELI_DOCUMENT_PORTS(
        {"cpu_side", "Connect to the watcher's highlink (requests out, responses in).", {"memHierarchy.MemEventBase"}},
        {"mem_side", "Connect to the watcher's lowlink (requests in, responses out).",  {"memHierarchy.MemEventBase"}})

    FramePipelineDriver(SST::ComponentId_t id, SST::Params& params);
    ~FramePipelineDriver() override;

    void finish() override;

private:
    struct Op {
        enum class Kind { Publish, Read, Stamp } kind;
        int         kernelId  = -1;   // Publish
        std::string kernelName;       // Publish
        int         cycle     = 0;    // Publish
        uint64_t    addr      = 0;    // Read
        uint32_t    size      = 0;    // Read
        uint8_t     seed      = 0;    // Read
        bool        corrupt   = false;// Read: XOR 0x80 into payload[0]
    };

    static uint8_t patternByte(uint8_t seed, int frame, uint64_t addr) {
        // Mirrored by tests/framePipelineCommon.py: keep in sync.
        return static_cast<uint8_t>((seed + 29 * frame + (addr & 0xFF)) & 0xFF);
    }

    void buildScript();
    bool clockTick(SST::Cycle_t cycle);
    void handleCpuSide(SST::Event* ev);
    void handleMemSide(SST::Event* ev);
    void stampFrame();

    SST::Output* out_     = nullptr;
    bool         verbose_ = false;

    std::string state_key_;
    std::string region_name_;
    uint64_t    region_base_ = 8192;
    uint64_t    region_size_ = 64;
    int         frames_      = 3;
    int         corrupt_frame_   = -1;
    int         close_kernel_id_ = 1;
    std::vector<uint64_t> expect_checksums_;
    std::vector<uint64_t> frame_tokens_;
    std::vector<int>      dropped_frames_;
    std::vector<int>      escape_frames_;
    int64_t     expect_corrupted_frames_ = -1;

    PipelineStateBase* state_ptr_ = nullptr;

    std::vector<Op> script_;
    size_t          pc_        = 0;
    bool            awaiting_  = false;
    bool            done_      = false;
    int             cur_frame_ = 0;
    uint8_t         cur_seed_  = 0;
    bool            cur_corrupt_ = false;

    SST::Link* cpu_side_ = nullptr;
    SST::Link* mem_side_ = nullptr;
};

} // namespace Carcosa
} // namespace SST

#endif /* SST_ELEMENTS_CARCOSA_FRAME_PIPELINE_DRIVER_H */
