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


#ifndef _H_SST_MIRANDA_MEMORY_MANAGER
#define _H_SST_MIRANDA_MEMORY_MANAGER

#include <vector>
#include <sst/core/rng/marsaglia.h>
#include <sst/core/output.h>

using namespace SST::RNG;

namespace SST {
namespace Miranda {

enum MirandaPageMappingPolicy {
	LINEAR,
	RANDOMIZED
};

class MirandaMemoryManager {

public:
	MirandaMemoryManager(
		SST::Output* mgrOutput,
		const uint64_t _pageSize,
		const uint64_t _pageCount,
		const MirandaPageMappingPolicy mapPolicy) :
		pageSize(_pageSize),
		pageCount(_pageCount),
		maxMemoryAddress(_pageSize * _pageCount),
		output(mgrOutput) {
		
		output->verbose(CALL_INFO, 2, 0, "Creating memory manager, page size=%" PRIu64 ", page count=%" PRIu64 ", max address=%" PRIu64 "\n",
			pageSize, pageCount, maxMemoryAddress);

		pageMap.resize(pageCount);
		
		// Allocate pages into their standard linear mapping scheme
		for(uint64_t i = 0; i < pageCount; ++i) {
			pageMap[i] = i * pageSize;
		}
		
		switch(mapPolicy) {
		case LINEAR:
			output->verbose(CALL_INFO, 2, 0, "Memory is set to LINEAR mapping, will not adjust current page maps\n");
			// Nothing to do
			break;
			
		case RANDOMIZED:
			output->verbose(CALL_INFO, 2, 0, "Memory is set to RANDOMIZED mapping, will perform a randomized shuffle of pages...\n");
			MarsagliaRNG rng(11, 200009011);
			
			// Random swap pages all over the space so we can distribute accesses
			// across the memory system unevenly
			for(uint64_t i = 0; i < (pageCount * 2); ++i) {
				const uint64_t selectA = (rng.generateNextUInt64() % pageCount);
				const uint64_t selectB = (rng.generateNextUInt64() % pageCount);
				
				if( selectA != selectB ) {
					const uint64_t pageA = pageMap[selectA];
					pageMap[selectA] = pageMap[selectB];
					pageMap[selectB] = pageA;
					
					output->verbose(CALL_INFO, 64, 0, "Swapping index %" PRIu64 " with index %" PRIu64 ", pageA=%" PRIu64 ", pageB=%" PRIu64 "\n",
						selectA, selectB, pageA, pageMap[selectA]);
				}
			}
			
			for(uint64_t i = 0; i < pageCount; ++i) {
				output->verbose(CALL_INFO, 32, 0, "Virtual Start = %20" PRIu64 " Physical Start = %20" PRIu64 "\n",
					(i * pageSize), pageMap[i]);
			}
			
			break;
		}
		
	}
	
	uint64_t mapAddress(const uint64_t addrIn) const {
		if( __builtin_expect(addrIn >= maxMemoryAddress, 0) ) {
			output->fatal(CALL_INFO, -1, "Error: address %" PRIu64 " exceeds the maximum address in Miranda calculated by pageSize (%" PRIu64 ") * pageCount (%" PRIu64 ") = %" PRIu64 "\n",
				addrIn, pageSize, pageCount, maxMemoryAddress);
		}
		
		const uint64_t pageOffset       = addrIn % pageSize;
		const uint64_t virtualPageStart = (addrIn - pageOffset) / pageSize;
		
		const uint64_t physPageStart    = pageMap[virtualPageStart];
		const uint64_t physAddress      = physPageStart + pageOffset;
		
		#define MIRANDA_MAP_ADDRESS_VERBOSE_LEVEL 16
		
		if( __builtin_expect(output->getVerboseLevel() >= MIRANDA_MAP_ADDRESS_VERBOSE_LEVEL, 0) ) {
			output->verbose(CALL_INFO, MIRANDA_MAP_ADDRESS_VERBOSE_LEVEL, 0, "Mapping v-addr: %" PRIu64 " pageOffset: %" PRIu64 " -> (%" PRIu64 " - %" PRIu64 ")/%" PRIu64 " = v-page-index: %" PRIu64 ", p-start: %" PRIu64 " + poffset %" PRIu64 " = %" PRIu64 "\n",
				addrIn, pageOffset, addrIn, pageOffset, pageSize, virtualPageStart, physPageStart, pageOffset, physAddress);
		}
		
		#undef MIRANDA_MAP_ADDRESS_VERBOSE_LEVEL
		
		return physAddress;
	}

private:
	uint64_t pageSize;
	uint64_t pageCount;
	uint64_t maxMemoryAddress;
	SST::Output* output;
	
	std::vector<uint64_t> pageMap;
	
};

}
}

#endif