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

#ifndef _streamCPU_H
#define _streamCPU_H

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
#include <sst/core/elementinfo.h>

#include <sst/core/interfaces/simpleMem.h>
#include <sst/core/rng/marsaglia.h>
#include "memEvent.h"

namespace SST {
namespace MemHierarchy {

class streamCPU : public SST::Component {
public:
/* Element Library Info */
    SST_ELI_REGISTER_COMPONENT(streamCPU, "memHierarchy", "streamCPU", SST_ELI_ELEMENT_VERSION(1,0,0),
            "Simple demo streaming CPU for testing", COMPONENT_CATEGORY_PROCESSOR)

    SST_ELI_DOCUMENT_PARAMS(
            {"commFreq",                "(int) How often to do a memory operation."},
            {"memSize",                 "(uint) Size of physical memory."},
            {"verbose",                 "(uint) Determine how verbose the output from the CPU is", "0"},
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

/* Begin class definiton */
    streamCPU(SST::ComponentId_t id, SST::Params& params);
    void init();
    void finish() {
        out.output("streamCPU Finished after %" PRIu64 " issued reads, %" PRIu64 " returned\n",
                num_reads_issued, num_reads_returned);
        out.output("Completed @ %" PRIu64 " ns\n", getCurrentSimTimeNano());
    }

private:
    streamCPU();  // for serialization only
    streamCPU(const streamCPU&); // do not implement
    void operator=(const streamCPU&); // do not implement
    void init(unsigned int phase);
    
    void handleEvent( SST::Interfaces::SimpleMem::Request * req );
    virtual bool clockTic( SST::Cycle_t );

    Output out;
    int numLS;
    int commFreq;
    bool do_write;
    uint32_t maxAddr;
    uint32_t maxOutstanding;
    uint32_t maxReqsPerIssue;
    uint32_t nextAddr;
    uint64_t num_reads_issued, num_reads_returned;
    uint64_t addrOffset;

    std::map<uint64_t, SimTime_t> requests;

    Interfaces::SimpleMem * memory;

    SST::RNG::MarsagliaRNG rng;

    TimeConverter *clockTC;
    Clock::HandlerBase *clockHandler;

};

}
}
#endif /* _streamCPU_H */
