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

#ifndef SCHEDULER_TIME_BASE

#define SCHEDULER_TIME_BASE "1 us" //original
//#define SCHEDULER_TIME_BASE "1 ns"

#endif


//NetworkSim: Moved ITMI definition to here
#ifndef SCHEDULER_ITMI
#define SCHEDULER_ITMI

namespace SST {
    namespace Scheduler {
    	class TaskMapInfo;
		
		struct ITMI {int i; TaskMapInfo *tmi;};

	}
}

#endif
