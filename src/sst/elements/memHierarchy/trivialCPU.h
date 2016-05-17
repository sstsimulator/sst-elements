// Copyright 2009-2015 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2015, Sandia Corporation
// All rights reserved.
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
#include <sst/core/stats/histo/histo.h>
#include <sst/core/interfaces/simpleMem.h>
#include <sst/core/rng/marsaglia.h>


using namespace SST::Statistics;

namespace SST {
namespace MemHierarchy {


class trivialCPU : public SST::Component {
public:

    trivialCPU(SST::ComponentId_t id, SST::Params& params);
    void init();
    void finish() {
    	out.output("TrivialCPU %s Finished after %" PRIu64 " issued reads, %" PRIu64 " returned (%" PRIu64 " clocks)\n",
    		getName().c_str(), num_reads_issued, num_reads_returned, clock_ticks);
    	if ( noncacheableReads || noncacheableWrites )
	    out.output("\t%zu Noncacheable Reads\n\t%zu Noncacheable Writes\n", noncacheableReads, noncacheableWrites);

    	out.output("Number of Pending Requests per Cycle (Binned by 2 Requests)\n");
    	for(uint64_t i = requestsPendingCycle->getBinStart(); i < requestsPendingCycle->getBinEnd(); i += requestsPendingCycle->getBinWidth()) {
            out.output("  [%" PRIu64 ", %" PRIu64 "]  %" PRIu64 "\n",
	    	i, i + requestsPendingCycle->getBinWidth(), requestsPendingCycle->getBinCountByBinStart(i));
	}
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
    uint64_t maxAddr;
    uint64_t num_reads_issued, num_reads_returned;
    uint64_t noncacheableRangeStart, noncacheableRangeEnd;
    uint64_t clock_ticks;
    size_t noncacheableReads, noncacheableWrites;
    Histogram<uint64_t, uint64_t>* requestsPendingCycle;

    std::map<uint64_t, SimTime_t> requests;

    Interfaces::SimpleMem *memory;

    SST::RNG::MarsagliaRNG rng;

    TimeConverter *clockTC;
    Clock::HandlerBase *clockHandler;

};

}
}
#endif /* _TRIVIALCPU_H */
