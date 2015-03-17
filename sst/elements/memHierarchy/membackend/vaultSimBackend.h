// Copyright 2009-2015 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2015, Sandia Corporation
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#ifndef _H_SST_MEMH_VAULT_SIM_BACKEND
#define _H_SST_MEMH_VAULT_SIM_BACKEND

#include "membackend/memBackend.h"

namespace SST {
namespace MemHierarchy {

class VaultSimMemory : public MemBackend {
public:
    VaultSimMemory(Component *comp, Params &params);
    bool issueRequest(MemController::DRAMReq *req);
private:
    void handleCubeEvent(SST::Event *event);

    typedef std::map<MemEvent::id_type,MemController::DRAMReq*> memEventToDRAMMap_t;
    memEventToDRAMMap_t outToCubes; // map of events sent out to the cubes
    SST::Link *cube_link;
};

}
}

#endif
