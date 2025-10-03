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

    injectionProbability_ = params.find<double>("injectionProbability", "0.5");
    if ( injectionProbability_ < 0.0 || injectionProbability_ > 1.0 ) {
        out_->fatal(CALL_INFO_LONG, -1, "\tInjection probability outside of bounds. Must be in the following range: [0.0,1.0].\n");
    }
#ifdef __SST_DEBUG_OUTPUT__
    dbg_->debug(CALL_INFO_LONG, 1, 0, "\tInjection Probability: %f\n", injectionProbability_);
#endif
    // init randomizer 
    distribution_ = std::uniform_real_distribution<double>(0,1);
}

void
FaultInjectorBase::eventSent(uintptr_t key, Event*& ev) 
{
    if (doInjection()){
#ifdef __SST_DEBUG_OUTPUT__
        dbg_->(CALL_INFO_LONG, 1, 0, "Injection triggered.\n")
#endif
        if (fault) {
            fault->faultLogic(ev);
        } else {
            out_->fatal(CALL_INFO_LONG, -1, "No valid fault object.\n");
        }
    }
#ifdef __SST_DEBUG_OUTPUT__
    else {
        dbg_->(CALL_INFO_LONG, 1, 0, "Injection skipped.\n")
    }
#endif
    
}

void
FaultInjectorBase::interceptHandler(uintptr_t key, Event*& ev, bool& cancel) 
{
    // do not cancel delivery by default
    cancel = false;
    cancel_ = &cancel;

    if (doInjection()){
#ifdef __SST_DEBUG_OUTPUT__
        dbg_->(CALL_INFO_LONG, 1, 0, "Injection triggered.\n")
#endif
        if (fault) {
            fault->faultLogic(ev);
        } else {
            out_->fatal(CALL_INFO_LONG, -1, "No valid fault object.\n");
        }
    }
#ifdef __SST_DEBUG_OUTPUT__
    else {
        dbg_->(CALL_INFO_LONG, 1, 0, "Injection skipped.\n")
    }
#endif
}

bool FaultInjectorBase::doInjection() {
    double rand_val = distribution_(generator_);
    return rand_val <= injector_->getInjectionProb();
}

void FaultInjectorBase::setInstallDirection(std::string param) {
    if ( param == "Receive" ) {
        if (valid_installation_[0]) {
            installDirection_= installDirection::Receive;
        } else {
            injector_->out_->fatal(CALL_INFO_LONG, 1, 0, "This PortModule Fault Injector cannot intercept Receive events.\n");
        }
    } else if ( param == "Send" ) {
        if (valid_installation_[1]) {
            installDirection_ = return installDirection::Send;
        } else {
            injector_->out_->fatal(CALL_INFO_LONG, 1, 0, "This PortModule Fault Injector cannot intercept Send events.\n");
        }
    }
    installDirection_= installDirection::Invalid;
}

FaultInjectorBase::memEventType FaultInjectorBase::getMemEventCommandType(Event*& ev) {
    SST::MemHierarchy::MemEvent* mem_ev = convertMemEvent(ev);
    if (mem_ev->isDataRequest()) {
        return FaultInjectorBase::memEventType::DataRequest;
    } else if (mem_ev->isResponse()) {
        return FaultInjectorBase::memEventType::Response;
    } else if (mem_ev->isWriteback()) {
        return FaultInjectorBase::memEventType::Writeback;
    } else if (mem_ev->isRoutedByAddress()) {
        return FaultInjectorBase::memEventType::RoutedByAddr;
    } else {
        return FaultInjectorBase::memEventType::Invalid;
    }
}