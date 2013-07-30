
#ifndef _H_ZODIAC_SIRIUS_READER
#define _H_ZODIAC_SIRIUS_READER

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>

#include <string>
#include <iostream>
#include <queue>

#include "sst/core/serialization.h"
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

namespace SST {
namespace Zodiac {

class SiriusReader {
    public:
	SiriusReader(char* file, uint32_t rank, uint32_t qLimit, std::queue<ZodiacEvent*>* eventQueue);
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
