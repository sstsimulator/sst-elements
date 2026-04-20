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

#ifndef CARCOSA_FAULTINJCPU_H
#define CARCOSA_FAULTINJCPU_H

#include "sst/elements/carcosa/Components/CarcosaCPUBase.h"

namespace SST {
namespace MemHierarchy {

class FaultInjCPU : public CarcosaCPUBase {
public:
    SST_ELI_REGISTER_COMPONENT(FaultInjCPU, "Carcosa", "FaultInjCPU", SST_ELI_ELEMENT_VERSION(1,0,0),
            "Simple demo CPU for testing with dynamic fault injection", COMPONENT_CATEGORY_PROCESSOR)

    SST_ELI_DOCUMENT_PORTS(
        {"haliToCPU", "Link from Hali to CPU", { "Carcosa.CpuEvent", "Carcosa.FaultInjEvent" } }
    )

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
        {"addressoffset",           "(uint) Apply an offset to a calculated address to check for non-alignment issues", "0"}
    )

    SST_ELI_DOCUMENT_STATISTICS(
        {"pendCycle",    "Number of pending requests per cycle", "count", 1},
        {"reads",        "Number of reads issued (including noncacheable)", "count", 1},
        {"writes",       "Number of writes issued (including noncacheable)", "count", 1},
        {"flushes",      "Number of flushes issued", "count", 1},
        {"flushinvs",    "Number of flush-invs issued", "count", 1},
        {"customReqs",   "Number of custom requests issued", "count", 1},
        {"llsc",         "Number of LL-SC pairs issued", "count", 1},
        {"llsc_success", "Number of successful LLSC pairs issued", "count", 1},
        {"readNoncache", "Number of noncacheable reads issued", "count", 1},
        {"writeNoncache","Number of noncacheable writes issued", "count", 1}
    )

    SST_ELI_DOCUMENT_SUBCOMPONENT_SLOTS(
        { "memory", "Interface to memory hierarchy", "SST::Interfaces::StandardMem" }
    )

    FaultInjCPU(SST::ComponentId_t id, SST::Params& params);

protected:
    bool clockTic(SST::Cycle_t) override;

private:
    double currentFaultRate;
};

} // namespace MemHierarchy
} // namespace SST

#endif // CARCOSA_FAULTINJCPU_H
