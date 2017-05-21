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
    DelayBuffer();
    DelayBuffer(Component *comp, Params &params);
	virtual bool issueRequest( ReqId, Addr, bool isWrite, unsigned numBytes );
    void handleNextRequest(SST::Event * ev);
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
    unsigned int fwdDelay;
    Link * delay_self_link;

    std::queue<Req> requestBuffer;
};

}
}

#endif
