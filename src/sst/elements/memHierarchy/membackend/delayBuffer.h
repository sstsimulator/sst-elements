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


#ifndef _H_SST_MEMH_DELAY_BUFFER
#define _H_SST_MEMH_DELAY_BUFFER

#include "sst/elements/memHierarchy/membackend/memBackend.h"
#include <queue>

namespace SST {
namespace MemHierarchy {

class DelayBuffer : public SimpleMemBackend {
public:
/* Element Library Info */
    SST_ELI_REGISTER_SUBCOMPONENT_DERIVED(DelayBuffer, "memHierarchy", "DelayBuffer", SST_ELI_ELEMENT_VERSION(1,0,0),
            "Delays requests by a specified time", SST::MemHierarchy::SimpleMemBackend)

    SST_ELI_DOCUMENT_PARAMS( MEMBACKEND_ELI_PARAMS,
            /* Own parameters */
            {"verbose", "Sets the verbosity of the backend output", "0"},
            {"backend", "Backend memory system", "memHierarchy.simpleMem"},
            {"request_delay", "Constant delay to be added to requests with units (e.g., 1us)", "0ns"} )

    SST_ELI_DOCUMENT_SUBCOMPONENT_SLOTS( {"backend", "Backend memory model", "SST::MemHierarchy::SimpleMemBackend"} )

/* Begin class definition */
    DelayBuffer();
    DelayBuffer(ComponentId_t id, Params &params);
    virtual bool issueRequest( ReqId, Addr, bool isWrite, unsigned numBytes );
    void handleNextRequest(SST::Event * ev);
    void setup();
    void finish();
    virtual bool clock(Cycle_t cycle);
    virtual bool isClocked() { return backend->isClocked(); }

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
    unsigned int fwdDelay;
    Link * delay_self_link;

    std::queue<Req> requestBuffer;
};

}
}

#endif
