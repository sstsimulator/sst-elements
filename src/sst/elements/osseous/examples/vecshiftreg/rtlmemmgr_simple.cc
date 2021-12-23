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

#include <sst_config.h>
#include <inttypes.h>
#include <stdio.h>

#include "rtlmemmgr_simple.h"

using namespace SST::RtlComponent;

RtlMemoryManagerSimple::RtlMemoryManagerSimple(ComponentId_t id, Params& params) :
            RtlMemoryManagerCache(id, params) { }

RtlMemoryManagerSimple::~RtlMemoryManagerSimple() { /*free((void*)(&freePages));*/ }

void RtlMemoryManagerSimple::AssignRtlMemoryManagerSimple(std::unordered_map<uint64_t, uint64_t> pagetable, std::deque<uint64_t>* freepages, uint64_t pagesize) {
    pageTable = pagetable; 
    freePages = freepages;
    pageSize = pagesize;
    return; 
}

void RtlMemoryManagerSimple::allocate(const uint64_t size, const uint32_t level, const uint64_t virtualAddress) {
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
        if(freePages->empty()) {
                output->fatal(CALL_INFO, -1, "Requested a memory allocation of size: %" PRIu64 " which failed due to not having enough free pages\n",
                    size);
        }

        fprintf(stderr, "\nAllocation, Popping freepages");
        const uint64_t nextPhysPage = freePages->front();
        freePages->pop_front();

        pageTable.insert( std::pair<uint64_t, uint64_t>(nextVirtPage, nextPhysPage) );

        output->verbose(CALL_INFO, 4, 0, "Allocating memory page, physical page=%" PRIu64 ", virtual page=%" PRIu64 "\n",
                nextPhysPage, nextVirtPage);

        nextVirtPage += pageSize;
    }

    output->verbose(CALL_INFO, 4, 0, "Request leaves: %" PRIu32 " free pages\n",
        (uint32_t) freePages->size());

}

uint64_t RtlMemoryManagerSimple::translateAddress(uint64_t virtAddr) {
    // If translation is disabled, then just return address
    if( ! translationEnabled ) {
        return virtAddr;
    }

    if( output->getVerboseLevel() > 15 ) {
	printTable();
    }

    // Keep track of how many translations we are performing
    statTranslationQueries->addData(1);

    output->verbose(CALL_INFO, 4, 0, "Page Table: translate virtual address %" PRIu64 "\n", virtAddr);

    // Check the translation cache otherwise carry on
    auto checkCache = translationCache.find(virtAddr);
    if(checkCache != translationCache.end()) {
        statTranslationCacheHits->addData(1);
        output->verbose(CALL_INFO, 1, 0, "\nCacheTranslation successful");
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

        output->verbose(CALL_INFO, 1, 0, "\nPage table hit successful");
        cacheTranslation(virtAddr, physAddr);
        return physAddr;

    } else {
        output->verbose(CALL_INFO, 4, 0, "Page table miss for virtual address: %" PRIu64 "\n", virtAddr);

        // We did not find the address in memory, that means we should allocate it one from our default pool
        uint64_t offset = virtAddr % pageSize;

        output->verbose(CALL_INFO, 4, 0, "Page offset calculation (generating a new page allocation request) for address %" PRIu64 ", offset=%" PRIu64 ", requesting virtual map to address: %" PRIu64 "\n",
                virtAddr, offset, (virtAddr - offset));

        output->verbose(CALL_INFO, 1, 0, "\nPage table miss. Allocation");
        // Perform an allocation so we can then re-find the address
        allocate(8, 0, virtAddr - offset);

        // Now attempt to refind it
        const uint64_t newPhysAddr = translateAddress(virtAddr);

        output->verbose(CALL_INFO, 4, 0, "Page allocation routine mapped to address: %" PRIu64 "\n", newPhysAddr );

        return newPhysAddr;
    }
}

void RtlMemoryManagerSimple::printStats() {
    output->output("\n");
    output->output("Rtl Memory Management Statistics:\n");
    output->output("---------------------------------------------------------------------\n");
    output->output("Page Table Sizes:\n");

    output->output("- Map entries         %" PRIu32 "\n",
        (uint32_t) pageTable.size());

    output->output("Page Table Coverages:\n");

    output->output("- Bytes               %" PRIu64 "\n",
        ((uint64_t) pageTable.size()) * ((uint64_t) pageSize));
}

void RtlMemoryManagerSimple::printTable() {

    	output->output("---------------------------------------------------------------------\n");
	output->verbose(CALL_INFO, 16, 0, "Page Table Map:\n");

	for( auto table_itr : pageTable ) {
		output->verbose(CALL_INFO, 16, 0, "-> VA: %15" PRIu64 " -> PA: %15" PRIu64 "\n",
			table_itr.first, table_itr.second);
	}

    	output->output("---------------------------------------------------------------------\n");

}
