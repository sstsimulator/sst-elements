// Copyright 2009-2025 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2025, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// of the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#ifndef _H_ARIEL_MEM_MANAGER_SIMPLE
#define _H_ARIEL_MEM_MANAGER_SIMPLE

#include <sst/core/output.h>

#include <stdint.h>
#include <deque>
#include <vector>
#include <unordered_map>

#include "arielmemmgr_cache.h"

using namespace SST;

namespace SST {
namespace ArielComponent {

class ArielMemoryManagerSimple : public ArielMemoryManagerCache {

    public:
        /* SST ELI */
        SST_ELI_REGISTER_SUBCOMPONENT(
            ArielMemoryManagerSimple,
            "ariel",
            "MemoryManagerSimple",
            SST_ELI_ELEMENT_VERSION(1,0,0),
            "Simple allocate-on-first touch memory manager",
            SST::ArielComponent::ArielMemoryManager
        )

#define MEMMGR_SIMPLE_ELI_PARAMS ARIEL_ELI_MEMMGR_CACHE_PARAMS,\
            {"pagesize0", "Page size", "4096"},\
            {"pagecount0", "Page count", "131072"},\
            {"page_populate_0", "Pre-populate/partially pre-populate the page table, this is the file to read in.", ""}

        SST_ELI_DOCUMENT_PARAMS( MEMMGR_SIMPLE_ELI_PARAMS )
        SST_ELI_DOCUMENT_STATISTICS( ARIEL_ELI_MEMMGR_CACHE_STATS )

        /* ArielMemoryManagerSimple */
        ArielMemoryManagerSimple(ComponentId_t id, Params& params);
        ~ArielMemoryManagerSimple();

        uint64_t translateAddress(uint64_t virtAddr);
        void printStats();
        void get_page_info(std::unordered_map<uint64_t, uint64_t>*, std::deque<uint64_t>*, uint64_t&);

    private:
        void allocate(const uint64_t size, const uint32_t level, const uint64_t virtualAddress);
	void printTable();

        uint64_t pageSize;
        std::deque<uint64_t> freePages;

        std::unordered_map<uint64_t, uint64_t> pageTable;
};

}
}

#endif
