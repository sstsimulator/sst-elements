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


#ifndef _H_SST_MEMH_REQUEST_REORDER_ROW_BACKEND
#define _H_SST_MEMH_REQUEST_REORDER_ROW_BACKEND

#include "membackend/memBackend.h"
#include <list>
#include <vector>

namespace SST {
namespace MemHierarchy {

class RequestReorderRow : public MemBackend {
public:
    RequestReorderRow();
    RequestReorderRow(Component *comp, Params &params);
    bool issueRequest(DRAMReq *req);
    void setup();
    void finish();
    void clock();

private:
    MemBackend* backend;
    unsigned int maxReqsPerRow; // Maximum number of requests to issue per row before moving to a new row
    unsigned int banks;         // Number of banks we're issuing to
    unsigned int nextBank;      // Next bank to issue to
    unsigned int bankMask;      // Mask for determining request bank
    unsigned int rowOffset;     // Offset for determining request row
    unsigned int lineOffset;    // Offset for determining line (needed for finding bank)
    int reqsPerCycle;           // Number of requests to issue per cycle (max) -> memCtrl limits how many we accept
    std::vector< std::list<DRAMReq* >* > requestQueue;
    std::vector<unsigned int> reorderCount;
    std::vector<unsigned int> lastRow;

};

}
}

#endif
