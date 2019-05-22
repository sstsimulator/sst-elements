// Copyright 2009-2019 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2019, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#ifndef __SST_MEMH_SIMPLEMEMBACKENDCONVERTOR__
#define __SST_MEMH_SIMPLEMEMBACKENDCONVERTOR__


#include "sst/elements/memHierarchy/membackend/memBackendConvertor.h"

namespace SST {
namespace MemHierarchy {

class SimpleMemBackendConvertor : public MemBackendConvertor {
public:
/* Element Library Info */
    SST_ELI_REGISTER_SUBCOMPONENT_DERIVED_API(SST::MemHierarchy::SimpleMemBackendConvertor, SST::MemHierarchy::MemBackendConvertor)

    SST_ELI_REGISTER_SUBCOMPONENT_DERIVED(SimpleMemBackendConvertor, "memHierarchy", "simpleMemBackendConvertor", SST_ELI_ELEMENT_VERSION(1,0,0),
            "Converts a MemEventBase* for base MemBackend", SST::MemHierarchy::SimpleMemBackendConvertor)

    SST_ELI_DOCUMENT_PARAMS( MEMBACKENDCONVERTOR_ELI_PARAMS )

    SST_ELI_DOCUMENT_STATISTICS( MEMBACKENDCONVERTOR_ELI_STATS )
    
    SST_ELI_DOCUMENT_SUBCOMPONENT_SLOTS( MEMBACKENDCONVERTOR_ELI_SLOTS )

/* Begin class definition */
    SimpleMemBackendConvertor(Component *comp, Params &params);
    SimpleMemBackendConvertor(ComponentId_t id, Params &params);

    virtual bool issue( BaseReq* req );

    virtual void handleMemResponse( ReqId reqId ) {
        doResponse(reqId);
    }
};

}
}
#endif
