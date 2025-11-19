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

DoubleFaultInjector::DoubleFaultInjector(Params& params) : FaultInjectorBase(params) {
    // create fault
    fault.resize(2);
    fault[0] = new RandomDropFault(params, this);
    fault[1] = new RandomFlipFault(params, this);

    // read flipvsdrop param
    flipVsDropProb_ = params.find<double>("flipVsDropProb", 0.5);

    setValidInstallation(params, SEND_RECEIVE_VALID);
}

/**
 * Overridden execution function to cause faults to be chosen at random
 * from the vector once a fault has been triggered
 */
void DoubleFaultInjector::executeFaults(Event*& ev) {
    FaultBase* triggered_fault = nullptr;
    if (rng_.nextUniform() <= flipVsDropProb_) {
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