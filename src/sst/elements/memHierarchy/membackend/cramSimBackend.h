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


#ifndef _H_SST_MEMH_CRAM_SIM_BACKEND
#define _H_SST_MEMH_CRAM_SIM_BACKEND


#include "sst/elements/memHierarchy/membackend/memBackend.h"

namespace SST {
namespace MemHierarchy {

class CramSimMemory : public SimpleMemBackend {
public:
/* Element Library Info */
    SST_ELI_REGISTER_SUBCOMPONENT_DERIVED(CramSimMemory, "memHierarchy", "cramsim", SST_ELI_ELEMENT_VERSION(1,0,0),
            "CramSim memory timings", SST::MemHierarchy::SimpleMemBackend)

    SST_ELI_DOCUMENT_PARAMS( MEMBACKEND_ELI_PARAMS,
            /* Own parameters */
            {"verbose", "Sets the verbosity of the backend output", "0"},
            {"access_time", "(string) Link latency for link to CramSim. With units (SI ok).", "100ns"},
            {"max_outstanding_requests", "Maximum number of outstanding requests", "256"} )

    SST_ELI_DOCUMENT_PORTS(
            {"cramsim_link",   "Link to CramSim",  {"CramSim.MemReqEvent", "CramSim.MemRespEvent"} } )

/* Begin class definition */
    CramSimMemory(ComponentId_t id, Params &params);
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
