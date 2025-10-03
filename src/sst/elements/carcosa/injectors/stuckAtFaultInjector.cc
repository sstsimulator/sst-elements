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
    fault = new StuckAtFault(params, this);
    // toggle valid installation direction
    toggleSendReceiveValid();

    std::string install_dir = params.find<std::string>("installDirection", "Receive");
    installDirection_ = setInstallDirection(install_dir);

    if (installDirection_ == installDirection::Invalid) {
        out_->fatal(CALL_INFO_LONG, -1, "Install Direction should never be set to Invalid! Did you forget to set which directions are valid?\n");
    }

#ifdef __SST_DEBUG_OUTPUT__
    dbg_->debug(CALL_INFO_LONG, 1, 0, "\tInstall Direction: %s\n", install_dir.c_str());
#endif
}