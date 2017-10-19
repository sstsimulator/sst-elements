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
