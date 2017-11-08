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


#ifndef _H_SST_MEMH_CRAM_SIM_BACKEND
#define _H_SST_MEMH_CRAM_SIM_BACKEND

#include <sst/core/elementinfo.h>

#include "membackend/memBackend.h"

namespace SST {
namespace MemHierarchy {

class CramSimMemory : public SimpleMemBackend {
public:
/* Element Library Info */
    SST_ELI_REGISTER_SUBCOMPONENT(CramSimMemory, "memHierarchy", "cramsim", SST_ELI_ELEMENT_VERSION(1,0,0),
            "CramSim memory timings", "SST::MemHierarchy::MemBackend")
    
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
            {"access_time", "(string) Link latency for link to CramSim. With units (SI ok).", "100ns"}, 
            {"max_outstanding_requests", "Maximum number of outstanding requests", "256"} )

    SST_ELI_DOCUMENT_PORTS(
            {"cube_link",   "Link to CramSim",  {"CramSim.MemReqEvent", "CramSim.MemRespEvent"} } )

/* Begin class definition */
    CramSimMemory(Component *comp, Params &params);
    virtual bool issueRequest( ReqId, Addr, bool isWrite, unsigned numBytes );
    virtual bool isClocked() { return false; }

private:
    void handleCramsimEvent(SST::Event *event);

	std::set<ReqId> memReqs;
    SST::Link *cramsim_link;

	int m_maxNumOutstandingReqs;
};

}
}

#endif
