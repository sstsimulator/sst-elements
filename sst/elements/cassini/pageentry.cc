
#include "sst_config.h"
#include "sst/core/serialization/element.h"
#include "sst/core/element.h"

#include <sst/core/interfaces/memEvent.h>
#include <pageentry.h>

using namespace std;
using namespace SST;
using namespace SST::Interfaces;
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
