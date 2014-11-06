
#ifndef _H_SS_PROSPERO_MEM_MGR
#define _H_SS_PROSPERO_MEM_MGR

#include <sst/core/output.h>
#include <map>

namespace SST {
namespace Prospero {

class ProsperoMemoryManager {
public:
	ProsperoMemoryManager(const uint64_t pageSize, Output* output);
	~ProsperoMemoryManager();
	uint64_t translate(const uint64_t virtAddr);

private:
	std::map<uint64_t, uint64_t> pageTable;
	uint64_t nextPageStart;
	uint64_t pageSize;
	Output* output;
};

}
}

#endif

