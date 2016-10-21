// Copyright 2009-2016 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2016, Sandia Corporation
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

    }

ArielMemoryManagerSimple::~ArielMemoryManagerSimple() {

}

void ArielMemoryManagerSimple::cacheTranslation(uint64_t virtualA, uint64_t physicalA) {
	// Remove the oldest entry if we do not have enough slots
	if(translationCache->size() == translationCacheEntries) {
		statTranslationCacheEvict->addData(1);
		translationCache->erase(translationCache->begin());
	}

	// Insert the translated entry into the cache
	translationCache->insert(std::pair<uint64_t, uint64_t>(virtualA, physicalA));
}

void ArielMemoryManagerSimple::allocate(const uint64_t size, const uint32_t level, const uint64_t virtualAddress) {
	if(level >= memoryLevels) {
		output->fatal(CALL_INFO, -1, "Requested memory allocation of %" PRIu64 " bytes, in level: %" PRIu32 ", but only have: %" PRIu32 " levels.\n",
			size, level, memoryLevels);
	}

	output->verbose(CALL_INFO, 4, 0, "Requesting a memory allocation of %" PRIu64 " bytes, in level: %" PRIu32 ", Virtual mapping=%" PRIu64 "\n",
		size, level, virtualAddress);
	statPageAllocationCount->addData(1);

	const uint64_t pageSize = pageSizes[level];

	uint64_t roundedSize = size;
	uint64_t remainder = size % pageSize;
	
	// We will do all of our allocated based on whole pages, inefficient maybe but much
	// simpler to implement and debug
	if(remainder > 0) {
		roundedSize += (pageSize - remainder);
	}

	output->verbose(CALL_INFO, 4, 0, "Requesting rounded to %" PRIu64 " bytes\n",
		roundedSize);

	uint64_t nextVirtPage = virtualAddress;
	for(uint64_t bytesLeft = 0; bytesLeft < roundedSize; bytesLeft += pageSize) {
		if(freePages[level]->empty()) {
			output->fatal(CALL_INFO, -1, "Requested a memory allocation at level: %" PRIu32 " of size: %" PRIu64 " which failed due to not having enough free pages\n",
				level, size);
		}

		const uint64_t nextPhysPage = freePages[level]->front();
		freePages[level]->pop_front();

		pageTables[level]->insert( std::pair<uint64_t, uint64_t>(nextVirtPage, nextPhysPage) );

		output->verbose(CALL_INFO, 4, 0, "Allocating memory page, physical page=%" PRIu64 ", virtual page=%" PRIu64 "\n",
			nextPhysPage, nextVirtPage);

		nextVirtPage += pageSize;
	}
	
	output->verbose(CALL_INFO, 4, 0, "Request leaves: %" PRIu32 " free pages at level: %" PRIu32 "\n",
		(uint32_t) freePages[level]->size(), level);

	// Record the complete entry in the allocation table (what we allocated in size against the virtual address)
	// this means we know how much to free and can translate the address successfully.
	pageAllocations[level]->insert( std::pair<uint64_t, uint64_t>(virtualAddress, roundedSize) );
}

uint64_t ArielMemoryManagerSimple::translateAddress(uint64_t virtAddr) {
	// If translation is disabled, then just return address
	if( ! translationEnabled ) {
		return virtAddr;
	}

	// Keep track of how many translations we are performing
	statTranslationQueries->addData(1);

	uint64_t physAddr = (uint64_t) -1;
	bool found = false;

	output->verbose(CALL_INFO, 4, 0, "Page Table: translate virtual address %" PRIu64 "\n", virtAddr);

	// Check the translation cache otherwise carry on
	auto checkCache = translationCache->find(virtAddr);
	if(checkCache != translationCache->end()) {
		statTranslationCacheHits->addData(1);
		return checkCache->second;
	}

	// We will have to search every memory level to find where the address lies
	for(uint32_t i = 0; i < memoryLevels; ++i) {
		if(! found) {
			std::unordered_map<uint64_t, uint64_t>::iterator page_itr;
			const uint64_t pageSize = pageSizes[i];
			const uint64_t page_offset = virtAddr % pageSize;
			const uint64_t page_start = virtAddr - page_offset;

			page_itr = pageTables[i]->find(page_start);

			if(page_itr != pageTables[i]->end()) {
				// Located
				physAddr = page_itr->second + page_offset;

				output->verbose(CALL_INFO, 4, 0, "Page table hit: virtual address=%" PRIu64 " hit in level: %" PRIu32 ", virtual page start=%" PRIu64 ", virtual end=%" PRIu64 ", translates to phys page start=%" PRIu64 " translates to: phys address: %" PRIu64 " (offset added to phys start=%" PRIu64 ")\n",
					virtAddr, i, page_itr->first, page_itr->first + pageSize, page_itr->second, physAddr, page_offset);

				found = true;
				break;
			}
		} else {
			break;
		}
	}

	if(found) {
		cacheTranslation(virtAddr, physAddr);
		return physAddr;
	} else {
		output->verbose(CALL_INFO, 4, 0, "Page table miss for virtual address: %" PRIu64 "\n", virtAddr);

		// We did not find the address in memory, that means we should allocate it one from our default pool
		uint64_t offset = virtAddr % pageSizes[defaultLevel];

		output->verbose(CALL_INFO, 4, 0, "Page offset calculation (generating a new page allocation request) for address %" PRIu64 ", offset=%" PRIu64 ", requesting virtual map to address: %" PRIu64 "\n", 
			virtAddr, offset, (virtAddr - offset));

		// Perform an allocation so we can then re-find the address
		allocate(8, defaultLevel, virtAddr - offset);

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

	for(uint32_t i = 0; i < memoryLevels; ++i) {
	output->output("- Map entries at level %" PRIu32 "         %" PRIu32 "\n",
		i, (uint32_t) pageTables[i]->size());
	}

	output->output("Page Table Coverages:\n");

	for(uint32_t i = 0; i < memoryLevels; ++i) {
	output->output("- Bytes at level %" PRIu32 "              %" PRIu64 "\n",
		i, ((uint64_t) pageTables[i]->size()) * ((uint64_t) pageSizes[i]));
	}
}
