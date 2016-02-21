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


#ifndef _H_ARIEL_MEM_MANAGER
#define _H_ARIEL_MEM_MANAGER

#include <sst/core/component.h>
#include <sst/core/output.h>

#include <stdint.h>
#include <deque>
#include <vector>
#include <unordered_map>

using namespace SST;

namespace SST {
namespace ArielComponent {

class ArielMemoryManager {

	public:
		ArielMemoryManager(SST::Component* ownMe, uint32_t memoryLevels, uint64_t* pageSize, uint64_t* stdPageCount, Output* output,
			uint32_t defLevel, uint32_t translateCacheEntryCount);
		~ArielMemoryManager();
		void allocate(const uint64_t size, const uint32_t level, const uint64_t virtualAddress);
		void free(const uint64_t vAddr);

		uint32_t countMemoryLevels();
		uint64_t translateAddress(uint64_t virtAddr);
		void setDefaultPool(uint32_t pool);
		void cacheTranslation(uint64_t virtualA, uint64_t physicalA);
		void populateTables(const char* populateFilePath, const uint32_t level);
		void printStats();
		void disableTranslation();
		void enableTranslation();

	private:
		Output* output;
		SST::Component* owner;

		Statistic<uint64_t>* statTranslationCacheHits;
		Statistic<uint64_t>* statTranslationCacheEvict;
		Statistic<uint64_t>* statTranslationQueries;
		Statistic<uint64_t>* statTranslationShootdown;
		Statistic<uint64_t>* statPageAllocationCount;

		uint32_t defaultLevel;
		uint32_t memoryLevels;
		uint64_t* pageSizes;
		std::deque<uint64_t>** freePages;
		std::unordered_map<uint64_t, uint64_t>** pageAllocations;
		std::unordered_map<uint64_t, uint64_t>** pageTables;
		std::unordered_map<uint64_t, uint64_t>* translationCache;
		const uint32_t translationCacheEntries;
		bool translationEnabled;
		bool reportUnmatchedFree;
};

}
}

#endif
