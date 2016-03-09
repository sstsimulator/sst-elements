// Copyright 2009-2015 Sandia Corporation. Under the terms
// of Contract DE-AC04-94AL85000 with Sandia Corporation, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2015, Sandia Corporation
// All rights reserved.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#ifndef _H_EMBER_SIRIUS_TRACE_MOTIF
#define _H_EMBER_SIRIUS_TRACE_MOTIF

#include "mpi/embermpigen.h"
#include <unordered_map>

#include "sirius/siriusglobals.h"

namespace SST {
namespace Ember {

class EmberSIRIUSTraceGenerator : public EmberMessagePassingGenerator {

public:
	EmberSIRIUSTraceGenerator(SST::Component* owner, Params& params);
	~EmberSIRIUSTraceGenerator();
    	bool generate( std::queue<EmberEvent*>& evQ );

	void printLiveRequestMap() {
		for(auto itr = liveRequests.begin();
			itr != liveRequests.end(); itr++) {

			output("Request: %" PRIu64 " maps to MessageRequest at %p\n",
				itr->first, itr->second);
		}
	}

private:
	FILE* trace_file;
	std::unordered_map<uint32_t, Communicator*> communicatorMap;
	std::unordered_map<uint64_t, MessageRequest*> liveRequests;
	double currentTraceTime;

	double readTime() const;
	uint32_t readUINT32() const;
	uint64_t readUINT64() const;
	int32_t readINT32() const;
	int32_t readTag() const;
	PayloadDataType readDataType() const;
	const Communicator* readCommunicator() const;
	size_t getTypeElementSize(const PayloadDataType dType) const;
	ReductionOperation readReductionOp() const;

	void enqueueCompute( std::queue<EmberEvent*>& evQ,
                const double nextStartTime,
                const double nextEndTime);
	void readMPISend( std::queue<EmberEvent*>& evQ );
	void readMPIIsend( std::queue<EmberEvent*>& evQ );
	void readMPIRecv( std::queue<EmberEvent*>& evQ );
	void readMPIIrecv( std::queue<EmberEvent*>& evQ );
	void readMPIFinalize( std::queue<EmberEvent*>& evQ );
	void readMPIInit( std::queue<EmberEvent*>& evQ );
	void readMPIReduce( std::queue<EmberEvent*>& evQ );
	void readMPIAllreduce( std::queue<EmberEvent*>& evQ );
	void readMPIBarrier( std::queue<EmberEvent*>& evQ );
	void readMPIWait( std::queue<EmberEvent*>& evQ );
	void readMPIWaitall( std::queue<EmberEvent*>& evQ );
	void readMPIBcast( std::queue<EmberEvent*>& evQ );
	void readMPICommSplit( std::queue<EmberEvent*>& evQ );
	void readMPICommDisconnect( std::queue<EmberEvent*>& evQ );

};

}
}

#endif
