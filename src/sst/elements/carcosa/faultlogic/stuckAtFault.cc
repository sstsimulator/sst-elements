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

#include "stuckAtFault.h"

using namespace SST::Carcosa;

/********** StuckAtFault **********/

StuckAtFault::StuckAtFault(Params& params) : FaultInjectorBase(params) 
{
    // assign fault logic function
    this->faultLogic = &StuckAtFault::fault;

#ifdef DEBUG
    getSimulationOutput().debug(CALL_INFO_LONG, 1, 0, "\Fault Type: Stuck-At Fault");
#endif
}

void fault(Event&* ev) {
    // see comment in header file
}