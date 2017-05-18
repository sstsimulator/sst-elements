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


#ifndef _H_SST_MEMH_REQUEST_REORDER_SIMPLE_BACKEND
#define _H_SST_MEMH_REQUEST_REORDER_SIMPLE_BACKEND

#include "membackend/memBackend.h"
#include <list>

namespace SST {
namespace MemHierarchy {

class RequestReorderSimple : public SimpleMemBackend {
public:
    RequestReorderSimple();
    RequestReorderSimple(Component *comp, Params &params);
	virtual bool issueRequest( ReqId, Addr, bool isWrite, unsigned numBytes );
    void setup();
    void finish();
    void clock();
    virtual const std::string& getClockFreq() { return backend->getClockFreq(); }

private:
    void handleMemReponse( ReqId id ) {
        SimpleMemBackend::handleMemResponse( id );
    }
    struct Req {
        Req( ReqId id, Addr addr, bool isWrite, unsigned numBytes ) :
            id(id), addr(addr), isWrite(isWrite), numBytes(numBytes)
        { }
        ReqId id;
        Addr addr;
        bool isWrite;
        unsigned numBytes;
    };
    SimpleMemBackend* backend;
    int reqsPerCycle;       // Number of requests to issue per cycle (max) -> memCtrl limits how many we accept
    int searchWindowSize;   // Number of requests to search when looking for requests to issue
    std::list<Req> requestQueue;   // List of requests waiting to be issued
};

}
}

#endif
