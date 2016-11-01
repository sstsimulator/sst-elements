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


#ifndef __SST_MEMH_HMCMEMBACKENDCONVERTOR__
#define __SST_MEMH_HMCMEMBACKENDCONVERTOR__

#include "sst/elements/memHierarchy/membackend/simpleMemBackendConvertor.h"

namespace SST {
namespace MemHierarchy {

class HMCMemBackendConvertor : public SimpleMemBackendConvertor {
 
  public:
    HMCMemBackendConvertor(Component *comp, Params &params) : 
        SimpleMemBackendConvertor(comp,params) {}

    virtual bool clock( Cycle_t cycle );

    virtual void handleMemResponse( ReqId, uint32_t  );

    ~HMCMemBackendConvertor() {} 

  protected:
    void sendResponse( MemReq*, uint32_t flags );
};

}
}
#endif
