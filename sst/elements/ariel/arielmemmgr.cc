#include "sst_config.h"
#include "sst/core/serialization.h"
#include "sst/core/element.h"
#include <sst/core/params.h>
#include <sst/core/simulation.h>

#include "arielmemmgr.h"

using namespace SST::ArielComponent;

ArielMemoryManager::ArielMemoryManager(uint32_t mLevels, uint64_t* pSizes, uint64_t* stdPCounts, Output* out, uint32_t defLevel) {
	output = out;
	memoryLevels = mLevels;
	defaultLevel = defLevel;

	pageSizes = (uint64_t*) malloc(sizeof(uint64_t) * memoryLevels);
	for(uint32_t i = 0; i < mLevels; ++i) {
		pageSizes[i] = pSizes[i];
	}

	freePages = (std::queue<uint64_t>**) malloc( sizeof(std::queue<uint64_t>*) * mLevels );
	uint64_t nextMemoryAddress = 0;
	for(uint32_t i = 0; i < mLevels; ++i) {
		freePages[i] = new std::queue<uint64_t>();

		for(uint64_t j = 0; j < stdPCounts[i]; ++j) {
			freePages[i]->push(nextMemoryAddress);
			nextMemoryAddress += pageSizes[i];
		}
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

	uint64_t pageSize = pageSizes[level];

	uint64_t roundedSize = size;
	uint64_t remainder = size % pageSize;
	// We will do all of our allocated based on whole pages, inefficient maybe but much
	// simpler to implement and debug
	if(roundedSize > 0) {
		roundedSize += (pageSize - remainder);
	}

	uint64_t nextVirtPage = virtualAddress;
	for(uint64_t bytesLeft = 0; bytesLeft < roundedSize; bytesLeft += pageSize) {
		if(freePages[level]->empty()) {
			output->fatal(CALL_INFO, -1, 0, 0, "Requested a memory allocation at level: %" PRIu32 " of size: %" PRIu64 " which failed due to not having enough free pages\n",
				level, size);
		}

		uint64_t nextPhysPage = freePages[level]->front();
		freePages[level]->pop();
		pageTables[level]->insert( std::pair<uint64_t, uint64_t>(nextVirtPage, nextPhysPage) );

		nextVirtPage += pageSize;
	}

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

	// We will have to search every memory level to find where the address lies
	for(uint32_t i = 0; i < memoryLevels; ++i) {
		if(! found) {
			std::map<uint64_t, uint64_t>::iterator page_itr;
			const uint64_t pageSize = pageSizes[i];

			for(page_itr = pageTables[i]->begin(); page_itr != pageTables[i]->end(); page_itr++) {
				if((virtAddr >= page_itr->second) &&
					(virtAddr < page_itr->second + pageSize)) {
					physAddr = page_itr->second + (virtAddr % pageSize);

					found = true;
					break;
				}
			}
		}
	}

	if(found) {
		return physAddr;
	} else {
		// We did not find the address in memory, that means we should allocate it one from our default pool
		uint64_t offset = virtAddr % pageSizes[defaultLevel];
		// Perform an allocation so we can then re-find the address
		allocate(8, defaultLevel, virtAddr - offset);

		// Now attempt to refind it
		uint64_t newPhysPage = translateAddress(virtAddr);

		return newPhysPage + offset;
	}
}
