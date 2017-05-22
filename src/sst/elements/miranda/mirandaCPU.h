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

#ifndef _H_SST_MEM_H_REQ_GEN_CPU
#define _H_SST_MEM_H_REQ_GEN_CPU

#include <sst/core/component.h>
#include <sst/core/interfaces/simpleMem.h>
#include <sst/core/statapi/stataccumulator.h>

#include "mirandaGenerator.h"
#include "mirandaEvent.h"
#include "mirandaMemMgr.h"

using namespace SST;
using namespace SST::Interfaces;
using namespace SST::Statistics;

namespace SST {
namespace Miranda {

class CPURequest {
public:
	CPURequest(const uint64_t origID) :
		originalID(origID), issueTime(0), outstandingParts(0) {}
	void incPartCount() { outstandingParts++; }
	void decPartCount() { outstandingParts--; }
	bool completed() const { return 0 == outstandingParts; }
	void setIssueTime(const uint64_t now) { issueTime = now; }
	uint64_t getIssueTime() const { return issueTime; }
	uint64_t getOriginalReqID() const { return originalID; }
	uint32_t countParts() const { return outstandingParts; }
protected:
	uint64_t originalID;
	uint64_t issueTime;
	uint32_t outstandingParts;
};

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

	void loadGenerator( MirandaReqEvent* );
	void loadGenerator( const std::string& name, SST::Params& params);
	void handleEvent( SimpleMem::Request* ev );
	bool clockTick( SST::Cycle_t );
	void issueRequest(MemoryOpRequest* req);
	void handleSrcEvent( SST::Event* );

 	Output* out;

	TimeConverter* timeConverter;
	Clock::HandlerBase* clockHandler;
	RequestGenerator* reqGen;
	std::map<SimpleMem::Request::id_t, CPURequest*> requestsInFlight;
	SimpleMem* cache_link;
	Link* srcLink;
	MirandaReqEvent* srcReqEvent;	

	MirandaRequestQueue<GeneratorRequest*> pendingRequests;
	MirandaMemoryManager* memMgr;

	uint32_t maxLoadRequestsPending;
	uint32_t maxStoreRequestsPending;
	uint32_t requestsLoadPending;
	uint32_t requestsStorePending;
	uint32_t reqMaxPerCycle;
	uint64_t cacheLine;
	uint32_t maxOpLookup;

	Statistic<uint64_t>* statReadReqs;
	Statistic<uint64_t>* statWriteReqs;
	Statistic<uint64_t>* statSplitReadReqs;
	Statistic<uint64_t>* statSplitWriteReqs;
	Statistic<uint64_t>* statCyclesWithIssue;
	Statistic<uint64_t>* statMaxIssuePerCycle;
	Statistic<uint64_t>* statCyclesWithoutIssue;
	Statistic<uint64_t>* statBytesRead;
	Statistic<uint64_t>* statBytesWritten;
	Statistic<uint64_t>* statReqLatency;
	Statistic<uint64_t>* statTime;
	Statistic<uint64_t>* statCyclesHitFence;
	Statistic<uint64_t>* statCyclesHitReorderLimit;
	Statistic<uint64_t>* statCycles;
};

}
}
#endif /* _RequestGenCPU_H */

