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

#include "sst/elements/carcosa/injectors/stuckAtFaultInjector.h"
#include "sst/elements/carcosa/faultlogic/stuckAtFault.h"

using namespace SST::Carcosa;

StuckAtFaultInjector::StuckAtFaultInjector(Params& params) : FaultInjectorBase(params) {
    // create fault
    fault.push_back(new StuckAtFault(params, this));
    setValidInstallation(params, SEND_RECEIVE_VALID);
}