// Copyright 2009-2016 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2016, Sandia Corporation
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
    SimpleMemBackendConvertor(Component *comp, Params &params) :
         MemBackendConvertor(comp,params) {}

    virtual bool issue( MemReq* req );

    virtual void handleMemResponse( ReqId reqId ) {
        MemEvent* resp;
        if ( ( resp = doResponse( reqId ) ) ) {
            sendResponse( resp );
        }
    }
};

}
}
#endif
