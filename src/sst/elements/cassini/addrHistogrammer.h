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

class AddrHistogrammer : public SST::MemHierarchy::CacheListener {
public:
    AddrHistogrammer(Component*, Params& params);
    ~AddrHistogrammer() {};

    void notifyAccess(const CacheListenerNotification& notify);
    void registerResponseCallback(Event::HandlerBase *handler);

    SST_ELI_REGISTER_SUBCOMPONENT(
        AddrHistogrammer,
            "cassini",
            "AddrHistogrammer",
            SST_ELI_ELEMENT_VERSION(1,0,0),
            "Address access histogram generator",
            "SST::Cassini::CacheListener"
    )

    SST_ELI_DOCUMENT_PARAMS(
                            { "addr_cutoff", "Addresses above this cutoff won't be recorded", "1TB" },
                            { "virtual_addr", "Record virtual addresses (1) or physical (0)", 0}
    )

    SST_ELI_DOCUMENT_STATISTICS(
        { "histogram_reads", "Histogram of page read counts", "counts", 1 },
        { "histogram_writes", "Histogram of page write counts", "counts", 1 }
    )

private:
    std::vector<Event::HandlerBase*> registeredCallbacks;
    bool captureVirtual; 
    Addr cutoff; // Don't bin addresses above the cutoff. Helps avoid creating
                //  histogram entries for the vast address range between the
                //  heap and the stack.
    Statistic<Addr>* rdHisto;
    Statistic<Addr>* wrHisto;
};

}
}

#endif
