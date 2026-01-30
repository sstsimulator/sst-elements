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

#include "sst/elements/carcosa/injectors/dropFlipFaultInjector.h"
#include "sst/elements/carcosa/faultlogic/randomFlipFault.h"
#include "sst/elements/carcosa/faultlogic/randomDropFault.h"

using namespace SST::Carcosa;

DropFlipFaultInjector::DropFlipFaultInjector(Params& params) : FaultInjectorBase(params) {
    // create fault
    fault.resize(2);
    fault[0] = new RandomDropFault(params, this);
    fault[1] = new RandomFlipFault(params, this);

    // read probability params
    drop_probability_ = params.find<double>("drop_probability", 0.0);
#ifdef __SST_DEBUG_OUTPUT__
    if (drop_probability_ > 0.0){
        dbg_->debug(CALL_INFO_LONG, 1, 0, "Drop probability set to %lf.\n", drop_probability_);
    }
#endif

    flip_probability_ = params.find<double>("flip_probability", 0.0);
#ifdef __SST_DEBUG_OUTPUT__
    if (flip_probability_ > 0.0){
        dbg_->debug(CALL_INFO_LONG, 1, 0, "Flip probability set to %lf.\n", flip_probability_);
    }
#endif

    setValidInstallation(params, SEND_RECEIVE_VALID);
}

bool DropFlipFaultInjector::doInjection() {
    if (this->randFloat(0.0, 1.0) <= this->drop_probability_) {
#ifdef __SST_DEBUG_OUTPUT__
        dbg_->debug(CALL_INFO_LONG, 1, 0, "Drop triggered.\n");
#endif
        this->triggered_injection_[0] = true;
    } else {
#ifdef __SST_DEBUG_OUTPUT__
        dbg_->debug(CALL_INFO_LONG, 1, 0, "Drop skipped.\n");
#endif
        this->triggered_injection_[0] = false;
    }

    if (this->randFloat(0.0, 1.0) <= this->flip_probability_) {
#ifdef __SST_DEBUG_OUTPUT__
        dbg_->debug(CALL_INFO_LONG, 1, 0, "Flip triggered.\n");
#endif
        this->triggered_injection_[1] = true;
    }
    else {
#ifdef __SST_DEBUG_OUTPUT__
        dbg_->debug(CALL_INFO_LONG, 1, 0, "Flip skipped.\n");
#endif
        this->triggered_injection_[1] = false;
    }

    return this->triggered_injection_[0] || this->triggered_injection_[1];
}

/**
 * Overridden execution function to cause faults to be chosen at random
 * from the vector once a fault has been triggered
 */
void DropFlipFaultInjector::executeFaults(Event*& ev) {
    if (this->triggered_injection_[0]) {
        // do drop
        if (fault[0]) {
            fault[0]->faultLogic(ev);
#ifdef __SST_DEBUG_OUTPUT__
            dbg_->debug(CALL_INFO_LONG, 1, 0, "Drop triggered.\n");
#endif
        } else {
            out_->fatal(CALL_INFO_LONG, -1, "No valid drop fault object.\n");
        }
        return;
    }
    if (this->triggered_injection_[1]) {
        // do flip
        if (fault[1]) {
            fault[1]->faultLogic(ev);
#ifdef __SST_DEBUG_OUTPUT__
            dbg_->debug(CALL_INFO_LONG, 1, 0, "Flip triggered.\n");
#endif
        } else {
            out_->fatal(CALL_INFO_LONG, -1, "No valid flip fault object.\n");
        }
    }
}