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


#ifndef _H_SST_MEMH_VAULT_SIM_BACKEND
#define _H_SST_MEMH_VAULT_SIM_BACKEND

#include "sst/elements/memHierarchy/membackend/memBackend.h"

namespace SST {
namespace MemHierarchy {

class VaultSimMemory : public FlagMemBackend {
public:
/* Element Library Info */
    SST_ELI_REGISTER_SUBCOMPONENT_DERIVED(VaultSimMemory, "memHierarchy", "vaultsim", SST_ELI_ELEMENT_VERSION(1,0,0),
            "Backend to interface with VaultSimC, a generic vaulted memory model", SST::MemHierarchy::FlagMemBackend)

    SST_ELI_DOCUMENT_PARAMS( MEMBACKEND_ELI_PARAMS,
            /* Own parameters */
            {"access_time", "Link latency for the link to the VaultSim memory model. With units (SI ok).", "100ns"} )

    SST_ELI_DOCUMENT_PORTS( {"cube_link", "Link to VaultSim.", {"VaultSimC.MemRespEvent", "VaultSimC.MemReqEvent"} } )

/* Begin class definition */
    VaultSimMemory(ComponentId_t id, Params &params);
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
