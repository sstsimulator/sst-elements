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


#ifndef _H_SST_MEMH_HYBRIDSIM_BACKEND
#define _H_SST_MEMH_HYBRIDSIM_BACKEND

#include "membackend/memBackend.h"

#ifdef DEBUG
#define OLD_DEBUG DEBUG
#undef DEBUG
#endif

#include <HybridSim.h>

#ifdef OLD_DEBUG
#define DEBUG OLD_DEBUG
#undef OLD_DEBUG
#endif

namespace SST {
namespace MemHierarchy {

class HybridSimMemory : public SimpleMemBackend {
public:
    HybridSimMemory(Component *comp, Params &params);
    bool issueRequest( ReqId, Addr, bool, unsigned );
    void clock();
    void finish();
private:
    void hybridSimDone(unsigned int id, uint64_t addr, uint64_t clockcycle);

    HybridSim::HybridSystem *memSystem;
    std::map<uint64_t, std::deque<ReqId> > dramReqs;
};

}
}

#endif
