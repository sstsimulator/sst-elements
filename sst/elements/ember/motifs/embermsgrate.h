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


#ifndef _H_EMBER_MSGRATE
#define _H_EMBER_MSGRATE

#include <sst/core/params.h>
#include "embergen.h"
#include "embermpigen.h"

namespace SST {
namespace Ember {

class EmberMsgRateGenerator : public EmberMessagePassingGenerator {

public:
	EmberMsgRateGenerator(SST::Component* owner, Params& params);
	void configureEnvironment(const SST::Output* output, uint32_t rank, uint32_t worldSize);
        void generate(const SST::Output* output, const uint32_t phase, std::queue<EmberEvent*>* evQ);
	void finish(const SST::Output* output);


private:
    uint32_t size;
	uint32_t rank;

    uint32_t msgSize;
    uint32_t numMsgs;
    uint32_t iterations;
    uint64_t m_startTime;
    uint64_t m_stopTime;
    uint64_t m_totalTime;

    std::vector<MessageRequest> reqs;
    SST::Output* m_output;
};

}
}

#endif
