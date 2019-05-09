// Copyright 2009-2018 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2018, NTESS
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.
//


//
/* Author: Amro Awad
 * E-mail: aawad@sandia.gov
 */

#ifndef _H_SST_MEMPAGESTATS
#define _H_SST_MEMPAGESTATS


#include <sst_config.h>
#include <sst/core/component.h>
#include <sst/core/timeConverter.h>
#include <sst/core/link.h>
#include <sst/elements/memHierarchy/memEvent.h>
#include<map>
#include<list>

using namespace SST;
using namespace SST::MemHierarchy;

namespace SST { namespace MemHierarchy{

// This class structure represents NVM-Based DIMM, including the NVM-DIMM controller
class MemPageStats
{

public:

	Statistic<uint64_t>* density;

	MemPageStats(SST::Component* own, uint64_t id) {
		char* subID = (char*) malloc(sizeof(char) * 32);
		sprintf(subID, "%" PRIu32, id);
		density = own->registerStatistic<uint64_t>( "page_count", subID );
	}
};

}}

#endif

