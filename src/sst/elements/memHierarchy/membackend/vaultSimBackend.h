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


#ifndef _H_SST_MEMH_VAULT_SIM_BACKEND
#define _H_SST_MEMH_VAULT_SIM_BACKEND

#include "membackend/memBackend.h"

namespace SST {
namespace MemHierarchy {

class VaultSimMemory : public FlagMemBackend {
public:
/* Element Library Info */
    SST_ELI_REGISTER_SUBCOMPONENT(VaultSimMemory, "memHierarchy", "vaultsim", SST_ELI_ELEMENT_VERSION(1,0,0),
            "Basic constant-access-time memory timing model", "SST::MemHierarchy::MemBackend")
    
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
            {"access_time", "Link latency for the link to the VaultSim memory model. With units (SI ok).", "100ns"} )

    SST_ELI_DOCUMENT_PORTS( {"cube_link", "Link to VaultSim.", {"VaultSimC.MemRespEvent", "VaultSimC.MemReqEvent"} } )

/* Begin class definition */
    VaultSimMemory(Component *comp, Params &params);
    virtual bool issueRequest( ReqId, Addr, bool isWrite, uint32_t flags, unsigned numBytes );
    virtual bool isClocked() { return false; }

private:
    void handleCubeEvent(SST::Event *event);

    std::set<ReqId> outToCubes;
    SST::Link *cube_link;
};

}
}

#endif
