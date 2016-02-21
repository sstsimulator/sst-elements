// Copyright 2009-2015 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2015, Sandia Corporation
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#include <sst_config.h>
#include <stdio.h>

#include "arielmemmgr.h"

using namespace SST::ArielComponent;

ArielMemoryManager::ArielMemoryManager(SST::Component* ownMe,
		uint32_t mLevels, uint64_t* pSizes, uint64_t* stdPCounts, Output* out,
		uint32_t defLevel, uint32_t translateCacheEntryCount) :
	owner(ownMe),
	translationCacheEntries(translateCacheEntryCount),
	translationEnabled(true),
	reportUnmatchedFree(false) {

	output = out;
	memoryLevels = mLevels;
	defaultLevel = defLevel;

	output->verbose(CALL_INFO, 2, 0, "Creating a memory hierarchy of %" PRIu32 " levels.\n", mLevels);

	pageSizes = (uint64_t*) malloc(sizeof(uint64_t) * memoryLevels);
	for(uint32_t i = 0; i < mLevels; ++i) {
		output->verbose(CALL_INFO, 2, 0, "Level %" PRIu32 " page size is %" PRIu64 "\n", i, pSizes[i]);
		pageSizes[i] = pSizes[i];
	}

	freePages = (std::deque<uint64_t>**) malloc( sizeof(std::deque<uint64_t>*) * mLevels );
	uint64_t nextMemoryAddress = 0;
	for(uint32_t i = 0; i < mLevels; ++i) {
		freePages[i] = new std::deque<uint64_t>();

		output->verbose(CALL_INFO, 2, 0, "Level %" PRIu32 " page count is %" PRIu64 "\n", i, stdPCounts[i]);
		for(uint64_t j = 0; j < stdPCounts[i]; ++j) {
			freePages[i]->push_back(nextMemoryAddress);
			nextMemoryAddress += pageSizes[i];
		}
		output->verbose(CALL_INFO, 2, 0, "Level %" PRIu32 " usable (free) page queue contains %" PRIu32 " entries\n", i, 
			(uint32_t) freePages[i]->size());
	}

	pageAllocations = (std::unordered_map<uint64_t, uint64_t>**) malloc(sizeof(std::unordered_map<uint64_t, uint64_t>*) * memoryLevels);
	for(uint32_t i = 0; i < mLevels; ++i) {
		pageAllocations[i] = new std::unordered_map<uint64_t, uint64_t>();
	}

	pageTables = (std::unordered_map<uint64_t, uint64_t>**) malloc(sizeof(std::unordered_map<uint64_t, uint64_t>*) * memoryLevels);
	for(uint32_t i = 0; i < mLevels; ++i) {
		pageTables[i] = new std::unordered_map<uint64_t, uint64_t>();
	}

	translationCache = new std::unordered_map<uint64_t, uint64_t>();

	statTranslationCacheHits    = owner->registerStatistic<uint64_t>("tlb_hits");
	statTranslationCacheEvict   = owner->registerStatistic<uint64_t>("tlb_evicts");
	statTranslationQueries      = owner->registerStatistic<uint64_t>("tlb_translate_queries");
	statTranslationShootdown    = owner->registerStatistic<uint64_t>("tlb_shootdown");
	statPageAllocationCount     = owner->registerStatistic<uint64_t>("tlb_page_allocs");
}

void ArielMemoryManager::populateTables(const char* populateFilePath, const uint32_t level) {
	if(strcmp(populateFilePath, "") == 0) {
		output->fatal(CALL_INFO, -1, "Pre-populate of page tables called without a valid file path\n");
	}

	if(level >= memoryLevels) {
		output->fatal(CALL_INFO, -1, "Pre-populate of page tables called without a valid memory level (level=%" PRIu32 ", maxLevels=%" PRIu32 ")\n",
			level, memoryLevels);
	}

	// We need to load each entry and the create it in the page table for
	// this level
	FILE* popFile = fopen(populateFilePath, "rt");
	uint64_t pinAddr = 0;

	const uint64_t levelPageSize = pageSizes[level];

	while( ! feof(popFile) ) {
		if(EOF == fscanf(popFile, "%" PRIu64 "\n", &pinAddr)) {
			break;
		}

		if(freePages[level]->size() == 0) {
			output->fatal(CALL_INFO, -1, "Attempted to pin address %" PRIu64 " in level %" PRIu32 " but no free pages.\n",
				pinAddr, level);
		}

		if(pinAddr % levelPageSize > 0) {
			output->fatal(CALL_INFO, -1, "Attempted to pin address %" PRIu64 " in level %" PRIu32 " but address is not page aligned to page size %" PRIu64 "\n",
				pinAddr, level, levelPageSize);
		}

		const uint64_t freePhysical = freePages[level]->front();
		freePages[level]->pop_front();

		output->verbose(CALL_INFO, 4, 0, "Pinning address %" PRIu64 " in level %" PRIu32 " (physical=%" PRIu64 "\n",
			pinAddr, level, freePhysical);

		pageTables[level]->insert( std::pair<uint64_t, uint64_t>( pinAddr, freePhysical ) );
	}

	fclose(popFile);
}

// Set the new default level for continuing allocations.
void ArielMemoryManager::setDefaultPool(const uint32_t newLevel) {
	defaultLevel = newLevel;
}

ArielMemoryManager::~ArielMemoryManager() {

}

void ArielMemoryManager::disableTranslation() {
	translationEnabled = false;
}

void ArielMemoryManager::enableTranslation() {
	translationEnabled = true;
}

void ArielMemoryManager::cacheTranslation(uint64_t virtualA, uint64_t physicalA) {
	// Remove the oldest entry if we do not have enough slots
	if(translationCache->size() == translationCacheEntries) {
		statTranslationCacheEvict->addData(1);
		translationCache->erase(translationCache->begin());
	}

	// Insert the translated entry into the cache
	translationCache->insert(std::pair<uint64_t, uint64_t>(virtualA, physicalA));
}

void ArielMemoryManager::allocate(const uint64_t size, const uint32_t level, const uint64_t virtualAddress) {
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
	if(roundedSize > 0) {
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

uint32_t ArielMemoryManager::countMemoryLevels() {
	return memoryLevels;
}

void ArielMemoryManager::free(uint64_t virtAddress) {
	output->verbose(CALL_INFO, 4, 0, "Memory manager attempting to free virtual address: %" PRIu64 "\n", virtAddress);
	bool found = false;

	for(uint32_t i = 0; i < memoryLevels; ++i) {
		std::unordered_map<uint64_t, uint64_t>* level_allocations = pageAllocations[i];
		auto level_check = level_allocations->find(virtAddress);

		if(level_check != level_allocations->end()) {
			output->verbose(CALL_INFO, 4, 0, "Found entry for virtual address: %" PRIu64 " in the free process (level=%" PRIu32 ")\n",
				virtAddress, i);

			// We have found the allocation
			uint64_t allocation_length = level_check->second;
			uint64_t page_size = pageSizes[i];

			for(uint64_t free_size = 0; free_size < allocation_length; free_size += page_size) {
				uint64_t phys_addr = translateAddress(virtAddress + free_size);

				freePages[i]->push_front(phys_addr);
				pageTables[i]->erase(virtAddress + free_size);
			}

			found = true;
			break;
		}
	}

	if(! found) {
		if( reportUnmatchedFree ) {
			output->fatal(CALL_INFO, -1, "Error: asked to free virtual address: %" PRIu64 " but entry was not found in allocation map.\n",
				virtAddress);
		}
	} else {
		// Invalidate the cached entries
		translationCache->clear();
		statTranslationShootdown->addData(1);
	}
}

uint64_t ArielMemoryManager::translateAddress(uint64_t virtAddr) {
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

void ArielMemoryManager::printStats() {
	output->output("\n");
	output->output("Ariel Memory Management Statistics:\n");
	output->output("---------------------------------------------------------------------\n");
//	output->output("Translation Cache Queries:        %" PRIu64 "\n", translationQueries);
//	output->output("Translation Cache Hits:           %" PRIu64 "\n", translationCacheHits);
//	output->output("Translation Cache Evicts:         %" PRIu64 "\n", translationCacheEvicts);
//	output->output("Translation Cache Shoot down:     %" PRIu64 "\n", translationShootdown);
//	output->output("Total Page Allocations Performed: %" PRIu64 "\n", pageAllocationCount);
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
