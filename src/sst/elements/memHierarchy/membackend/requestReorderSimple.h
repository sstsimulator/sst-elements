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


#ifndef _H_SST_MEMH_REQUEST_REORDER_SIMPLE_BACKEND
#define _H_SST_MEMH_REQUEST_REORDER_SIMPLE_BACKEND

#include "membackend/memBackend.h"
#include <list>

namespace SST {
namespace MemHierarchy {

class RequestReorderSimple : public MemBackend {
public:
    RequestReorderSimple();
    RequestReorderSimple(Component *comp, Params &params);
    bool issueRequest(DRAMReq *req);
    void setup();
    void finish();
    void clock();

private:
    MemBackend* backend;
    int reqsPerCycle;       // Number of requests to issue per cycle (max) -> memCtrl limits how many we accept
    int searchWindowSize;   // Number of requests to search when looking for requests to issue
    std::list<DRAMReq*> requestQueue;   // List of requests waiting to be issued
};

}
}

#endif
