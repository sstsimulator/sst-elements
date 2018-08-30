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

#include <sst_config.h>
#include <stdio.h>

#include "arielmemmgr_simple.h"

using namespace SST::ArielComponent;

ArielMemoryManagerSimple::ArielMemoryManagerSimple(SST::Component* owner, Params& params) : 
            ArielMemoryManager(owner, params) {
    
    pageSize = (uint64_t) params.find<uint64_t>("pagesize0", 4096);
    output->verbose(CALL_INFO, 2, 0, "Page size is %" PRIu64 "\n", pageSize);

    uint64_t pageCount = (uint64_t) params.find<uint64_t>("pagecount0", 131072);
    output->verbose(CALL_INFO, 2, 0, "Page count is %" PRIu64 "\n", pageCount);

    if (mapPolicy == ArielPageMappingPolicy::LINEAR) {
        mapPagesLinear(pageCount, pageSize, 0, &freePages);
    } else {
        mapPagesRandom(pageCount, pageSize, 0, &freePages);
    }

    output->verbose(CALL_INFO, 2, 0, "Usable (free) page queue contains %" PRIu32 " entries\n", (uint32_t) freePages.size());

    std::string popFilePath = params.find<std::string>("page_populate_0", "");
    if (popFilePath != "") {
        output->verbose(CALL_INFO, 1, 0, "Populating page table from %s...\n", popFilePath.c_str());
        populatePageTable(popFilePath, &pageTable, &freePages, pageSize);
    }
    
}

ArielMemoryManagerSimple::~ArielMemoryManagerSimple() {
}


void ArielMemoryManagerSimple::allocate(const uint64_t size, const uint32_t level, const uint64_t virtualAddress) {
        // Simple manager ignores 'level' parameter	

    output->verbose(CALL_INFO, 4, 0, "Requesting a memory allocation of %" PRIu64 " bytes, Virtual mapping=%" PRIu64 "\n",
        size, virtualAddress);
    statPageAllocationCount->addData(1);

    uint64_t roundedSize = size;
    uint64_t remainder = size % pageSize;

    // We will do all of our allocated based on whole pages, inefficient maybe but much
    // simpler to implement and debug
    if (remainder > 0) {
        roundedSize += (pageSize - remainder);
    }

    output->verbose(CALL_INFO, 4, 0, "Requesting rounded to %" PRIu64 " bytes\n", roundedSize);

    uint64_t nextVirtPage = virtualAddress;
    for(uint64_t bytesLeft = 0; bytesLeft < roundedSize; bytesLeft += pageSize) {
        if(freePages.empty()) {
                output->fatal(CALL_INFO, -1, "Requested a memory allocation of size: %" PRIu64 " which failed due to not having enough free pages\n",
                    size);
        }

        const uint64_t nextPhysPage = freePages.front();
        freePages.pop_front();

        pageTable.insert( std::pair<uint64_t, uint64_t>(nextVirtPage, nextPhysPage) );

        output->verbose(CALL_INFO, 4, 0, "Allocating memory page, physical page=%" PRIu64 ", virtual page=%" PRIu64 "\n",
                nextPhysPage, nextVirtPage);

        nextVirtPage += pageSize;
    }

    output->verbose(CALL_INFO, 4, 0, "Request leaves: %" PRIu32 " free pages\n",
        (uint32_t) freePages.size());

}

uint64_t ArielMemoryManagerSimple::translateAddress(uint64_t virtAddr) {
    // If translation is disabled, then just return address
    if( ! translationEnabled ) {
        return virtAddr;
    }

    // Keep track of how many translations we are performing
    statTranslationQueries->addData(1);

    output->verbose(CALL_INFO, 4, 0, "Page Table: translate virtual address %" PRIu64 "\n", virtAddr);

    // Check the translation cache otherwise carry on
    auto checkCache = translationCache->find(virtAddr);
    if(checkCache != translationCache->end()) {
        statTranslationCacheHits->addData(1);
        return checkCache->second;
    }

    std::unordered_map<uint64_t, uint64_t>::iterator page_itr;
    const uint64_t page_offset = virtAddr % pageSize;
    const uint64_t page_start = virtAddr - page_offset;

    page_itr = pageTable.find(page_start);

    if(page_itr != pageTable.end()) {
        // Located
        uint64_t physAddr = page_itr->second + page_offset;

        output->verbose(CALL_INFO, 4, 0, "Page table hit: virtual address=%" PRIu64 " hit, virtual page start=%" PRIu64 ", virtual end=%" PRIu64 ", translates to phys page start=%" PRIu64 " translates to: phys address: %" PRIu64 " (offset added to phys start=%" PRIu64 ")\n",
                virtAddr, page_itr->first, page_itr->first + pageSize, page_itr->second, physAddr, page_offset);

        cacheTranslation(virtAddr, physAddr);
        return physAddr;

    } else {
        output->verbose(CALL_INFO, 4, 0, "Page table miss for virtual address: %" PRIu64 "\n", virtAddr);

        // We did not find the address in memory, that means we should allocate it one from our default pool
        uint64_t offset = virtAddr % pageSize;

        output->verbose(CALL_INFO, 4, 0, "Page offset calculation (generating a new page allocation request) for address %" PRIu64 ", offset=%" PRIu64 ", requesting virtual map to address: %" PRIu64 "\n",
                virtAddr, offset, (virtAddr - offset));

        // Perform an allocation so we can then re-find the address
        allocate(8, 0, virtAddr - offset);

        // Now attempt to refind it
        const uint64_t newPhysAddr = translateAddress(virtAddr);

        output->verbose(CALL_INFO, 4, 0, "Page allocation routine mapped to address: %" PRIu64 "\n", newPhysAddr );

        return newPhysAddr;
    }
}

void ArielMemoryManagerSimple::printStats() {
    output->output("\n");
    output->output("Ariel Memory Management Statistics:\n");
    output->output("---------------------------------------------------------------------\n");
    output->output("Page Table Sizes:\n");

    output->output("- Map entries         %" PRIu32 "\n",
        (uint32_t) pageTable.size());

    output->output("Page Table Coverages:\n");

    output->output("- Bytes               %" PRIu64 "\n",
        ((uint64_t) pageTable.size()) * ((uint64_t) pageSize));
}
