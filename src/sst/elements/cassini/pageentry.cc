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


#include "sst_config.h"
#include "sst/core/element.h"

#include <sst/elements/memHierarchy/memEvent.h>
#include <pageentry.h>

using namespace std;
using namespace SST;
using namespace SST::MemHierarchy;
using namespace SST::Cassini;

CassiniPageEntry::CassiniPageEntry(Addr vPageStart, Addr pPageStart, uint64_t pageLen) {
	virtualPageStart = vPageStart;
	physicalPageStart = pPageStart;
	pageLength = pageLen;
	allowExec = true;
	allowRead = true;
	allowWrite = true;
}

bool CassiniPageEntry::readAllowed() {
	return allowRead;
}

bool CassiniPageEntry::writeAllowed() {
	return allowWrite;
}

bool CassiniPageEntry::execAllowed() {
	return allowExec;
}

void CassiniPageEntry::markReadAllowed() {
	allowRead = true;
}

void CassiniPageEntry::markWriteAllowed() {
	allowWrite = true;
}

void CassiniPageEntry::markExecAllowed() {
	allowExec = true;
}

Addr CassiniPageEntry::getVirtualPageStart() {
	return virtualPageStart;
}

Addr CassiniPageEntry::getPhysicalPageStart() {
	return physicalPageStart;
}

uint64_t CassiniPageEntry::getPageLength() {
	return pageLength;
}
