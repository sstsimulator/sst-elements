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

std::set<int> parseIntSet(const std::string& csv) {
    std::set<int> out;
    for (const auto& tok : splitCsv(csv)) {
        try { out.insert(std::stoi(tok)); } catch (...) { /* skip bad tokens */ }
    }
    return out;
}

std::set<std::string> parseStringSet(const std::string& csv) {
    std::set<std::string> out;
    for (auto& tok : splitCsv(csv)) out.insert(std::move(tok));
    return out;
}

} // namespace

PortModuleStateGate::Mode
PortModuleStateGate::parseMode(const std::string& s) {
    std::string v;
    v.reserve(s.size());
    for (char c : s) v.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
    if (v == "drop")      return Mode::Drop;
    if (v == "flip")      return Mode::Flip;
    if (v == "drop_flip" || v == "dropflip" || v == "both") return Mode::DropFlip;
    return Mode::Drop;
}

PortModuleStateGate::PortModuleStateGate(Params& params)
    : FaultInjectorBase(params)
{
    stateKey_ = params.find<std::string>("state_key", "");
    if (stateKey_.empty()) {
        out_->fatal(CALL_INFO_LONG, -1,
                    "PortModuleStateGate: 'state_key' is required.\n");
    }

    mode_     = parseMode(params.find<std::string>("fault_mode", "drop"));
    dropProb_ = params.find<double>("drop_probability", 1.0);
    flipProb_ = params.find<double>("flip_probability", 1.0);

    // Fixed layout [drop=0, flip=1]. Drop is inline (cancelDelivery on any Event);
    // fault[0] stays null. fault[1] is RandomFlipFault only when flip is enabled.
    fault.resize(2);
    fault[0] = nullptr;
    fault[1] = (mode_ == Mode::Flip || mode_ == Mode::DropFlip)
                 ? new RandomFlipFault(params, this)
                 : nullptr;

    buildPredicates(params);
    setValidInstallation(params, SEND_RECEIVE_VALID);

#ifdef __SST_DEBUG_OUTPUT__
    dbg_->debug(CALL_INFO_LONG, 1, 0,
                "PortModuleStateGate: state_key='%s' mode=%d drop_p=%f flip_p=%f "
                "predicates=%zu\n",
                stateKey_.c_str(), static_cast<int>(mode_),
                dropProb_, flipProb_, predicates_.size());
#endif
}

void
PortModuleStateGate::buildPredicates(Params& params)
{
    // Parse into serializable members first; rebuildPredicates() turns them
    // into lambdas and runs again after checkpoint restore.
    kernelsCsv_     = params.find<std::string>("kernels", "");
    hasCycleRange_  = params.contains("pipeline_cycle_start")
                      || params.contains("pipeline_cycle_end");
    cycleStart_     = params.find<int>("pipeline_cycle_start", 0);
    cycleEnd_       = params.find<int>("pipeline_cycle_end", INT32_MAX);
    regionIdsCsv_   = params.find<std::string>("region_ids", "");
    regionNamesCsv_ = params.find<std::string>("region_names", "");

    rebuildPredicates();
}

void
PortModuleStateGate::rebuildPredicates()
{
    predicates_.clear();

    // kernel-id set predicate: matches when currentKernel is in the set.
    if (!kernelsCsv_.empty()) {
        auto allowed = parseIntSet(kernelsCsv_);
        predicates_.emplace_back(
            [allowed = std::move(allowed)](const PipelineStateBase& s,
                                           const EventAddress&) {
                return allowed.count(s.currentKernel) > 0;
            });
    }

    // pipeline cycle range predicate: [start, end] inclusive; either bound optional.
    // Use a large sentinel for "unset" to keep the comparison branch-free.
    if (hasCycleRange_) {
        const int start = cycleStart_;
        const int end   = cycleEnd_;
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

    if (!regionIdsCsv_.empty()) {
        auto allowed = parseIntSet(regionIdsCsv_);
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

    if (!regionNamesCsv_.empty()) {
        auto allowed = parseStringSet(regionNamesCsv_);
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
        PipelineStateRegistry<PipelineStateBase>::get(stateKey_);
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
            triggered_[0] = (this->randFloat(0.0, 1.0) <= dropProb_);
            return triggered_[0];
        case Mode::Flip:
            triggered_[1] = (this->randFloat(0.0, 1.0) <= flipProb_);
            return triggered_[1];
        case Mode::DropFlip:
            triggered_[0] = (this->randFloat(0.0, 1.0) <= dropProb_);
            // Only roll for flip if we didn't already decide to drop the event;
            // a dropped event has nothing left to flip.
            triggered_[1] = !triggered_[0] &&
                            (this->randFloat(0.0, 1.0) <= flipProb_);
            return triggered_[0] || triggered_[1];
    }
    return false;
}

void
PortModuleStateGate::executeFaults(Event*& ev)
{
    if (triggered_[0]) {
        // Generic drop via cancelDelivery(); interceptor must delete the event
        // (same contract as RandomDropFault::faultLogic).
        if (getInstallDirection() == installDirection::Receive) {
            delete ev;
            ev = nullptr;
            this->cancelDelivery();
        } else {
#ifdef __SST_DEBUG_OUTPUT__
            dbg_->debug(CALL_INFO_LONG, 1, 0,
                        "PortModuleStateGate: drop requested in Send direction is a no-op "
                        "(the framework doesn't expose a cancel hook on Send).\n");
#endif
        }
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
