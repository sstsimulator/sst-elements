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


#ifndef __SST_MEMH_SIMPLEMEMSCRATCHBACKENDCONVERTOR__
#define __SST_MEMH_SIMPLEMEMSCRATCHBACKENDCONVERTOR__

#include "sst/elements/memHierarchy/membackend/scratchBackendConvertor.h"

namespace SST {
namespace MemHierarchy {

class SimpleMemScratchBackendConvertor : public ScratchBackendConvertor {
 

  public:
    SimpleMemScratchBackendConvertor(Component *comp, Params &params);

    virtual bool issue( ScratchReq* req );

    virtual void handleMemResponse( ReqId reqId ) {
        SST::Event::id_type respId;
        if ( doResponse( reqId, respId ) ) {
            notifyResponse( respId );
        }
    }
};

}
}
#endif
