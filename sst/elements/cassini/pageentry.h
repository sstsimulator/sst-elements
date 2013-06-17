
#ifndef _H_CASSINI_PAGE_ENTRY
#define _H_CASSINI_PAGE_ENTRY

#include <sst/core/event.h>
#include <sst/core/sst_types.h>
#include <sst/core/component.h>
#include <sst/core/link.h>
#include <sst/core/timeConverter.h>
#include <sst/core/interfaces/memEvent.h>

using namespace SST;
using namespace SST::Interfaces;

namespace SST {
namespace Cassini {

class CassiniPageEntry {

	public:
		CassiniPageEntry(Addr vPageStart, Addr pPageStart, uint64_t pageLength);
		bool readAllowed();
		bool writeAllowed();
		bool execAllowed();
		Addr getVirtualPageStart();
		Addr getPhysicalPageStart();
		uint64_t getPageLength();
		void markReadAllowed();
		void markWriteAllowed();
		void markExecAllowed();

	private:
		Addr virtualPageStart;
		Addr physicalPageStart;
		uint64_t pageLength;
		bool allowExec;
		bool allowRead;
		bool allowWrite;

};

}
}

#endif
