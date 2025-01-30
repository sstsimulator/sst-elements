// Copyright 2009-2024 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2024, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// of the distribution for more information.
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
        {"rngseed",                 "(int) Set a seed for the random generation of requests", "7"},
        {"maxOutstanding",          "(uint) Maximum number of outstanding memory requests at a time.", "10"},
        {"opCount",                 "(uint) Number of operations to issue."},
        {"reqsPerIssue",            "(uint) Maximum number of requests to issue at a time", "1"},
        {"write_freq",              "(uint) Relative write frequency", "25"},
        {"read_freq",               "(uint) Relative read frequency", "75"},
        {"flush_freq",              "(uint) Relative flush frequency", "0"},
        {"flushinv_freq",           "(uint) Relative flush-inv frequency", "0"},
        {"flushcache_freq",         "(uint) Relative frequency to flush the entire cache", "0"},
        {"custom_freq",             "(uint) Relative custom op frequency", "0"},
        {"llsc_freq",               "(uint) Relative LLSC frequency", "0"},
        {"mmio_addr",               "(uint) Base address of the test MMIO component. 0 means not present.", "0"},
        {"noncacheableRangeStart",  "(uint) Beginning of range of addresses that are noncacheable.", "0x0"},
        {"noncacheableRangeEnd",    "(uint) End of range of addresses that are noncacheable.", "0x0"},
        {"addressoffset",           "(uint) Apply an offset to a calculated address to check for non-alignment issues", "0"},
        {"test_init",               "(uint) Number of write messages to initialize memory with", "0"} )

    SST_ELI_DOCUMENT_STATISTICS( 
        {"pendCycle", "Number of pending requests per cycle", "count", 1},
        {"reads", "Number of reads issued (including noncacheable)", "count", 1},
        {"writes", "Number of writes issued (including noncacheable)", "count", 1},
        {"flushes", "Number of line flushes issued", "count", 1},
        {"flushinvs", "Number of line flush-invs issued", "count", 1},
        {"flushcaches", "Number of cache flushes issued", "count", 1},
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

    Output out_;
    uint64_t op_count_;
    uint64_t mem_freq_;
    uint64_t max_addr_;
    uint64_t mmio_addr_;
    uint64_t line_size_;
    uint64_t max_outstanding_;
    unsigned high_mark_;
    unsigned write_mark_;
    unsigned flush_mark_;
    unsigned flushinv_mark_;
    unsigned flushcache_mark_;
    unsigned custom_mark_;
    unsigned llsc_mark_;
    unsigned mmio_mark_;
    uint32_t max_reqs_per_issue_;
    uint64_t noncacheable_range_start_, noncacheable_range_end_, noncacheable_size_;
    uint64_t clock_ticks_;
    uint64_t init_write_count_;
    Statistic<uint64_t>* stat_requests_pending_per_cycle_;
    Statistic<uint64_t>* stat_num_reads_issued_;
    Statistic<uint64_t>* stat_num_writes_issued_;
    Statistic<uint64_t>* stat_num_flushes_issued_;
    Statistic<uint64_t>* stat_num_flushcache_issued_;
    Statistic<uint64_t>* stat_num_flushinvs_issued_;
    Statistic<uint64_t>* stat_num_custom_issued_;
    Statistic<uint64_t>* stat_num_llsc_issued_;
    Statistic<uint64_t>* stat_num_llsc_success_;
    Statistic<uint64_t>* stat_noncacheable_reads_;
    Statistic<uint64_t>* stat_noncacheable_writes_;

    bool ll_issued_;
    Interfaces::StandardMem::Addr ll_addr_;

    std::map<Interfaces::StandardMem::Request::id_t, std::pair<SimTime_t, std::string>> requests_;

    Interfaces::StandardMem *memory_;

    SST::RNG::MarsagliaRNG rng_;
    SST::RNG::MarsagliaRNG rng_comm_;

    TimeConverter *clock_timeconverter_;
    Clock::HandlerBase *clock_handler_;

    /* Functions for creating the requests tested by this CPU */
    Interfaces::StandardMem::Request* createWrite(uint64_t addr);
    Interfaces::StandardMem::Request* createRead(Addr addr);
    Interfaces::StandardMem::Request* createFlush(Addr addr);
    Interfaces::StandardMem::Request* createFlushInv(Addr addr);
    Interfaces::StandardMem::Request* createFlushCache();
    Interfaces::StandardMem::Request* createLL(Addr addr);
    Interfaces::StandardMem::Request* createSC();
    Interfaces::StandardMem::Request* createMMIOWrite();
    Interfaces::StandardMem::Request* createMMIORead();
};

}
}
#endif /* _STANDARDCPU_H */
