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
#include "addrHistogrammer.h"

#include <stdint.h>

#include "sst/core/element.h"
#include "sst/core/params.h"
#include <sst/core/unitAlgebra.h>

using namespace SST;
using namespace SST::MemHierarchy;
using namespace SST::Cassini;

AddrHistogrammer::AddrHistogrammer(Component* owner, Params& params) : CacheListener(owner, params) {
    std::string cutoff_s = params.find<std::string>("addr_cutoff", "16GiB");
    UnitAlgebra cutoff_u(cutoff_s);
    cutoff = cutoff_u.getRoundedValue();

    captureVirtual = params.find<bool>("virtual_addr", 0);

    rdHisto = registerStatistic<Addr>("histogram_reads");
    wrHisto = registerStatistic<Addr>("histogram_writes");

}


void AddrHistogrammer::notifyAccess(const CacheListenerNotification& notify) {
    const NotifyAccessType notifyType = notify.getAccessType();
    const NotifyResultType notifyResType = notify.getResultType();

    Addr vaddr;
    if (captureVirtual) {
        vaddr = notify.getVirtualAddress();
    } else {
        vaddr = notify.getPhysicalAddress();
    }


    if(notifyType == EVICT || notifyResType != MISS || vaddr >= cutoff) return;

    // // Remove the offset within a bin
    // Addr baseAddr = vaddr & binMask;
    switch (notifyType) {
      case READ:
        // Add to the read histogram
        rdHisto->addData(vaddr);
        return;
      case WRITE:
        // Add to the write hitogram
        wrHisto->addData(vaddr);
        return;
    case EVICT:
        return;
    }
}

void AddrHistogrammer::registerResponseCallback(Event::HandlerBase *handler) {
    registeredCallbacks.push_back(handler);
}
