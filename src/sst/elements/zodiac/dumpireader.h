// Copyright 2009-2018 NTESS. Under the terms
// of Contract DE-NA0003525 with NTESS, the U.S.
// Government retains certain rights in this software.
//
// Copyright (c) 2009-2018, NTESS
// All rights reserved.
//
// Portions are copyright of other developers:
// See the file CONTRIBUTORS.TXT in the top level directory
// the distribution for more information.
//
// This file is part of the SST software package. For license
// information, see the LICENSE file in the top level directory of the
// distribution.


#ifndef _H_ZODIAC_DUMPI_READER
#define _H_ZODIAC_DUMPI_READER

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <string>
#include <iostream>
#include <queue>

#include "sst/elements/hermes/msgapi.h"

#include "zevent.h"
#include "zsendevent.h"
#include "zrecvevent.h"

extern "C" {
#include "dumpi/libundumpi/libundumpi.h"
#include "dumpi/libundumpi/callbacks.h"
}

using namespace std;
using namespace SST::Hermes;

extern "C" {
int handleDUMPISend(const dumpi_send *prm, uint16_t thread, const dumpi_time *cpu, const dumpi_time *wall, const dumpi_perfinfo *perf, void *userarg);
int handleDUMPINullFunction(const void *prm, uint16_t thread, const dumpi_time *cpu, const dumpi_time *wall, const dumpi_perfinfo *perf, void *userarg);

}

namespace SST {
namespace Zodiac {

class DUMPIReader {
    public:
	DUMPIReader(string file, uint32_t rank, uint32_t qLimit, std::queue<ZodiacEvent*>* eventQueue);
        void close();
	uint32_t generateNextEvents();
	uint32_t getQueueLimit();
	uint32_t getCurrentQueueSize();
	void enqueueEvent(ZodiacEvent* ev);

    private:
	uint32_t rank;
	uint32_t qLimit;
	bool foundFinalize;
	std::queue<ZodiacEvent*>* eventQ;
	dumpi_profile* trace;
	libundumpi_callbacks* callbacks;
};

}
}

#endif
