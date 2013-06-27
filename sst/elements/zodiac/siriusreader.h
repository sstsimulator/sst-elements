
#ifndef _H_ZODIAC_SIRIUS_READER
#define _H_ZODIAC_SIRIUS_READER

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include <string>
#include <iostream>
#include <queue>

#include "sst/elements/hermes/msgapi.h"

#include "siriusconst.h"

#include "zevent.h"
#include "zsendevent.h"
#include "zrecvevent.h"
#include "zbarrierevent.h"
#include "zcomputeevent.h"

using namespace std;
using namespace SST::Hermes;

namespace SST {
namespace Zodiac {

class SiriusReader {
    public:
	SiriusReader(string file, uint32_t rank, uint32_t qLimit, std::queue<ZodiacEvent*>* eventQueue);
        void close();
	uint32_t generateNextEvents();
	uint32_t getQueueLimit();
	uint32_t getCurrentQueueSize();
	bool hasReachedFinalize();

    private:
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
	void readRecv();
	void readInit();
	void readFinalize();
	void readBarrier();
	PayloadDataType convertToHermesType(uint32_t dtype);

};

}
}

#endif
