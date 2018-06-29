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

#include "arielmemmgr_malloc.h"

using namespace SST::ArielComponent;

ArielMemoryManagerMalloc::ArielMemoryManagerMalloc(SST::Component* owner, Params& params) : 
            ArielMemoryManager(owner, params) {
    
    memoryLevels = (uint32_t) params.find<uint32_t>("memorylevels", 1);
    defaultLevel = (uint32_t) params.find<uint32_t>("defaultlevel", 0);
    output->verbose(CALL_INFO, 1, 0, "Configuring for %" PRIu32 " memory levels; default level is %" PRIu32 ".\n", memoryLevels, defaultLevel);

    // Configure each memory level's free page pool
    freePages = (std::deque<uint64_t>**) malloc(sizeof(std::deque<uint64_t>*) * memoryLevels);
    pageSizes = (uint64_t*) malloc(sizeof(uint64_t) * memoryLevels);

    // PageAllocation and PageTable structures
    pageAllocations = (std::unordered_map<uint64_t, uint64_t>**) malloc(sizeof(std::unordered_map<uint64_t, uint64_t>*) * memoryLevels);
    pageTables = (std::unordered_map<uint64_t, uint64_t>**) malloc(sizeof(std::unordered_map<uint64_t, uint64_t>*) * memoryLevels);
    for (uint32_t i = 0; i <memoryLevels; ++i) {
        pageAllocations[i] = new std::unordered_map<uint64_t, uint64_t>();
        pageTables[i] = new std::unordered_map<uint64_t, uint64_t>();
    }

    // Initialize data structures
    char * level_buffer = (char*) malloc(sizeof(char) * 256);
    uint64_t nextMemoryAddress = 0;
    for (uint32_t i = 0; i < memoryLevels; ++i) {
        // Page size
        sprintf(level_buffer, "pagesize%" PRIu32, i);
        pageSizes[i] = (uint64_t) params.find<uint64_t>(level_buffer, 4096);
        output->verbose(CALL_INFO, 2, 0, "Level %" PRIu32 " page size is %" PRIu64 "\n", i, pageSizes[i]);

        // Page count
        sprintf(level_buffer, "pagecount%" PRIu32, i);
        uint64_t pageCount = (uint64_t) params.find<uint64_t>(level_buffer, 131072);
        output->verbose(CALL_INFO, 2, 0, "Level %" PRIu32 " page count is %" PRIu64 "\n", i, pageCount);

        // Configure page pool
        freePages[i] = new std::deque<uint64_t>();

        if (ArielPageMappingPolicy::LINEAR == mapPolicy) {
            mapPagesLinear(pageCount, pageSizes[i], nextMemoryAddress, freePages[i]);
        } else {
            mapPagesRandom(pageCount, pageSizes[i], nextMemoryAddress, freePages[i]);
        }
        nextMemoryAddress += pageCount * pageSizes[i];

        output->verbose(CALL_INFO, 2, 0, "Level %" PRIu32 " usable (free) page queue contains %" PRIu32 " entries\n", i, (uint32_t) freePages[i]->size());

        // Populate page table if needed
        sprintf(level_buffer, "page_populate_%" PRIu32, i);
        std::string popFilePath = params.find<std::string>(level_buffer, "");
        if (popFilePath != "") {
            output->verbose(CALL_INFO, 1, 0, "Populating page tables for level %" PRIu32 " from %s...\n", i, popFilePath.c_str());
            populatePageTable(popFilePath, pageTables[i], freePages[i], pageSizes[i]);
        }

        /* Register statistics per pool */
        sprintf(level_buffer, "mempool_%" PRIu32, i);
        statBytesAlloc.push_back(registerStatistic<uint64_t>("bytes_allocated_in_pool", level_buffer));
        statBytesFree.push_back(registerStatistic<uint64_t>("bytes_freed_from_pool", level_buffer));
        statDemandAllocs.push_back(registerStatistic<uint64_t>("demand_page_allocs", level_buffer));
    }

    free(level_buffer);
}

ArielMemoryManagerMalloc::~ArielMemoryManagerMalloc() {
}


void ArielMemoryManagerMalloc::setDefaultPool(uint32_t pool) {
    defaultLevel = pool;
}

uint32_t ArielMemoryManagerMalloc::getDefaultPool() {
    return defaultLevel;
}


/*
 *  Determine whether there are enough free pages in a level for an allocation request
 */
bool ArielMemoryManagerMalloc::canAllocateInLevel(const uint64_t size, const uint32_t level) {
    int pageCount = size / pageSizes[level] + ((size % pageSizes[level] == 0) ? 0 : 1);
    return freePages[level]->size() >= pageCount;
}


void ArielMemoryManagerMalloc::allocate(const uint64_t size, const uint32_t level, const uint64_t virtualAddress) {
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


    // We will do all of our allocation based on whole pages, inefficient maybe but much
    // simpler to implement and debug
    if(remainder > 0) {
        roundedSize += (pageSize - remainder);
    }

    output->verbose(CALL_INFO, 4, 0, "Request rounded to %" PRIu64 " bytes\n",
        roundedSize);
    
    statDemandAllocs[level]->addData(roundedSize/pageSize);

    uint64_t nextVirtPage = virtualAddress;
    for(uint64_t bytesLeft = 0; bytesLeft < roundedSize; bytesLeft += pageSize) {
        if(freePages[level]->empty()) {
                output->verbose(CALL_INFO, 4, 0, "Requesting a memory allocation at level: %" PRIu32 " which will fail due to not having enough free pages\n",
                    level);
                    for (uint32_t i = 0; i < memoryLevels; ++i) {
                        output->verbose(CALL_INFO, -1, 0, "Free pages at level %" PRIu32 " : %" PRIu64 "\n", i, static_cast<uint64_t>(freePages[i]->size()));
                    }
                    output->fatal(CALL_INFO, -1, "Requested a memory allocation at level: %" PRIu32 " of size %" PRIu64 " which failed due to not having enough free pages\n",
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

/*
 *  Note on allocation alignment:
 *  Physical and virtual mapping may not be aligned the same (have different page/cache-line offsets).
 *  This happens because the VA assigned by the OS may be different than the VA that would have been assigned in an MLM environment.
 *  For example the app allocates location A to VA 0 and location B to VA 8, but in simulation we'd like A to be in memory pool 0 and B to be in pool 1.
 *  Then A & B share a virtual cache block but not a simulated physical cache block.
 *
 *  Therefore, Ariel should immediately translate virtual to physical addresses and ALWAYS use physical addresses when computing offsets.
 */
bool ArielMemoryManagerMalloc::allocateMalloc(const uint64_t size, const uint32_t level, const uint64_t virtualAddress) {
    output->verbose(CALL_INFO, 4, 0, "Allocate malloc received. VA: %" PRIu64 ". Size: %" PRIu64 ". Level: %" PRIu32 ".\n", virtualAddress, size, level);

    // Check whether a malloc mapping already exists (i.e., we missed a free)
    std::map<uint64_t, uint64_t>::iterator it = mallocTranslations.upper_bound(virtualAddress);
    if (it == mallocTranslations.begin()) it = mallocTranslations.end();
    else if (!mallocTranslations.empty()) it--;

    if (it != mallocTranslations.end() && it->first <= virtualAddress) {
        uint64_t primaryAddr = mallocPrimaryVAMap.find(it->first)->second;
        if (virtualAddress < primaryAddr + (mallocInformation.find(primaryAddr)->second).size) {
            output->verbose(CALL_INFO, 4, 0, "Found conflicting malloc, freeing address %" PRIu64 "\n", primaryAddr);
            freeMalloc(primaryAddr);
        }
    }

    // Allocate new page(s). Round malloc to nearest whole page TODO fix so we can map partial pages -> needs a local VA->Ariel_VA mapping
    uint64_t pageCount = size / pageSizes[level];
    if (size % pageSizes[level] != 0) pageCount++;

    // Check whether enough pages are available
    if (freePages[level]->size() < pageCount) {
        output->verbose(CALL_INFO, 4, 0, "Requested memory cannot be allocated, not enough pages. Have: %" PRIu64 ", Need: %" PRIu64 "\n", static_cast<uint64_t>(freePages[level]->size()), pageCount);
        return false;
    }

    // Allocate the pages
    std::unordered_set<uint64_t>* virtualPages = new std::unordered_set<uint64_t>;
    uint64_t nextVirtPage = virtualAddress;
    uint64_t firstPhysAddr, lastPhysAddr;
    firstPhysAddr = freePages[level]->front();
    for (uint64_t i = 0; i != pageCount; i++) {
        uint64_t nextPhysPage = freePages[level]->front();
        mallocTranslations.insert(std::make_pair(nextVirtPage, nextPhysPage));
        mallocPrimaryVAMap.insert(std::make_pair(nextVirtPage, virtualAddress));
        freePages[level]->pop_front();
        virtualPages->insert(nextVirtPage);
        nextVirtPage += pageSizes[level];
        lastPhysAddr = nextPhysPage;
    }

    output->verbose(CALL_INFO, 4, 0, "Malloc mapped %" PRIu64 " to [%" PRIu64 ", %" PRIu64 "] (%" PRIu64 " pages).\n", virtualAddress, firstPhysAddr, lastPhysAddr, pageCount);

    // Record malloc
    mallocInformation.insert(std::make_pair(virtualAddress, mallocInfo(size, level, virtualPages)));
    
    statBytesAlloc[level]->addData(size);
    return true;
}


void ArielMemoryManagerMalloc::freeMalloc(const uint64_t virtualAddress) {
    output->verbose(CALL_INFO, 4, 0, "Freeing %" PRIu64 "\n", virtualAddress);
    
    // Lookup VA in mallocInformation
    std::map<uint64_t, mallocInfo>::iterator it = mallocInformation.find(virtualAddress);
    if (it == mallocInformation.end()) return;
    
    statBytesFree[it->second.level]->addData(it->second.size);

    // Free each VA in mallocInformation from mallocTranslations & mallocPrimaryVAMap TODO fix so that mapping stays but address is available for future mallocs
    std::unordered_set<uint64_t>* myKeys = (it->second.VAKeys);
    for (std::unordered_set<uint64_t>::iterator vaIt = myKeys->begin(); vaIt != myKeys->end(); vaIt++) {
        mallocPrimaryVAMap.erase(*vaIt);
        freePages[(it->second).level]->push_front(mallocTranslations.find(*vaIt)->second);
        mallocTranslations.erase(*vaIt);
    }

    // Remove mallocInformation entry
    delete myKeys;
    mallocInformation.erase(virtualAddress);
}


uint64_t ArielMemoryManagerMalloc::translateAddress(uint64_t virtAddr) {
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

    // Check malloc mappings
    if (!mallocTranslations.empty()) {
        std::map<uint64_t, uint64_t>::iterator it = mallocTranslations.upper_bound(virtAddr);
        if (it == mallocTranslations.begin()) it = mallocTranslations.end();
        else it--;

        if (it != mallocTranslations.end() && (it->first <= virtAddr)) {
            uint64_t primaryAddr = mallocPrimaryVAMap.find(it->first)->second;
            if (virtAddr < (primaryAddr + (mallocInformation.find(primaryAddr)->second).size)) {
                uint64_t offset = virtAddr - it->first;
                physAddr = offset + it->second;
                found = true;
            }
        }
    }

    // We will have to search every memory level to find where the address lies
    for(uint32_t i = 0; i < memoryLevels; ++i) {
        if (!found) {
        std::unordered_map<uint64_t, uint64_t>::iterator page_itr;
        const uint64_t pageSize = pageSizes[i];
            const uint64_t page_offset = virtAddr % pageSize;
        const uint64_t page_start = virtAddr - page_offset;

        page_itr = pageTables[i]->find(page_start);

            if (page_itr != pageTables[i]->end()) {
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
        // Attempt defaultLevel but fall through to other levels if needed/available
            if (canAllocateInLevel(8, defaultLevel)) {
                allocate(8, defaultLevel, virtAddr - offset);
            } else {
                bool allocated = false;
                for (uint32_t i = 0; i < memoryLevels; i++) {
                    if (canAllocateInLevel(8, i)) {
                        offset = virtAddr % pageSizes[i];
                        allocate(8, i, virtAddr - offset);
                        allocated = true;
                        break;
                    }
                }
                if (!allocated) output->fatal(CALL_INFO, -1, "Attempted to allocate page for address %" PRIu64 " but no free pages are available\n", virtAddr);
            }

        // Now attempt to refind it
        const uint64_t newPhysAddr = translateAddress(virtAddr);

        output->verbose(CALL_INFO, 4, 0, "Page allocation routine mapped to address: %" PRIu64 "\n", newPhysAddr );

        return newPhysAddr;
    }
}

void ArielMemoryManagerMalloc::printStats() {
    output->output("\n");
    output->output("Ariel Memory Management Statistics:\n");
    output->output("---------------------------------------------------------------------\n");
    output->output("Page Table Sizes:\n");

    for(uint32_t i = 0; i < memoryLevels; ++i) {
        output->output("- Demand map entries at level %" PRIu32 "         %" PRIu32 "\n",
            i, (uint32_t) pageTables[i]->size());
    }

    output->output("Page Table Coverages:\n");

    for(uint32_t i = 0; i < memoryLevels; ++i) {
        output->output("- Demand bytes at level %" PRIu32 "              %" PRIu64 "\n",
            i, ((uint64_t) pageTables[i]->size()) * ((uint64_t) pageSizes[i]));
    }
}
