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

#include <sst_config.h>
#include "sst/elements/carcosa/faultInjectorBase.h"
#include "sst/core/params.h"

using namespace SST::Carcosa;

/************** FaultBase **************/

FaultInjectorBase::FaultBase::FaultBase(Params& params, FaultInjectorBase* injector) : _injector(injector)
{
    // what do we need in here?
}

SST::MemHierarchy::MemEvent* FaultInjectorBase::FaultBase::convertMemEvent(Event*& ev) {
    SST::MemHierarchy::MemEvent* mem_ev = dynamic_cast<SST::MemHierarchy::MemEvent*>(ev);

    if (mem_ev == nullptr) {
        _injector->getSimulationOutput().fatal(CALL_INFO_LONG, -1, "Attempting to inject mem fault on a non-MemEvent type.\n");
    }
    return mem_ev;
}

/********** FaultInjectorBase **********/

FaultInjectorBase::FaultInjectorBase(SST::Params& params) : PortModule()
{
#ifdef DEBUG
    getSimulationOutput().debug(CALL_INFO_LONG, 1, 0, "Initializing FaultInjector:\n")
#endif
    std::string install_dir = params.find<string>("installDirection", "Receive");

    if ( install_dir != "Receive" ) {
        if ( install_dir == "Send" ) {
            installDirection_ = installDirection::Send;
        } else {
            installDirection_ = installDirection::Receive;
        }
    }
#ifdef DEBUG
    getSimulationOutput().debug(CALL_INFO_LONG, 1, 0, "\tInstall Direction: %s\n", install_dir);
#endif

    injectionProbability_ = params.find<double>("injectionProbability", "0.5");
    if ( injectionProbability_ < 0.0 || injectionProbability_ > 1.0 ) {
        getSimulationOutput().fatal(CALL_INFO_LONG, -1, "\tInjection probability outside of bounds. Must be in the following range: [0.0,1.0].\n");
    }
#ifdef DEBUG
    getSimulationOutput().debug(CALL_INFO_LONG, 1, 0, "\tInjection Probability: %d\n", injectionProbability_);
#endif
    
    fault = new FaultBase(params, this);
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