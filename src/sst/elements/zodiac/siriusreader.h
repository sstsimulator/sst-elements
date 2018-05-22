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


#ifndef _H_ZODIAC_SIRIUS_READER
#define _H_ZODIAC_SIRIUS_READER

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>

#include <string>
#include <iostream>
#include <queue>

#include "sst/core/output.h"
#include "sst/elements/hermes/msgapi.h"

#include "sirius/siriusconst.h"

#include "zevent.h"
#include "zinitevent.h"
#include "zsendevent.h"
#include "zirecvevent.h"
#include "zrecvevent.h"
#include "zbarrierevent.h"
#include "zcomputeevent.h"
#include "zwaitevent.h"
#include "zfinalizeevent.h"
#include "zallredevent.h"

using namespace std;
using namespace SST::Hermes;
using namespace SST::Hermes::MP;

namespace SST {
namespace Zodiac {

class SiriusReader {
    public:
	SiriusReader(char* file, uint32_t rank, uint32_t qLimit, std::queue<ZodiacEvent*>* eventQueue, int verbose);
        void close();
	void setOutput(Output* oput);
	uint32_t generateNextEvents();
	uint32_t getQueueLimit();
	uint32_t getCurrentQueueSize();
	bool hasReachedFinalize();

    private:
	Output* output;
	uint32_t rank;
	uint32_t qLimit;
	bool foundFinalize;
	std::queue<ZodiacEvent*>* eventQ;
	FILE* trace;
	double prevEventTime;
	void generateNextEvent();
	inline uint32_t readUINT32();
	inline uint64_t readUINT64();
	inline double readTime();
	inline int32_t readINT32();
	inline int64_t readINT64();
	void readSend();
	void readIrecv();
	void readRecv();
	void readInit();
	void readFinalize();
	void readBarrier();
	void readWait();
	void readAllreduce();

	PayloadDataType convertToHermesType(uint32_t dtype);
	ReductionOperation convertToHermesOp(uint32_t op);
};

}
}

#endif
