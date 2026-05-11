// Copyright 2009-2026 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2026, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// of the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include <sst_config.h>
#include "sst/elements/carcosa/components/faultInjCPU.h"
#include "sst/elements/carcosa/components/faultInjEvent.h"

#include "sst/elements/memHierarchy/util.h"

using namespace SST;
using namespace SST::Interfaces;
using namespace SST::MemHierarchy;

FaultInjCPU::FaultInjCPU(ComponentId_t id, Params& params)
    : CarcosaCPUBase(id, params)
{
    currentFaultRate = 0.1;
}

bool FaultInjCPU::clockTic(Cycle_t cycle)
{
    // Send fault injection commands every 100 clock cycles
    if (clock_ticks % 100 == 0 && clock_ticks > 0) {
        Carcosa::FaultInjEvent* ev = nullptr;
        unsigned commandType = (clock_ticks / 100) % 4;

        switch (commandType) {
            case 0:
                ev = new Carcosa::FaultInjEvent("injection_rate", 0.0, currentFaultRate);
#ifdef __SST_DEBUG_OUTPUT__
                out.output("%s: Sent injection_rate %.2f at cycle %" PRIu64 "\n",
                           getName().c_str(), currentFaultRate, clock_ticks);
#endif
                break;
            case 1:
                ev = new Carcosa::FaultInjEvent("set_range 0.1", 0.0, 0.9);
#ifdef __SST_DEBUG_OUTPUT__
                out.output("%s: Sent set_range 0.1 0.9 at cycle %" PRIu64 "\n",
                           getName().c_str(), clock_ticks);
#endif
                break;
            case 2:
                ev = new Carcosa::FaultInjEvent("config 0x1000 64", 0.0, 0.0);
#ifdef __SST_DEBUG_OUTPUT__
                out.output("%s: Sent config 0x1000 64 at cycle %" PRIu64 "\n",
                           getName().c_str(), clock_ticks);
#endif
                break;
            case 3:
                ev = new Carcosa::FaultInjEvent("disable_faults", 0.0, 0.0);
#ifdef __SST_DEBUG_OUTPUT__
                out.output("%s: Sent disable_faults at cycle %" PRIu64 "\n",
                           getName().c_str(), clock_ticks);
#endif
                break;
        }

        if (ev) {
            HaliLink->send(ev);
        }

        currentFaultRate += 0.05;
        if (currentFaultRate >= 1.0) currentFaultRate = 0.1;
    }

    return CarcosaCPUBase::clockTic(cycle);
}
