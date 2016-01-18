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

private:
	FILE* trace_file;
	std::unordered_map<uint32_t, Communicator> communicatorMap;

	double readTime() const;
	uint32_t readUINT32() const;
	uint64_t readUINT64() const;
	int32_t readINT32() const;
	PayloadDataType readDataType() const;
	Communicator readCommunicator() const;
	size_t getTypeElementSize(const PayloadDataType dType) const;

	void readMPISend( std::queue<EmberEvent*>& evQ );
	void readMPIIsend( std::queue<EmberEvent*>& evQ );
	void readMPIRecv( std::queue<EmberEvent*>& evQ );
	void readMPIIrecv( std::queue<EmberEvent*>& evQ );
	void readMPIFinalize( std::queue<EmberEvent*>& evQ );
	void readMPIInit( std::queue<EmberEvent*>& evQ );

};

}
}

#endif
