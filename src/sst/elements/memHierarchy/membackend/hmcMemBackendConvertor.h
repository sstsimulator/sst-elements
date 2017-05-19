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


#ifndef __SST_MEMH_HMCMEMBACKENDCONVERTOR__
#define __SST_MEMH_HMCMEMBACKENDCONVERTOR__

#include "sst/elements/memHierarchy/membackend/memBackendConvertor.h"

namespace SST {
namespace MemHierarchy {

class HMCMemBackendConvertor : public MemBackendConvertor {
 
  public:
    HMCMemBackendConvertor(Component *comp, Params &params);

    virtual bool issue( MemReq* req );
    virtual void handleMemResponse( ReqId reqId, uint32_t flags  ) {
        MemEvent* resp;
        if ( ( resp = doResponse( reqId ) ) ) {
            resp->setFlags(flags);
            sendResponse( resp );
        }
    }
};

}
}
#endif
