
#ifndef _H_SST_SIMPLE_TLB
#define _H_SST_SIMPLE_TLB

#include <vector>
#include <map>

#include <sst/core/event.h>
#include <sst/core/sst_types.h>
#include <sst/core/component.h>
#include <sst/core/link.h>
#include <sst/core/timeConverter.h>
#include <sst/core/interfaces/memEvent.h>
#include <sst/elements/memHierarchy/cacheListener.h>

#include "pageentry.h"

using namespace SST;
using namespace SST::Interfaces;
using namespace SST::MemHierarchy;
using namespace std;

namespace SST {
namespace Cassini {

enum PageAccessType {
	READ_ONLY,
	READ_WRITE,
	READ_EXECUTE
};

class SimpleTLB {
    public:
	SimpleTLB(Params& params);
        ~SimpleTLB();
	Addr lookupPhysicalAddress(Addr vAddr, PageAccessType access);
	void clear();
	bool containsEntry(Addr vAddr);
	void setPageSize(uint64_t pageSize);

    private:
	map<Addr, CassiniPageEntry*> pageTable;
	uint64_t pageSize;
};

}
}

#endif
