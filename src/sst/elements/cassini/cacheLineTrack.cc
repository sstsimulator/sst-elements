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

static const int tab64[64] = {
    63,  0, 58,  1, 59, 47, 53,  2,
    60, 39, 48, 27, 54, 33, 42,  3,
    61, 51, 37, 40, 49, 18, 28, 20,
    55, 30, 34, 11, 43, 14, 22,  4,
    62, 57, 46, 52, 38, 26, 32, 41,
    50, 36, 17, 19, 29, 10, 13, 21,
    56, 45, 25, 31, 35, 16,  9, 12,
    44, 24, 15,  8, 23,  7,  6,  5};

static int log2_64(uint64_t value)
{
    value |= value >> 1;
    value |= value >> 2;
    value |= value >> 4;
    value |= value >> 8;
    value |= value >> 16;
    value |= value >> 32;
    return tab64[((uint64_t)((value - (value >> 1))*0x07EDD5E59A4E28C2)) >> 58];
}

cacheLineTrack::cacheLineTrack(Component* owner, Params& params) : CacheListener(owner, params) {
    std::string cutoff_s = params.find<std::string>("addr_cutoff", "16GiB");
    UnitAlgebra cutoff_u(cutoff_s);
    cutoff = cutoff_u.getRoundedValue();

    rdHisto = registerStatistic<Addr>("hist_reads_log2");
    wrHisto = registerStatistic<Addr>("hist_writes_log2");
    useHisto = registerStatistic<uint>("hist_word_accesses");
    ageHisto = registerStatistic<SimTime_t>("hist_age_log2");
    evicts = registerStatistic<uint>("evicts");

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
            auto iter = cacheLines.find(cacheAddr);
            if (iter == cacheLines.end()) {
                // insert a new one
                SimTime_t now = getSimulation()->getCurrentSimCycle();
                iter = (cacheLines.insert({cacheAddr, lineTrack(now)})).first;
            } 
            // update
            if (notify.getSize() > 8) {
	      //printf("Not sure what to do here. access size > 8, %d\n", notify.getSize());
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
        // find the cacheline record
        {
            auto iter = cacheLines.find(cacheAddr);
            if (iter != cacheLines.end()) {
                // record it
                rdHisto->addData(log2_64(iter->second.reads));
                wrHisto->addData(log2_64(iter->second.writes));
                SimTime_t now = getSimulation()->getCurrentSimCycle();
                ageHisto->addData(log2_64(now - iter->second.entered));
                uint touched = iter->second.touched.count();
                useHisto->addData(touched);
		evicts->addData(1);
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
