
#include "arielmemmgr.h"

using namespace SST::ArielComponent;

ArielMemoryManager::ArielMemoryManager(uint32_t mLevels, uint64_t* pSizes, uint64_t* stdPCounts, Output* out, uint32_t defLevel) {
	output = out;
	memoryLevels = mLevels;
	defaultLevel = defLevel;
	
	output->verbose(CALL_INFO, 2, 0, "Creating a memory hierarchy of %" PRIu32 " levels.\n", mLevels);

	pageSizes = (uint64_t*) malloc(sizeof(uint64_t) * memoryLevels);
	for(uint32_t i = 0; i < mLevels; ++i) {
		output->verbose(CALL_INFO, 2, 0, "Level %" PRIu32 " page size is %" PRIu64 "\n", i, pSizes[i]);
		pageSizes[i] = pSizes[i];
	}

	freePages = (std::queue<uint64_t>**) malloc( sizeof(std::queue<uint64_t>*) * mLevels );
	uint64_t nextMemoryAddress = 0;
	for(uint32_t i = 0; i < mLevels; ++i) {
		freePages[i] = new std::queue<uint64_t>();

		output->verbose(CALL_INFO, 2, 0, "Level %" PRIu32 " page count is %" PRIu64 "\n", i, stdPCounts[i]);
		for(uint64_t j = 0; j < stdPCounts[i]; ++j) {
			freePages[i]->push(nextMemoryAddress);
			nextMemoryAddress += pageSizes[i];
		}
		output->verbose(CALL_INFO, 2, 0, "Level %" PRIu32 " usable (free) page queue contains %" PRIu32 " entries\n", i, 
			(uint32_t) freePages[i]->size());
	}

	pageAllocations = (std::map<uint64_t, uint64_t>**) malloc(sizeof(std::map<uint64_t, uint64_t>*) * memoryLevels);
	for(uint32_t i = 0; i < mLevels; ++i) {
		pageAllocations[i] = new std::map<uint64_t, uint64_t>();
	}

	pageTables = (std::map<uint64_t, uint64_t>**) malloc(sizeof(std::map<uint64_t, uint64_t>*) * memoryLevels);
	for(uint32_t i = 0; i < mLevels; ++i) {
		pageTables[i] = new std::map<uint64_t, uint64_t>();
	}
}

ArielMemoryManager::~ArielMemoryManager() {

}

void ArielMemoryManager::allocate(const uint64_t size, const uint32_t level, const uint64_t virtualAddress) {
	assert(level < memoryLevels);
	
	output->verbose(CALL_INFO, 4, 0, "Requesting a memory allocation of %" PRIu64 " bytes, in level: %" PRIu32 ", Virtual mapping=%" PRIu64 "\n",
		size, level, virtualAddress);

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
		freePages[level]->pop();
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

uint64_t ArielMemoryManager::translateAddress(uint64_t virtAddr) {
	uint64_t physAddr = (uint64_t) -1;
	bool found = false;
	
	output->verbose(CALL_INFO, 4, 0, "Page Table: translate virtual address %" PRIu64 "\n", virtAddr);
	
	// We will have to search every memory level to find where the address lies
	for(uint32_t i = 0; i < memoryLevels; ++i) {
		if(! found) {
			std::map<uint64_t, uint64_t>::iterator page_itr;
			const uint64_t pageSize = pageSizes[i];
			const uint64_t page_offset = virtAddr % pageSize;

			for(page_itr = pageTables[i]->begin(); page_itr != pageTables[i]->end(); page_itr++) {
				if((virtAddr >= page_itr->first) &&
					(virtAddr < (page_itr->first + pageSize))) {
					
					physAddr = page_itr->second + page_offset;

					output->verbose(CALL_INFO, 4, 0, "Page table hit: virtual address=%" PRIu64 " hit in level: %" PRIu32 ", virtual page start=%" PRIu64 ", virtual end=%" PRIu64 ", translates to phys page start=%" PRIu64 " translates to: phys address: %" PRIu64 " (offset added to phys start=%" PRIu64 ")\n",
						virtAddr, i, page_itr->first, page_itr->first + pageSize, page_itr->second, physAddr, page_offset);

					found = true;
					break;
				}
			}
		} else {
			break;
		}
	}

	if(found) {
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
