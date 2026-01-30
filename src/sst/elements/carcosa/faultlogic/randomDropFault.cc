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

#include "sst/elements/carcosa/faultlogic/randomDropFault.h"

using namespace SST::Carcosa;

RandomDropFault::RandomDropFault(Params& params, FaultInjectorBase* injector) : FaultBase(params, injector) {
    //
}

bool RandomDropFault::faultLogic(Event*& ev) {
    SST::MemHierarchy::MemEvent* mem_ev = convertMemEvent(ev);

    delete mem_ev;
    if (injector_->getInstallDirection() == installDirection::Receive) {
        injector_->cancelDelivery();
    }
#ifdef __SST_DEBUG_OUTPUT__
    getSimulationDebug()->debug(CALL_INFO_LONG, 1, 0, "Event dropped.\n");
#endif
    return true;
}