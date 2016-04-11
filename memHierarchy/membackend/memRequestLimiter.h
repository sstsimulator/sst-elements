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


#ifndef _H_SST_MEMH_MEMORY_REQUEST_LIMITER
#define _H_SST_MEMH_MEMORY_REQUEST_LIMITER

#include "membackend/memBackend.h"

namespace SST {
namespace MemHierarchy {

class MemoryRequestLimiter : public MemBackend {
public:
    MemoryRequestLimiter();
    MemoryRequestLimiter(Component *comp, Params &params);
    bool issueRequest(DRAMReq *req);
    void clock();
    void setup();
    void finish();
private:
    MemBackend * backend;

    int requestCount;
    int maxReqsPerCycle;
};

}
}

#endif
