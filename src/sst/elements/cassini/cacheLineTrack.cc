// Copyright 2009-2018 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2018, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

// File:   addrHistogrammer.cc
// Author: Jagan Jayaraj (derived from Cassini prefetcher modules written by Si Hammond)

#include "sst_config.h"
#include "cacheLineTrack.h"

#include <stdint.h>

#include "sst/core/element.h"
#include "sst/core/params.h"
#include <sst/core/unitAlgebra.h>

using namespace SST;
using namespace SST::MemHierarchy;
using namespace SST::Cassini;

cacheLineTrack::cacheLineTrack(Component* owner, Params& params) : CacheListener(owner, params) {
    std::string cutoff_s = params.find<std::string>("addr_cutoff", "16GiB");
    UnitAlgebra cutoff_u(cutoff_s);
    cutoff = cutoff_u.getRoundedValue();

    rdHisto = registerStatistic<Addr>("histogram_reads");
    wrHisto = registerStatistic<Addr>("histogram_writes");

}


void cacheLineTrack::notifyAccess(const CacheListenerNotification& notify) {
    const NotifyAccessType notifyType = notify.getAccessType();
    const NotifyResultType notifyResType = notify.getResultType();

    Addr addr = notify.getTargetAddress();

    //if(notifyResType == MISS || addr >= cutoff) return;

    switch (notifyType) {
    case READ:
        printf("R %llx\n", addr);
        return;
    case WRITE:
        printf("W %llx\n", addr);
        return;
    case EVICT:
        printf("E %llx\n", addr);
        return;
    }
}

void cacheLineTrack::registerResponseCallback(Event::HandlerBase *handler) {
    registeredCallbacks.push_back(handler);
}
