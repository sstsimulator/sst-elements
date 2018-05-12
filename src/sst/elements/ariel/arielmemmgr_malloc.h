// Copyright 2009-2017 Sandia Corporation. Under the terms
// of Contract DE-NA0003525 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2017, Sandia Corporation
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#ifndef _H_ARIEL_MEM_MANAGER_MALLOC
#define _H_ARIEL_MEM_MANAGER_MALLOC

#include <sst/core/component.h>
#include <sst/core/output.h>
#include <sst/core/elementinfo.h>

#include "arielmemmgr.h"

#include <stdint.h>
#include <deque>
#include <vector>
#include <unordered_map>

using namespace SST;

namespace SST {
namespace ArielComponent {

class ArielMemoryManagerMalloc : public ArielMemoryManager {

    public:
        /* SST ELI */
        SST_ELI_REGISTER_SUBCOMPONENT(ArielMemoryManagerMalloc, "ariel", "MemoryManagerMalloc", SST_ELI_ELEMENT_VERSION(1,0,0),
                "MLM memory manager which supports malloc/free in different memory pools", "SST::ArielComponent::ArielMemoryManager")

#define MEMMGR_MALLOC_ELI_PARAMS ARIEL_ELI_MEMMGR_PARAMS,\
            {"memorylevels",    "Number of memory levels in the system", "1"},\
            {"defaultlevel",    "Default memory level", "0"},\
            {"pagesize%(memorylevels)d", "Page size for memory Level x", "4096"},\
            {"pagecount%(memorylevels)d", "Page count for memory Level x", "131072"},\
            {"page_populate_%(memorylevels)d", "Pre-populate/partially pre-populate a page table for a level in memory, this is the file to read in.", ""}

        SST_ELI_DOCUMENT_PARAMS( MEMMGR_MALLOC_ELI_PARAMS )
        SST_ELI_DOCUMENT_STATISTICS( ARIEL_ELI_MEMMGR_STATS )


        /* ArielMemoryManagerMalloc */
        ArielMemoryManagerMalloc(SST::Component* owner, Params& params);
        ~ArielMemoryManagerMalloc();

        void setDefaultPool(uint32_t pool);
        uint32_t getDefaultPool();

        uint64_t translateAddress(uint64_t virtAddr);
        void printStats();

        void freeMalloc(const uint64_t vAddr);
        bool allocateMalloc(const uint64_t size, const uint32_t level, const uint64_t virtualAddress);
        
    private:
        void allocate(const uint64_t size, const uint32_t level, const uint64_t virtualAddress);
        bool canAllocateInLevel(const uint64_t size, const uint32_t level);

        struct mallocInfo {
            uint64_t size;
            uint32_t level;
            std::unordered_set<uint64_t>* VAKeys;
            mallocInfo(uint64_t size, uint32_t level, std::unordered_set<uint64_t>* VAKeys) : size(size), level(level), VAKeys(VAKeys) {};
        };

        std::map<uint64_t, uint64_t> mallocPrimaryVAMap;    // Map VA of each PA to the primary VA of the malloc -> used to find the mallocInfo
        std::map<uint64_t, uint64_t> mallocTranslations;    // Map VA to PA for mallocs -> primary lookup
        std::map<uint64_t, mallocInfo> mallocInformation;   // Map mallocID to information about the malloc -> use for frees/allocs

        uint32_t defaultLevel;
        uint32_t memoryLevels;
        uint64_t* pageSizes;

        std::deque<uint64_t>** freePages;
        std::unordered_map<uint64_t, uint64_t>** pageAllocations;
        std::unordered_map<uint64_t, uint64_t>** pageTables;
};

}
}

#endif
