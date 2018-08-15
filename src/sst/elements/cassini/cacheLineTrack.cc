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
    useHisto = registerStatistic<uint>("histogram_word_accesses");
    ageHisto = registerStatistic<SimTime_t>("histogram_age");

}


void cacheLineTrack::notifyAccess(const CacheListenerNotification& notify) {
    const NotifyAccessType notifyType = notify.getAccessType();
    const NotifyResultType notifyResType = notify.getResultType();

    Addr addr = notify.getTargetAddress(); // target address
    Addr cacheAddr = notify.getPhysicalAddress(); // cacheline (base) address

    // if get a MISS notification, do we get a HIT later?
    if(addr >= cutoff) return;

    // size

    switch (notifyType) {
    case READ:
    case WRITE:
        {
            printf("R/W %llx\n", addr);
            auto iter = cacheLines.find(cacheAddr);
            if (iter == cacheLines.end()) {
                // insert a new one
                SimTime_t now = getSimulation()->getCurrentSimCycle();
                iter = (cacheLines.insert({cacheAddr, lineTrack(now)})).first;
            } 
            // update
            if (notify.getSize() > 8) {
                printf("Not sure what to do here. access size > 8\n");
            }
            Addr offset = (addr - cacheAddr) / 8;
            iter->second.touched[offset] = 1;
            if (notifyType == READ) {
                iter->second.reads++;
            } else {
                iter->second.writes++;
            }   
        }
        break;
    case EVICT:
        printf("E %llx\n", addr);
        // find the cacheline record
        {
            auto iter = cacheLines.find(cacheAddr);
            if (iter != cacheLines.end()) {
                // record it
                rdHisto->addData(iter->second.reads);
                wrHisto->addData(iter->second.writes);
                SimTime_t now = getSimulation()->getCurrentSimCycle();
                ageHisto->addData(now - iter->second.entered);
                uint touched = iter->second.touched.count();
                useHisto->addData(touched);
                printf(" t %d\n", touched);
                //delete it
                cacheLines.erase(iter);
            } else {
                // couldn't find record?
                printf("Not sure what to do here. Couldn't find record\n");
            }
        } 
        break;
    default:
        printf("Invalid notify Type\n");
    }
}

void cacheLineTrack::registerResponseCallback(Event::HandlerBase *handler) {
    registeredCallbacks.push_back(handler);
}
