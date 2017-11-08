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


#ifndef _H_SST_MEMH_DELAY_BUFFER
#define _H_SST_MEMH_DELAY_BUFFER

#include "membackend/memBackend.h"
#include <queue>

namespace SST {
namespace MemHierarchy {

class DelayBuffer : public SimpleMemBackend {
public:
/* Element Library Info */
    SST_ELI_REGISTER_SUBCOMPONENT(DelayBuffer, "memHierarchy", "DelayBuffer", SST_ELI_ELEMENT_VERSION(1,0,0),
            "Delays requests by a specified time", "SST::MemHierarchy::MemBackend")
    
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
            {"backend", "Backend memory system", "memHierarchy.simpleMem"},
            {"request_delay", "Constant delay to be added to requests with units (e.g., 1us)", "0ns"} )

/* Begin class definition */
    DelayBuffer();
    DelayBuffer(Component *comp, Params &params);
	virtual bool issueRequest( ReqId, Addr, bool isWrite, unsigned numBytes );
    void handleNextRequest(SST::Event * ev);
    void setup();
    void finish();
    virtual bool clock(Cycle_t cycle);
    virtual const std::string& getClockFreq() { return backend->getClockFreq(); }
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
