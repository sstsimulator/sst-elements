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

#include "sst/elements/carcosa/injectors/doubleFaultInjector.h"
#include "sst/elements/carcosa/faultlogic/randomFlipFault.h"
#include "sst/elements/carcosa/faultlogic/randomDropFault.h"

using namespace SST::Carcosa;

DoubleFaultInjector::DoubleFaultInjector(Params& params) : FaultInjectorBase(params, SEND_RECEIVE_VALID) {
    // create fault
    fault = new FaultBase*[2];
    fault[0] = new RandomDropFault(params, this);
    fault[1] = new RandomFlipFault(params, this);

    // read flipvsdrop param
    flipVsDropProb_ = params.find<double>("flipVsDropProb", 0.5);
    distribution_ = std::uniform_real_distribution<double>(0,1);
}

DoubleFaultInjector::~DoubleFaultInjector() {
    if (fault) {
        if (fault[0]) {
            delete fault[0];
        }
        if (fault[1]) {
            delete fault[1];
        }
        delete fault;
    }
}

void DoubleFaultInjector::eventSent(uintptr_t key, Event*& ev) {
    if (doInjection()){
#ifdef __SST_DEBUG_OUTPUT__
        dbg_->debug(CALL_INFO_LONG, 1, 0, "Injection triggered.\n");
#endif
        if (!fault) {
            out_->fatal(CALL_INFO_LONG, -1, "No valid fault object.\n");
        }
        FaultBase* triggered_fault = nullptr;
        if (distribution_(generator_) <= flipVsDropProb_) {
            // do flip
#ifdef __SST_DEBUG_OUTPUT__
            dbg_->debug(CALL_INFO_LONG, 1, 0, "Flip triggered.\n");
#endif
            triggered_fault = fault[1];
        } else {
            // do drop
#ifdef __SST_DEBUG_OUTPUT__
            dbg_->debug(CALL_INFO_LONG, 1, 0, "Drop triggered.\n");
#endif
            triggered_fault = fault[0];
        }
        if (triggered_fault) {
            triggered_fault->faultLogic(ev);
        } else {
            out_->fatal(CALL_INFO_LONG, -1, "No valid fault object.\n");
        }
    }
#ifdef __SST_DEBUG_OUTPUT__
    else {
        dbg_->debug(CALL_INFO_LONG, 1, 0, "Injection skipped.\n");
    }
#endif
}

void DoubleFaultInjector::interceptHandler(uintptr_t key, Event*& ev, bool& cancel) {
    // do not cancel delivery by default
    cancel = false;
    cancel_ = &cancel;

    if (doInjection()){
#ifdef __SST_DEBUG_OUTPUT__
        dbg_->debug(CALL_INFO_LONG, 1, 0, "Injection triggered.\n");
#endif
        FaultBase* triggered_fault = nullptr;
        if (distribution_(generator_) <= flipVsDropProb_) {
            // do flip
#ifdef __SST_DEBUG_OUTPUT__
            dbg_->debug(CALL_INFO_LONG, 1, 0, "Flip triggered.\n");
#endif
            triggered_fault = fault[1];
        } else {
            // do drop
#ifdef __SST_DEBUG_OUTPUT__
            dbg_->debug(CALL_INFO_LONG, 1, 0, "Drop triggered.\n");
#endif
            triggered_fault = fault[0];
        }
        if (triggered_fault) {
            triggered_fault->faultLogic(ev);
        } else {
            out_->fatal(CALL_INFO_LONG, -1, "No valid fault object.\n");
        }
    }
#ifdef __SST_DEBUG_OUTPUT__
    else {
        dbg_->debug(CALL_INFO_LONG, 1, 0, "Injection skipped.\n");
    }
#endif
}