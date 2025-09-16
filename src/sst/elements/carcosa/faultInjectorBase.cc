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

#include "sst/elements/carcosa/faultInjectorBase.h"
#include "sst/core/params.h"
#include "sst/elements/carcosa/faultlogic/stuckAtFault.h"

using namespace SST::Carcosa;

/************** FaultBase **************/

FaultInjectorBase::FaultBase::FaultBase(Params& params, FaultInjectorBase* injector) : _injector(injector)
{
    // what do we need in here?
}

SST::Output*& FaultInjectorBase::FaultBase::getSimulationOutput() {
    return _injector->out_;
}

SST::Output*& FaultInjectorBase::FaultBase::getSimulationDebug() {
    return _injector->dbg_;
}

SST::MemHierarchy::MemEvent* FaultInjectorBase::FaultBase::convertMemEvent(Event*& ev) {
    SST::MemHierarchy::MemEvent* mem_ev = dynamic_cast<SST::MemHierarchy::MemEvent*>(ev);

    if (mem_ev == nullptr) {
        _injector->getSimulationOutput().fatal(CALL_INFO_LONG, -1, "Attempting to inject mem fault on a non-MemEvent type.\n");
    }

#ifdef __SST_DEBUG_OUTPUT__
    _injector->dbg_->debug(CALL_INFO_LONG, 2, 0, "Intercepted event %zu/%d\n", mem_ev->getID().first, mem_ev->getID().second);
#endif
    return mem_ev;
}

dataVec& FaultInjectorBase::FaultBase::getMemEventPayload(Event*& ev) {
    return convertMemEvent(ev)->getPayload();
}

void FaultInjectorBase::FaultBase::setMemEventPayload(Event*& ev, dataVec newPayload) {
#ifdef __SST_DEBUG_OUTPUT__
    _injector->getSimulationOutput().debug(CALL_INFO_LONG, 1, 0, "Payload before replacement:\n");
    for (int i: convertMemEvent(ev)->getPayload()) {
        _injector->getSimulationOutput().debug(CALL_INFO_LONG, 1, 0, "%d\t");
    }
    _injector->getSimulationOutput().debug(CALL_INFO_LONG, 1, 0, "\n");
#endif
    convertMemEvent(ev)->setPayload(newPayload);

#ifdef __SST_DEBUG_OUTPUT__
    _injector->getSimulationOutput().debug(CALL_INFO_LONG, 1, 0, "Payload after replacement:\n");
    for (int i: convertMemEvent(ev)->getPayload()) {
        _injector->getSimulationOutput().debug(CALL_INFO_LONG, 1, 0, "%d\t");
    }
    _injector->getSimulationOutput().debug(CALL_INFO_LONG, 1, 0, "\n");
#endif
}

FaultInjectorBase::FaultBase::memEventType FaultInjectorBase::FaultBase::getMemEventCommandType(Event*& ev) {
    SST::MemHierarchy::MemEvent* mem_ev = convertMemEvent(ev);
    if (mem_ev->isDataRequest()) {
        return FaultBase::memEventType::DataRequest;
    } else if (mem_ev->isResponse()) {
        return FaultBase::memEventType::Response;
    } else if (mem_ev->isWriteback()) {
        return FaultBase::memEventType::Writeback;
    } else if (mem_ev->isRoutedByAddress()) {
        return FaultBase::memEventType::RoutedByAddr;
    } else {
        return FaultBase::memEventType::Invalid;
    }
}

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
    std::string install_dir = params.find<string>("installDirection", "Receive");

    if ( install_dir != "Receive" ) {
        if ( install_dir == "Send" ) {
            installDirection_ = installDirection::Send;
        } else {
            installDirection_ = installDirection::Receive;
        }
    }
#ifdef __SST_DEBUG_OUTPUT__
    dbg_->debug(CALL_INFO_LONG, 1, 0, "\tInstall Direction: %s\n", install_dir.c_str());
#endif

    injectionProbability_ = params.find<double>("injectionProbability", "0.5");
    if ( injectionProbability_ < 0.0 || injectionProbability_ > 1.0 ) {
        out_->fatal(CALL_INFO_LONG, -1, "\tInjection probability outside of bounds. Must be in the following range: [0.0,1.0].\n");
    }
#ifdef __SST_DEBUG_OUTPUT__
    dbg_->debug(CALL_INFO_LONG, 1, 0, "\tInjection Probability: %f\n", injectionProbability_);
#endif
    
    fault = new StuckAtFault(params, this);//FaultBase(params, this);
}

void
FaultInjectorBase::eventSent(uintptr_t key, Event*& ev) 
{
    fault->faultLogic(ev);
}

void
FaultInjectorBase::interceptHandler(uintptr_t key, Event*& ev, bool& cancel) 
{
    // do not cancel delivery by default
    cancel = false;

    fault->faultLogic(ev);
}