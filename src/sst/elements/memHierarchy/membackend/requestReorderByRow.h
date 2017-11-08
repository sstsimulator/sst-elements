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


#ifndef _H_SST_MEMH_REQUEST_REORDER_ROW_BACKEND
#define _H_SST_MEMH_REQUEST_REORDER_ROW_BACKEND

#include "membackend/memBackend.h"
#include <list>
#include <vector>

namespace SST {
namespace MemHierarchy {

class RequestReorderRow : public SimpleMemBackend {
public:
/* Element Library Info */
    SST_ELI_REGISTER_SUBCOMPONENT(RequestReorderRow, "memHierarchy", "reorderSimple", SST_ELI_ELEMENT_VERSION(1,0,0),
            "Simple request re-orderer, issues the first N requests that are accepted by the backend", "SST::MemHierarchy::MemBackend")
    
    SST_ELI_DOCUMENT_PARAMS(
            /* Inherited from MemBackend */
            {"debug_level",     "(uint) Debugging level: 0 (no output) to 10 (all output). Output also requires that SST Core be compiled with '--enable-debug'", "0"},
            {"debug_mask",      "(uint) Mask on debug_level", "0"},
            {"debug_location",  "(uint) 0: No debugging, 1: STDOUT, 2: STDERR, 3: FILE", "0"},
            {"clock", "(string) Clock frequency - inherited from MemController", NULL},
            {"max_requests_per_cycle", "(int) Maximum number of requests to accept each cycle. Use 0 or -1 for unlimited.", "-1"},
            {"request_width", "(int) Maximum size, in bytes, for a request", "64"},
            {"mem_size", "(string) Size of memory with units (SI ok). E.g., '2GiB'.", NULL},
            /* Own parameters */
            {"verbose", "Sets the verbosity of the backend output", "0"},
            {"max_issue_per_cycle", "Maximum number of requests to issue per cycle. 0 or negative is unlimited.", "-1"},
            {"search_window_size",  "Maximum number of requests to search each cycle. 0 or negative is unlimited.", "-1"},
            {"backend",             "Backend memory system", "memHierarchy.simpleDRAM"} )

/* Begin class definition */
    RequestReorderRow();
    RequestReorderRow(Component *comp, Params &params);
	virtual bool issueRequest( ReqId, Addr, bool isWrite, unsigned numBytes );
    void setup();
    void finish();
    bool clock(Cycle_t cycle);
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
