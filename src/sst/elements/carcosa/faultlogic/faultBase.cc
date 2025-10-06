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

#include "sst/elements/carcosa/faultlogic/faultBase.h"

using namespace SST::Carcosa;

FaultBase::FaultBase(Params& params, FaultInjectorBase* injector) : injector_(injector) {
    //
}

bool FaultBase::faultLogic(Event*& ev) {
    return true;
}

SST::Output* FaultBase::getSimulationOutput() {
    return injector_->getOutput();
}

SST::Output* FaultBase::getSimulationDebug() {
    return injector_->getDebug();
}

SST::MemHierarchy::MemEvent* FaultBase::convertMemEvent(Event*& ev) {
    SST::MemHierarchy::MemEvent* mem_ev = dynamic_cast<SST::MemHierarchy::MemEvent*>(ev);

    if (mem_ev == nullptr) {
        getSimulationOutput()->fatal(CALL_INFO_LONG, -1, "Attempting to inject mem fault on a non-MemEvent type.\n");
    }

#ifdef __SST_DEBUG_OUTPUT__
    getSimulationDebug()->debug(CALL_INFO_LONG, 2, 0, "Intercepted event %zu/%d\n", mem_ev->getID().first, mem_ev->getID().second);
#endif
    return mem_ev;
}

dataVec& FaultBase::getMemEventPayload(Event*& ev) {
    return convertMemEvent(ev)->getPayload();
}

void FaultBase::setMemEventPayload(Event*& ev, dataVec newPayload) {
#ifdef __SST_DEBUG_OUTPUT__
    getSimulationDebug()->debug(CALL_INFO_LONG, 2, 0, "Payload before replacement:\n[");
    for (int i: getMemEventPayload(ev)) {
        getSimulationDebug()->debug(CALL_INFO_LONG, 2, 0, "%d\t", i);
    }
    getSimulationDebug()->debug(CALL_INFO_LONG, 2, 0, "]\n");
#endif
    convertMemEvent(ev)->setPayload(newPayload);

#ifdef __SST_DEBUG_OUTPUT__
    getSimulationDebug()->debug(CALL_INFO_LONG, 2, 0, "Payload after replacement:\n[");
    for (int i: getMemEventPayload(ev)) {
        getSimulationDebug()->debug(CALL_INFO_LONG, 2, 0, "%d\t", i);
    }
    getSimulationDebug()->debug(CALL_INFO_LONG, 2, 0, "]\n");
#endif
}

