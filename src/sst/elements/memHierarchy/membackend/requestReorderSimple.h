// Copyright 2009-2021 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2021, NTESS
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

#include "sst/elements/memHierarchy/membackend/memBackend.h"
#include <list>

namespace SST {
namespace MemHierarchy {

class RequestReorderSimple : public SimpleMemBackend {
public:
/* Element Library Info */
    SST_ELI_REGISTER_SUBCOMPONENT_DERIVED(RequestReorderSimple, "memHierarchy", "reorderSimple", SST_ELI_ELEMENT_VERSION(1,0,0),
            "Simple request re-orderer, issues the first N requests that are accepted by the backend", SST::MemHierarchy::SimpleMemBackend)

    SST_ELI_DOCUMENT_PARAMS( MEMBACKEND_ELI_PARAMS,
            /* Own parameters */
            {"verbose", "Sets the verbosity of the backend output", "0"},
            {"max_issue_per_cycle", "Maximum number of requests to issue per cycle. 0 or negative is unlimited.", "-1"},
            {"search_window_size",  "Maximum number of requests to search each cycle. 0 or negative is unlimited.", "-1"},
            {"backend",             "Backend memory system", "memHierarchy.simpleDRAM"} )

    SST_ELI_DOCUMENT_SUBCOMPONENT_SLOTS({"backend", "Backend memory model", "SST::MemHierarchy::SimpleMemBackend"} )

/* Begin class definition */
    RequestReorderSimple();
    RequestReorderSimple(ComponentId_t id, Params &params);
	virtual bool issueRequest( ReqId, Addr, bool isWrite, unsigned numBytes );
    void setup();
    void finish();
    bool clock(Cycle_t cycle);

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
