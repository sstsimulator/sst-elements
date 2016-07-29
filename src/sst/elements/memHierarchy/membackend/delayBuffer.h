// Copyright 2009-2016 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2016, Sandia Corporation
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#ifndef _H_SST_MEMH_DELAY_BUFFER
#define _H_SST_MEMH_DELAY_BUFFER

#include "membackend/memBackend.h"
#include <queue>

namespace SST {
namespace MemHierarchy {

class DelayBuffer : public MemBackend {
public:
    DelayBuffer();
    DelayBuffer(Component *comp, Params &params);
    bool issueRequest(DRAMReq *req);
    void handleNextRequest(SST::Event * ev);
    void setup();
    void finish();
    void clock();

private:
    MemBackend* backend;
    unsigned int fwdDelay;
    Link * delay_self_link;
    std::queue<DRAMReq*> requestBuffer;
};

}
}

#endif
