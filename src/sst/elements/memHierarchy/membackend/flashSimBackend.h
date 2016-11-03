// Copyright 2009-2016 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2016, Sandia Corporation
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#ifndef _H_SST_MEMH_FLASH_DIMM_SIM_BACKEND
#define _H_SST_MEMH_FLASH_DIMM_SIM_BACKEND

#include "membackend/memBackend.h"

#ifdef DEBUG
#define OLD_DEBUG DEBUG
#undef DEBUG
#endif

#include <FlashDIMM.h>

#ifdef OLD_DEBUG
#define DEBUG OLD_DEBUG
#undef OLD_DEBUG
#endif

namespace SST {
namespace MemHierarchy {

class FlashDIMMSimMemory : public SimpleMemBackend {
public:
    FlashDIMMSimMemory(Component *comp, Params &params);
    bool issueRequest(ReqId, Addr, bool, unsigned );
    void clock();
    void finish();

private:
    void FlashDIMMSimDone(unsigned int id, uint64_t addr, uint64_t clockcycle);
    uint32_t pendingRequests;
    uint32_t maxPendingRequests;

    FDSim::FlashDIMM *memSystem;
    std::map<uint64_t, std::deque<ReqId> > dramReqs;

};

}
}

#endif
