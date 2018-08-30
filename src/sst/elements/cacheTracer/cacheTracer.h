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

#ifndef _CACHETRACER_H
#define _CACHETRACER_H

#include <sst/core/output.h>
#include <sst/core/elementinfo.h>
#include <sst/core/event.h>
#include <sst/core/component.h>
#include <sst/core/params.h>
#include <sst/core/simulation.h>
#include <sst/core/timeLord.h>
#include <sst/core/interfaces/simpleMem.h>
#include <sst/core/sst_types.h>
#include <sst/core/link.h>
#include <sst/core/timeConverter.h>
#include <sst/elements/memHierarchy/memEvent.h>
#include <assert.h>
#include <errno.h>
#include <execinfo.h>

#include <iostream>
#include <fstream>
#include <map>

using namespace std;
using namespace SST;
using namespace SST::MemHierarchy;

namespace SST{
namespace CACHETRACER {

class cacheTracer : public SST::Component {
public:
    cacheTracer(SST::ComponentId_t id, Params& params);
    ~cacheTracer();
    void finish();
    void init(unsigned int phase);

    SST_ELI_REGISTER_COMPONENT(
       	cacheTracer,
       	"cacheTracer",
       	"cacheTracer",
       	SST_ELI_ELEMENT_VERSION(1,0,0),
       	"Cache Tracing Plugin for MemHierarchy",
	COMPONENT_CATEGORY_UNCATEGORIZED
    )

    SST_ELI_DOCUMENT_PARAMS(
	{ "clock", "Frequency, same as system clock frequency", "1 GHz" },
    	{ "statsPrefix", "writes stats to statsPrefix file", "" },
    	{ "tracePrefix", "writes trace to tracePrefix tracing is enable", "" },
    	{ "debug", "Print debug statements with increasing verbosity [0-10]", "0" },
    	{ "statistics", "0-No-stats, 1-print-stats", "0" },
    	{ "pageSize", "Page Size (bytes), used for selecting number of bins for address histogram ", "4096" },
    	{"accessLatencyBins", "Number of bins for access latency histogram" "10" }
    )

    SST_ELI_DOCUMENT_PORTS(
	{ "northBus", "Connect towards cpu side", { "memHierarchy.MemEvent", "" } },
    	{ "southBus", "Connect towards memory side", { "memHierarchy.MemEvent", "" } }
    )

private:
    // Functions
    bool clock(SST::Cycle_t);
    void FinalStats(FILE*, unsigned int);
    void PrintAddrHistogram(FILE*, vector<SST::MemHierarchy::Addr>);
    void PrintAccessLatencyDistribution(FILE*, unsigned int);

    Output* out;
    FILE* traceFile;
    FILE* statsFile;

    // Links
    SST::Link *northBus;
    SST::Link *southBus;

    // Input Parameters
    unsigned int stats;
    unsigned int pageSize;
    unsigned int accessLatBins;

    // Flags
    bool writeTrace;
    bool writeStats;
    bool writeDebug_8;

    unsigned int nbCount;
    unsigned int sbCount;
    uint64_t timestamp;

    vector<SST::MemHierarchy::Addr>AddrHist;   // Address Histogram
    vector<unsigned int> AccessLatencyDist;

    map<MemEvent::id_type,uint64_t>InFlightReqQueue;

    TimeConverter* picoTimeConv;
    TimeConverter* nanoTimeConv;

    // Serialization
    cacheTracer();                         // for serialization only
    cacheTracer(const cacheTracer&);      // do not implement
    void operator=(const cacheTracer&);    // do not implement
}; // class cacheTracer

} // namespace CACHETRACER
} // namespace SST

#endif //_CACHETRACER_H

