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

#ifndef SST_ELEMENTS_CARCOSA_ACTION_SCORER_H
#define SST_ELEMENTS_CARCOSA_ACTION_SCORER_H

#include "sst/elements/carcosa/components/componentTestBounds.h"

#include <sst/core/component.h>
#include <sst/core/output.h>
#include <cstdint>
#include <string>
#include <vector>

namespace SST {
namespace Carcosa {

class ActionScorer : public SST::Component {
public:
    SST_ELI_REGISTER_COMPONENT(
        ActionScorer,
        "carcosa",
        "ActionScorer",
        SST_ELI_ELEMENT_VERSION(1, 0, 0),
        "End-task behavioral scorer for VLA fault-injection runs. Reads the per-frame history "
        "from PipelineStateBase::frames and emits a per-frame CSV plus a summary "
        "(unsafe_action_rate, frame_drop_rate, mean_escapes_per_frame).",
        COMPONENT_CATEGORY_UNCATEGORIZED)

    SST_ELI_DOCUMENT_PARAMS(
        {"state_key",          "Required. PipelineStateRegistry<PipelineStateBase> key whose ::frames vector this scorer ingests at finish().", ""},
        {"golden_log",         "Optional path to a CSV (pipeline_cycle,kernel_at_close,action_checksum[,action_token]) whose checksums/tokens are treated as fault-free truth. Each line provides the golden values for the matching cycle. If empty, every frame's argmax_changed/action_changed is reported as 0 (used by the BER=0 emit-golden pass).", ""},
        {"golden_required",    "If true (default) and golden_log is set but cannot be opened or contains no entries, fatal at setup() instead of silently scoring every frame as not-argmax-changed. Set to false only for self-replay sanity checks.", "true"},
        {"emit_golden",        "If true and golden_log is empty, dump the observed (cycle,kernel,checksum) trace to STDOUT under a '=== Action Scorer Golden Emit ===' block so a baseline run can produce the file the comparison runs need.", "false"},
        {"verbose",            "Enable verbose output.", "false"},
        {"expect_frames_total",       "Test hook: expected frames_total at finish(); fatal on mismatch. -1 disables.", "-1"},
        {"expect_frames_dropped",     "Test hook: expected frames_dropped at finish(); fatal on mismatch. -1 disables.", "-1"},
        {"expect_frames_argmax_diff", "Test hook: expected frames_argmax_diff at finish(); fatal on mismatch. -1 disables.", "-1"},
        {"expect_frames_action_diff", "Test hook: expected frames_action_diff at finish(); fatal on mismatch. -1 disables.", "-1"},
        {"expect_frames_unsafe",      "Test hook: expected unsafe-action count at finish(); fatal on mismatch. -1 disables.", "-1"},
        {"expect_frames_o1",          "Test hook: expected O1 count; -1 disables.", "-1"},
        {"expect_frames_o2",          "Test hook: expected O2 count; -1 disables.", "-1"},
        {"expect_frames_o3",          "Test hook: expected O3 count; -1 disables.", "-1"},
        {"expect_frames_o4",          "Test hook: expected O4 count; -1 disables.", "-1"})

    SST_ELI_DOCUMENT_STATISTICS(
        {"frames_total",        "Frames recorded in PipelineStateBase::frames.", "count", 1},
        {"frames_dropped",      "Frames flagged dropped (DUE-on-frame).",        "count", 1},
        {"frames_argmax_diff",  "Frames whose action_checksum differs from golden (debug oracle).", "count", 1},
        {"frames_action_diff",  "Frames whose decoded-action token differs from golden (headline divergence metric; falls back to checksum when tokens are unavailable).", "count", 1},
        {"frames_safety_violated", "Frames flagged unsafe (dropped OR argmax_diff).", "count", 1},
        {"frames_outcome_O1", "Frames classified O1 (clean).", "count", 1},
        {"frames_outcome_O2", "Frames classified O2 (late-but-correct).", "count", 1},
        {"frames_outcome_O3", "Frames classified O3 (SDC).", "count", 1},
        {"frames_outcome_O4", "Frames classified O4 (silent-benign).", "count", 1})

    ActionScorer(SST::ComponentId_t id, SST::Params& params);
    ~ActionScorer() override;

    void setup() override;
    void finish() override;

private:
    struct GoldenEntry {
        int      pipelineCycle;
        int      kernelAtClose;
        uint64_t checksum;
        uint64_t token    = 0;     // decoded-action token; optional 4th column
        bool     hasToken = false; // false for legacy 3-column goldens
    };

    void loadGoldenLog();

    std::string state_key_;
    std::string golden_path_;
    bool        emit_golden_     = false;
    bool        golden_required_ = true;
    bool        verbose_         = false;

    std::vector<GoldenEntry> golden_;
    bool                     golden_loaded_ = false;

    ComponentTestBounds test_bounds_;

    SST::Output* out_ = nullptr;

    Statistics::Statistic<uint64_t>* stat_frames_total_       = nullptr;
    Statistics::Statistic<uint64_t>* stat_frames_dropped_     = nullptr;
    Statistics::Statistic<uint64_t>* stat_frames_argmax_diff_ = nullptr;
    Statistics::Statistic<uint64_t>* stat_frames_action_diff_ = nullptr;
    Statistics::Statistic<uint64_t>* stat_frames_unsafe_      = nullptr;
    Statistics::Statistic<uint64_t>* stat_frames_o1_          = nullptr;
    Statistics::Statistic<uint64_t>* stat_frames_o2_          = nullptr;
    Statistics::Statistic<uint64_t>* stat_frames_o3_          = nullptr;
    Statistics::Statistic<uint64_t>* stat_frames_o4_          = nullptr;

    static const char* outcomeClassLabel(int oclass);
};

} // namespace Carcosa
} // namespace SST

#endif // SST_ELEMENTS_CARCOSA_ACTION_SCORER_H
