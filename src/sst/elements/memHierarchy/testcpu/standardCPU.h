// Copyright 2009-2021 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2021, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef MEMHIERARCHY_STANDARD_CPU_H
#define MEMHIERARCHY_STANDARD_CPU_H

#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS
#endif
#include <inttypes.h>

#include <sst/core/interfaces/stdMem.h>
#include <sst/core/event.h>
#include <sst/core/sst_types.h>
#include <sst/core/component.h>
#include <sst/core/link.h>
#include <sst/core/timeConverter.h>
#include <sst/core/output.h>
#include <sst/core/rng/marsaglia.h>

#include "util.h"

using namespace SST::Statistics;

namespace SST {
namespace MemHierarchy {
using Req = SST::Interfaces::StandardMem::Request;

class standardCPU : public SST::Component {
public:
/* Element Library Info */
    SST_ELI_REGISTER_COMPONENT(standardCPU, "memHierarchy", "standardCPU", SST_ELI_ELEMENT_VERSION(1,0,0),
            "Simple demo CPU for testing", COMPONENT_CATEGORY_PROCESSOR)

    SST_ELI_DOCUMENT_PARAMS(
        {"memFreq",                 "(int) Average cycles between memory operations."},
        {"memSize",                 "(UnitAlgebra/string) Size of physical memory with units."},
        {"verbose",                 "(uint) Determine how verbose the output from the CPU is", "1"},
        {"clock",                   "(UnitAlgebra/string) Clock frequency", "1GHz"},
        {"rngseed",                 "(int) Set a seed for the random generation of addresses", "7"},
        {"maxOutstanding",          "(uint) Maximum number of outstanding memory requests at a time.", "10"},
        {"opCount",                 "(uint) Number of operations to issue."},
        {"reqsPerIssue",            "(uint) Maximum number of requests to issue at a time", "1"},
        {"write_freq",              "(uint) Relative write frequency", "25"},
        {"read_freq",               "(uint) Relative read frequency", "75"},
        {"flush_freq",              "(uint) Relative flush frequency", "0"},
        {"flushinv_freq",           "(uint) Relative flush-inv frequency", "0"},
        {"custom_freq",             "(uint) Relative custom op frequency", "0"},
        {"llsc_freq",               "(uint) Relative LLSC frequency", "0"},
        {"mmio_addr",               "(uint) Base address of the test MMIO component. 0 means not present.", "0"},
        {"noncacheableRangeStart",  "(uint) Beginning of range of addresses that are noncacheable.", "0x0"},
        {"noncacheableRangeEnd",    "(uint) End of range of addresses that are noncacheable.", "0x0"},
        {"addressoffset",           "(uint) Apply an offset to a calculated address to check for non-alignment issues", "0"} )

    SST_ELI_DOCUMENT_STATISTICS( 
        {"pendCycle", "Number of pending requests per cycle", "count", 1},
        {"reads", "Number of reads issued (including noncacheable)", "count", 1},
        {"writes", "Number of writes issued (including noncacheable)", "count", 1},
        {"flushes", "Number of flushes issued", "count", 1},
        {"flushinvs", "Number of flush-invs issued", "count", 1},
        {"customReqs", "Number of custom requests issued", "count", 1},
        {"llsc", "Number of LL-SC pairs issued", "count", 1},
        {"llsc_success", "Number of successful LLSC pairs issued", "count", 1},
        {"readNoncache", "Number of noncacheable reads issued", "count", 1},
        {"writeNoncache", "Number of noncacheable writes issued", "count", 1}
    )

    /* Slot for a memory interface. This must be user defined (aka defined in Python config) */
    SST_ELI_DOCUMENT_SUBCOMPONENT_SLOTS( { "memory", "Interface to memory hierarchy", "SST::Interfaces::StandardMem" } )

/* Begin class definition */
    standardCPU(SST::ComponentId_t id, SST::Params& params);
    void init(unsigned int phase) override;
    void setup() override;
    void finish() override;
    void emergencyShutdown() override;

private:
    void handleEvent( Interfaces::StandardMem::Request *ev );
    virtual bool clockTic( SST::Cycle_t );

    Output out;
    uint64_t ops;
    uint64_t memFreq;
    uint64_t maxAddr;
    uint64_t mmioAddr;
    uint64_t lineSize;
    uint64_t maxOutstanding;
    unsigned high_mark;
    unsigned write_mark;
    unsigned flush_mark;
    unsigned flushinv_mark;
    unsigned custom_mark;
    unsigned llsc_mark;
    unsigned mmio_mark;
    uint32_t maxReqsPerIssue;
    uint64_t noncacheableRangeStart, noncacheableRangeEnd, noncacheableSize;
    uint64_t clock_ticks;
    Statistic<uint64_t>* requestsPendingCycle;
    Statistic<uint64_t>* num_reads_issued;
    Statistic<uint64_t>* num_writes_issued;
    Statistic<uint64_t>* num_flushes_issued;
    Statistic<uint64_t>* num_flushinvs_issued;
    Statistic<uint64_t>* num_custom_issued;
    Statistic<uint64_t>* num_llsc_issued;
    Statistic<uint64_t>* num_llsc_success;
    Statistic<uint64_t>* noncacheableReads;
    Statistic<uint64_t>* noncacheableWrites;

    bool ll_issued;
    Interfaces::StandardMem::Addr ll_addr;

    std::map<Interfaces::StandardMem::Request::id_t, std::pair<SimTime_t, std::string>> requests;

    Interfaces::StandardMem *memory;

    SST::RNG::MarsagliaRNG rng;

    TimeConverter *clockTC;
    Clock::HandlerBase *clockHandler;

    /* Functions for creating the requests tested by this CPU */
    Interfaces::StandardMem::Request* createWrite(uint64_t addr);
    Interfaces::StandardMem::Request* createRead(Addr addr);
    Interfaces::StandardMem::Request* createFlush(Addr addr);
    Interfaces::StandardMem::Request* createFlushInv(Addr addr);
    Interfaces::StandardMem::Request* createLL(Addr addr);
    Interfaces::StandardMem::Request* createSC();
    Interfaces::StandardMem::Request* createMMIOWrite();
    Interfaces::StandardMem::Request* createMMIORead();
};

}
}
#endif /* _STANDARDCPU_H */
