// Copyright 2009-2025 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2025, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// of the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#ifndef _H_SST_MEMH_RAMULATOR2_BACKEND
#define _H_SST_MEMH_RAMULATOR2_BACKEND

#include "sst/elements/memHierarchy/membackend/memBackend.h"


#include "base/base.h"
#include "base/request.h"
#include "frontend/frontend.h"

#include <cstddef>
#include <map>
#include <set>
#include <string>
//#include "memory_system/memory_system.h"


#ifdef OLD_DEBUG
#define DEBUG OLD_DEBUG
#undef OLD_DEBUG
#endif

namespace SST {
namespace MemHierarchy {

class ramulator2Memory : public SimpleMemBackend {
public:
/* Element Library Info */
    SST_ELI_REGISTER_SUBCOMPONENT(ramulator2Memory, "memHierarchy", "ramulator2", SST_ELI_ELEMENT_VERSION(1,0,0),
            "Ramulator2-driven memory timings", SST::MemHierarchy::SimpleMemBackend)

    SST_ELI_DOCUMENT_PARAMS( MEMBACKEND_ELI_PARAMS,
            /* Own parameters */
            {"configFile",  "Name of the Ramulator2 Device config file", NULL} )

/* Begin class definition */
    ramulator2Memory(ComponentId_t id, Params &params);
    bool issueRequest(ReqId, Addr, bool, unsigned );
    virtual bool clock(Cycle_t cycle);
    virtual void finish();

protected:
    std::string config_path;
    Ramulator::IFrontEnd* ramulator2_frontend;
    Ramulator::IMemorySystem* ramulator2_memorysystem;

    // Track outstanding requests
    std::map<uint64_t, std::deque<ReqId> > dramReqs;
    std::set<ReqId> writes;

private:
};

}
}

#endif
