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

#ifndef _H_EMBER_CTXT_SWITCH_MPI_GENERATOR
#define _H_EMBER_CTXT_SWITCH_MPI_GENERATOR

#include "embermpigen.h"

#ifdef _XOPEN_SOURCE
#include <ucontext.h>
#else
#define _XOPEN_SOURCE
#include <ucontext.h>
#undef _XOPEN_SOURCE
#endif

namespace SST {
namespace Ember {

class EmberContextSwitchingMessagePassingGenerator : public EmberMessagePassingGenerator {

public:
	EmberContextSwitchingMessagePassingGenerator( ComponentId_t id, Params& params);
	bool autoInitialize() { return true; }

	virtual void configureEnvironment(const SST::Output* output, uint32_t rank, uint32_t worldSize) = 0;
        virtual void generate(const SST::Output* output, const uint32_t phase,
                std::queue<EmberEvent*>* evQ) = 0;
        virtual void finish(const SST::Output* output) = 0;

protected:
	ucontext_t genContext;

	~EmberContextSwitchingMessagePassingGenerator();
	void suspendGenerator();
	void resumeGenerator();

};

}
}

#endif
