// Copyright 2009-2019 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2019, NTESS
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.
//


/* Author: Vamsee Reddy Kommareddy
 * E-mail: vamseereddy@knights.ucf.edu
 */


#include <cstring>
#include <string>
#include <fstream>
#include <sstream>
#include <map>
#include <algorithm>

#include <stdio.h>
#include <stdint.h>
#include <poll.h>
#include <queue>

#include <sst/core/sst_types.h>
#include <sst/core/event.h>
#include <sst/core/link.h>
#include <sst/core/timeConverter.h>
#include <sst/core/interfaces/simpleMem.h>
#include <sst/core/output.h>

#include "Opal_Event.h"
#include "mempool.h"


using namespace SST;

using namespace SST::OpalComponent;


namespace SST
{
	namespace OpalComponent
	{
		class MemoryPrivateInfo;



	}
}
