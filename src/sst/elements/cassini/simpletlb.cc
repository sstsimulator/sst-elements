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


#include <vector>

#include "sst_config.h"
#include "sst/core/element.h"

#include "simpletlb.h"
#include <stdint.h>

using namespace SST;
using namespace SST::MemHierarchy;
using namespace SST::Cassini;

SimpleTLB::SimpleTLB(Params& params)
{

}

SimpleTLB::~SimpleTLB()
{

}

Addr SimpleTLB::lookupPhysicalAddress(Addr virt, PageAccessType access)
{
	Addr offset = (virt) % pageSize;
	Addr blockStart = virt - offset;
	Addr phys = 0;

	if(pageTable.find(blockStart) != pageTable.end()) {
		CassiniPageEntry* entry = pageTable[blockStart];
		phys = entry->getPhysicalPageStart() + offset;

		if(access == READ_ONLY) {
			if(! entry->readAllowed()) {
				std::cerr << "Segmentation Fault: Memory address " << virt <<
					" attemped a read but is not permitted by the page table" << std::endl;
				exit(-1);
			}
		}

		if(access == READ_WRITE) {
			if(! entry->writeAllowed()) {
				std::cerr << "Segmentation Fault: Memory address " << virt <<
					" attemped a read/write but is not permitted by the page table" << std::endl;
				exit(-1);
			}
		}

		if(access == READ_EXECUTE) {
			if(! (entry->execAllowed() && entry->readAllowed())) {
				std::cerr << "Segmentation Fault: Memory address " << virt <<
					" attemped a read for execute but is not permitted by the page table" << std::endl;
				exit(-1);
			}
		}


	} else {
		std::cerr << "Segmentation Fault: Memory lookup was attempted at address: "
			<< virt << ", but is not in the TLB" << std::endl;
		exit(-1);
	}

	return phys;
}

void SimpleTLB::clear()
{
	pageTable.clear();
}

bool SimpleTLB::containsEntry(Addr vAddr)
{
	uint64_t offset = (vAddr) % pageSize;
	uint64_t blockStart = vAddr - offset;

	return (pageTable.find(blockStart) != pageTable.end());
}

void SimpleTLB::setPageSize(uint64_t nwPageSz) {
	pageSize = nwPageSz;
}
