// Copyright 2009-2014 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2014, Sandia Corporation
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.

#ifndef _H_SST_MEM_H_REQ_GEN_CPU
#define _H_SST_MEM_H_REQ_GEN_CPU

#include <sst/core/component.h>
#include <sst/core/interfaces/simpleMem.h>

#include "reqGenModule.h"

using namespace SST::Interfaces;

namespace SST {
namespace Miranda {

class RequestGenCPU : public SST::Component {
public:

	RequestGenCPU(SST::ComponentId_t id, SST::Params& params);
	void finish();
	void init(unsigned int phase);

private:
	RequestGenCPU();  // for serialization only
	RequestGenCPU(const RequestGenCPU&); // do not implement
	void operator=(const RequestGenCPU&); // do not implement
	~RequestGenCPU();

	void handleEvent( SimpleMem::Request* ev );
	bool clockTick( SST::Cycle_t );
	void issueRequest(RequestGeneratorRequest* req);

    	Output* out;

	RequestGenerator* reqGen;
	std::map<uint64_t, SimTime_t> requests;
    	SimpleMem* cache_link;
	RequestGeneratorRequest* nextReq;

	uint32_t maxRequestsPending;
	uint32_t requestsPending;
	uint64_t cacheLine;

};

}
}
#endif /* _RequestGenCPU_H */
