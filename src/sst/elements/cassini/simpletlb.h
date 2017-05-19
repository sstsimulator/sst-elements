// Copyright 2009-2017 Sandia Corporation. Under the terms
// of Contract DE-NA0003525 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2017, Sandia Corporation
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#ifndef _H_SST_SIMPLE_TLB
#define _H_SST_SIMPLE_TLB

#include <vector>
#include <map>

#include <sst/core/event.h>
#include <sst/core/sst_types.h>
#include <sst/core/component.h>
#include <sst/core/link.h>
#include <sst/core/timeConverter.h>
#include <sst/elements/memHierarchy/cacheListener.h>

#include "pageentry.h"

using namespace SST;
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
