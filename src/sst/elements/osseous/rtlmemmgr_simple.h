// Copyright 2009-2020 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2020, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#ifndef _H_RTL_MEM_MANAGER_SIMPLE
#define _H_RTL_MEM_MANAGER_SIMPLE

#include <sst/core/output.h>

#include <stdint.h>
#include <deque>
#include <vector>
#include <unordered_map>

#include "rtlmemmgr_cache.h"

using namespace SST;

namespace SST {
namespace RtlComponent {

class RtlMemoryManagerSimple : public RtlMemoryManagerCache {

    public:
        /* SST ELI */
        SST_ELI_REGISTER_SUBCOMPONENT_DERIVED(RtlMemoryManagerSimple, "rtl", "MemoryManagerSimple", SST_ELI_ELEMENT_VERSION(1,0,0),
                "Simple allocate-on-first touch memory manager", SST::RtlComponent::RtlMemoryManager)

        SST_ELI_DOCUMENT_STATISTICS( RTL_ELI_MEMMGR_CACHE_STATS )

        /* RtlMemoryManagerSimple */
        RtlMemoryManagerSimple(ComponentId_t id, Params& params);
        ~RtlMemoryManagerSimple();

        uint64_t translateAddress(uint64_t virtAddr);
        void printStats();
        void AssignRtlMemoryManagerSimple(std::unordered_map<uint64_t, uint64_t>, std::deque<uint64_t>*, uint64_t);

    private:
        void allocate(const uint64_t size, const uint32_t level, const uint64_t virtualAddress);
	void printTable();

        uint64_t pageSize;
        std::deque<uint64_t>* freePages;

        std::unordered_map<uint64_t, uint64_t> pageTable;
};

}
}

#endif
