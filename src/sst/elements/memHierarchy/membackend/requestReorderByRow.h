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


#ifndef _H_SST_MEMH_REQUEST_REORDER_ROW_BACKEND
#define _H_SST_MEMH_REQUEST_REORDER_ROW_BACKEND

#include "sst/elements/memHierarchy/membackend/memBackend.h"
#include <list>
#include <vector>

namespace SST {
namespace MemHierarchy {

class RequestReorderRow : public SimpleMemBackend {
public:
/* Element Library Info */
    SST_ELI_REGISTER_SUBCOMPONENT_DERIVED(RequestReorderRow, "memHierarchy", "reorderByRow", SST_ELI_ELEMENT_VERSION(1,0,0),
            "Request re-orderer, groups requests by row", SST::MemHierarchy::SimpleMemBackend)

    SST_ELI_DOCUMENT_PARAMS( MEMBACKEND_ELI_PARAMS,
            /* Own parameters */
            {"verbose",                     "Sets the verbosity of the backend output", "0"},
            {"max_issue_per_cycle",         "Maximum number of requests to issue per cycle. 0 or negative is unlimited.", "-1"},
            {"banks",                       "Number of banks", "8"},
            {"bank_interleave_granularity", "Granularity of interleaving in bytes (B), generally a cache line. Must be a power of 2.", "64B"},
            {"row_size",                    "Size of a row in bytes (B). Must be a power of 2.", "8KiB"},
            {"reorder_limit",               "Maximum number of request to reorder to a rwo before changing rows.", "1"},
            {"backend",                     "Backend memory system.", "memHierarchy.simpleDRAM"} )

    SST_ELI_DOCUMENT_SUBCOMPONENT_SLOTS( {"backend", "Backend memory model.", "SST::MemHierarchy::SimpleMemBackend"} )

/* Begin class definition */
    RequestReorderRow();
    RequestReorderRow(ComponentId_t id, Params &params);

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
    unsigned int maxReqsPerRow; // Maximum number of requests to issue per row before moving to a new row
    unsigned int banks;         // Number of banks we're issuing to
    unsigned int nextBank;      // Next bank to issue to
    unsigned int bankMask;      // Mask for determining request bank
    unsigned int rowOffset;     // Offset for determining request row
    unsigned int lineOffset;    // Offset for determining line (needed for finding bank)
    int reqsPerCycle;           // Number of requests to issue per cycle (max) -> memCtrl limits how many we accept
    std::vector< std::list<Req>* > requestQueue;
    std::vector<unsigned int> reorderCount;
    std::vector<unsigned int> lastRow;

};

}
}

#endif
