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

#include "sst/elements/carcosa/injectors/randomFlipFaultInjector.h"
#include "sst/elements/carcosa/faultlogic/randomFlipFault.h"

using namespace SST::Carcosa;

RandomFlipFaultInjector::RandomFlipFaultInjector(Params& params) : FaultInjectorBase(params) {
    // read injection probability
    this->injection_probability_ = params.find("injection_probability", 0.0);
#ifdef __SST_DEBUG_OUTPUT__
    if (injection_probability_ > 0.0){
        dbg_->debug(CALL_INFO_LONG, 1, 0, "Injection probability set to %lf.\n", injection_probability_);
    }
#endif

    // create fault
    fault.push_back(new RandomFlipFault(params, this));
    setValidInstallation(params, SEND_RECEIVE_VALID);
}

bool RandomFlipFaultInjector::doInjection() {
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

void RandomFlipFaultInjector::executeFaults(Event*& ev) {
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