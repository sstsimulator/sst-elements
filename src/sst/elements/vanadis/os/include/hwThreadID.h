// Copyright 2009-2022 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2022, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// of the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef _H_VANADIS_NODE_OS_INCLUDE_HW_THREAD_ID
#define _H_VANADIS_NODE_OS_INCLUDE_HW_THREAD_ID

#include <string>

#include <set>
namespace SST {
namespace Vanadis {

namespace OS {

struct HwThreadID {
    HwThreadID( int core, int hwThread ) : core(core), hwThread(hwThread) {}
    int core;
    int hwThread;
};

}
}
}

#endif
