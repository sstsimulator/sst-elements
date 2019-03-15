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


#ifndef _H_SST_ADDR_HISTOGRAMMER
#define _H_SST_ADDR_HISTOGRAMMER

#include <bitset>

#include <sst/core/event.h>
#include <sst/core/sst_types.h>
#include <sst/core/component.h>
#include <sst/core/link.h>
#include <sst/core/timeConverter.h>
#include <sst/core/output.h>
#include <sst/elements/memHierarchy/memEvent.h>
#include <sst/elements/memHierarchy/cacheListener.h>

#include <sst/core/elementinfo.h>

using namespace SST;
using namespace SST::MemHierarchy;
using namespace std;

namespace SST {
namespace Cassini {

struct lineTrack {
    bitset<8> touched; // currently hardcoded for 64B (8-word) lines
    SimTime_t entered; // when the line entered the cache
    uint64_t reads;
    uint64_t writes;

    lineTrack(SimTime_t now) : touched(0), entered(now), reads(0), writes(0) {;}
};

class cacheLineTrack : public SST::MemHierarchy::CacheListener {
public:
    cacheLineTrack(Component*, Params& params);
    ~cacheLineTrack() {};

    void notifyAccess(const CacheListenerNotification& notify);
    void registerResponseCallback(Event::HandlerBase *handler);

    SST_ELI_REGISTER_SUBCOMPONENT(
        cacheLineTrack,
            "cassini",
            "cacheLineTrack",
            SST_ELI_ELEMENT_VERSION(1,0,0),
            "Tracks cacheline usage before eviction",
            "SST::Cassini::CacheListener"
    )

    SST_ELI_DOCUMENT_PARAMS(
                            { "addr_cutoff", "Addresses above this cutoff won't be recorded", "1TB" }
    )

    SST_ELI_DOCUMENT_STATISTICS(
        { "hist_reads_log2", "Histogram of log2(cacheline reads before eviction)", "counts", 1 },
        { "hist_writes_log2", "Histogram of log2(cacheline write before eviction)", "counts", 1 },
        { "hist_age_log2", "Histogram of log2(cacheline ages before eviction)", "counts", 1 },
        { "hist_word_accesses", "Histogram of cacheline words accessed before eviction", "counts", 1 },
        { "evicts", "Number of evictions seen", "counts", 1 }
    )

private:
    typedef unordered_map<Addr, lineTrack> cacheTrack_t;
    cacheTrack_t cacheLines;
    std::vector<Event::HandlerBase*> registeredCallbacks;
    bool captureVirtual; 
    Addr cutoff; // Don't bin addresses above the cutoff. Helps avoid creating
                //  histogram entries for the vast address range between the
                //  heap and the stack.
    Statistic<Addr>* rdHisto;
    Statistic<Addr>* wrHisto;
    Statistic<uint>* useHisto;
    Statistic<SimTime_t>* ageHisto;
    Statistic<uint>* evicts;
};

}
}

#endif
