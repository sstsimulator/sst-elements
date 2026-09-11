// Copyright 2009-2026 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2026, NTESS
// All rights reserved.

#ifndef SST_ELEMENTS_CARCOSA_CRITICAL_ACTION_WATCHER_H
#define SST_ELEMENTS_CARCOSA_CRITICAL_ACTION_WATCHER_H

#include "sst/elements/carcosa/components/pipelineStateRegistry.h"
#include "sst/elements/memHierarchy/memEvent.h"
#include <sst/core/component.h>
#include <sst/core/link.h>
#include <sst/core/output.h>
#include <cstdint>
#include <map>
#include <string>
#include <utility>
#include <vector>

namespace SST {
namespace Carcosa {

/** Snapshots labeled DRAM bytes; publishes evolving checksum during ACTUATE. */
class CriticalActionWatcher : public SST::Component {
public:
    SST_ELI_REGISTER_COMPONENT(
        CriticalActionWatcher,
        "carcosa",
        "CriticalActionWatcher",
        SST_ELI_ELEMENT_VERSION(1, 0, 0),
        "Snapshots CPU-observed bytes in a critical memory region, publishes "
        "watcherActionChecksum into PipelineStateBase during ACTUATE, "
        "and compares each frame against an explicit fault-free golden log.",
        COMPONENT_CATEGORY_MEMORY)

    SST_ELI_DOCUMENT_PARAMS(
        {"verbose",            "Enable verbose output.", "false"},
        {"state_key",          "PipelineStateRegistry key (required).", ""},
        {"critical_region",    "Published region name to watch (e.g. action_queue).", "action_queue"},
        {"critical_len",       "Max bytes to hash (0 = entire region).", "64"},
        {"apply_on_responses_only", "Only snapshot read responses (these arrive on the lowlink side; requests are observed on highlink when false).", "true"},
        {"actuation_kernel",   "Workload-supplied kernel name that marks frame commitment. The snapshot is taken during this kernel and frozen on the trailing edge (kernel != actuation_kernel). Falls back to PipelineStateBase::actuationKernelName when this param is empty.", ""},
        {"golden_log",         "Optional ActionScorer-compatible CSV (pipeline_cycle,kernel_at_close,action_checksum[,action_token]) containing fault-free per-frame watcher checksums. Without it, checksums are still published but corruption is not classified. In the full pipeline the driver stamps the watcher checksum into FrameRecord::actionChecksum and ActionScorer owns golden comparison; configure this only in watcher-focused tests (tests/testFramePipeline*.py) or deployments without a scorer, so a regen does not have to keep two golden sets consistent.", ""},
        {"golden_required",    "If true (default) and golden_log is configured, fatal when the file is missing, empty, ambiguous, or lacks a frame observed by this run.", "true"},
        {"emit_golden",        "Emit observed watcher checksums in ActionScorer-compatible CSV form, normally during a BER=0 run with golden_log empty.", "false"})

    SST_ELI_DOCUMENT_PORTS(
        {"highlink", "Toward directory/cache", {"memHierarchy.MemEventBase"}},
        {"lowlink",  "Toward EccGuard/memory", {"memHierarchy.MemEventBase"}})

    SST_ELI_DOCUMENT_STATISTICS(
        {"frames_critical_region_corrupted", "Frames whose critical-window checksum differs from the matching explicit golden-log entry.", "count", 1})

    CriticalActionWatcher(SST::ComponentId_t id, SST::Params& params);
    ~CriticalActionWatcher() override;

    void setup() override;
    void init(unsigned phase) override;
    void complete(unsigned phase) override;
    void finish() override;

private:
    void handleHighlink(SST::Event* ev);
    void handleLowlink(SST::Event* ev);
    void observeEvent(SST::MemHierarchy::MemEvent* mev);

    bool isResponseCmd(SST::MemHierarchy::MemEvent* mev) const;
    bool resolveCriticalBounds(uint64_t& base_out, uint64_t& len_out) const;
    bool eventOverlapsCritical(SST::MemHierarchy::MemEvent* mev,
                               uint64_t& rel_off, uint64_t& payload_off,
                               uint64_t& overlap_len) const;
    void mergePayloadIntoSnapshot(uint64_t rel_off, const std::vector<uint8_t>& payload,
                                  uint64_t payload_off, uint64_t copy_len);
    uint64_t hashSnapshot() const;
    void loadGoldenLog();
    bool findGoldenChecksum(int pipeline_cycle, int kernel_at_close,
                            uint64_t& checksum_out) const;
    void finalizeActuateFrame();

    SST::Output* out_      = nullptr;
    bool         verbose_  = false;
    std::string  state_key_;
    std::string  critical_region_;
    uint64_t     critical_len_ = 64;

    bool apply_on_responses_only_ = true;

    PipelineStateBase* state_ptr_ = nullptr;
    std::string        actuation_kernel_name_;
    std::string        last_kernel_name_;
    int                last_kernel_id_ = -1;
    bool               saw_kernel_ = false;

    std::string golden_path_;
    bool golden_required_ = true;
    bool emit_golden_ = false;
    std::map<std::pair<int, int>, uint64_t> golden_by_frame_;
    std::map<int, uint64_t> golden_by_cycle_;
    std::map<int, int> golden_cycle_count_;
    std::vector<std::pair<std::pair<int, int>, uint64_t>> emitted_golden_;
    bool warned_unscored_frame_ = false;

    uint64_t crit_base_ = 0;
    uint64_t crit_len_  = 0;
    std::vector<uint8_t> snapshot_;
    bool observed_this_frame_ = false;

    SST::Link* highlink_ = nullptr;
    SST::Link* lowlink_  = nullptr;

    Statistics::Statistic<uint64_t>* stat_frames_critical_corrupted_ = nullptr;
};

} // namespace Carcosa
} // namespace SST

#endif /* SST_ELEMENTS_CARCOSA_CRITICAL_ACTION_WATCHER_H */
