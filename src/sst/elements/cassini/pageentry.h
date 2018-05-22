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


#ifndef _H_CASSINI_PAGE_ENTRY
#define _H_CASSINI_PAGE_ENTRY

#include <sst/core/event.h>
#include <sst/core/sst_types.h>
#include <sst/core/component.h>
#include <sst/core/link.h>
#include <sst/core/timeConverter.h>
#include <sst/elements/memHierarchy/memEvent.h>

using namespace SST;
using namespace SST::MemHierarchy;

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
