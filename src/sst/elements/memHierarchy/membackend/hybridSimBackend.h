// Copyright 2009-2021 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2021, NTESS
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

#include "sst/elements/memHierarchy/membackend/memBackend.h"

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
/* Element Library Info */
    SST_ELI_REGISTER_SUBCOMPONENT_DERIVED(HybridSimMemory, "memHierarchy", "hybridsim", SST_ELI_ELEMENT_VERSION(1,0,0),
            "HybridSim-driven memory timings", SST::MemHierarchy::SimpleMemBackend)

    SST_ELI_DOCUMENT_PARAMS( MEMBACKEND_ELI_PARAMS,
            /* Own parameters */
            {"verbose", "Sets the verbosity of the backend output", "0"},
            {"device_ini", "Name of the HybridSim Device config file", NULL},
            {"system_ini", "Name of the HybridSim Device system file", NULL} )

/* Begin class definition */
    HybridSimMemory(ComponentId_t id, Params &params);
    bool issueRequest( ReqId, Addr, bool, unsigned );
    bool clock(Cycle_t cycle);
    void finish();
private:
    void hybridSimDone(unsigned int id, uint64_t addr, uint64_t clockcycle);

    HybridSim::HybridSystem *memSystem;
    std::map<uint64_t, std::deque<ReqId> > dramReqs;
};

}
}

#endif
