// Copyright 2009-2026 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2026, NTESS
// All rights reserved.

#include "sst_config.h"
#include "sst/elements/carcosa/components/criticalActionWatcher.h"
#include "sst/elements/carcosa/components/carcosaHash.h"
#include <algorithm>
#include <cinttypes>
#include <fstream>
#include <sstream>

using namespace SST;
using namespace SST::MemHierarchy;
using namespace SST::Carcosa;

CriticalActionWatcher::CriticalActionWatcher(ComponentId_t id, Params& params)
    : Component(id) {
    out_ = new Output("", 1, 0, Output::STDOUT);
    verbose_         = params.find<bool>("verbose", false);
    state_key_       = params.find<std::string>("state_key", "");
    critical_region_ = params.find<std::string>("critical_region", "action_queue");
    critical_len_    = params.find<uint64_t>("critical_len", 64);
    applyOnResponsesOnly_ =
        params.find<bool>("apply_on_responses_only", true);
    actuation_kernel_name_ = params.find<std::string>("actuation_kernel", "");
    golden_path_     = params.find<std::string>("golden_log", "");
    golden_required_ = params.find<bool>("golden_required", true);
    emit_golden_     = params.find<bool>("emit_golden", false);

    if (state_key_.empty()) {
        out_->fatal(CALL_INFO, -1,
            "CriticalActionWatcher '%s': state_key is required.\n",
            getName().c_str());
    }

    stat_frames_critical_corrupted_ =
        registerStatistic<uint64_t>("frames_critical_region_corrupted");

    if (isPortConnected("highlink")) {
        highlink_ = configureLink("highlink",
            new Event::Handler<CriticalActionWatcher, &CriticalActionWatcher::handleHighlink>(this));
    }
    if (isPortConnected("lowlink")) {
        lowlink_ = configureLink("lowlink",
            new Event::Handler<CriticalActionWatcher, &CriticalActionWatcher::handleLowlink>(this));
    }
    if (!highlink_ || !lowlink_) {
        out_->fatal(CALL_INFO, -1,
            "CriticalActionWatcher '%s': both highlink and lowlink must be connected.\n",
            getName().c_str());
    }
}

CriticalActionWatcher::~CriticalActionWatcher() {
    delete out_;
}

void CriticalActionWatcher::setup() {
    state_ptr_ = PipelineStateRegistry<PipelineStateBase>::getMutable(state_key_);
    if (!state_ptr_) {
        out_->fatal(CALL_INFO, -1,
            "CriticalActionWatcher '%s': no PipelineStateBase for key '%s'.\n",
            getName().c_str(), state_key_.c_str());
    }
    if (actuation_kernel_name_.empty()) {
        actuation_kernel_name_ = state_ptr_->actuationKernelName;
    }
    loadGoldenLog();
}

void CriticalActionWatcher::init(unsigned phase) {
    if (highlink_ && lowlink_) {
        SST::Event* ev;
        while ((ev = highlink_->recvUntimedData()) != nullptr) {
            lowlink_->sendUntimedData(ev);
        }
        while ((ev = lowlink_->recvUntimedData()) != nullptr) {
            highlink_->sendUntimedData(ev);
        }
    }
    (void)phase;
}

void CriticalActionWatcher::complete(unsigned phase) {
    if (highlink_ && lowlink_) {
        SST::Event* ev;
        while ((ev = highlink_->recvUntimedData()) != nullptr) {
            lowlink_->sendUntimedData(ev);
        }
        while ((ev = lowlink_->recvUntimedData()) != nullptr) {
            highlink_->sendUntimedData(ev);
        }
    }
    (void)phase;
}

void CriticalActionWatcher::finish() {
    if (saw_kernel_ && last_kernel_name_ == actuation_kernel_name_)
        finalizeActuateFrame();
    if (emit_golden_) {
        std::ostringstream os;
        os << "=== Critical Action Watcher Golden Emit ===\n"
           << "pipeline_cycle,kernel_at_close,action_checksum\n";
        for (const auto& row : emitted_golden_) {
            os << row.first.first << "," << row.first.second << ","
               << row.second << "\n";
        }
        os << "=== End Critical Action Watcher Golden Emit ===\n";
        out_->output("%s", os.str().c_str());
    }
    if (verbose_ && state_ptr_) {
        out_->output("CriticalActionWatcher '%s': frames_critical_region_corrupted=%" PRIu64 "\n",
                     getName().c_str(), state_ptr_->framesCriticalRegionCorrupted);
    }
}

bool CriticalActionWatcher::isResponseCmd(MemEvent* mev) const {
    if (!mev) return false;
    Command c = mev->getCmd();
    return c == Command::GetSResp || c == Command::GetXResp || c == Command::WriteResp;
}

bool CriticalActionWatcher::resolveCriticalBounds(uint64_t& base_out, uint64_t& len_out) const {
    base_out = 0;
    len_out  = 0;
    if (!state_ptr_) return false;
    for (const auto& r : state_ptr_->regions) {
        if (!r.valid || r.name != critical_region_) continue;
        base_out = r.base;
        len_out  = r.size;
        if (critical_len_ > 0 && critical_len_ < len_out)
            len_out = critical_len_;
        return len_out > 0;
    }
    return false;
}

bool CriticalActionWatcher::eventOverlapsCritical(MemEvent* mev,
                                                    uint64_t& rel_off,
                                                    uint64_t& payload_off,
                                                    uint64_t& overlap_len) const {
    rel_off = 0;
    payload_off = 0;
    overlap_len = 0;
    if (!mev) return false;
    uint64_t fbase = crit_base_, flen = crit_len_;
    if (fbase == 0 && flen == 0 && !resolveCriticalBounds(fbase, flen))
        return false;
    uint64_t vaddr = mev->getVirtualAddress();
    uint64_t addr  = (vaddr != 0) ? vaddr : mev->getAddr();
    uint64_t size  = mev->getPayload().empty() ? 64u : mev->getPayload().size();
    uint64_t end   = addr + size;
    uint64_t fend  = fbase + flen;
    if (!(addr < fend && end > fbase)) return false;
    uint64_t ostart = std::max(addr, fbase);
    uint64_t oend   = std::min(end, fend);
    rel_off      = ostart - fbase;
    payload_off  = ostart - addr;
    overlap_len  = oend - ostart;
    return overlap_len > 0;
}

void CriticalActionWatcher::mergePayloadIntoSnapshot(uint64_t rel_off,
                                                     const std::vector<uint8_t>& payload,
                                                     uint64_t payload_off,
                                                     uint64_t copy_len) {
    if (snapshot_.empty()) return;
    for (uint64_t i = 0; i < copy_len; ++i) {
        uint64_t src = payload_off + i;
        uint64_t pos = rel_off + i;
        if (src >= payload.size() || pos >= snapshot_.size()) break;
        snapshot_[pos] = payload[src];
        observed_this_frame_ = true;
    }
    // Publish after every merge, not on the later kernel-transition observation:
    // the pipeline driver closes its FrameRecord on the ACTUATE status write,
    // before this watcher necessarily sees traffic from the next kernel.
    if (observed_this_frame_ && state_ptr_) {
        state_ptr_->watcherActionChecksum = hashSnapshot();
        state_ptr_->watcherActionChecksumValid = true;
    }
}

uint64_t CriticalActionWatcher::hashSnapshot() const {
    if (snapshot_.empty()) return 0;
    return fnv1a64(snapshot_.data(), snapshot_.size());
}

void CriticalActionWatcher::loadGoldenLog() {
    if (golden_path_.empty()) {
        if (!emit_golden_) {
            out_->output("CriticalActionWatcher '%s': WARNING no golden_log; "
                         "checksums will be published but critical-region "
                         "corruption will not be classified. Run a BER=0 pass "
                         "with emit_golden=true, then provide that CSV.\n",
                         getName().c_str());
        }
        return;
    }

    std::ifstream f(golden_path_);
    if (!f.is_open()) {
        if (golden_required_) {
            out_->fatal(CALL_INFO, -1,
                "CriticalActionWatcher '%s': golden_log='%s' could not be opened "
                "and golden_required=true.\n",
                getName().c_str(), golden_path_.c_str());
        }
        out_->output("CriticalActionWatcher '%s': WARNING golden_log '%s' could "
                     "not be opened; corruption classification is disabled.\n",
                     getName().c_str(), golden_path_.c_str());
        return;
    }

    std::string line;
    int line_no = 0;
    while (std::getline(f, line)) {
        ++line_no;
        if (line.empty() || line[0] == '#') continue;
        std::istringstream ss(line);
        std::string cycle_s, kernel_s, checksum_s;
        if (!std::getline(ss, cycle_s, ',') ||
            !std::getline(ss, kernel_s, ',') ||
            !std::getline(ss, checksum_s, ',')) {
            out_->output("CriticalActionWatcher '%s': skipping malformed "
                         "golden_log line %d: '%s'\n",
                         getName().c_str(), line_no, line.c_str());
            continue;
        }
        if (cycle_s.find("pipeline_cycle") != std::string::npos) continue;

        try {
            int cycle = std::stoi(cycle_s);
            int kernel = std::stoi(kernel_s);
            uint64_t checksum = std::stoull(checksum_s, nullptr, 0);
            auto key = std::make_pair(cycle, kernel);
            if (!golden_by_frame_.emplace(key, checksum).second) {
                out_->fatal(CALL_INFO, -1,
                    "CriticalActionWatcher '%s': duplicate golden entry for "
                    "pipeline_cycle=%d kernel_at_close=%d.\n",
                    getName().c_str(), cycle, kernel);
            }
            if (++golden_cycle_count_[cycle] == 1)
                golden_by_cycle_[cycle] = checksum;
        } catch (...) {
            out_->output("CriticalActionWatcher '%s': skipping malformed "
                         "golden_log line %d: '%s'\n",
                         getName().c_str(), line_no, line.c_str());
        }
    }

    if (golden_by_frame_.empty()) {
        if (golden_required_) {
            out_->fatal(CALL_INFO, -1,
                "CriticalActionWatcher '%s': golden_log='%s' contained no valid "
                "entries and golden_required=true.\n",
                getName().c_str(), golden_path_.c_str());
        }
        out_->output("CriticalActionWatcher '%s': WARNING golden_log '%s' "
                     "contained no valid entries; corruption classification "
                     "is disabled.\n",
                     getName().c_str(), golden_path_.c_str());
    }
    if (verbose_)
        out_->output("CriticalActionWatcher '%s': loaded %zu golden entries from '%s'.\n",
                     getName().c_str(), golden_by_frame_.size(), golden_path_.c_str());
}

bool CriticalActionWatcher::findGoldenChecksum(int pipeline_cycle,
                                               int kernel_at_close,
                                               uint64_t& checksum_out) const {
    auto exact = golden_by_frame_.find({pipeline_cycle, kernel_at_close});
    if (exact != golden_by_frame_.end()) {
        checksum_out = exact->second;
        return true;
    }
    auto count = golden_cycle_count_.find(pipeline_cycle);
    auto cycle = golden_by_cycle_.find(pipeline_cycle);
    if (count != golden_cycle_count_.end() && count->second == 1 &&
        cycle != golden_by_cycle_.end()) {
        checksum_out = cycle->second;
        return true;
    }
    return false;
}

void CriticalActionWatcher::finalizeActuateFrame() {
    if (!state_ptr_) return;
    uint64_t checksum = hashSnapshot();

    int pipeline_cycle = state_ptr_->pipelineCycle;
    if (emit_golden_ && observed_this_frame_)
        emitted_golden_.push_back({{pipeline_cycle, last_kernel_id_}, checksum});

    bool corrupted = false;
    if (!golden_by_frame_.empty()) {
        uint64_t golden = 0;
        bool matched = observed_this_frame_ &&
            findGoldenChecksum(pipeline_cycle, last_kernel_id_, golden);
        if (!matched) {
            if (golden_required_) {
                out_->fatal(CALL_INFO, -1,
                    "CriticalActionWatcher '%s': frame (pipeline_cycle=%d, "
                    "kernel_at_close=%d) %s; golden_required=true.\n",
                    getName().c_str(), pipeline_cycle, last_kernel_id_,
                    observed_this_frame_ ? "has no golden entry"
                                         : "observed no critical-region bytes");
            }
            if (!warned_unscored_frame_) {
                warned_unscored_frame_ = true;
                out_->output("CriticalActionWatcher '%s': WARNING at least one "
                             "frame could not be compared with golden_log; it "
                             "will not be classified (warning only prints once).\n",
                             getName().c_str());
            }
        } else {
            corrupted = checksum != golden;
        }
    }
    state_ptr_->watcherCriticalCorrupted = corrupted;
    if (corrupted) {
        ++state_ptr_->framesCriticalRegionCorrupted;
        if (stat_frames_critical_corrupted_)
            stat_frames_critical_corrupted_->addData(1);
    }
    // Frame-close status write (no intervening critical traffic) already
    // consumed this fold; retire the flag so the next frame cannot inherit a
    // stale checksum if nothing downstream reads it.
    state_ptr_->watcherActionChecksumValid = false;
    snapshot_.assign(crit_len_, 0);
    observed_this_frame_ = false;
}

void CriticalActionWatcher::observeEvent(MemEvent* mev) {
    if (!mev || !state_ptr_) return;
    if (!resolveCriticalBounds(crit_base_, crit_len_)) {
        crit_base_ = 0;
        crit_len_  = 0;
    } else if (snapshot_.size() != crit_len_) {
        snapshot_.assign(crit_len_, 0);
    }

    const std::string& k = state_ptr_->currentKernelName;
    if (saw_kernel_ && last_kernel_name_ == actuation_kernel_name_
        && k != actuation_kernel_name_) {
        finalizeActuateFrame();
    }
    last_kernel_name_ = k;
    last_kernel_id_   = state_ptr_->currentKernel;
    saw_kernel_       = true;

    if ((!applyOnResponsesOnly_ || isResponseCmd(mev))
        && k == actuation_kernel_name_) {
        uint64_t rel = 0, poff = 0, olen = 0;
        if (eventOverlapsCritical(mev, rel, poff, olen)) {
            const auto& payload = mev->getPayload();
            if (!payload.empty())
                mergePayloadIntoSnapshot(rel, payload, poff, olen);
        }
    }
}

void CriticalActionWatcher::handleHighlink(Event* ev) {
    observeEvent(dynamic_cast<MemEvent*>(ev));
    if (lowlink_) lowlink_->send(ev);
}

void CriticalActionWatcher::handleLowlink(Event* ev) {
    observeEvent(dynamic_cast<MemEvent*>(ev));
    if (highlink_) highlink_->send(ev);
}
