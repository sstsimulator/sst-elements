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

#ifndef _TRIVIALCPU_H
#define _TRIVIALCPU_H

#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS
#endif
#include <inttypes.h>

#include <sst/core/event.h>
#include <sst/core/sst_types.h>
#include <sst/core/component.h>
#include <sst/core/link.h>
#include <sst/core/timeConverter.h>
#include <sst/core/output.h>
#include <sst/core/interfaces/simpleMem.h>
#include <sst/core/rng/marsaglia.h>
#include <sst/core/elementinfo.h>

using namespace SST::Statistics;

namespace SST {
namespace MemHierarchy {


class trivialCPU : public SST::Component {
public:
/* Element Library Info */
    SST_ELI_REGISTER_COMPONENT(trivialCPU, "memHierarchy", "trivialCPU", SST_ELI_ELEMENT_VERSION(1,0,0), 
            "Simple demo CPU for testing", COMPONENT_CATEGORY_PROCESSOR)

    SST_ELI_DOCUMENT_PARAMS(
            {"commFreq",                "(int) How often to do a memory operation."},
            {"memSize",                 "(uint) Size of physical memory."},
            {"verbose",                 "(uint) Determine how verbose the output from the CPU is", "1"},
            {"clock",                   "(string) Clock frequency", "1GHz"},
            {"rngseed",                 "(int) Set a seed for the random generation of addresses", "7"},
            {"lineSize",                "(uint) Size of a cache line - used for flushes", "64"},
            {"maxOutstanding",          "(uint) Maximum Number of Outstanding memory requests.", "10"},
            {"num_loadstore",           "(int) Stop after this many reads and writes.", "-1"},
            {"reqsPerIssue",            "(uint) Maximum number of requests to issue at a time", "1"},
            {"do_write",                "(bool) Enable writes to memory (versus just reads).", "1"},
            {"do_flush",                "(bool) Enable flushes", "0"},
            {"noncacheableRangeStart",  "(uint) Beginning of range of addresses that are noncacheable.", "0x0"},
            {"noncacheableRangeEnd",    "(uint) End of range of addresses that are noncacheable.", "0x0"},
            {"addressoffset",           "(uint) Apply an offset to a calculated address to check for non-alignment issues", "0"} )

    SST_ELI_DOCUMENT_PORTS( {"mem_link", "Connection to cache", { "memHierarchy.MemEventBase" } } )

    SST_ELI_DOCUMENT_STATISTICS( {"pendCycle", "Number of pending requests per cycle", "count", 1} )

/* Begin class definition */
    trivialCPU(SST::ComponentId_t id, SST::Params& params);
    void init();
    void finish() {
    	out.verbose(CALL_INFO, 1, 0, "TrivialCPU %s Finished after %" PRIu64 " issued reads, %" PRIu64 " returned (%" PRIu64 " clocks)\n",
    		getName().c_str(), num_reads_issued, num_reads_returned, clock_ticks);
    	if ( noncacheableReads || noncacheableWrites )
	    out.verbose(CALL_INFO, 1, 0, "\t%zu Noncacheable Reads\n\t%zu Noncacheable Writes\n", noncacheableReads, noncacheableWrites);

    	//out.output("Number of Pending Requests per Cycle (Binned by 2 Requests)\n");
    	//for(uint64_t i = requestsPendingCycle->getBinStart(); i < requestsPendingCycle->getBinEnd(); i += requestsPendingCycle->getBinWidth()) {
        //    out.output("  [%" PRIu64 ", %" PRIu64 "]  %" PRIu64 "\n",
	    //	i, i + requestsPendingCycle->getBinWidth(), requestsPendingCycle->getBinCountByBinStart(i));
	    //}
    }

private:
    trivialCPU();  // for serialization only
    trivialCPU(const trivialCPU&); // do not implement
    void operator=(const trivialCPU&); // do not implement
    void init(unsigned int phase);

    void handleEvent( Interfaces::SimpleMem::Request *ev );
    virtual bool clockTic( SST::Cycle_t );

    Output out;
    int numLS;
    int commFreq;
    bool do_write;
    bool do_flush;
    uint64_t maxAddr;
    uint64_t lineSize;
    uint64_t maxOutstanding;
    uint32_t maxReqsPerIssue;
    uint64_t num_reads_issued, num_reads_returned;
    uint64_t noncacheableRangeStart, noncacheableRangeEnd, noncacheableSize;
    uint64_t clock_ticks;
    size_t noncacheableReads, noncacheableWrites;
    Statistic<uint64_t>* requestsPendingCycle;

    std::map<uint64_t, SimTime_t> requests;

    Interfaces::SimpleMem *memory;

    SST::RNG::MarsagliaRNG rng;

    TimeConverter *clockTC;
    Clock::HandlerBase *clockHandler;

};

}
}
#endif /* _TRIVIALCPU_H */
