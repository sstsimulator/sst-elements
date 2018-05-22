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

