// Copyright 2009-2014 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2014, Sandia Corporation
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#ifndef _H_ARIEL_MEM_MANAGER
#define _H_ARIEL_MEM_MANAGER

#include <sst/core/output.h>

#include <stdint.h>
#include <queue>
#include <vector>
#include <map>

namespace SST {
namespace ArielComponent {

class ArielMemoryManager {

	public:
		ArielMemoryManager(uint32_t memoryLevels, uint64_t* pageSize, uint64_t* stdPageCount, Output* output, uint32_t defLevel);
		~ArielMemoryManager();
		void allocate(const uint64_t size, const uint32_t level, const uint64_t virtualAddress);
		void free(const uint64_t vAddr);

		uint32_t countMemoryLevels();
		uint64_t translateAddress(uint64_t virtAddr);
		void setDefaultPool(uint32_t pool);

	private:
		Output* output;
		uint32_t defaultLevel;
		uint32_t memoryLevels;
		uint64_t* pageSizes;
		std::queue<uint64_t>** freePages;
		std::map<uint64_t, uint64_t>** pageAllocations;
		std::map<uint64_t, uint64_t>** pageTables;
};

}
}

#endif
