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

#include "sst/elements/carcosa/injectors/randomDropFaultInjector.h"
#include "sst/elements/carcosa/faultlogic/randomDropFault.h"

using namespace SST::Carcosa;

RandomDropFaultInjector::RandomDropFaultInjector(Params& params) : FaultInjectorBase(params) {
    // read injection probability
    injection_probability_ = params.find<double>("injection_probability", 0.0);
    // create fault
    fault.push_back(new RandomDropFault(params, this));
    setValidInstallation(params, RECEIVE_VALID);
}

bool RandomDropFaultInjector::doInjection() {
    if (this->randFloat(0.0, 1.0) <= this->injection_probability_) {
#ifdef __SST_DEBUG_OUTPUT__
        dbg_->debug(CALL_INFO_LONG, 1, 0, "Injection triggered.\n");
#endif
        return true;
    } else {
#ifdef __SST_DEBUG_OUTPUT__
        dbg_->debug(CALL_INFO_LONG, 1, 0, "Injection skipped.\n");
#endif
        return false;
    }
}

/**
 * Custom execution is required to ensure delivery is canceled
 *
 * In the base interceptHandler, a reference to a boolean called
 * 'cancel' is accepted as an argument. That function assigns the
 * injector's pointer (called 'cancel_') to that reference's address,
 * and that reference must be updated here after the event is destroyed
 * if the installation direction of this PortModule was set to 'Receive'.
 */
void RandomDropFaultInjector::executeFaults(Event*& ev) {
    if (fault[0]) {
        if (this->doInjection()) {
            if (!fault[0]->faultLogic(ev)) {
                out_->fatal(CALL_INFO_LONG, -1, "Fault execution failed.\n");
            }
        }
    } else {
        out_->fatal(CALL_INFO_LONG, -1, "No valid fault object.\n");
    }
}