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


#ifndef _H_EMBER_DUMPI_TRACE_MOTIF
#define _H_EMBER_DUMPI_TRACE_MOTIF

#include "mpi/embermpigen.h"

#include "dumpi/libundumpi/libundumpi.h"
#include "dumpi/libundumpi/bindings.h"
#include "dumpi/libundumpi/callbacks.h"

extern "C" {
static int ember_dumpi_callback(const void *parsearg,
	uint16_t thread, const dumpi_time *cpu, const dumpi_time *wall,
	const dumpi_perfinfo *perf, void *userarg);
}

namespace SST {
namespace Ember {


class EmberDUMPITraceGenerator : public EmberMessagePassingGenerator {

public:
	EmberDUMPITraceGenerator(SST::Component* owner, Params& params);
    	bool generate( std::queue<EmberEvent*>& evQ);

protected:
	int trace_send(const dumpi_send *prm, uint16_t thread,
		const dumpi_time *cpu, const dumpi_time *wall,
		const dumpi_perfinfo *perf, void *userarg);

private:
	dumpi_profile* trace_file;
	libundumpi_callbacks trace_callbacks;

};

}
}

#endif
