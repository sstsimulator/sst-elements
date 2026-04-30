// Copyright 2009-2025 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2025, NTESS
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include "sst/elements/carcosa/injectors/faultInjectorMemH.h"
#include "sst/elements/carcosa/components/pmDataRegistry.h"
#include "sst/elements/carcosa/faultlogic/randomFlipMemHFault.h"
#include "sst/elements/carcosa/faultlogic/randomDropFault.h"
#include "sst/elements/carcosa/faultlogic/stuckAtFault.h"
#include "sst/elements/carcosa/faultlogic/corruptMemFault.h"
#include <sstream>

using namespace SST::Carcosa;

namespace {
// Parse comma-separated registry id list; trim spaces; use "default" if empty.
std::vector<std::string> splitComma(const std::string& s) {
    std::vector<std::string> out;
    std::istringstream iss(s);
    std::string id;
    while (std::getline(iss, id, ',')) {
        while (!id.empty() && id.front() == ' ') id.erase(0, 1);
        while (!id.empty() && id.back() == ' ') id.pop_back();
        if (!id.empty()) out.push_back(id);
    }
    if (out.empty()) out.push_back("default");
    return out;
}
}

/********** FaultInjectorMemH **********/

FaultInjectorMemH::FaultInjectorMemH(SST::Params& params) : FaultInjectorBase(params) {
    pmId_ = params.find<std::string>("pmId", "");
    debugManagerLogic_ = params.find<bool>("debugManagerLogic", false);
    std::string regIdsStr = params.find<std::string>("pmRegistryIds", "default");
    pmRegistryIds_ = splitComma(regIdsStr);

    stat_skipped_ = registerStatistic<uint64_t>("skipped_pm_events");

    injection_probability_ = params.find<double>("injection_probability", params.find<double>("injectionProbability", 0.5));
    if (injection_probability_ < 0.0 || injection_probability_ > 1.0) {
        out_->fatal(CALL_INFO_LONG, -1,
            "Injection probability must be in range [0.0, 1.0], got %f\n", injection_probability_);
    }

    std::string faultType = params.find<std::string>("faultType", "");
    if (faultType == "stuckAt") {
        fault.push_back(new StuckAtFault(params, this));
    } else if (faultType == "randomFlip") {
        fault.push_back(new RandomFlipMemHFault(params, this));
    } else if (faultType == "randomDrop") {
        fault.push_back(new RandomDropFault(params, this));
    } else if (faultType == "corruptMemRegion") {
        fault.push_back(new CorruptMemFault(params, this));
    } else if (faultType == "custom") {
        out_->fatal(CALL_INFO_LONG, -1,
            "FaultInjectorMemH: faultType 'custom' is not implemented.\n");
    } else {
        out_->fatal(CALL_INFO_LONG, -1,
            "Invalid fault type '%s'. Valid options: stuckAt, randomFlip, randomDrop, corruptMemRegion\n",
            faultType.c_str());
    }

    // Accept legacy camelCase "installDirection" in addition to "install_direction".
    std::string installDir = params.find<std::string>("install_direction", params.find<std::string>("installDirection", "Receive"));
    params.insert("install_direction", installDir);
    setValidInstallation(params, SEND_RECEIVE_VALID);

#ifdef __SST_DEBUG_OUTPUT__
    dbg_->debug(CALL_INFO_LONG, 1, 0, "FaultInjectorMemH initialized: probability=%f, faultType=%s\n",
                injection_probability_, faultType.c_str());
#endif
}

bool FaultInjectorMemH::doInjection() {
    return randFloat(0.0, 1.0) <= injection_probability_;
}

void FaultInjectorMemH::executeFaults(Event*& ev) {
    ensureRegistriesResolved();

    MemHierarchy::MemEventBase* memEv = dynamic_cast<MemHierarchy::MemEventBase*>(ev);
    if (memEv) {
        auto eventId = memEv->getID();
        for (PMDataRegistry* reg : registries_) {
            if (reg->hasPMData(eventId)) {
                if (debugManagerLogic_) {
                    PMData pmData = reg->lookupPMData(eventId);
                    out_->output("[FaultInjectorMemH] pmId=%s read PM data: eventId=<%llu,%d> cmd=%s\n",
                        pmId_.c_str(), (unsigned long long)eventId.first, eventId.second, pmData.command.c_str());
                }
                if (stat_skipped_) stat_skipped_->addData(1);
                return;
            }
        }
    }

    FaultInjectorBase::executeFaults(ev);
}

void FaultInjectorMemH::ensureRegistriesResolved() {
    if (!registries_.empty()) return;
    for (const std::string& id : pmRegistryIds_) {
        PMDataRegistry* reg = PMRegistryResolver::getRegistry(id);
        if (reg) registries_.push_back(reg);
    }
    if (registries_.empty()) {
        std::string idsStr;
        for (size_t i = 0; i < pmRegistryIds_.size(); ++i) {
            if (i) idsStr += ",";
            idsStr += pmRegistryIds_[i];
        }
        out_->fatal(CALL_INFO_LONG, -1,
            "FaultInjectorMemH: no valid registry found for pmRegistryIds '%s'\n", idsStr.c_str());
    }
    if (!registerPMSent_) {
        for (PMDataRegistry* reg : registries_) {
            reg->pushMessageToManager(ManagerMessage::makeRegisterPM(pmId_));
        }
        registerPMSent_ = true;
        if (debugManagerLogic_) {
            std::string idsStr;
            for (size_t i = 0; i < pmRegistryIds_.size(); ++i) {
                if (i) idsStr += ",";
                idsStr += pmRegistryIds_[i];
            }
            out_->output("[ManagerLogic] FaultInjectorMemH pmId=%s: resolved registries [%s], sent RegisterPM to %zu manager(s)\n",
                pmId_.c_str(), idsStr.c_str(), registries_.size());
        }
    }
}

void FaultInjectorMemH::init(unsigned phase) {
    if (phase == 0) {
        ensureRegistriesResolved();
    }
}
