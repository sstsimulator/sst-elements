// Copyright 2009-2018 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2018, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#ifndef _H_SST_MESSIER_SIM_BACKEND
#define _H_SST_MESSIER_SIM_BACKEND

#include "sst/elements/memHierarchy/membackend/memBackend.h"

namespace SST {
namespace MemHierarchy {

class Messier : public SimpleMemBackend {
public:
/* Element Library Info */
    SST_ELI_REGISTER_SUBCOMPONENT(Messier, "memHierarchy", "Messier", SST_ELI_ELEMENT_VERSION(1,0,0),
            "Messier memory timings", "SST::MemHierarchy::MemBackend")
    
    SST_ELI_DOCUMENT_PARAMS( MEMBACKEND_ELI_PARAMS,
            /* Own parameters */
            {"verbose", "Sets the verbosity of the backend output", "0"},
            {"access_time", "Link latency for the link to the Messier memory model. With units (SI ok).", "1ns"} )

    SST_ELI_DOCUMENT_PORTS( {"cube_link", "Link to Messier", {"Messier.MemReqEvent", "Messier.MemRespEvent"} } )

/* Begin class definition */
    Messier(Component *comp, Params &params);
    virtual bool issueRequest( ReqId, Addr, bool isWrite, unsigned numBytes );
    void handleMessierResp(SST::Event *event);
    virtual bool isClocked() { return false; }

private:
    SST::Link *cube_link;
    std::set<ReqId> outToNVM;
    SST::Link *nvm_link;

};

}
}

#endif
