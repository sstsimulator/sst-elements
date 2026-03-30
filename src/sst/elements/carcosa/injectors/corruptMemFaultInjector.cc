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

#include "sst/elements/carcosa/injectors/corruptMemFaultInjector.h"
#include "sst/elements/carcosa/faultlogic/corruptMemFault.h"

using namespace SST::Carcosa;

CorruptMemFaultInjector::CorruptMemFaultInjector(Params& params) : FaultInjectorBase(params) {
    // create fault
    fault.push_back(new CorruptMemFault(params, this));
    setValidInstallation(params, SEND_RECEIVE_VALID);
}

void CorruptMemFaultInjector::executeFaults(Event*& ev) {
    // is this addr in a corrupt region?
    std::vector<uint32_t>* regionsToUse = dynamic_cast<CorruptMemFault*>(fault[0])->checkAddrUsage(ev);
    // if returned vec is not empty, save to fault-accessible location and execute
    if (regionsToUse->size() != 0) {
#ifdef __SST_DEBUG_OUTPUT__
        dbg_->debug(CALL_INFO_LONG, 2, 0, "Corruption region detected.\n");
#endif
        if (!fault[0]) {
            out_->fatal(CALL_INFO_LONG, -1, "No valid fault to execute.\n");
        }
        if (!fault[0]->faultLogic(ev)) {
            out_->fatal(CALL_INFO_LONG, -1, "Fault somehow returned unsuccessful... How?\n");
        }
        // reset vec
        regionsToUse->clear();
    }
}