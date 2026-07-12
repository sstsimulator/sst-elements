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

#include "sst/elements/carcosa/injectors/portModuleStateGate.h"
#include "sst/elements/carcosa/components/configParse.h"
#include "sst/elements/carcosa/faultlogic/randomFlipFault.h"
#include "sst/core/params.h"

#include <algorithm>
#include <cctype>
#include <set>
#include <sstream>
#include <string>
#include <vector>

using namespace SST::Carcosa;

namespace {

std::string trim(const std::string& s) {
    size_t b = 0;
    while (b < s.size() && std::isspace(static_cast<unsigned char>(s[b]))) ++b;
    size_t e = s.size();
    while (e > b && std::isspace(static_cast<unsigned char>(s[e - 1]))) --e;
    return s.substr(b, e - b);
}

std::vector<std::string> splitCsv(const std::string& csv) {
    std::vector<std::string> out;
    std::stringstream ss(csv);
    std::string tok;
    while (std::getline(ss, tok, ',')) {
        tok = trim(tok);
        if (!tok.empty()) out.push_back(tok);
    }
    return out;
}

std::set<int> parseIntSet(const std::string& csv, bool& valid) {
    std::set<int> out;
    valid = true;
    for (const auto& tok : splitCsv(csv)) {
        int value = 0;
        if (!ConfigParse::parseInt(tok, value)) valid = false;
        else out.insert(value);
    }
    return out;
}

std::set<std::string> parseStringSet(const std::string& csv) {
    std::set<std::string> out;
    for (auto& tok : splitCsv(csv)) out.insert(std::move(tok));
    return out;
}

} // namespace

bool PortModuleStateGate::parseMode(const std::string& s, Mode& mode) {
    std::string v;
    v.reserve(s.size());
    for (char c : s) v.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
    if (v == "drop") mode = Mode::Drop;
    else if (v == "flip") mode = Mode::Flip;
    else if (v == "drop_flip" || v == "dropflip" || v == "both") mode = Mode::DropFlip;
    else return false;
    return true;
}

PortModuleStateGate::PortModuleStateGate(Params& params)
    : FaultInjectorBase(params)
{
    state_key_ = params.find<std::string>("state_key", "");
    if (state_key_.empty()) {
        out_->fatal(CALL_INFO_LONG, -1,
                    "PortModuleStateGate: 'state_key' is required.\n");
    }

    std::string mode = params.find<std::string>("fault_mode", "drop");
    if (!parseMode(mode, mode_)) {
        out_->fatal(CALL_INFO_LONG, -1,
                    "PortModuleStateGate: unknown fault_mode '%s'.\n", mode.c_str());
    }
    drop_probability_ = params.find<double>("drop_probability", 1.0);
    flip_probability_ = params.find<double>("flip_probability", 1.0);
    if (!ConfigParse::isProbability(drop_probability_) ||
        !ConfigParse::isProbability(flip_probability_)) {
        out_->fatal(CALL_INFO_LONG, -1,
                    "PortModuleStateGate: probabilities must be in [0,1].\n");
    }

    // Fixed layout [drop=0, flip=1]. Drop is inline (cancelDelivery on any Event);
    // fault[0] stays null. fault[1] is RandomFlipFault only when flip is enabled.
    fault.resize(2);
    fault[0] = nullptr;
    fault[1] = (mode_ == Mode::Flip || mode_ == Mode::DropFlip)
                 ? new RandomFlipFault(params, this)
                 : nullptr;

    buildPredicates(params);
    setValidInstallation(params, SEND_RECEIVE_VALID);
    if (getInstallDirection() == installDirection::Send &&
        (mode_ == Mode::Drop || mode_ == Mode::DropFlip)) {
        out_->fatal(CALL_INFO_LONG, -1,
                    "PortModuleStateGate: drop modes require install_direction='Receive'.\n");
    }

#ifdef __SST_DEBUG_OUTPUT__
    dbg_->debug(CALL_INFO_LONG, 1, 0,
                "PortModuleStateGate: state_key='%s' mode=%d drop_p=%f flip_p=%f "
                "predicates=%zu\n",
                state_key_.c_str(), static_cast<int>(mode_),
                drop_probability_, flip_probability_, predicates_.size());
#endif
}

void
PortModuleStateGate::buildPredicates(Params& params)
{
    // Parse into serializable members first; rebuildPredicates() turns them
    // into lambdas and runs again after checkpoint restore.
    kernels_csv_     = params.find<std::string>("kernels", "");
    has_cycle_range_ = params.contains("pipeline_cycle_start")
                      || params.contains("pipeline_cycle_end");
    cycle_start_     = params.find<int>("pipeline_cycle_start", 0);
    cycle_end_       = params.find<int>("pipeline_cycle_end", INT32_MAX);
    region_ids_csv_  = params.find<std::string>("region_ids", "");
    region_names_csv_ = params.find<std::string>("region_names", "");

    rebuildPredicates();
}

void
PortModuleStateGate::rebuildPredicates()
{
    predicates_.clear();

    // kernel-id set predicate: matches when currentKernel is in the set.
    if (!kernels_csv_.empty()) {
        bool valid = false;
        auto allowed = parseIntSet(kernels_csv_, valid);
        if (!valid) out_->fatal(CALL_INFO_LONG, -1,
                                "PortModuleStateGate: invalid kernels CSV '%s'.\n",
                                kernels_csv_.c_str());
        predicates_.emplace_back(
            [allowed = std::move(allowed)](const PipelineStateBase& s,
                                           const EventAddress&) {
                return allowed.count(s.currentKernel) > 0;
            });
    }

    // pipeline cycle range predicate: [start, end] inclusive; either bound optional.
    // Use a large sentinel for "unset" to keep the comparison branch-free.
    if (has_cycle_range_) {
        if (cycle_start_ > cycle_end_) {
            out_->fatal(CALL_INFO_LONG, -1,
                        "PortModuleStateGate: pipeline_cycle_start exceeds pipeline_cycle_end.\n");
        }
        const int start = cycle_start_;
        const int end   = cycle_end_;
        predicates_.emplace_back(
            [start, end](const PipelineStateBase& s, const EventAddress&) {
                return s.pipelineCycle >= start && s.pipelineCycle <= end;
            });
    }

    // Region predicates are address filters: the event's range must overlap
    // an allowed published region. An event with no address (ea.valid ==
    // false) touches no region and never matches.
    auto overlaps = [](const MemoryRegion& r, const EventAddress& ea) {
        if (!ea.valid) return false;
        uint64_t sz = ea.size ? ea.size : 1;
        return ea.addr < r.base + r.size && ea.addr + sz > r.base;
    };

    if (!region_ids_csv_.empty()) {
        bool valid = false;
        auto allowed = parseIntSet(region_ids_csv_, valid);
        if (!valid) out_->fatal(CALL_INFO_LONG, -1,
                                "PortModuleStateGate: invalid region_ids CSV '%s'.\n",
                                region_ids_csv_.c_str());
        predicates_.emplace_back(
            [allowed = std::move(allowed), overlaps](const PipelineStateBase& s,
                                                     const EventAddress& ea) {
                for (const auto& r : s.regions) {
                    if (r.valid && allowed.count(r.id) > 0 && overlaps(r, ea))
                        return true;
                }
                return false;
            });
    }

    if (!region_names_csv_.empty()) {
        auto allowed = parseStringSet(region_names_csv_);
        predicates_.emplace_back(
            [allowed = std::move(allowed), overlaps](const PipelineStateBase& s,
                                                     const EventAddress& ea) {
                for (const auto& r : s.regions) {
                    if (r.valid && allowed.count(r.name) > 0 && overlaps(r, ea))
                        return true;
                }
                return false;
            });
    }
}

bool
PortModuleStateGate::matchesState(const PipelineStateBase& state,
                                  const EventAddress& ea) const
{
    for (const auto& p : predicates_) {
        if (!p(state, ea)) return false;
    }
    return true;
}

bool
PortModuleStateGate::doInjection(Event* ev)
{
    triggered_ = {{false, false}};

    const PipelineStateBase* state =
        PipelineStateRegistry<PipelineStateBase>::get(state_key_);
    if (!state) {
        // Agent hasn't published yet; no gate can match.
        return false;
    }

    EventAddress ea;
    if (auto* mev = dynamic_cast<SST::MemHierarchy::MemEvent*>(ev)) {
        // Same address convention as CriticalActionWatcher/EccGuard: prefer
        // the virtual address when the CPU supplied one (published regions
        // are workload-virtual), else the physical address.
        uint64_t vaddr = mev->getVirtualAddress();
        ea.valid = true;
        ea.addr  = (vaddr != 0) ? vaddr : mev->getAddr();
        ea.size  = mev->getSize() ? mev->getSize() : mev->getPayload().size();
    }

    if (!matchesState(*state, ea)) {
        return false;
    }

    switch (mode_) {
        case Mode::Drop:
            triggered_[0] = (this->randFloat(0.0, 1.0) < drop_probability_);
            return triggered_[0];
        case Mode::Flip:
            triggered_[1] = (this->randFloat(0.0, 1.0) < flip_probability_);
            return triggered_[1];
        case Mode::DropFlip:
            triggered_[0] = (this->randFloat(0.0, 1.0) < drop_probability_);
            // Only roll for flip if we didn't already decide to drop the event;
            // a dropped event has nothing left to flip.
            triggered_[1] = !triggered_[0] &&
                            (this->randFloat(0.0, 1.0) < flip_probability_);
            return triggered_[0] || triggered_[1];
    }
    return false;
}

void
PortModuleStateGate::executeFaults(Event*& ev)
{
    if (triggered_[0]) {
        delete ev;
        ev = nullptr;
        this->cancelDelivery();
        return;
    }
    if (triggered_[1]) {
        if (!fault[1]) {
            out_->fatal(CALL_INFO_LONG, -1,
                        "PortModuleStateGate: flip triggered but fault[1] is null "
                        "(fault_mode must be 'flip' or 'drop_flip' to enable flip).\n");
        }
        fault[1]->faultLogic(ev);
    }
}
