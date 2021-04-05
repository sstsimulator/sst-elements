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


#ifndef _H_SST_MEMH_RAMULATOR_BACKEND
#define _H_SST_MEMH_RAMULATOR_BACKEND

#include "sst/elements/memHierarchy/membackend/memBackend.h"

#include "Gem5Wrapper.h"

#ifdef OLD_DEBUG
#define DEBUG OLD_DEBUG
#undef OLD_DEBUG
#endif

namespace SST {
namespace MemHierarchy {

class ramulatorMemory : public SimpleMemBackend {
public:
/* Element Library Info */
    SST_ELI_REGISTER_SUBCOMPONENT_DERIVED(ramulatorMemory, "memHierarchy", "ramulator", SST_ELI_ELEMENT_VERSION(1,0,0),
            "Ramulator-driven memory timings", SST::MemHierarchy::SimpleMemBackend)

    SST_ELI_DOCUMENT_PARAMS( MEMBACKEND_ELI_PARAMS,
            /* Own parameters */
            {"verbose",     "Sets the verbosity of the backend output", "0"},
            {"configFile",  "Name of the Ramulator Device config file", NULL} )

/* Begin class definition */
    ramulatorMemory(ComponentId_t id, Params &params);
    bool issueRequest(ReqId, Addr, bool, unsigned );
    //virtual bool issueRequest(DRAMReq *req);
    virtual bool clock(Cycle_t cycle);
    virtual void finish();

protected:
    ramulator::Gem5Wrapper *memSystem;
    std::function<void(ramulator::Request&)> callBackFunc;

    // Track outstanding requests
    std::map<uint64_t, std::deque<ReqId> > dramReqs;
    std::set<ReqId> writes;

    void ramulatorDone(ramulator::Request& req);

private:
};

}
}

#endif
