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
#include <sst/core/statapi/stataccumulator.h>

#include "mirandaGenerator.h"

using namespace SST;
using namespace SST::Interfaces;
using namespace SST::Statistics;

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
	void issueRequest(MemoryOpRequest* req);

    	Output* out;

	RequestGenerator* reqGen;
	std::map<SimpleMem::Request::id_t, SimTime_t> requestsInFlight;
    	SimpleMem* cache_link;

	MirandaRequestQueue pendingRequests;

	uint32_t maxRequestsPending;
	uint32_t requestsPending;
	uint32_t reqMaxPerCycle;
	uint64_t cacheLine;

	Statistic<uint64_t>* statReadReqs;
	Statistic<uint64_t>* statWriteReqs;
	Statistic<uint64_t>* statSplitReadReqs;
	Statistic<uint64_t>* statSplitWriteReqs;
	Statistic<uint64_t>* statCyclesWithIssue;
	Statistic<uint64_t>* statCyclesWithoutIssue;
	Statistic<uint64_t>* statBytesRead;
	Statistic<uint64_t>* statBytesWritten;
	Statistic<uint64_t>* statReqLatency;
};

}
}
#endif /* _RequestGenCPU_H */
