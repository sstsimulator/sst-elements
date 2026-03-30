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

#include "sst/elements/carcosa/injectors/faultInjectorBase.h"
#include "sst/core/params.h"

using namespace SST::Carcosa;

/********** FaultInjectorBase **********/

FaultInjectorBase::FaultInjectorBase(SST::Params& params) : PortModule()
{
    out_ = new Output();
    out_->init("", params.find<int>("verbose", 1), 0, Output::STDOUT);

    dbg_ = new Output();
    dbg_->init("", params.find<int>("debug_level", 1), 0, (Output::output_location_t)params.find<int>("debug", 0));

#ifdef __SST_DEBUG_OUTPUT__
    dbg_->debug(CALL_INFO_LONG, 1, 0, "Initializing FaultInjector:\n");
#endif
    seed_ = params.find<uint64_t>("seed", 0);
    if (seed_ != 0) {
        base_rng_.seed(seed_);
#ifdef __SST_DEBUG_OUTPUT__
        dbg_->debug(CALL_INFO_LONG, 1, 0, "\tRNG Seed: %d\n", seed_);
#endif
    }
}

/**
 * Default behavior is to delete all fault objects in the order they were
 * added to the vector
 */
FaultInjectorBase::~FaultInjectorBase() {
    for (int i = 0; i < fault.size(); i++) {
        if (fault[i]) {
            delete fault[i];
        }
    }
}

void
FaultInjectorBase::eventSent(uintptr_t key, Event*& ev)
{
    if (!valid_installs_set) {
        out_->fatal(CALL_INFO_LONG, -1, "Valid installation directions not set -- did you forget to call setValidInstallation() in your constructor?\n");
    }
    if (doInjection()){
#ifdef __SST_DEBUG_OUTPUT__
        dbg_->debug(CALL_INFO_LONG, 3, 0, "Injection triggered.\n");
#endif
        this->executeFaults(ev);
    }
#ifdef __SST_DEBUG_OUTPUT__
    else {
        dbg_->debug(CALL_INFO_LONG, 3, 0, "Injection skipped.\n");
    }
#endif
}

void
FaultInjectorBase::interceptHandler(uintptr_t key, Event*& ev, bool& cancel)
{
    if (!valid_installs_set) {
        out_->fatal(CALL_INFO_LONG, -1, "Valid installation directions not set -- did you forget to call setValidInstallation() in your constructor?\n");
    }
    // do not cancel delivery by default
    cancel = false;
    cancel_ = &cancel;

    if (doInjection()){
#ifdef __SST_DEBUG_OUTPUT__
        dbg_->debug(CALL_INFO_LONG, 3, 0, "Injection triggered.\n");
#endif
        this->executeFaults(ev);
    }
#ifdef __SST_DEBUG_OUTPUT__
    else {
        dbg_->debug(CALL_INFO_LONG, 3, 0, "Injection skipped.\n");
    }
#endif
}

uint32_t FaultInjectorBase::randUInt32(uint32_t start, uint32_t end) {
    uint32_t range = end - start;
    return start + (base_rng_.generateNextUInt32() % range);
}

int32_t FaultInjectorBase::randInt32(int32_t start, int32_t end) {
    int32_t range = end - start;
    return start + (base_rng_.generateNextInt32() % range);
}

uint64_t FaultInjectorBase::randUInt64(uint64_t start, uint64_t end) {
    uint64_t range = end - start;
    return start + (base_rng_.generateNextUInt64() % range);
}

int64_t FaultInjectorBase::randInt64(int64_t start, int64_t end) {
    int64_t range = end - start;
    return start + (base_rng_.generateNextInt64() % range);
}

double FaultInjectorBase::randFloat(double start, double end) {
    double range = end - start;
    return start + (base_rng_.nextUniform() * range);
}

bool FaultInjectorBase::doInjection() {
    return true;
}

installDirection FaultInjectorBase::setInstallDirection(std::string param) {
    if ( param == "Receive" ) {
        if (valid_installation_[0]) {
            return installDirection::Receive;
        } else {
            out_->fatal(CALL_INFO_LONG, 1, 0, "This PortModule Fault Injector cannot intercept Receive events.\n");
        }
    } else if ( param == "Send" ) {
        if (valid_installation_[1]) {
            return installDirection::Send;
        } else {
            out_->fatal(CALL_INFO_LONG, 1, 0, "This PortModule Fault Injector cannot intercept Send events.\n");
        }
    }
    return installDirection::Invalid;
}

void FaultInjectorBase::setValidInstallation(Params& params, std::array<bool,2> valid_install) {
    valid_installation_ = valid_install;
    std::string install_dir = params.find<std::string>("install_direction", "Receive");
    install_direction_ = setInstallDirection(install_dir);

    if (install_direction_ == installDirection::Invalid) {
        out_->fatal(CALL_INFO_LONG, -1, "Install Direction should never be set to Invalid! Did you forget to set which directions are valid?\n");
    }

#ifdef __SST_DEBUG_OUTPUT__
    dbg_->debug(CALL_INFO_LONG, 1, 0, "\tInstall Direction: %s\n", install_dir.c_str());
#endif
    valid_installs_set = true;
}

/**
 * Default behavior is to execute faults in the order they were
 * added to the vector
 */
void FaultInjectorBase::executeFaults(Event*& ev) {
    bool success = false;
    for (int i = 0; i < fault.size(); i++) {
        if (fault[i]) {
            success = fault[i]->faultLogic(ev);
        }
    }
    if (!success) {
        out_->fatal(CALL_INFO_LONG, -1, "No valid fault object, or no fault successfully executed.\n");
    }
}