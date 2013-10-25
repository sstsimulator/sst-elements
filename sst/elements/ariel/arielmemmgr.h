
#ifndef _H_ARIEL_MEM_MANAGER
#define _H_ARIEL_MEM_MANAGER

#include <sst/core/sst_types.h>
#include <sst/core/event.h>
#include <sst/core/component.h>
#include <sst/core/link.h>
#include <sst/core/timeConverter.h>

#include <sst/core/interfaces/memEvent.h>
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
		uint32_t countMemoryLevels();
		uint64_t translateAddress(uint64_t virtAddr);

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
