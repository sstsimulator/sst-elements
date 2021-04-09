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


#ifndef _H_SST_MEMH_DRAMSIM3_BACKEND
#define _H_SST_MEMH_DRAMSIM3_BACKEND

#include "sst/elements/memHierarchy/membackend/memBackend.h"

#ifdef DEBUG
#define OLD_DEBUG DEBUG
#undef DEBUG
#endif

#include <dramsim3.h>

#ifdef OLD_DEBUG
#define DEBUG OLD_DEBUG
#undef OLD_DEBUG
#endif

namespace SST {
namespace MemHierarchy {

class DRAMSim3Memory : public SimpleMemBackend {
public:
/* Element Library Info */
    SST_ELI_REGISTER_SUBCOMPONENT_DERIVED(DRAMSim3Memory, "memHierarchy", "dramsim3", SST_ELI_ELEMENT_VERSION(1,0,0),
            "DRAMSim3-driven memory timings", SST::MemHierarchy::SimpleMemBackend)

#define DRAMSIM3_ELI_PARAMS MEMBACKEND_ELI_PARAMS,\
            /* Own parameters */\
            {"verbose",     "Sets the verbosity of the backend output", "0"},\
            {"config_ini",  "Name of the DRAMSim3 Device config file",   NULL},\
            {"output_dir",  "Name of the DRAMSim3 output directory",   "./"}

    SST_ELI_DOCUMENT_PARAMS( DRAMSIM3_ELI_PARAMS )

/* Begin class definition */
    DRAMSim3Memory(ComponentId_t id, Params &params);

    virtual bool issueRequest(ReqId, Addr, bool, unsigned );
    virtual bool clock(Cycle_t cycle);
    virtual void finish();

protected:
    void dramSimDone(unsigned int id, uint64_t addr, uint64_t clockcycle);

    dramsim3::MemorySystem *memSystem;
    std::map<uint64_t, std::deque<ReqId> > dramReqs;

private:
    std::function<void(uint64_t)> readCB;
    std::function<void(uint64_t)> writeCB;
};

}
}

#endif
