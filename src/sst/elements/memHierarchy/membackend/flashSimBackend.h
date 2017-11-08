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
/* Element Library Info */
    SST_ELI_REGISTER_SUBCOMPONENT(FlashDIMMSimMemory, "memHierarchy", "FlashDIMMSim", SST_ELI_ELEMENT_VERSION(1,0,0),
            "Backend interface to FlashDIMM simulator for memory timings", "SST::MemHierarchy::MemBackend")
    
    SST_ELI_DOCUMENT_PARAMS(
            /* Inherited from MemBackend */
            {"debug_level",     "(uint) Debugging level: 0 (no output) to 10 (all output). Output also requires that SST Core be compiled with '--enable-debug'", "0"},
            {"debug_mask",      "(uint) Mask on debug_level", "0"},
            {"debug_location",  "(uint) 0: No debugging, 1: STDOUT, 2: STDERR, 3: FILE", "0"},
            {"clock", "(string) Clock frequency - inherited from MemController", NULL},
            {"max_requests_per_cycle", "(int) Maximum number of requests to accept each cycle. Use 0 or -1 for unlimited.", "-1"},
            {"request_width", "(int) Maximum size, in bytes, for a request", "64"},
            {"mem_size", "(string) Size of memory with units (SI ok). E.g., '2GiB'.", NULL},
            /* Own parameters */
            {"device_ini",          "Name of HybridSim Device config file", ""},
            {"verbose",             "Sets the verbosity of the backend output", "0"},
            {"trace",               "Sets the name of a file to record trace output", ""},
            {"max_pending_reqs",    "Sets the maximum number of requests that can be outstanding", "32" } )

/* Begin class definition */
    FlashDIMMSimMemory(Component *comp, Params &params);
    bool issueRequest(ReqId, Addr, bool, unsigned );
    virtual bool clock(Cycle_t cycle);
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
